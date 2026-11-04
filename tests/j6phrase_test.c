/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* ARP > J-6 (firmware/src/j6phrase.c, the Roland J-6's phrases instead of the arp), on the firmware's own sources
 * (ui_test.c's harness):
 *   DATA     the 108 phrases of j6phrase_data.h (tools/gen_j6phrase.py): 9 STYLE x 12 VARIATION, each note inside its
 *            cycle, sorted, a chord tone 0..3; the Phrase List's notated ones as it notates them (STYLE 4 VAR 2 _o__:
 *            one chord on the second 16th of each beat; STYLE 5 VAR 9 __o: on the third 16th of a 3/16 cycle).
 *   PARAMS   STYLE (OFF .. STR R), VAR (1..12) and SNAP appended before the engine values, OFF the default; the arp's
 *            modes as before; STYLE OFF: the arp as before; a sound load keeps them as it keeps the arp's.
 *   TIMING   every one of the 108 phrases over the chord the J-6 held (G3 A#3 D4 F4) through a whole cycle and into
 *            the next: every note starts in the block its tick falls in, as the note the J-6 played, lasts as long,
 *            the accented at velocity 127, the others 100; the rest before the first note kept; another tempo.
 *   ALONE    the J-6 plays with the arp's MODE OFF and whatever MODE, RATE, GATE, PROB, OCT say (the same notes); one
 *            key held: the chord tones in octaves; a triad: the fourth tone the root an octave up.
 *   SNAP     stopped, mid-bar: the phrase starts on the next one of the free beat (the ARP LED's), silent till then;
 *            on the one: at once; OFF: at once; playing: on the sequencer's next one; a chord changed with keys down
 *            plays on; the ARP LED's beat kept on the sequencer's while it plays.
 *   ENDS     every note started ends: the keys let go, STYLE OFF over a sounding phrase (the arp's MODE OFF: nothing;
 *            CHORD: the arp again), the track silenced (arp_forget); HOLD (ARP 2's) keeps it playing with no key down.
 *   KEPT     STYLE, VAR and SNAP with the project (FUN10 by count), an older project's tracks OFF.
 *   PAGE     ARP > J-6 after ARP 2 (the ARP button cycles ARP, ARP 2, J-6), KNOB 1 STYLE, 2 VAR, 3 SNAP, 4 HOLD.
 *   With a directory: the J-6 screens.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("j6phrase: %-98s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static const uint8_t J6CHORD[4] = {55, 58, 62, 65};   /* G3 A#3 D4 F4: the chord the J-6 held when recorded */

static void arp_set(track_t *t, int32_t mode, int32_t style, int32_t var, const uint8_t *notes, uint32_t n)
{
    uint32_t i;
    if (arp_sounding(t))
        arp_off(t);
    t->nheld = 0; t->arp_phys = 0;
    t->p[P_AMODE] = (int16_t)mode; t->p[P_ASTYLE] = (int16_t)style; t->p[P_AVAR] = (int16_t)var;
    t->p[P_AOCT] = 1; t->p[P_APROB] = 127; t->p[P_AGATE] = 64; t->p[P_AHOLD] = 0; t->p[P_AORDER] = 0;
    t->p[P_VOICE] = V_POLY; t->p[P_J6SNAP] = 0;
    song.playing = 0; beat_n = 0; beat_pos = 0;          /* (the free beat on its one: SNAP starts at once) */
    for (i = 0; i < n; i++)
        arp_add(t, notes[i]);
}

/* one block of the arp; what started in it: the slots whose note is new or whose length was set again */
typedef struct { uint8_t note, vel; uint32_t len; } start_t;
static uint32_t step_block(track_t *t, start_t *st, uint32_t max)
{
    j6run_t *r = &j6run[trk_index(t)], before = *r;
    uint32_t k, n = 0, i;
    arp_tick(t, CTL);
    for (k = 0; k < J6_SLOTS; k++)
        if (r->note[k] && (r->note[k] != before.note[k] || r->off[k] + CTL > before.off[k]) && n < max) {
            st[n].note = r->note[k];
            st[n].len = r->off[k];
            st[n].vel = 0;
            for (i = 0; i < NVOICE; i++)
                if (t->v[i].active && t->v[i].gate && t->v[i].note == r->note[k])
                    st[n].vel = t->v[i].vel;
            n++;
        }
    return n;
}

/* the block note k of phrase ph (k / n cycles on) starts in: the cycle's samples (cycle x beat / 24, as j6_step
 * counts them) times the cycles before it, its tick's samples, rounded up to the block whose start reaches it */
static uint32_t note_due(uint32_t ph, uint32_t k)
{
    uint32_t beat = beat_samples(), n = J6PH[ph].n, e = J6PH_EV[J6PH[ph].first + k % n];
    uint32_t at = (k / n) * (J6PH[ph].cycle * beat / 24u) + (e & 511u) * beat / 24u;
    return (at + CTL - 1u) / CTL;
}
/* the phrase ph (over the J-6's chord, the arp's MODE OFF) through `cycles` cycles: every note in the block of its tick, the
 * chord tone the data names, its length; 1 if all as expected */
static int phrase_plays(track_t *t, uint32_t ph, uint32_t cycles, int verbose)
{
    uint32_t beat = beat_samples(), cyc = J6PH[ph].cycle, first = J6PH[ph].first, n = J6PH[ph].n;
    uint32_t total = (cycles * (cyc * beat / 24u) + CTL - 1u) / CTL, b, i, k, got = 0, want = 0, vels = 0;   /* (up to the
                                                                          * next cycle's start, not into it) */
    static uint8_t seen[2048];
    start_t st[16];
    int ok = 1;
    arp_set(t, AM_OFF, (int32_t)(ph / 12u) + 1, (int32_t)(ph % 12u) + 1, J6CHORD, 4);
    memset(seen, 0, sizeof seen);
    for (b = 0; b < total; b++) {
        uint32_t ns = step_block(t, st, 16);
        for (i = 0; i < ns; i++) {
            int found = 0;
            got++;
            for (k = 0; k < n * cycles && !found; k++) {   /* the note due in this block, not taken yet */
                uint32_t e = J6PH_EV[first + k % n], due = note_due(ph, k);
                uint32_t note = J6CHORD[(e >> 18) & 3u] + 12u * ((e >> 20) & 3u);
                uint32_t len = (((e >> 9) & 511u) * beat / 24u);
                if (!seen[k] && note == st[i].note && b == due && st[i].len == (len ? len : 1u) &&
                    (!st[i].vel || st[i].vel == ((e >> 22) & 1u ? 127u : 100u))) {   /* (vel 0: its voice stolen) */
                    vels += st[i].vel != 0;
                    seen[k] = 1;
                    found = 1;
                }
            }
            if (!found) {
                ok = 0;
                if (verbose)
                    printf("    phrase %u block %u: note %u len %u vel %u unexpected\n", (unsigned)ph, (unsigned)b,
                           (unsigned)st[i].note, (unsigned)st[i].len, (unsigned)st[i].vel);
            }
        }
    }
    for (k = 0; k < n * cycles; k++)
        want += note_due(ph, k) < total;
    if (got && !vels)
        ok = 0;                                         /* (no velocity seen at all) */
    if (got != want) {
        ok = 0;
        if (verbose)
            printf("    phrase %u: %u notes started, %u due\n", (unsigned)ph, (unsigned)got, (unsigned)want);
    }
    return ok;
}

static void data_tests(void)
{
    uint32_t i, k, total = 0, sorted = 1, inside = 1;
    for (i = 0; i < J6PH_N; i++) {
        total += J6PH[i].n;
        for (k = 0; k < J6PH[i].n; k++) {
            uint32_t e = J6PH_EV[J6PH[i].first + k];
            inside &= (e & 511u) < J6PH[i].cycle && ((e >> 9) & 511u) >= 1u && J6PH[i].first + k < J6PH_NEV;
            if (k)
                sorted &= (J6PH_EV[J6PH[i].first + k - 1u] & 511u) <= (e & 511u);
        }
        inside &= J6PH[i].n > 0 && J6PH[i].cycle > 0;
        if (i)
            inside &= J6PH[i].first == J6PH[i - 1u].first + J6PH[i - 1u].n;
    }
    ck("DATA: 108 phrases (9 STYLE x 12 VAR), 1404 notes, back to back", J6PH_N == 108u && total == J6PH_NEV && total == 1404u);
    ck("DATA: every note inside its cycle, at least a tick long, sorted by time", inside && sorted);
    {   /* STYLE 4 VAR 2, _o__: the four chord tones on the second 16th of each beat */
        uint32_t ph = 3u * 12u + 1u, ok = J6PH[ph].cycle == 24u && J6PH[ph].n == 4u;
        for (k = 0; k < 4u; k++)
            ok &= (J6PH_EV[J6PH[ph].first + k] & 511u) == 6u && ((J6PH_EV[J6PH[ph].first + k] >> 18) & 3u) == k;
        ck("DATA: STYLE 4 VAR 2 (_o__): the chord on the second 16th of a beat, its rest kept", ok);
        ph = 4u * 12u + 8u;                              /* STYLE 5 VAR 9, __o: a 3/16 cycle, the chord on its third */
        ck("DATA: STYLE 5 VAR 9 (__o): a 3/16 cycle, the chord on its third 16th", J6PH[ph].cycle == 18u &&
           (J6PH_EV[J6PH[ph].first] & 511u) == 12u);
    }
}

static void param_tests(track_t *t)
{
    static const uint8_t CEG[3] = {60, 64, 67};
    int ok = TP[P_ASTYLE].max == 9 && TP[P_ASTYLE].def == 0 && str_eq(TP[P_ASTYLE].names[0], "OFF") &&
             str_eq(TP[P_ASTYLE].names[4], "PHR A") && str_eq(TP[P_ASTYLE].names[9], "STR R") &&
             TP[P_AVAR].min == 1 && TP[P_AVAR].max == 12 && TP[P_AVAR].def == 1 &&
             TP[P_J6SNAP].max == 1 && TP[P_J6SNAP].def == 0 && str_eq(TP[P_J6SNAP].label, "SNAP") &&
             P_ASTYLE == P_SHIM + 1u && P_AVAR == P_ASTYLE + 1u && P_J6SNAP == P_AVAR + 1u && P_E0 == P_J6SNAP + 1u;
    uint32_t i;
    ck("PARAMS: STYLE OFF..STR R (OFF default), VAR 1..12, SNAP OFF / ON, appended before the engine values", ok);
    ck("PARAMS: the arp's modes as before (CHORD the last, 14)", TP[P_AMODE].max == AM_CHORD && AM_CHORD == 14);
    ck("PARAMS: a sound load keeps STYLE VAR SNAP, as the arp's", param_kept(P_ASTYLE) && param_kept(P_AVAR) &&
       param_kept(P_J6SNAP) && param_kept(P_AMODE) && !param_kept(P_E0));
    arp_set(t, AM_UP, 0, 1, CEG, 3);                     /* STYLE OFF: the arp's own steps, RATE */
    ok = 1;
    for (i = 0; i < 4u; i++) {
        t->arp_pos = 0xFFFFFFF;
        arp_tick(t, CTL);
        ok &= t->arp_note == CEG[i % 3u] && !j6run[0].on;
    }
    ck("PARAMS: STYLE OFF: the arp as before (UP over C E G: C E G C, no phrase)", ok);
    arp_set(t, AM_OFF, 0, 1, CEG, 0);
}

static void timing_tests(track_t *t)
{
    uint32_t ph, fails = 0, verbose = 3;
    for (ph = 0; ph < J6PH_N; ph++)
        if (!phrase_plays(t, ph, 2u, verbose > 0)) {
            fails++;
            if (verbose)
                verbose--;
        }
    ck("TIMING: all 108 phrases over the J-6's chord, two cycles: each note in its block, its tone, length, velocity", !fails);
    if (fails)
        printf("    %u phrases off\n", (unsigned)fails);
    song.g[G_BPM] = 93;                                  /* another tempo: the ticks follow it */
    ck("TIMING: at 93 BPM too (STYLE 8 VAR 1, a strum; STYLE 5 VAR 7, 3/16)", phrase_plays(t, 7u * 12u, 2u, 1) &&
       phrase_plays(t, 4u * 12u + 6u, 2u, 1));
    song.g[G_BPM] = 120;
    {   /* STYLE 4 VAR 4, ___o: nothing until the fourth 16th */
        start_t st[16];
        uint32_t b, first = 0, due = (18u * beat_samples() / 24u + CTL - 1u) / CTL;
        arp_set(t, AM_OFF, 4, 4, J6CHORD, 4);
        for (b = 0; b <= due + 2u && !first; b++)
            if (step_block(t, st, 16))
                first = b + 1u;
        ck("TIMING: STYLE 4 VAR 4 (___o): the first chord on the fourth 16th, the rest before it kept", first == due + 1u);
    }
    arp_set(t, AM_OFF, 0, 1, J6CHORD, 0);
}

/* the notes and lengths the first n blocks of a phrase start, as a list (for comparing two settings) */
static uint32_t record(track_t *t, uint32_t blocks, uint32_t *out, uint32_t max)
{
    start_t st[16];
    uint32_t b, i, n = 0;
    for (b = 0; b < blocks; b++) {
        uint32_t ns = step_block(t, st, 16);
        for (i = 0; i < ns && n < max; i++)
            out[n++] = b << 16 ^ st[i].note << 8 ^ st[i].vel ^ st[i].len * 2654435761u;
    }
    return n;
}
static void alone_tests(track_t *t)
{
    static const uint8_t CEG[3] = {60, 64, 67}, C[1] = {60};
    static uint32_t a[512], b[512];
    start_t st[16];
    uint32_t na, nb, blk, n = 0, beat = beat_samples();
    blk = (2u * 96u * beat / 24u) / CTL;
    arp_set(t, AM_OFF, 6, 3, J6CHORD, 4);
    na = record(t, blk, a, 512);
    arp_set(t, AM_UP, 6, 3, J6CHORD, 4);                 /* the arp on, set otherwise: ignored */
    t->p[P_ARATE] = 0; t->p[P_AGATE] = 20; t->p[P_APROB] = 0; t->p[P_AOCT] = 3; t->p[P_ASWING] = 80;
    nb = record(t, blk, b, 512);
    ck("ALONE: the J-6 plays with the arp's MODE OFF; MODE UP, RATE, GATE, PROB, OCT, SWG change nothing",
       na > 8u && na == nb && !memcmp(a, b, na * sizeof a[0]) && !t->arp_note);
    arp_set(t, AM_REPEAT, 4, 1, J6CHORD, 4);             /* (REPEAT would keep one note of the list: not here) */
    for (blk = 0, n = 0; blk < 3u && !n; blk++)
        n = step_block(t, st, 16);
    ck("ALONE: MODE REPEAT does not cut the chord down (all four tones)", n == 4u);
    arp_set(t, AM_OFF, 4, 1, C, 1);                      /* one key: the four chord tones as that key in octaves */
    for (blk = 0, n = 0; blk < 3u && !n; blk++)
        n = step_block(t, st, 16);
    ck("ALONE: one key held: the four chord tones as that key in octaves", n == 4u && st[0].note == 60u &&
       st[1].note == 72u && st[2].note == 84u && st[3].note == 96u);
    arp_set(t, AM_OFF, 4, 1, CEG, 3);                    /* a triad: the fourth tone the root an octave up */
    for (blk = 0, n = 0; blk < 3u && !n; blk++)
        n = step_block(t, st, 16);
    ck("ALONE: a triad: C E G and the fourth tone C an octave up", n == 4u && st[0].note == 60u &&
       st[1].note == 64u && st[2].note == 67u && st[3].note == 72u);
    arp_set(t, AM_OFF, 0, 1, CEG, 0);
}

/* the block the phrase's first notes start in (0: none in max blocks) */
static uint32_t first_block(track_t *t, uint32_t max)
{
    start_t st[16];
    uint32_t b;
    for (b = 0; b < max; b++)
        if (step_block(t, st, 16))
            return b + 1u;
    return 0;
}
static void snap_tests(track_t *t)
{
    uint32_t beat = beat_samples(), bar = 4u * beat, at, want, got, p = div_samples(2), i;
    /* stopped, the free beat on beat 3 + 100 samples: the next one is bar - at away */
    arp_set(t, AM_OFF, 4, 1, J6CHORD, 0);                /* (o___: a chord on the phrase's first tick) */
    t->p[P_J6SNAP] = 1;
    beat_n = 2; beat_pos = 100; at = 2u * beat + 100u;
    for (i = 0; i < 4u; i++) arp_add(t, J6CHORD[i]);
    want = (bar - at + CTL - 1u) / CTL + 1u;             /* (the one in the block before: it plays in the next) */
    got = first_block(t, want + 4u);
    ck("SNAP: stopped, mid-bar: silent, then the phrase starts on the free beat's next one", got == want);
    if (got != want) printf("    first notes in block %u, not %u\n", (unsigned)got, (unsigned)want);
    arp_set(t, AM_OFF, 4, 1, J6CHORD, 0);
    t->p[P_J6SNAP] = 1;
    beat_n = 0; beat_pos = 0;                            /* on the one: at once */
    for (i = 0; i < 4u; i++) arp_add(t, J6CHORD[i]);
    ck("SNAP: a chord on the one starts at once", first_block(t, 4u) == 1u);
    arp_set(t, AM_OFF, 4, 1, J6CHORD, 0);
    beat_n = 2; beat_pos = 100;                          /* SNAP OFF: at once, wherever the bar is */
    for (i = 0; i < 4u; i++) arp_add(t, J6CHORD[i]);
    ck("SNAP OFF: at once, mid-bar", first_block(t, 4u) == 1u);
    /* playing: the sequencer's bar (the click's 1/16 steps), step 13 + 50 samples */
    arp_set(t, AM_OFF, 4, 1, J6CHORD, 0);
    t->p[P_J6SNAP] = 1;
    song.playing = 1; clk_step = 13; clk_pos = 50; beat_n = 0; beat_pos = 0;
    for (i = 0; i < 4u; i++) arp_add(t, J6CHORD[i]);
    want = (16u * p - (13u * p + 50u) + CTL - 1u) / CTL + 1u;
    got = first_block(t, want + 4u);
    ck("SNAP: playing: on the sequencer's next one (not the free beat's)", got == want);
    if (got != want) printf("    first notes in block %u, not %u\n", (unsigned)got, (unsigned)want);
    {   /* a chord changed with a key still down: no new wait */
        uint32_t n0 = j6run[0].on;
        arp_remove(t, 55); arp_add(t, 53);
        ck("SNAP: a chord changed with keys down plays on (no new wait)", n0 && !j6_waiting(t) && first_block(t, 2u) <= 2u);
    }
    song.playing = 0;
    clk_pos = CLK_START;
    arp_set(t, AM_OFF, 0, 1, J6CHORD, 0);
    {   /* the ARP LED's beat on the sequencer's while it plays: through the ISR's blocks */
        uint32_t b, ok = 1;
        ui_power_on();
        song.g[G_BPM] = 120;
        song.g[G_BPM] = 120;
        transport_req = 1;
        for (b = 0; b < 3000u; b++) {
            events_block(CTL);
            if (song.playing && clk_pos != CLK_START)
                ok &= beat_n == clk_step >> 2 && beat_pos == (clk_step & 3u) * div_samples(2) + clk_pos + CTL;
        }
        ck("SNAP: playing, the ARP LED's beat is the sequencer's (beat, place in it)", ok && song.playing);
        transport_req = 2;
        events_block(CTL);
    }
}

static uint32_t gates_on(const track_t *t)             /* voices holding a note */
{
    uint32_t i, n = 0;
    for (i = 0; i < NVOICE; i++)
        n += t->v[i].active && t->v[i].gate;
    return n;
}
static void end_tests(track_t *t)
{
    start_t st[16];
    uint32_t b, n;
    /* STYLE 8 VAR 1: a strum ringing long (its four notes a tick apart); let go mid-note */
    arp_set(t, AM_OFF, 8, 1, J6CHORD, 4);
    for (b = 0; b < note_due(7u * 12u, 3u) + 2u; b++)
        step_block(t, st, 16);
    n = j6run[0].on;
    arp_remove(t, 55); arp_remove(t, 58); arp_remove(t, 62); arp_remove(t, 65);
    arp_tick(t, CTL);
    ck("ENDS: keys let go mid-strum: every phrase note ends", n == 4u && !j6run[0].on && !gates_on(t));
    arp_set(t, AM_OFF, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    events_block(CTL);                                   /* (the ISR sees the J-6 on: armp) */
    t->p[P_ASTYLE] = 0;
    events_block(CTL);                                   /* (the ISR: the J-6 off, the arp's MODE OFF: the chord dropped) */
    ck("ENDS: STYLE OFF, the arp's MODE OFF: the phrase's notes end, nothing more plays", !j6run[0].on && !gates_on(t) &&
       !t->nheld);
    arp_set(t, AM_CHORD, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    t->p[P_ASTYLE] = 0;
    t->arp_pos = 0xFFFFFFF;
    arp_tick(t, CTL);
    ck("ENDS: STYLE OFF, the arp's MODE CHORD: the phrase's notes end, the arp plays again", !j6run[0].on &&
       t->arp_note == 55u && gates_on(t) == 4u);
    arp_set(t, AM_OFF, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    trk_all_off(t);
    arp_forget(t);
    ck("ENDS: the track silenced (trk_all_off, arp_forget): nothing left in the phrase", !j6run[0].on && !gates_on(t));
    /* HOLD (ARP 2's): the chord latched, no key down: the phrase goes on */
    arp_set(t, AM_OFF, 4, 2, J6CHORD, 4);
    t->p[P_AHOLD] = 1;
    t->arp_phys = 4;
    arp_remove(t, 55); arp_remove(t, 58); arp_remove(t, 62); arp_remove(t, 65);
    for (b = 0, n = 0; b < (2u * 24u * beat_samples() / 24u) / CTL; b++)
        n += step_block(t, st, 16);
    ck("ENDS: HOLD keeps the phrase playing with no key down (two chords of _o__ in two beats)", n == 8u);
    t->p[P_AHOLD] = 0;
    arp_set(t, AM_OFF, 0, 1, J6CHORD, 0);
    arp_tick(t, CTL);
}

static void kept_tests(void)
{
    static project_t a, q;
    static project_store_t st;
    ui_power_on();
    trk[1].p[P_ASTYLE] = 7; trk[1].p[P_AVAR] = 11; trk[1].p[P_J6SNAP] = 1; trk[2].p[P_ASTYLE] = 4;
    project_capture(&a);
    ck("KEPT: STYLE, VAR and SNAP in the FUN10 project (round trip)", proj_pack(&st, &a) && proj_import(&q, &st, sizeof st) &&
       q.t[1].p[P_ASTYLE] == 7 && q.t[1].p[P_AVAR] == 11 && q.t[1].p[P_J6SNAP] == 1 && q.t[2].p[P_ASTYLE] == 4 &&
       q.t[0].p[P_ASTYLE] == 0 && !q.t[2].p[P_J6SNAP]);
    {   /* a FUN10 of the parameters before the J-6 (by count): STYLE OFF, VAR 1, SNAP OFF, the engine values kept */
        uint32_t span = P_COUNT + 2u + NSTEP_FMT * 9u, old = span - 3u, t, sum;
        static project_store_t o;
        memcpy(&o, &st, sizeof o);
        for (t = 0; t < NTRK; t++) {                  /* each track's record three bytes shorter */
            uint8_t *src = st.raw + 68u + t * span, *dst = o.raw + 68u + t * old;
            memmove(dst, src, P_ASTYLE);
            memmove(dst + P_ASTYLE, src + P_ASTYLE + 3u, span - P_ASTYLE - 3u);
        }
        memmove(o.raw + 68u + NTRK * old, st.raw + 68u + NTRK * span, PROJ_FM6_OFF - (68u + NTRK * span));
        o.raw[66] = (uint8_t)(P_COUNT - 3u);
        sum = proj_hash(o.raw, sizeof o.raw - 4u);
        memcpy(o.raw + sizeof o.raw - 4u, &sum, 4);
        ck("KEPT: an older project (3 parameters fewer) loads with STYLE OFF, VAR 1, SNAP OFF, engine values kept",
           proj_import(&q, &o, sizeof o) && q.t[1].p[P_ASTYLE] == 0 && q.t[1].p[P_AVAR] == 1 && !q.t[1].p[P_J6SNAP] &&
           q.t[1].p[P_E0] == a.t[1].p[P_E0] && q.t[3].p[P_E7] == a.t[3].p[P_E7]);
    }
}

static void page_tests(const char *dir)
{
    int found = 0;
    uint32_t i;
    ui_power_on();
    for (i = 0; i + 1u < NPAGES; i++)
        if (str_eq(PAGES[i].title, "ARP 2"))
            found = str_eq(PAGES[i + 1u].title, "J-6") && PAGES[i + 1u].fam == FAM_ARP && PAGES[i + 1u].graph == GR_ARPH &&
                    PAGES[i + 1u].id[0] == P_ASTYLE && PAGES[i + 1u].id[1] == P_AVAR && PAGES[i + 1u].id[2] == P_J6SNAP &&
                    PAGES[i + 1u].id[3] == P_AHOLD;
    ck("PAGE: J-6 after ARP 2 in the ARP family: STYLE, VAR, SNAP, HOLD (ARP 2's)", found);
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP opens ARP", str_eq(cur_page()->title, "ARP"));
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP again: ARP 2", str_eq(cur_page()->title, "ARP 2"));
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP again: J-6", str_eq(cur_page()->title, "J-6"));
    if (dir)
        screen_save(dir, "j6-off");
    turn(EN_K1, 6); frame();
    turn(EN_K2, 2); frame();
    turn(EN_K3, 1); frame();
    turn(EN_K4, 1); frame();
    ck("PAGE: KNOB 1 STYLE (CHD S), 2 VAR (3), 3 SNAP ON, 4 HOLD ON", TSEL->p[P_ASTYLE] == 6 && TSEL->p[P_AVAR] == 3 &&
       TSEL->p[P_J6SNAP] == 1 && TSEL->p[P_AHOLD] == 1);
    if (dir)
        screen_save(dir, "j6-chd-s-3-snap");
    TSEL->p[P_AHOLD] = 0;
    beat_n = 1; beat_pos = 0;                            /* a chord held mid-bar: SNAP waits */
    for (i = 0; i < 4u; i++) arp_add(TSEL, J6CHORD[i]);
    arp_tick(TSEL, CTL);
    frame();
    ck("PAGE: SNAP waiting for the one, said on the panel", j6_waiting(TSEL));
    if (dir)
        screen_save(dir, "j6-snap-waiting");
    TSEL->nheld = 0; arp_tick(TSEL, CTL);
    TSEL->p[P_ASTYLE] = 9; TSEL->p[P_AVAR] = 1;
    frame();
    if (dir)
        screen_save(dir, "j6-str-r-1");
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP again: back to ARP", str_eq(cur_page()->title, "ARP"));
}

int main(int argc, char **argv)
{
    track_t *t;
    ui_power_on();
    t = &trk[0];
    song.g[G_BPM] = 120;
    data_tests();
    param_tests(t);
    timing_tests(t);
    alone_tests(t);
    snap_tests(t);
    ui_power_on();
    t = &trk[0];
    song.g[G_BPM] = 120;
    end_tests(t);
    kept_tests();
    page_tests(argc > 1 ? argv[1] : 0);
    printf(bad ? "j6phrase: %d FAILED\n" : "j6phrase: all ok\n", bad);
    return bad ? 1 : 0;
}
