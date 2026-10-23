/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SCL > CSET (M-Vave-OP1): chord sets after the Roland J-6's. Included by seq.c after the ARP's modes. A track with
 * CSET on (cset_en, SCL > CSET's KNOB 1, kept in the project's extras) that is no kit (chord.c chord_kit) plays a
 * chord on every key, a MIDI note too: the chord of the note's key (C .. B) in the track's set (cset_set, KNOB 2:
 * one of the hundred of the J-6 owner's manual's Chord Set List, cset_data.h; KNOB 3 steps by genre), voiced as the
 * list voices it. The keys F3 .. E4 play the chords where the list has them (its basses F2 .. E3, so a set's twelve
 * chords lie as the J-6 plays them), every octave above or below an octave higher or lower (the octave buttons
 * too); the track's TRN moves the keys' chords (not a MIDI note's, as a MIDI note is not mapped). It comes before the
 * track's CHRD and QNT: every key plays its own chord, a black key under WHITE too.
 * CSET ON makes the track POLY (a chord needs its voices), keeping the VOICE it had (cset_voice) for OFF to give back
 * (if VOICE is still POLY then); a sound loaded while it is on (ui.c load_end: cset_loaded) is made POLY too, its
 * own VOICE kept instead. A track set to MONO / LEGATO / UNISON by hand plays the chord's lowest note.
 * The ISR builds the chords (keyboard_block, midi_play); the UI sets cset_en / cset_set a byte at a time. */
#include "cset_data.h"
#define CSET_WIN 53                              /* F3: the first key that plays the list's own voicing */
#define CSET_VNONE 0xFFu
static uint8_t cset_en;                          /* main: bit k, track k plays its chord set (CSET ON) */
static uint8_t cset_set[NTRK];                   /* main: each track's set, 0 .. CSET_N - 1 */
static uint8_t cset_voice[NTRK] = {CSET_VNONE, CSET_VNONE, CSET_VNONE, CSET_VNONE};   /* main: the VOICE before ON */
_Static_assert(NTRK == 4u && CHORD_MAX >= 4u, "a chord set's chord: four notes at most, as a key's chord holds");
static volatile struct {
    uint8_t trk, set, key, n, note[CHORD_MAX], gen;   /* the last chord played (the CSET page): its track, set, key */
} cset_last;

/* track t plays its chord set */
static int cset_on(const track_t *t) { return ((cset_en >> trk_index(t)) & 1u) && !chord_kit(t); }

/* key k's note with the octave buttons, unmapped (a chord set takes the key as it is) */
static uint32_t cset_key(uint32_t k) { return (uint32_t)clamp(CSET_WIN + (int32_t)k + 12 * song.octave, 0, 127); }

/* the chord note plays on track t, its notes moved by trn semitones -> out (ascending), their number (1 .. 4): the
 * chord of note's key in t's set, as many octaves from the list's voicing as note is from F3 .. E4; notes out of
 * 0..127 left out (none left: the note alone); a one-voice part: the chord's lowest note */
static uint32_t cset_make(const track_t *t, uint32_t note, int32_t trn, uint8_t *out)
{
    const uint8_t *c = CSET_NOTE[cset_set[trk_index(t)] % CSET_N][note % 12u];
    int32_t sh = (((int32_t)note - CSET_WIN + 120) / 12 - 10) * 12 + trn;   /* (whole octaves, rounded down) */
    uint32_t i, n = 0;
    for (i = 0; i < 4u && c[i]; i++) {
        int32_t x = c[i] + sh;
        if (x >= 0 && x <= 127)
            out[n++] = (uint8_t)x;
    }
    if (!n) {
        out[0] = (uint8_t)(note > 127u ? 127u : note);
        return 1;
    }
    return trk_vmode(t) != V_POLY ? 1u : n;
}

/* cset_make for a note played now (a key, a MIDI note): kept as the last chord (the CSET page shows it) */
static uint32_t cset_build(const track_t *t, uint32_t note, int32_t trn, uint8_t *out)
{
    uint32_t n = cset_make(t, note, trn, out), i;
    cset_last.trk = (uint8_t)trk_index(t);
    cset_last.set = cset_set[trk_index(t)];
    cset_last.key = (uint8_t)(note % 12u);
    cset_last.n = (uint8_t)n;
    for (i = 0; i < CHORD_MAX; i++)
        cset_last.note[i] = i < n ? out[i] : 0u;
    cset_last.gen++;
    return n;
}

/* the name of chord `key` (0 C .. 11 B) of set s as the list names it ("Cadd9", "C#M9/C", "Eb") -> b (16 bytes) */
static void cset_name(char *b, uint32_t s, uint32_t key)
{
    const uint8_t *nm = CSET_NAME[s % CSET_N][key % 12u];
    uint32_t n = 0;
    b[n++] = "C?D?EF?G?A?B"[nm[0] & 15u];
    if (nm[0] & 0x30u)
        b[n++] = nm[0] & 0x10u ? '#' : 'b';
    b[n] = 0;
    str_cpy(b + n, CSET_QUAL[nm[1] % CSET_NQ], 16 - n);
}

/* KNOB 3: from set s to the first set of the next (d > 0) or this / the previous genre's run (the list's order:
 * POP, JAZZ, BLUES, .. JAZZ again later, MODERN); stops at the ends */
static uint32_t cset_run(uint32_t s)             /* the first set of s's run */
{
    while (s && CSET_GENRE_OF[s - 1u] == CSET_GENRE_OF[s])
        s--;
    return s;
}
static uint32_t cset_genre_step(uint32_t s, int32_t d)
{
    uint32_t r = cset_run(s % CSET_N);
    if (d < 0)
        return r != s % CSET_N || !r ? r : cset_run(r - 1u);
    for (s = r + 1u; s < CSET_N && CSET_GENRE_OF[s] == CSET_GENRE_OF[r]; s++)
        ;
    return s < CSET_N ? s : r;
}

/* KNOB 1: CSET on track t on / off: ON makes it POLY (its VOICE kept), OFF gives that back if it is still POLY;
 * what sounds is let go (as a sound load does: the voices' mode changes) */
static void cset_switch(track_t *t, uint32_t on)
{
    uint32_t k = trk_index(t);
    if (!on == !((cset_en >> k) & 1u))
        return;
    if (on) {
        cset_voice[k] = (uint8_t)t->p[P_VOICE];
        t->p[P_VOICE] = V_POLY;
        cset_en |= (uint8_t)(1u << k);
    } else {
        cset_en &= (uint8_t)~(1u << k);
        if (cset_voice[k] <= V_UNISON && t->p[P_VOICE] == V_POLY)
            t->p[P_VOICE] = cset_voice[k];
        cset_voice[k] = CSET_VNONE;
    }
    panic_req |= (uint8_t)(1u << k);
}

/* a sound loaded on track t (ui.c load_end): with CSET on it plays POLY, its own VOICE kept for OFF */
static void cset_loaded(track_t *t)
{
    uint32_t k = trk_index(t);
    if (!((cset_en >> k) & 1u) || t->p[P_VOICE] == V_POLY)
        return;
    cset_voice[k] = (uint8_t)t->p[P_VOICE];
    t->p[P_VOICE] = V_POLY;
}
