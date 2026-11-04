/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, ARP 3: the arp timed by one of the Roland J-6's 108 phrases (STYLE 1..9 x VARIATION 1..12, its owner's
 * manual's Phrase List), as a J-6 plays them (j6phrase_data.h: recorded from one, tools/gen_j6phrase.py). Included by
 * seq.c after the arp's note list (arp_list) and its steps (arp_pick).
 *
 * With STYLE OFF (the default, and every project and preset before it) the arp plays as it always did. With a STYLE
 * and the arp on (MODE not OFF), the phrase takes over the timing: its notes start where the phrase has them, last
 * as long as it holds them (x GATE / 64: GATE's default 64 is the J-6's own length) and the accented ones sound at
 * velocity 127 (the J-6's ACCENT VELOCITY at its highest), the others at the arp's 100. RATE and SWG rest; OCT,
 * PROB, HOLD and ORD keep their meaning. What it plays:
 *   - MODE J-6 (appended to the arp's modes): the phrase's own notes, the way the J-6 voices them over a chord. A
 *     note of the phrase is a chord tone (0 the lowest of the four the J-6 held) and an octave above it; here the
 *     held notes, low to high (after TRN, the KEY, a CSET chord: arp_list), give the chord tones, a tone past the
 *     held ones going round them again an octave up (one key held: every tone that key, in octaves). Chords, strums
 *     and runs come out as the J-6 plays them. With STYLE OFF, J-6 plays as CHORD.
 *   - any other MODE: the phrase's rhythm. Each moment it starts notes on (a chord, a strum's first string) is one
 *     step of the arp: MODE's next note, CHORD's chord, for as long as the phrase's first note there lasts.
 * The phrase starts from its beginning (its rest before the first note included) on the first key of a chord, as
 * the arp does (arp_add: arp_idx all ones), and when STYLE or VAR change; it then loops for as long as notes are held,
 * on the arp's clock (n per block: the external clock's while it runs, as the arp's RATE).
 *
 * The audio ISR's cost: arp_step tests STYLE and MODE once a block per part; everything else is here, out of line
 * (tests/target_budget.py), a few multiplies a block while a phrase plays and a note list (arp_list) per onset. Up
 * to J6_SLOTS notes of a phrase sound at once (the J-6's longest overlap is 7: a strum ringing into the next): each
 * its own note-off. A note started again while it sounds ends first; with every slot taken the one nearest its end
 * gives way. Every note started gets its note-off: the phrase's own length, the keys let go, STYLE / MODE off, the
 * transport (j6_all_off from arp_off / arp_silence), the track silenced (arp_forget: trk_all_off ended them). */
#include "j6phrase_data.h"

static uint32_t beat_samples(void);                 /* (fx.c) */
static uint32_t arp_list(const track_t *t, uint32_t *list, uint32_t *cnt);
static uint32_t arp_pick(track_t *t, uint32_t *list, uint32_t len);
static void arp_off(track_t *t);

#define J6_SLOTS 8u
#define J6_TICKS 24u                     /* the phrases' ticks a quarter (MIDI clock's 24 PPQN) */
#define J6_ACC_VEL 127u
#define J6_VEL 100u

typedef struct {
    uint32_t pos;                        /* samples into the cycle (the arp's n: its clock) */
    uint32_t off[J6_SLOTS];              /* samples to each sounding note's end */
    uint8_t note[J6_SLOTS];              /* the notes sounding, 0 = a free slot */
    uint8_t idx;                         /* the phrase's next note */
    uint8_t ph;                          /* the phrase playing + 1, 0 = none */
    uint8_t on;                          /* notes sounding (the guards' quick test) */
    uint8_t step_ok;                     /* the moment plays (PROB, once a moment) */
    uint16_t step_t;                     /* the moment (onset tick + 1) PROB and the step were taken for, 0 = none */
    uint8_t nh, held[16];                /* MODE J-6: the moment's chord tones, the held notes low to high */
} j6run_t;
static j6run_t j6run[NTRK];

static int j6_active(const track_t *t)   /* ARP 3 times the arp */
{
    return t->p[P_ASTYLE] > 0 && t->p[P_ASTYLE] <= 9 && t->p[P_AMODE] != 0;
}
static uint32_t j6_phrase(const track_t *t)   /* J6PH's index of STYLE / VAR */
{
    uint32_t s = (uint32_t)clamp(t->p[P_ASTYLE], 1, 9) - 1u, v = (uint32_t)clamp(t->p[P_AVAR], 1, 12) - 1u;
    return s * 12u + v;
}

static void j6_end(track_t *t, j6run_t *r, uint32_t k, int rec)   /* slot k's note ends */
{
    if (rec)
        rec_release(t, r->note[k]);
    trk_note_off(t, r->note[k]);
    r->note[k] = 0;
    r->on--;
}

/* every phrase note ends (rec: a recording takes the note-offs: arp_off), the phrase starts again next time */
static __attribute__((noinline)) void j6_all_off(track_t *t, int rec)
{
    j6run_t *r = &j6run[trk_index(t)];
    uint32_t k;
    for (k = 0; k < J6_SLOTS && r->on; k++)
        if (r->note[k])
            j6_end(t, r, k, rec);
    r->on = 0;
    r->ph = 0;
}
static void j6_forget(track_t *t)        /* (trk_all_off ended them) */
{
    j6run_t *r = &j6run[trk_index(t)];
    memset(r->note, 0, sizeof r->note);
    r->on = 0;
    r->ph = 0;
}

/* the notes a phrase sounds now (the UI: the ARP keys, the keys' LEDs), at most max; their count */
static uint32_t j6_sounding(const track_t *t, uint8_t *out, uint32_t max)
{
    const j6run_t *r = &j6run[trk_index(t)];
    uint32_t k, n = 0;
    for (k = 0; k < J6_SLOTS && n < max; k++)
        if (r->note[k])
            out[n++] = r->note[k];
    return n;
}
/* the playing phrase's place in its cycle, 0..255 (the ARP 3 panel's cursor); 0 when none plays */
static uint32_t j6_place(const track_t *t)
{
    const j6run_t *r = &j6run[trk_index(t)];
    uint32_t cyc;
    if (!r->ph || !r->on)
        return 0;
    cyc = J6PH[r->ph - 1u].cycle * beat_samples() / J6_TICKS;
    return cyc ? (r->pos < cyc ? r->pos : cyc - 1u) * 256u / cyc : 0u;
}

static void j6_start(track_t *t, j6run_t *r, uint32_t note, uint32_t vel, uint32_t len)   /* one note, len samples */
{
    uint32_t k, free = J6_SLOTS, near = 0;
    if (!note || note > 127u)
        return;
    for (k = 0; k < J6_SLOTS; k++) {
        if (r->note[k] == note) {           /* (sounding: ends first, its slot reused) */
            j6_end(t, r, k, rec_on(t));
            free = k;
            break;
        }
        if (!r->note[k] && free == J6_SLOTS)
            free = k;
        if (r->note[k] && r->off[k] < r->off[near])
            near = k;
    }
    if (free == J6_SLOTS) {                 /* (every slot sounding: the one nearest its end gives way) */
        j6_end(t, r, near, rec_on(t));
        free = near;
    }
    r->note[free] = (uint8_t)note;
    r->off[free] = len ? len : 1u;
    r->on++;
    if (rec_on(t))
        rec_note(t, note, vel, 0);          /* recording: what the phrase plays */
    trk_note_on(t, note, vel);
}

/* note e of the phrase (J6PH_EV) starts: MODE J-6 its chord tone, another MODE an arp step on its onset */
static void j6_fire(track_t *t, j6run_t *r, uint32_t e, uint32_t beat)
{
    uint32_t list[64], cnt, len, i, j, k, x, deg = (e >> 18) & 3u, oct = (e >> 20) & 3u;
    uint32_t vel = (e >> 22) & 1u ? J6_ACC_VEL : J6_VEL, mode = (uint32_t)t->p[P_AMODE];
    uint32_t onset = e & 511u, samples = (((e >> 9) & 511u) * beat / J6_TICKS) * (uint32_t)t->p[P_AGATE] / 64u;
    if (r->step_t == onset + 1u) {          /* the moment's other notes: its step and PROB were taken */
        if (mode != AM_J6 || !r->step_ok || !r->nh)
            return;
    } else {
        r->step_t = (uint16_t)(onset + 1u);
        r->step_ok = (uint32_t)(rng() & 127u) <= (uint32_t)t->p[P_APROB];
        r->nh = 0;
        if (!r->step_ok || !(len = arp_list(t, list, &cnt)))
            return;                         /* (PROB said no; every key silent now: QNT WHITE's black keys) */
        if (mode == AM_J6) {                /* the held notes low to high (ORD PLAY keeps them as played: sorted here) */
            cnt = cnt > 16u ? 16u : cnt;
            for (i = 0; i < cnt; i++) {
                x = list[i];
                for (k = i; k > 0 && r->held[k - 1u] > x; k--)
                    r->held[k] = r->held[k - 1u];
                r->held[k] = (uint8_t)x;
            }
            r->nh = (uint8_t)cnt;
        }
    }
    if (mode != AM_J6) {                    /* one arp step a moment, as long as the phrase's first note there; arp_idx
                                             * the step (0 the chord's first, as arp_fire's), counted on after it */
        if (mode == AM_CHORD) {             /* CHORD: the held notes (the first 4), an octave up each step through OCT */
            uint32_t o = t->arp_idx % (uint32_t)(t->p[P_AOCT] > 0 ? t->p[P_AOCT] : 1);
            for (i = 0; i < cnt && i < 4u; i++)
                j6_start(t, r, (uint32_t)clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127), vel, samples);
        } else {
            j = arp_pick(t, list, len);
            j6_start(t, r, list[j], vel, samples);
        }
        t->arp_idx++;
        return;
    }
    x = r->held[deg % r->nh] + 12u * (oct + deg / r->nh);   /* J-6: the chord tone, round the held notes again */
    j6_start(t, r, x > 127u ? 127u : x, vel, samples);
}

/* a block of the phrase (n samples of its clock, gate_n of the notes' lengths: arp_step's) */
static __attribute__((noinline)) void j6_step(track_t *t, uint32_t n, uint32_t gate_n)
{
    j6run_t *r = &j6run[trk_index(t)];
    uint32_t ph = j6_phrase(t), beat = beat_samples(), k, cyc, first, cnt, lap;
    if (t->arp_note)
        arp_off(t);                         /* (STYLE turned on over the arp's own note) */
    for (k = 0; k < J6_SLOTS && r->on; k++)  /* the notes' lengths run in real time, as the arp's gate */
        if (r->note[k]) {
            if (r->off[k] <= gate_n)
                j6_end(t, r, k, rec_on(t));
            else
                r->off[k] -= gate_n;
        }
    if (!t->nheld) {                        /* (the keys let go: the notes end, the next chord starts afresh) */
        if (r->on)
            j6_all_off(t, rec_on(t));
        r->ph = 0;
        return;
    }
    if (r->ph != ph + 1u || t->arp_idx == 0xFFFFFFFFu) {   /* a fresh chord, another phrase: from its start */
        r->ph = (uint8_t)(ph + 1u);
        r->pos = 0;
        r->idx = 0;
        r->step_t = 0;
        t->arp_idx = 0;
    } else {
        r->pos += n;
    }
    first = J6PH[ph].first;
    cnt = J6PH[ph].n;
    cyc = J6PH[ph].cycle * beat / J6_TICKS;
    for (lap = 0; lap < 2u; lap++) {        /* the notes due, then (a cycle ended) the rest of it and the new one's */
        while (r->idx < cnt) {
            uint32_t e = J6PH_EV[first + r->idx];
            if (r->pos < cyc && (e & 511u) * beat / J6_TICKS > r->pos)
                break;
            r->idx++;
            j6_fire(t, r, e, beat);
        }
        if (r->pos < cyc || !cyc)
            break;
        r->pos -= cyc;
        if (r->pos >= cyc)
            r->pos = 0;                     /* (a tempo jump: from the cycle's start) */
        r->idx = 0;
        r->step_t = 0;
    }
}
