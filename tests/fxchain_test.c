/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The SLICER's place in the chain (firmware/src/slicer.c, fx.c), on the firmware's own sources (ui_test.c):
 *   SLICER   at the START (GATE STUT PUMP) before the DRIVE: a half-closed gate on a driven sound stays loud (the
 *            drive's input gated, not its output). Nothing after the reverb any more: the retired END values (4 GATE END,
 *            5 PUMP END, in old projects) leave the mix bit for bit as OFF.
 *   PUMP     the level drops by DEPTH at every step (ramped, from the step's first sample) and comes back along PAT's
 *            curve over its share of the step (SMOOTH PUNCH LINEAR HOLD; all, 3/4, 1/2, 1/4); no clicks with the
 *            settings turned while it runs; RATE 1/4 a beat a step; the SLICER page draws the curve (its signature
 *            follows PAT and DEPTH).
 * Built and run by tests/run_tests.sh (a directory argument: the screens). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("fxchain: %-90s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

/* ------------------------------------------------------------------------------------------- the buses --- */

static void bus_reset(void)
{
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    memset(rev_comb, 0, sizeof rev_comb);
    memset(&rev_u, 0, sizeof rev_u);
    memset(&cld, 0, sizeof cld);
    memset(&hl, 0, sizeof hl);
    memset(&fx, 0, sizeof fx);
    song.g[G_RTYPE] = 0;
    song.g[G_RSIZE] = 60;
    song.g[G_RDAMP] = 40;
    song.g[G_BPM] = 120;
    song.g[G_DTIME] = 2;
    song.g[G_DFDBK] = 0;
    song.g[G_DMIX] = 100;
    song.g[G_DCOLOR] = 127;
    song.g[G_CRATE] = 20;
    song.g[G_CDEPTH] = 40;
    perf_act = 0;
    chos.type = CHO_CHORUS;
    chos.fdbk = 0;
    fx1s.type = FX1_DELAY;
    fx1s.size = 50; fx1s.dens = 80; fx1s.spread = 0; fx1s.pitch = 0; fx1s.rnd = 0; fx1s.fdbk = 0; fx1s.mix = 100;
}

static uint32_t eng_by_name(const char *n)
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (str_eq(ENGINES[e]->name, n))
            return e;
    return 0;
}
static uint32_t preset_by_name(uint32_t e, const char *n)
{
    uint32_t k;
    for (k = 0; k < ENGINES[e]->npresets; k++)
        if (str_eq(ENGINES[e]->presets[k].name, n))
            return k;
    return 0;
}
/* the boot state, the FX quiet, every track an ANALOG SOFT PAD with no sends, no SLICER */
static void sl_fresh(void)
{
    uint32_t k, e = eng_by_name("ANALOG");
    ui_power_on();
    stop_transport();
    host_tracks_init();
    bus_reset();
    song.g[G_RTYPE] = 0;
    memset(&pf, 0, sizeof pf);
    for (k = 0; k < NTRK; k++)
        pf.mg[k] = 32768;
    memset(ins, 0, sizeof ins);
    memset(sl, 0, sizeof sl);
    lim_env = LIM_T;
    dc_l = dc_r = dce_l = dce_r = 0;
    for (k = 0; k < NTRK; k++) {
        host_preset(&trk[k], e, preset_by_name(e, "SOFT PAD"));
        trk[k].p[P_CHOR] = trk[k].p[P_DLY] = trk[k].p[P_REV] = 0;
        trk[k].p[P_SLCR] = SL_OFF;
        trk[k].p[P_ITYPE] = IT_OFF;
    }
    song.g[G_BPM] = 120;
    slicer_start();
}
static void set_sl(uint32_t k, int mode, int pat, int rate, int depth)
{
    trk[k].p[P_SLCR] = (int16_t)mode;
    trk[k].p[P_SLPAT] = (int16_t)pat;
    trk[k].p[P_SLRATE] = (int16_t)rate;
    trk[k].p[P_SLDEPTH] = (int16_t)depth;
}
static int32_t ob[2 * CTL];
/* blocks of the mix; the mean |left| over the middle of the open and of the closed steps of track k's clock (*op, *cl) */
static void sl_measure(uint32_t blocks, uint32_t k, double *op, double *cl, uint32_t from)
{
    double so = 0, sc2 = 0;
    uint32_t no = 0, nc = 0, b, i;
    for (b = 0; b < blocks; b++) {
        mix_block(ob, CTL);
        if (b < from)
            continue;
        if (sl[k].pos < SL_RAMP + CTL || sl[k].len - sl[k].pos < SL_RAMP + CTL)
            continue;                                   /* (the ramps out) */
        for (i = 0; i < CTL; i++) {
            if (sl[k].bit)
                so += abs(ob[2u * i]), no++;
            else
                sc2 += abs(ob[2u * i]), nc++;
        }
    }
    *op = no ? so / no : 0;
    *cl = nc ? sc2 / nc : 0;
}
static void test_slicer_pos(void)
{
    double op, cl, op2, cl2;
    char what[200];
    /* before the DRIVE: DEPTH 64 (the gate half closed) on a pad through a SOFT DRIVE 127: the closed steps nearly as loud
     * as the open ones (the drive's input halved, its output saturated alike); the drive before it would halve them */
    sl_fresh();
    trk[0].p[P_ITYPE] = IT_SOFT;
    trk[0].p[P_IA] = 127;
    set_sl(0, SL_GATE, 1, 0, 64);
    trk_note_on(&trk[0], 48, 120);
    trk_note_on(&trk[0], 55, 120);
    sl_measure(FS * 3u / CTL, 0, &op, &cl, FS / CTL);
    sl_fresh();
    set_sl(0, SL_GATE, 1, 0, 64);
    trk_note_on(&trk[0], 48, 120);
    trk_note_on(&trk[0], 55, 120);
    sl_measure(FS * 3u / CTL, 0, &op2, &cl2, FS / CTL);
    snprintf(what, sizeof what, "SLICER before the DRIVE: DEPTH 64, closed / open steps on a SOFT DRIVE 127 %.2f, without it "
             "%.2f (after the drive both alike)", cl / op, cl2 / op2);
    ck(what, op > 100 && cl2 / op2 > 0.4 && cl2 / op2 < 0.75 && cl / op > cl2 / op2 + 0.15);
    /* END retired: GATE END and PUMP END (4, 5, still stored in old projects) play as OFF: track 2 on them, track 1's pad
     * through the mix as loud in track 2's closed steps as in its open ones (GATE END: closed / open < 0.01) and just
     * after a step's start as at its end (PUMP END: ducked to < 0.05); track 2's own gate stays open */
    {
        int m;
        for (m = 4; m <= 5; m++) {
            double lo = 0, hi = 0;
            uint32_t b, i, nl = 0, nh = 0;
            sl_fresh();
            set_sl(1, m, 1, 6, 127);
            trk_note_on(&trk[0], 48, 120);
            sl_measure(FS * 3u / CTL, 1, &op, &cl, FS / CTL);
            for (b = 0; b < 2u * FS / CTL; b++) {
                double s2 = 0;
                mix_block(ob, CTL);
                for (i = 0; i < CTL; i++)
                    s2 += abs(ob[2u * i]);
                if (sl[1].pos > SL_RAMP + CTL && sl[1].pos < SL_RAMP + 40u * CTL)   /* (PUMP END: still ducked here) */
                    lo += s2, nl++;
                if (sl[1].pos > sl[1].len - 40u * CTL)
                    hi += s2, nh++;
            }
            lo /= nl ? nl : 1;
            hi /= nh ? nh : 1;
            snprintf(what, sizeof what, "SLICER: the retired END value %d plays as OFF: by track 2's closed / open steps %.3f, "
                     "a step's start / end %.3f", m, cl / op, lo / hi);
            ck(what, op > 100 && cl / op > 0.8 && cl / op < 1.25 && lo > hi * 0.8 && !sl[1].gc);
        }
        ck("SLICER: its modes OFF GATE STUT PUMP, nothing after (the page's range ends at PUMP)",
           SL_NMODE == 4 && TP[P_SLCR].max == SL_PUMP && str_eq(TP[P_SLCR].names[SL_PUMP], "PUMP"));
    }
}

/* --------------------------------------------------------------------------------------------- PUMP --- */
/* a constant (or a sine of f Hz, f > 0) through track 1's SLICER alone for blocks: the gain sample by sample into pg[], the
 * step's position of each into pp[], its index into pi[] */
#define PGN (4u * FS)
static int32_t pg[PGN], ppos[PGN], pidx[PGN];
static int32_t pump_run(uint32_t frames, double f, int turn)
{
    uint32_t t, i;
    int32_t prev = 0, mx = 0;
    track_t *tk = &trk[0];
    memset(&sl[0], 0, sizeof sl[0]);
    slicer_start();
    for (t = 0; t < frames; t += CTL) {
        int32_t b[CTL];
        if (turn && t % 3008u == 0u) {                  /* PAT, DEPTH, RATE while it runs */
            uint32_t k = t / 3008u;
            set_sl(0, SL_PUMP, (int)(1u + k * 5u % 16u), (int)(k % 7u), (int)(40u + k * 37u % 88u));
        }
        for (i = 0; i < CTL; i++)
            b[i] = f > 0 ? (int32_t)lrint(20000 * sin(2 * M_PI * f * (double)(t + i) / FS)) : 20000;
        slicer_track(tk, b, CTL);
        for (i = 0; i < CTL && t + i < PGN; i++) {
            pg[t + i] = b[i];
            if (t + i) {
                int32_t d = abs(b[i] - prev);
                mx = d > mx ? d : mx;
            }
            prev = b[i];
        }
    }
    return mx;
}
/* the gain (of 20000) at fraction fr of step 6's length, from a run of PAT pat */
static double pump_at(uint32_t pat, int depth, double fr)
{
    uint32_t t;
    track_t *tk = &trk[0];
    sl_fresh();
    set_sl(0, SL_PUMP, (int)pat, 0, depth);
    memset(&sl[0], 0, sizeof sl[0]);
    slicer_start();
    for (t = 0; t < PGN; t++) {                         /* (a sample at a time: the step and the place in it known) */
        int32_t one = 20000;
        if (sl[0].pos >= sl[0].len)
            sl_enter(tk, &sl[0]);
        if (sl[0].idx == 6u && sl[0].pos == (uint32_t)(fr * sl[0].len)) {
            slicer_track(tk, &one, 1);
            return one / 20000.0;
        }
        slicer_track(tk, &one, 1);
    }
    return -1;
}
static void test_pump(const char *dir)
{
    char what[220];
    double a, z, m[4], e[4];
    uint32_t k;
    /* the drop: at the step's start still open, after SL_RAMP down by DEPTH */
    a = pump_at(1, 127, 0.0);
    z = pump_at(1, 127, (double)(SL_RAMP + 8) / 11025.0);
    snprintf(what, sizeof what, "PUMP: DEPTH 100 %%, the step's first sample %.3f, after the drop (SL_RAMP) %.3f", a, z);
    ck(what, a > 0.97 && z < 0.03);
    z = pump_at(1, 64, (double)(SL_RAMP + 8) / 11025.0);
    snprintf(what, sizeof what, "PUMP: DEPTH 50 %%: down to %.3f", z);
    ck(what, z > 0.45 && z < 0.55);
    /* the curves at the middle of their release (whole step: 1 SMOOTH 5 PUNCH 9 LINEAR 13 HOLD) and at the end */
    for (k = 0; k < 4u; k++) {
        m[k] = pump_at(1u + 4u * k, 127, 0.5);
        e[k] = pump_at(1u + 4u * k, 127, 0.995);
    }
    snprintf(what, sizeof what, "PUMP: the curves at half the step: PUNCH %.2f > SMOOTH %.2f ~ LINEAR %.2f > HOLD %.2f; back by the end "
             "(%.2f %.2f %.2f %.2f)", m[1], m[0], m[2], m[3], e[0], e[1], e[2], e[3]);
    ck(what, m[1] > m[0] + 0.1 && fabs(m[0] - m[2]) < 0.1 && m[2] > m[3] + 0.1 && e[0] > 0.95 && e[1] > 0.95 && e[2] > 0.95 &&
             e[3] > 0.9);
    a = pump_at(4, 127, 0.3);                           /* SMOOTH over a quarter of the step: back by 0.3 */
    z = pump_at(2, 127, 0.3);                           /* .. over 3/4: still down */
    snprintf(what, sizeof what, "PUMP: the release's length: PAT 4 (1/4 of the step) at 0.3 %.2f, PAT 2 (3/4) %.2f", a, z);
    ck(what, a > 0.97 && z < 0.8);
    /* clicks: a 110 Hz sine, the settings turned all along */
    {
        int32_t mx, own = 0;
        uint32_t i;
        sl_fresh();
        set_sl(0, SL_PUMP, 1, 1, 127);
        mx = pump_run(4u * FS, 110.0, 1);
        for (i = 1; i < 1000u; i++)
            own = abs((int32_t)lrint(20000 * sin(2 * M_PI * 110 * i / FS)) - (int32_t)lrint(20000 * sin(2 * M_PI * 110 * (i - 1) / FS))) > own
                      ? abs((int32_t)lrint(20000 * sin(2 * M_PI * 110 * i / FS)) - (int32_t)lrint(20000 * sin(2 * M_PI * 110 * (i - 1) / FS)))
                      : own;
        snprintf(what, sizeof what, "PUMP: a 110 Hz sine, PAT DEPTH RATE turned all along: the largest step %.2f x the sine's own",
                 (double)mx / own);
        ck(what, mx < 3 * own);
    }
    /* RATE 1/4: a beat a step */
    sl_fresh();
    set_sl(0, SL_PUMP, 1, 6, 127);
    song.g[G_BPM] = 120;
    sl_enter(&trk[0], &sl[0]);
    snprintf(what, sizeof what, "PUMP: RATE 1/4 at 120 BPM: a step of %u samples (22050)", sl[0].base);
    ck(what, sl[0].base == 22050u && str_eq(TP[P_SLRATE].names[6], "1/4"));
    /* the SLICER page draws the curve; its signature follows PAT and DEPTH */
    {
        uint32_t s0, s1, s2;
        sl_fresh();
        set_sl(0, SL_PUMP, 1, 6, 127);
        track_select(0);
        open_family(FAM_FX);
        while (!str_eq(cur_page()->title, "SLICER"))
            page_step(1);
        frame();
        s0 = graph_signature();
        trk[0].p[P_SLPAT] = 9;
        s1 = graph_signature();
        trk[0].p[P_SLDEPTH] = 64;
        s2 = graph_signature();
        ck("PUMP: the SLICER page's signature follows PAT and DEPTH", s0 != s1 && s1 != s2);
        if (dir) {
            trk[0].p[P_SLPAT] = 1;
            trk[0].p[P_SLDEPTH] = 110;
            screen_save(dir, "slicer_pump");
        }
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : 0;
    ui_power_on();
    test_slicer_pos();
    test_pump(dir);
    printf("fxchain: %s\n", bad ? "FAILED" : "all ok");
    return bad ? 1 : 0;
}
