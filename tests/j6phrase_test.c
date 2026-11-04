/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* ARP 3 (firmware/src/j6phrase.c, the arp timed by the Roland J-6's phrases), on the firmware's own sources (ui_test.c's
 * harness):
 *   DATA     the 108 phrases of j6phrase_data.h (tools/gen_j6phrase.py): 9 STYLE x 12 VARIATION, each note inside its
 *            cycle, sorted, a chord tone 0..3; the Phrase List's notated ones as it notates them (STYLE 4 VAR 2 _o__:
 *            one chord on the second 16th of each beat; STYLE 5 VAR 9 __o: on the third 16th of a 3/16 cycle).
 *   PARAMS   STYLE (OFF .. STR R) and VAR (1..12) appended before the engine values, OFF the default; MODE J-6 appended
 *            to the arp's modes; STYLE OFF plays the arp as before; J-6 without a STYLE as CHORD.
 *   TIMING   MODE J-6 over the chord the J-6 held (G3 A#3 D4 F4), every one of the 108 phrases through a whole cycle and
 *            into the next: every note starts in the block its tick falls in, as the note the J-6 played, and ends
 *            after its length (x GATE / 64), the accented at velocity 127, the others 100; the rest before the first
 *            note kept (STYLE 4 VAR 4 ___o: the first chord on the fourth 16th).
 *   MODES    another MODE takes the phrase's rhythm: UP over C E G steps once a moment (a chord is one step), as long
 *            as its first note; CHORD the held notes; one key held: the chord tones in octaves; PROB 0 plays nothing.
 *   ENDS     every note started ends: the keys let go, STYLE OFF and MODE OFF over a sounding phrase, the track
 *            silenced (arp_forget); HOLD keeps it playing with no key down; a fresh chord starts the phrase over.
 *   KEPT     STYLE and VAR with the project (FUN10 by count), an older project's tracks OFF.
 *   PAGE     ARP > ARP 3 after ARP 2 (the ARP button cycles ARP, ARP 2, ARP 3), KNOB 1 STYLE, KNOB 2 VAR.
 *   With a directory: the ARP 3 screens.
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
    t->p[P_VOICE] = V_POLY;
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
/* the phrase ph (MODE J-6 over the J-6's chord) through `cycles` cycles: every note in the block of its tick, the
 * chord tone the data names, its length; 1 if all as expected */
static int phrase_plays(track_t *t, uint32_t ph, uint32_t cycles, int verbose)
{
    uint32_t beat = beat_samples(), cyc = J6PH[ph].cycle, first = J6PH[ph].first, n = J6PH[ph].n;
    uint32_t total = (cycles * (cyc * beat / 24u) + CTL - 1u) / CTL, b, i, k, got = 0, want = 0, vels = 0;   /* (up to the
                                                                          * next cycle's start, not into it) */
    static uint8_t seen[2048];
    start_t st[16];
    int ok = 1;
    arp_set(t, AM_J6, (int32_t)(ph / 12u) + 1, (int32_t)(ph % 12u) + 1, J6CHORD, 4);
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
             P_ASTYLE == P_SHIM + 1u && P_AVAR == P_ASTYLE + 1u && P_E0 == P_AVAR + 1u;
    uint32_t i;
    ck("PARAMS: STYLE OFF..STR R (OFF default), VAR 1..12, appended before the engine values", ok);
    ck("PARAMS: MODE J-6 appended to the arp's modes (CHORD keeps 14)", TP[P_AMODE].max == AM_J6 && AM_J6 == 15 &&
       AM_CHORD == 14 && str_eq(TP[P_AMODE].names[AM_J6], "J-6"));
    arp_set(t, AM_UP, 0, 1, CEG, 3);                     /* STYLE OFF: the arp's own steps, RATE */
    ok = 1;
    for (i = 0; i < 4u; i++) {
        t->arp_pos = 0xFFFFFFF;
        arp_tick(t, CTL);
        ok &= t->arp_note == CEG[i % 3u] && !j6run[0].on;
    }
    ck("PARAMS: STYLE OFF: the arp as before (UP over C E G: C E G C, no phrase)", ok);
    arp_set(t, AM_J6, 0, 1, CEG, 3);                     /* J-6 without a STYLE: as CHORD */
    t->arp_pos = 0xFFFFFFF;
    arp_tick(t, CTL);
    ck("PARAMS: MODE J-6 with STYLE OFF plays as CHORD", t->arp_note == 60u && t->arp_ch[0] == 64u && t->arp_ch[1] == 67u);
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
        arp_set(t, AM_J6, 4, 4, J6CHORD, 4);
        for (b = 0; b <= due + 2u && !first; b++)
            if (step_block(t, st, 16))
                first = b + 1u;
        ck("TIMING: STYLE 4 VAR 4 (___o): the first chord on the fourth 16th, the rest before it kept", first == due + 1u);
    }
    {   /* GATE scales the lengths: 32 half the J-6's */
        start_t st[16];
        uint32_t b, n = 0, e = J6PH_EV[J6PH[3u * 12u].first];
        arp_set(t, AM_J6, 4, 1, J6CHORD, 4);
        t->p[P_AGATE] = 32;
        for (b = 0; b < 3u && !n; b++)
            n = step_block(t, st, 16);
        ck("TIMING: GATE 32 halves the notes' lengths", n == 4u &&
           st[0].len == (((e >> 9) & 511u) * beat_samples() / 24u) * 32u / 64u);
        t->p[P_AGATE] = 64;
    }
    arp_set(t, AM_OFF, 0, 1, J6CHORD, 0);
}

