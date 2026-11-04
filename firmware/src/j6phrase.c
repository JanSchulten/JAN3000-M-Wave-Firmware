/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, ARP > J-6: one of the Roland J-6's 108 phrases (STYLE 1..9 x VARIATION 1..12, its owner's manual's
 * Phrase List) played over the held notes instead of the arp, as a J-6 plays them (j6phrase_data.h: recorded from one,
 * tools/gen_j6phrase.py). Included by seq.c after the arp's modes and before its note list (arp_list).
 *
 * The J-6 is on while STYLE is not OFF (OFF: the default, and every project and preset before it). Then the keys go to
 * it whatever the arp's MODE (seq.c arp_on) and the arp rests: MODE, RATE, OCT, GATE, SWG, PROB and ORD keep their
 * values but do nothing until STYLE is OFF again. HOLD is the arp's (ARP 2's, on the J-6 page too): it latches the
 * chord. A note of the phrase is a chord tone (0 the lowest of the four the J-6 held when recorded) and an octave above
 * it: the held notes, low to high (after TRN, the KEY, a CSET chord: arp_list), give the chord tones, a tone past the
 * held ones going round them again an octave up (one key held: every tone that key, in octaves). Each note starts
 * where the phrase has it and lasts as long as the J-6 held it; the accented ones sound at velocity 127 (the J-6's
 * ACCENT VELOCITY at its highest), the others at 100.
 *
 * The phrase starts from its beginning (its rest before the first note included) on the first key of a chord (after
 * every key was let go: arp_add sets arp_idx all ones), and when STYLE or VAR change; it then loops for as long as
 * notes are held, on the arp's clock (n per block: the external clock's while it runs). SNAP ON: it starts on the next
 * bar's one instead (seq.c bar_to_one: the sequencer's bar while it plays, else the ARP LED's free beat), silent
 * until then; a chord changed with keys still down plays on in the running phrase.
 *
 * The audio ISR's cost: arp_step tests STYLE once a block per part; everything else is here, out of line
 * (tests/target_budget.py), a few multiplies a block while a phrase plays and a note list (arp_list) per onset. Up
 * to J6_SLOTS notes of a phrase sound at once (the J-6's longest overlap is 7: a strum ringing into the next): each
 * its own note-off. A note started again while it sounds ends first; with every slot taken the one nearest its end
 * gives way. Every note started gets its note-off: the phrase's own length, the keys let go, STYLE off, the transport
 * (j6_all_off from arp_off / arp_silence), the track silenced (arp_forget: trk_all_off ended them). */
#include "j6phrase_data.h"

static uint32_t beat_samples(void);                 /* (fx.c) */
static uint32_t arp_list(const track_t *t, uint32_t *list, uint32_t *cnt);
static void arp_off(track_t *t);
static uint32_t bar_to_one(void);                   /* (seq.c: SNAP's one) */

#define J6_SLOTS 8u
#define J6_TICKS 24u                     /* the phrases' ticks a quarter (MIDI clock's 24 PPQN) */
#define J6_ACC_VEL 127u
#define J6_VEL 100u

typedef struct {
    uint32_t pos;                        /* samples into the cycle (the arp's n: its clock) */
    uint32_t wait;                       /* SNAP: samples to the bar's one the phrase starts on, 0 = playing */
    uint32_t off[J6_SLOTS];              /* samples to each sounding note's end */
    uint8_t note[J6_SLOTS];              /* the notes sounding, 0 = a free slot */
    uint8_t idx;                         /* the phrase's next note */
    uint8_t ph;                          /* the phrase playing + 1, 0 = none */
    uint8_t on;                          /* notes sounding (the guards' quick test) */
    uint8_t nh;                          /* the moment's chord tones (held[]: the held notes low to high) */
    uint16_t step_t;                     /* the moment (onset tick + 1) held[] was taken for, 0 = none */
    uint8_t held[16];
} j6run_t;
static j6run_t j6run[NTRK];

static int j6_active(const track_t *t)   /* the J-6 plays instead of the arp: a STYLE */
{
    return t->p[P_ASTYLE] > 0 && t->p[P_ASTYLE] <= 9;
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
    r->wait = 0;
}
static void j6_forget(track_t *t)        /* (trk_all_off ended them) */
{
    j6run_t *r = &j6run[trk_index(t)];
    memset(r->note, 0, sizeof r->note);
    r->on = 0;
    r->ph = 0;
    r->wait = 0;
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
/* the playing phrase's place in its cycle, 0..255 (the J-6 panel's cursor); 0 when none plays or it waits */
static uint32_t j6_place(const track_t *t)
{
    const j6run_t *r = &j6run[trk_index(t)];
    uint32_t cyc;
    if (!r->ph || r->wait || !t->nheld)
        return 0;
    cyc = J6PH[r->ph - 1u].cycle * beat_samples() / J6_TICKS;
    return cyc ? (r->pos < cyc ? r->pos : cyc - 1u) * 256u / cyc : 0u;
}
static int j6_waiting(const track_t *t)  /* SNAP: a chord held, waiting for the bar's one */
{
    return j6run[trk_index(t)].wait != 0u && t->nheld;
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

/* note e of the phrase (J6PH_EV) starts: its chord tone over the held notes */
static void j6_fire(track_t *t, j6run_t *r, uint32_t e, uint32_t beat)
{
    uint32_t list[64], cnt, i, k, x, deg = (e >> 18) & 3u, oct = (e >> 20) & 3u, onset = e & 511u;
    if (r->step_t != onset + 1u) {          /* a new moment: the held notes low to high (ORD PLAY keeps them as played) */
        r->step_t = (uint16_t)(onset + 1u);
        r->nh = 0;
        if (!arp_list(t, list, &cnt))
            return;                         /* (every key silent now: QNT WHITE's black keys) */
        cnt = cnt > 16u ? 16u : cnt;
        for (i = 0; i < cnt; i++) {
            x = list[i];
            for (k = i; k > 0 && r->held[k - 1u] > x; k--)
                r->held[k] = r->held[k - 1u];
            r->held[k] = (uint8_t)x;
        }
        r->nh = (uint8_t)cnt;
    }
    if (!r->nh)
        return;
    x = r->held[deg % r->nh] + 12u * (oct + deg / r->nh);   /* the chord tone, round the held notes again */
    j6_start(t, r, x > 127u ? 127u : x, (e >> 22) & 1u ? J6_ACC_VEL : J6_VEL, ((e >> 9) & 511u) * beat / J6_TICKS);
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
        r->wait = 0;
        return;
    }
    if (r->ph != ph + 1u || t->arp_idx == 0xFFFFFFFFu) {   /* a fresh chord, another phrase: from its start */
        r->ph = (uint8_t)(ph + 1u);
        r->pos = 0;
        r->idx = 0;
        r->step_t = 0;
        t->arp_idx = 0;
        r->wait = t->p[P_J6SNAP] ? bar_to_one() : 0u;   /* SNAP: from the next bar's one */
        if (r->wait)
            return;
    } else if (r->wait) {                   /* SNAP: the one comes in this block's span, or later */
        if (r->wait > n) {
            r->wait -= n;
            return;
        }
        r->pos = n - r->wait;               /* (the one fell in the last block: that far into the phrase now) */
        r->wait = 0;
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
