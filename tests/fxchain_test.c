/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The SLICER's place in the chain (firmware/src/slicer.c, fx.c), on the firmware's own sources (ui_test.c):
 *   SLICER   at the START (GATE STUT PUMP) before DIST: a half-closed gate on a driven sound stays loud (DIST's
 *            input gated, not its output); at the END (GATE END, PUMP END) on the whole sum after the reverb: another
 *            track's sound and the reverb's tail gated by the driving track's pattern, the track itself silent; two on
 *            END: the lower drives; a track on END not gated at the start; END turned OFF while closed: it opens
 *            without a click.
 * Built and run by tests/run_tests.sh. */
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
    memset(&sle, 0, sizeof sle);
    lim_env = LIM_T;
    dc_l = dc_r = dce_l = dce_r = 0;
    for (k = 0; k < NTRK; k++) {
        host_preset(&trk[k], e, preset_by_name(e, "SOFT PAD"));
        trk[k].p[P_CHOR] = trk[k].p[P_DLY] = trk[k].p[P_REV] = 0;
        trk[k].p[P_SLCR] = SL_OFF;
        trk[k].p[P_DIST] = 0;
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
    uint32_t b, i;
    /* START before DIST: DEPTH 64 (the gate half closed) on a pad through DIST 127: the closed steps nearly as loud as the
     * open ones (DIST's input halved, its output saturated alike); DIST before it (as it was) would halve them */
    sl_fresh();
    trk[0].p[P_DIST] = 127;
    set_sl(0, SL_GATE, 1, 0, 64);
    trk_note_on(&trk[0], 48, 120);
    trk_note_on(&trk[0], 55, 120);
    sl_measure(FS * 3u / CTL, 0, &op, &cl, FS / CTL);
    sl_fresh();
    trk[0].p[P_DIST] = 0;
    set_sl(0, SL_GATE, 1, 0, 64);
    trk_note_on(&trk[0], 48, 120);
    trk_note_on(&trk[0], 55, 120);
    sl_measure(FS * 3u / CTL, 0, &op2, &cl2, FS / CTL);
    snprintf(what, sizeof what, "SLICER at the START, before DIST: DEPTH 64, closed / open steps on DIST 127 %.2f, without DIST "
             "%.2f (after DIST both alike)", cl / op, cl2 / op2);
    ck(what, op > 100 && cl2 / op2 > 0.4 && cl2 / op2 < 0.75 && cl / op > cl2 / op2 + 0.15);
    /* END: track 2 on GATE END (silent itself), track 1 playing: the whole sum gated by track 2's pattern */
    sl_fresh();
    set_sl(1, SL_GEND, 1, 0, 127);
    trk_note_on(&trk[0], 48, 120);
    sl_measure(FS * 3u / CTL, 1, &op, &cl, FS / CTL);
    snprintf(what, sizeof what, "SLICER at the END: track 2 GATE END (silent), track 1 playing: closed / open %.3f", cl / op);
    ck(what, op > 100 && cl / op < 0.01);
    ck("SLICER at the END: .. track 2 drove it (the closure from its clock)", sle.gc >= 0 && !sl[1].gc && !sl[0].gc);
    /* the reverb's tail in it: the pad sent to the reverb, its note let go: the tail gated too */
    sl_fresh();
    song.g[G_RTYPE] = 2;
    song.g[G_RSIZE] = 110;
    trk[0].p[P_REV] = 127;
    set_sl(1, SL_GEND, 1, 0, 127);
    trk_note_on(&trk[0], 48, 120);
    for (b = 0; b < FS / CTL; b++)
        mix_block(ob, CTL);
    trk_note_off(&trk[0], 48);
    for (b = 0; b < FS / 2u / CTL; b++)                /* (the voice's release over: the tail alone) */
        mix_block(ob, CTL);
    sl_measure(FS / CTL, 1, &op, &cl, 0);
    snprintf(what, sizeof what, "SLICER at the END: the reverb's tail gated too (closed / open %.3f, open %.0f)", cl / op, op);
    ck(what, op > 20 && cl / op < 0.02);
    /* two tracks on END with opposite patterns (1: x.x., 13: .x.x): the lower (track 2) drives */
    sl_fresh();
    set_sl(1, SL_GEND, 1, 0, 127);
    set_sl(2, SL_GEND, 13, 0, 127);
    trk_note_on(&trk[0], 48, 120);
    sl_measure(FS * 3u / CTL, 1, &op, &cl, FS / CTL);
    sl_measure(FS * 3u / CTL, 2, &op2, &cl2, 0);
    snprintf(what, sizeof what, "SLICER at the END: tracks 2 and 3 on it, the lower drives (by 2's steps %.3f, by 3's %.1f)",
             cl / op, cl2 / (op2 + 1));
    ck(what, cl / op < 0.01 && cl2 > 20 * op2);
    /* a track on END is not gated at the start: track 1 itself on GATE END, depth 127 */
    sl_fresh();
    set_sl(0, SL_GEND, 1, 0, 127);
    trk_note_on(&trk[0], 48, 120);
    {
        int32_t gmax = 0;
        for (b = 0; b < FS / CTL; b++) {
            mix_block(ob, CTL);
            gmax = sl[0].gc > gmax ? sl[0].gc : gmax;
        }
        ck("SLICER: a track on END is not gated at the start (its own gate stays open)", !gmax && sle.gc >= 0);
    }
    /* END turned OFF in a closed step: it opens over SL_RAMP, no click */
    sl_fresh();
    set_sl(1, SL_GEND, 7, 0, 127);                      /* (x... : three closed steps in four) */
    trk_note_on(&trk[0], 48, 120);
    for (b = 0; b < FS / CTL; b++)
        mix_block(ob, CTL);
    while (sl[1].bit || sl[1].pos < SL_RAMP + CTL)
        mix_block(ob, CTL);
    {
        int32_t prev = ob[2u * (CTL - 1u)], mx = 0, own = 0;
        uint32_t opened = 0;
        trk[1].p[P_SLCR] = SL_OFF;
        for (b = 0; b < 40u; b++) {
            mix_block(ob, CTL);
            for (i = 0; i < CTL; i++) {
                int32_t st = abs(ob[2u * i] - prev);
                prev = ob[2u * i];
                if (b < 8u)
                    mx = st > mx ? st : mx;
                else
                    own = st > own ? st : own;
            }
            if (!sle.gc && !opened)
                opened = b + 1u;
        }
        snprintf(what, sizeof what, "SLICER: END turned OFF while closed: open after %u blocks, steps %d (the pad's own %d)", opened,
                 mx, own);
        ck(what, opened >= 3u && opened <= 6u && mx <= own + own / 2);
    }
}


int main(void)
{
    ui_power_on();
    test_slicer_pos();
    printf("fxchain: %s\n", bad ? "FAILED" : "all ok");
    return bad ? 1 : 0;
}