static void mode_tests(track_t *t)
{
    static const uint8_t CEG[3] = {60, 64, 67}, C[1] = {60};
    start_t st[16];
    uint32_t b, n, i, seq[8], ns = 0, beat = beat_samples(), lenok = 1;
    /* UP over C E G with STYLE 4 VAR 1 (o___: a chord on each beat): one step a beat, C E G C */
    arp_set(t, AM_UP, 4, 1, CEG, 3);
    for (b = 0; b < (4u * 24u * beat / 24u) / CTL && ns < 8u; b++) {
        n = step_block(t, st, 16);
        for (i = 0; i < n && ns < 8u; i++) {
            seq[ns++] = st[i].note;
            lenok &= st[i].len == ((J6PH_EV[J6PH[36].first] >> 9) & 511u) * beat / 24u;
        }
    }
    if (!(ns == 4u && seq[0] == 60u && seq[1] == 64u && seq[2] == 67u && seq[3] == 60u && lenok))
        for (printf("    UP: %u notes, len ok %u:", (unsigned)ns, (unsigned)lenok), i = 0; i < ns; i++)
            printf(" %u%s", (unsigned)seq[i], i + 1u == ns ? "\n" : "");
    ck("MODES: UP takes the phrase's rhythm, a chord one step (o___: C E G C, one a beat, as long as the chord)",
       ns == 4u && seq[0] == 60u && seq[1] == 64u && seq[2] == 67u && seq[3] == 60u && lenok);
    /* CHORD: the held notes together on each moment */
    arp_set(t, AM_CHORD, 4, 1, CEG, 3);
    for (b = 0, n = 0; b < 3u && !n; b++)
        n = step_block(t, st, 16);
    ck("MODES: CHORD plays the held notes together on the phrase's moments", n == 3u);
    /* one key held under J-6: the chord tones in octaves (deg / held: 0 1 2 3 -> C C' C'' C''') */
    arp_set(t, AM_J6, 4, 1, C, 1);
    for (b = 0, n = 0; b < 3u && !n; b++)
        n = step_block(t, st, 16);
    ck("MODES: J-6 with one key: the four chord tones as that key in octaves", n == 4u && st[0].note == 60u &&
       st[1].note == 72u && st[2].note == 84u && st[3].note == 96u);
    /* a triad under J-6: the fourth tone the root an octave up */
    arp_set(t, AM_J6, 4, 1, CEG, 3);
    for (b = 0, n = 0; b < 3u && !n; b++)
        n = step_block(t, st, 16);
    ck("MODES: J-6 over a triad: C E G and the fourth tone C an octave up", n == 4u && st[0].note == 60u &&
       st[1].note == 64u && st[2].note == 67u && st[3].note == 72u);
    /* PROB: once a moment, as the arp's (rng & 127 <= PROB: 0 still lets 1 in 128 through, as ARP 2's PROB does) */
    arp_set(t, AM_J6, 2, 1, J6CHORD, 4);                 /* (ARP 16 UP1: a note every 16th, 64 moments in 4 bars) */
    t->p[P_APROB] = 0;
    for (b = 0, n = 0; b < (4u * 96u * beat / 24u) / CTL; b++)
        n += step_block(t, st, 16);
    ck("MODES: PROB 0 plays (almost) none of the phrase's 64 moments in four bars", n <= 4u);
    arp_set(t, AM_J6, 2, 1, J6CHORD, 4);
    t->p[P_APROB] = 63;
    for (b = 0, n = 0; b < (4u * 96u * beat / 24u) / CTL; b++)
        n += step_block(t, st, 16);
    ck("MODES: PROB 63 about half of them", n >= 16u && n <= 48u);
    arp_set(t, AM_OFF, 0, 1, CEG, 0);
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
    arp_set(t, AM_J6, 8, 1, J6CHORD, 4);
    for (b = 0; b < note_due(7u * 12u, 3u) + 2u; b++)
        step_block(t, st, 16);
    n = j6run[0].on;
    arp_remove(t, 55); arp_remove(t, 58); arp_remove(t, 62); arp_remove(t, 65);
    arp_tick(t, CTL);
    ck("ENDS: keys let go mid-strum: every phrase note ends", n == 4u && !j6run[0].on && !gates_on(t));
    arp_set(t, AM_J6, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    t->p[P_ASTYLE] = 0;
    arp_tick(t, CTL);
    ck("ENDS: STYLE OFF over a sounding phrase: its notes end (J-6 plays as CHORD: the four held)", !j6run[0].on &&
       t->arp_note == 55u && gates_on(t) == 4u);
    arp_set(t, AM_J6, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    t->p[P_AMODE] = AM_OFF;
    events_block(CTL);                                   /* (the ISR: ARP turned off drops the chord) */
    ck("ENDS: MODE OFF over a sounding phrase: its notes end", !j6run[0].on && !gates_on(t));
    arp_set(t, AM_J6, 8, 1, J6CHORD, 4);
    for (b = 0; b < 4u; b++)
        step_block(t, st, 16);
    trk_all_off(t);
    arp_forget(t);
    ck("ENDS: the track silenced (trk_all_off, arp_forget): nothing left in the phrase", !j6run[0].on && !gates_on(t));
    /* HOLD: the chord latched, no key down: the phrase goes on; a new chord starts it over */
    arp_set(t, AM_J6, 4, 2, J6CHORD, 4);
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
    trk[1].p[P_ASTYLE] = 7; trk[1].p[P_AVAR] = 11; trk[2].p[P_ASTYLE] = 4;
    project_capture(&a);
    ck("KEPT: STYLE and VAR in the FUN10 project (round trip)", proj_pack(&st, &a) && proj_import(&q, &st, sizeof st) &&
       q.t[1].p[P_ASTYLE] == 7 && q.t[1].p[P_AVAR] == 11 && q.t[2].p[P_ASTYLE] == 4 && q.t[0].p[P_ASTYLE] == 0);
    {   /* a FUN10 of the parameters before ARP 3 (by count): STYLE OFF, VAR 1, the engine values where they were */
        uint32_t span = P_COUNT + 2u + NSTEP_FMT * 9u, old = span - 2u, t, sum;
        static project_store_t o;
        memcpy(&o, &st, sizeof o);
        for (t = 0; t < NTRK; t++) {                  /* each track's record two bytes shorter: P_ASTYLE, P_AVAR out */
            uint8_t *src = st.raw + 68u + t * span, *dst = o.raw + 68u + t * old;
            memmove(dst, src, P_ASTYLE);
            memmove(dst + P_ASTYLE, src + P_ASTYLE + 2u, span - P_ASTYLE - 2u);
        }
        memmove(o.raw + 68u + NTRK * old, st.raw + 68u + NTRK * span, PROJ_FM6_OFF - (68u + NTRK * span));
        o.raw[66] = (uint8_t)(P_COUNT - 2u);
        sum = proj_hash(o.raw, sizeof o.raw - 4u);
        memcpy(o.raw + sizeof o.raw - 4u, &sum, 4);
        ck("KEPT: an older project (2 parameters fewer) loads its tracks with STYLE OFF, VAR 1, engine values kept",
           proj_import(&q, &o, sizeof o) && q.t[1].p[P_ASTYLE] == 0 && q.t[1].p[P_AVAR] == 1 &&
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
            found = str_eq(PAGES[i + 1u].title, "ARP 3") && PAGES[i + 1u].fam == FAM_ARP &&
                    PAGES[i + 1u].graph == GR_ARPH && PAGES[i + 1u].id[0] == P_ASTYLE && PAGES[i + 1u].id[1] == P_AVAR;
    ck("PAGE: ARP 3 after ARP 2 in the ARP family, KNOB 1 STYLE, KNOB 2 VAR", found);
    fam_next(FAM_ARP);
    frames(400);
    ck("PAGE: ARP opens ARP", str_eq(cur_page()->title, "ARP"));
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP again: ARP 2", str_eq(cur_page()->title, "ARP 2"));
    fam_next(FAM_ARP); frames(400);
    ck("PAGE: ARP again: ARP 3", str_eq(cur_page()->title, "ARP 3"));
    if (dir)
        screen_save(dir, "arp3-off");
    turn(EN_K1, 6); frame();
    turn(EN_K2, 2); frame();
    ck("PAGE: KNOB 1 STYLE (CHD S), KNOB 2 VAR (3)", TSEL->p[P_ASTYLE] == 6 && TSEL->p[P_AVAR] == 3);
    TSEL->p[P_AMODE] = AM_J6;
    frame();
    if (dir)
        screen_save(dir, "arp3-chd-s-3");
    TSEL->p[P_ASTYLE] = 9; TSEL->p[P_AVAR] = 1; TSEL->p[P_AMODE] = AM_OFF;
    frame();
    if (dir)
        screen_save(dir, "arp3-str-r-1-mode-off");
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
    mode_tests(t);
    end_tests(t);
    kept_tests();
    page_tests(argc > 1 ? argv[1] : 0);
    printf(bad ? "j6phrase: %d FAILED\n" : "j6phrase: all ok\n", bad);
    return bad ? 1 : 0;
}
