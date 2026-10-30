/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* HOME > MASTER (firmware/src/dyn.c, fx.c master_block, params.c dyn_set, ui_dyn.c graph_dyn), on the firmware's own
 * sources (ui_test.c's harness):
 *   AS BEFORE  the defaults (power-on, a new project, extras of before): no compressor, LIMIT at the old threshold; at
 *              MASTER's top the master's output is bit for bit what the master of before made of the same mix
 *   KNOB LAST  MASTER scales what leaves the master: at half the knob exactly half of the full output (the limiter and the
 *              soft clip work as hard at any volume); USB LEVEL FIXED: the full output whatever the knob
 *   CURVE      COMP's gain reduction on steady sines -30 .. 0 dBFS against the gain computer in double (threshold, ratio,
 *              the 6 dB knee); GAIN alone is a plain gain into the limiter
 *   TIMES      the reduction's attack and release (RELEASE 0, 50, 100) against their time constants
 *   LIMIT      its steps' thresholds (-15.2 .. -0.2 dBFS, LIM_T at the default), a loud sine held at the threshold
 *   METERS     the reductions dyn_poll shows (half dB) and their fall
 *   PAGE       HOME, SELECT: MIXER, CLOCK, EQ, MASTER, HOME; KNOB 1..4 COMP RELEASE LIMIT GAIN, the cards' text, the panel
 *              redraws on a knob and on the meters
 *   KEPT       saved with the project and loaded back, the autosave told; a damaged value: its default; extras of before
 *              and a new project: the defaults
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <math.h>

static int bad;
static int ck(const char *what, int ok)
{
    printf("dyn: %-104s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static void master_rest(void)                         /* the master's state as at power-on */
{
    lim_env = LIM_T;
    dc_l = dc_r = dce_l = dce_r = 0;
    fx_lowcut = 0;
    fx_usb_fixed = 0;
    song.master_q12 = MASTER_FULL;
}
static void set(int16_t comp, int16_t rel, int16_t lim, int16_t gain)
{
    dyn_set[DYN_COMP] = comp, dyn_set[DYN_REL] = rel, dyn_set[DYN_LIM] = lim, dyn_set[DYN_GAIN] = gain;
    dyn_poll();
}
/* a sine of amplitude a at f Hz into the mix, block by block from sample s0 (multiples of 4: the old master's x >> 2 x 4
 * keeps them) */
static void fill(double a, double f, uint32_t s0)
{
    uint32_t i;
    for (i = 0; i < CTL; i++) {
        int32_t x = (int32_t)lrint(a * sin(2 * M_PI * f * (s0 + i) / FS) / 4.0) * 4;
        mix_l[i] = x;
        mix_r[i] = x;
        wet[i] = 0;
    }
}
/* nb blocks of the sine through master_block; the output's peak over the last half */
static int32_t run(double a, double f, uint32_t nb)
{
    static int32_t o[2 * CTL];
    uint32_t b, i;
    int32_t pk = 0;
    for (b = 0; b < nb; b++) {
        fill(a, f, b * CTL);
        master_block(o, CTL);
        if (b >= nb / 2u)
            for (i = 0; i < 2u * CTL; i++)
                pk = abs(o[i]) > pk ? abs(o[i]) : pk;
    }
    return pk;
}
/* the gain computer in double: the reduction (dB <= 0) of a peak at in dBFS, COMP c */
static double ref_gr(double in, double c)
{
    double thr = -0.24 * c, slope = 1.0 / (1.0 + 0.05 * c) - 1.0, ov = in - thr, k = 3.0;
    return ov <= -k ? 0.0 : ov < k ? slope * (ov + k) * (ov + k) / (4.0 * k) : slope * ov;
}

/* --------------------------------------------------------- AS BEFORE --- */
static void test_before(void)
{
    static int32_t ref[400][2 * CTL], o[2 * CTL];
    uint32_t b, i;
    int ok, same = 1, half = 1, fixed = 1;
    ui_power_on();
    dyn_poll();
    ok = !dynx.on && lim_t == LIM_T && dyn_lim_t(DYN_LIM_DEF) == LIM_T && !memcmp(dyn_set, DYN_DEF, sizeof DYN_DEF);
    ck("AS BEFORE: power-on: COMP 0, RELEASE 50, LIMIT -5.2 dB (LIM_T), GAIN 0: no compressor", ok);
    master_rest();                                     /* the master of before, at MASTER's top: ((x >> 2) x 4096) >> 10 */
    for (b = 0; b < 400u; b++) {                       /* into master_out (a loud chord's worth: the limiter works) */
        fill(b < 200u ? 40000.0 : 9000.0, 220.0 + (b & 7u), b * CTL);
        for (i = 0; i < CTL; i++) {
            int32_t l = ((mix_l[i] >> 2) * MASTER_FULL) >> 10, r = ((mix_r[i] >> 2) * MASTER_FULL) >> 10;
            master_out(&l, &r);
            ref[b][2u * i] = l, ref[b][2u * i + 1u] = r;
        }
    }
    master_rest();
    for (b = 0; b < 400u; b++) {
        fill(b < 200u ? 40000.0 : 9000.0, 220.0 + (b & 7u), b * CTL);
        master_block(o, CTL);
        same &= !memcmp(o, ref[b], sizeof o);
    }
    ck("  at MASTER's top the output is bit for bit the master of before (the limiter at work, then not)", same);
    master_rest();
    song.master_q12 = 2048;
    for (b = 0; b < 400u; b++) {
        fill(b < 200u ? 40000.0 : 9000.0, 220.0 + (b & 7u), b * CTL);
        master_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++)
            half &= o[i] == (ref[b][i] * 2048) >> 12;
    }
    ck("KNOB LAST: at half the knob exactly half of the full output (the same limiting at any volume)", half);
    master_rest();
    fx_usb_fixed = 1;
    song.master_q12 = 1000;
    for (b = 0; b < 400u; b++) {
        fill(b < 200u ? 40000.0 : 9000.0, 220.0 + (b & 7u), b * CTL);
        master_block(o, CTL);
        fixed &= !memcmp(o, ref[b], sizeof o);
    }
    ck("  USB LEVEL FIXED: the full output whatever the knob (audio.c scales the DAC's after USB took it)", fixed);
    master_rest();
}

/* ------------------------------------------------------------- CURVE --- */
static void test_curve(void)
{
    static const int16_t COMPS[3] = {25, 50, 100};
    uint32_t k, c;
    int ok = 1;
    double worst = 0;
    ui_power_on();
    for (c = 0; c < 3u; c++)
        for (k = 0; k <= 10u; k++) {
            double in = -30.0 + 3.0 * k, a = 32768.0 * pow(10.0, in / 20.0), want = ref_gr(in, COMPS[c]);
            master_rest();
            set(COMPS[c], 50, DYN_LIM_DEF, 0);
            dyna.grs = 0, dyna.g = 1, dyna.pk = 0;
            run(a, 1000.0, 1500);                      /* (~1 s: settled) */
            worst = fabs(dyna.grs - want) > worst ? fabs(dyna.grs - want) : worst;
            ok &= fabs(dyna.grs - want) < 0.25;
        }
    printf("dyn:   (the largest error %.3f dB: the block's peak of a sampled sine)\n", worst);
    ck("CURVE: COMP 25 / 50 / 100 on sines -30 .. 0 dBFS: the reduction as threshold, ratio and knee say (0.25 dB)", ok);
    master_rest();
    set(50, 50, DYN_LIM_DEF, 0);
    dyna.grs = 0, dyna.g = 1;
    run(32768.0 * pow(10.0, -6.0 / 20.0), 1000.0, 1500);
    ok = fabs(dyna.grs - (1.0 / 3.5 - 1.0) * 6.0) < 0.25 && fabs(ref_gr(-6.0, 50) + 4.2857) < 1e-3;
    ck("  COMP 50: -12 dB at 3.5:1, a sine at -6 dBFS down 4.3 dB", ok);
    {
        int32_t p0, p6;
        master_rest();
        set(0, 50, DYN_LIM_DEF, 0);
        p0 = run(1000.0, 500.0, 200);
        master_rest();
        set(0, 50, DYN_LIM_DEF, 12);
        ok = dynx.on;
        p6 = run(1000.0, 500.0, 200);
        ok &= fabs((double)p6 / p0 - pow(10.0, 6.0 / 20.0)) < 0.02;
        printf("dyn:   (GAIN +6 dB: x%.4f)\n", (double)p6 / p0);
        ck("  GAIN alone (COMP 0): the compressor's gain stage on, a quiet sine +6 dB", ok);
    }
    master_rest();
}

/* ------------------------------------------------------------- TIMES --- */
/* blocks until the reduction passes frac of the way from its start to where it settles */
static double time_ms(double a0, double a1, int16_t rel, double frac)
{
    uint32_t b;
    double g0, g1;
    master_rest();
    set(100, rel, DYN_LIM_DEF, 0);
    dyna.grs = 0, dyna.g = 1, dyna.pk = 0;
    run(a0, 1000.0, 12000);
    g0 = dyna.grs;
    {
        float keep_grs = dyna.grs, keep_g = dyna.g;
        int32_t keep_pk = dyna.pk, keep_env = lim_env;
        run(a1, 1000.0, 12000);
        g1 = dyna.grs;
        dyna.grs = keep_grs, dyna.g = keep_g, dyna.pk = keep_pk, lim_env = keep_env;
    }
    for (b = 0; b < 3000u; b++) {
        static int32_t o[2 * CTL];
        fill(a1, 1000.0, b * CTL);
        master_block(o, CTL);
        if (fabs(dyna.grs - g0) >= frac * fabs(g1 - g0))
            break;
    }
    return (b + 1u) * 1000.0 * CTL / FS;
}
static void test_times(void)
{
    static const int16_t RELS[3] = {0, 50, 100};
    uint32_t k;
    int ok = 1;
    ui_power_on();
    for (k = 0; k < 3u; k++) {
        double rel = dyn_rel_ms(RELS[k]), att = rel / 20.0;
        double ta = time_ms(100.0, 30000.0, RELS[k], 1.0 - exp(-1.0));
        double tr = time_ms(30000.0, 100.0, RELS[k], 1.0 - exp(-1.0));
        double blk = 1000.0 * CTL / FS;
        printf("dyn:   (RELEASE %d: release %.0f ms measured %.1f; attack %.2f ms measured %.2f)\n", RELS[k], rel, tr, att,
               ta);
        ok &= tr > rel * 0.9 && tr < rel * 1.1 + 2 * blk;
        ok &= ta > att * 0.8 && ta < att + 3 * blk;    /* (a block late: the gain worked from the block before) */
    }
    ok &= dyn_rel_ms(0) == 20 && dyn_rel_ms(50) == 141 && dyn_rel_ms(100) == 1000;
    ck("TIMES: RELEASE 20 / 141 / 1000 ms, the attack a twentieth: the reduction's 63 % in its time (+ a block or two)", ok);
    master_rest();
}

/* ------------------------------------------------------------- LIMIT --- */
static void test_limit(void)
{
    int32_t pk;
    int ok;
    ui_power_on();
    ok = abs(dyn_lim_t(0) - 5692) <= 2 && dyn_lim_t(DYN_LIM_DEF) == LIM_T && abs(dyn_lim_t(DYN_NLIM - 1) - 32009) <= 3;
    printf("dyn:   (LIMIT steps: %d .. %d .. %d)\n", (int)dyn_lim_t(0), (int)dyn_lim_t(DYN_LIM_DEF),
           (int)dyn_lim_t(DYN_NLIM - 1));
    ck("LIMIT: -15.2 dBFS 5692, -5.2 dBFS exactly 18000 (LIM_T), -0.2 dBFS 32009 (the power's 1e-4)", ok);
    master_rest();
    set(0, 50, 0, 0);
    ok = lim_t == dyn_lim_t(0) && !dynx.on;
    pk = run(30000.0, 300.0, 1500);
    ok &= abs(pk - softclip(5692)) < softclip(5692) / 25;
    printf("dyn:   (LIMIT -15.2 dB, a sine at -0.8 dBFS: the peak %d, the threshold through the soft clip %d)\n", (int)pk,
           (int)softclip(5692));
    ck("  LIMIT -15.2 dB: a loud sine held at the threshold (4 %), without the compressor", ok);
    master_rest();
}

/* ------------------------------------------------------------ METERS --- */
static void test_meters(void)
{
    uint32_t k;
    int ok;
    ui_power_on();
    master_rest();
    set(50, 50, DYN_LIM_DEF, 0);
    dyn_poll();
    run(32768.0 * pow(10.0, -6.0 / 20.0), 1000.0, 1500);
    dyn_poll();
    ok = dyn_mtr[0] >= 8u && dyn_mtr[0] <= 9u && dyn_mtr[1] == 0u;
    printf("dyn:   (COMP 50 at -6 dBFS: COMP's meter %u half dB, LIMIT's %u)\n", dyn_mtr[0], dyn_mtr[1]);
    for (k = 0; k < 3u; k++)
        dyn_poll();
    ok &= dyn_mtr[0] >= 5u && dyn_mtr[0] <= 6u;       /* (nothing new from the ISR: it falls a half dB a pass) */
    ck("METERS: COMP 50, a sine at -6 dBFS: COMP's meter -4.0 .. -4.5 dB, LIMIT's none; then it falls 0.5 dB a pass", ok);
    master_rest();
    set(0, 50, 0, 0);
    dyn_mtr[0] = dyn_mtr[1] = 0;
    dyn_poll();
    run(30000.0, 300.0, 300);
    dyn_poll();
    ok = dyn_mtr[0] == 0u && dyn_mtr[1] >= 27u && dyn_mtr[1] <= 30u;
    printf("dyn:   (LIMIT -15.2 dB, a sine at -0.8 dBFS: LIMIT's meter %u half dB)\n", dyn_mtr[1]);
    ck("  LIMIT -15.2 dB, a sine at -0.8 dBFS: LIMIT's meter 13.5 .. 15 dB (14.4 over), COMP's none", ok);
    master_rest();
}

/* -------------------------------------------------------------- PAGE --- */
static void test_page(const char *dir)
{
    uint32_t sig;
    int ok;
    ui_power_on();
    go_home(); frame();
    turn(EN_SELECT, 1); frame();
    ok = str_eq(cur_page()->title, "MIXER");
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "CLOCK");
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "EQ");
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "MASTER") && cur_page()->graph == GR_DYN;
    turn(EN_SELECT, 1); frame();
    ok &= ui.home;
    turn(EN_SELECT, -1); frame();
    ok &= str_eq(cur_page()->title, "MASTER");
    ck("PAGE: HOME, SELECT: MIXER, CLOCK, EQ, MASTER, HOME (and back: MASTER)", ok);
    sig = ui.graph_sig;
    turn(EN_K1, 10); frame();
    ok = dyn_set[DYN_COMP] == 10 && ui.graph_sig != sig && dynx.on;
    turn(EN_K2, -5); frame();
    ok &= dyn_set[DYN_REL] == 45;
    turn(EN_K3, 4); frame();
    ok &= dyn_set[DYN_LIM] == DYN_LIM_DEF + 4 && lim_t == dyn_lim_t(DYN_LIM_DEF + 4);
    turn(EN_K4, 6); frame();
    ok &= dyn_set[DYN_GAIN] == 6 && fabs(dynx.mk - 3.0f) < 1e-6;
    turn(EN_K4, 40); frame();
    ok &= dyn_set[DYN_GAIN] == DYN_GMAX;
    turn(EN_K3, -40); frame();
    ok &= dyn_set[DYN_LIM] == 0;
    ck("  KNOB 1 COMP, 2 RELEASE, 3 LIMIT, 4 GAIN (to their ends), the audio's terms follow", ok);
    {
        int16_t *vp;
        char v[12];
        const char *u;
#define FMT(c) do { const param_desc_t *d_ = page_desc(cur_page(), (c), &vp); param_format(d_, *vp, v, &u); } while (0)
        FMT(0);
        ok = str_eq(v, "10") && str_eq(u, "%");
        dyn_set[DYN_REL] = 50;
        FMT(1);
        ok &= str_eq(v, "141") && str_eq(u, "ms");
        FMT(2);
        ok &= str_eq(v, "-15.2") && str_eq(u, "dB");
        dyn_set[DYN_LIM] = DYN_LIM_DEF;
        FMT(2);
        ok &= str_eq(v, "-5.2");
        dyn_set[DYN_LIM] = DYN_NLIM - 1;
        FMT(2);
        ok &= str_eq(v, "-0.2");
        FMT(3);
        ok &= str_eq(v, "+12.0") && str_eq(u, "dB");
        dyn_set[DYN_GAIN] = 0;
        FMT(3);
        ok &= str_eq(v, "0.0");
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, "COMP") && str_eq(page_desc(cur_page(), 3, &vp)->label, "GAIN");
#undef FMT
    }
    ck("  the cards: COMP 10 %, RELEASE 141 ms, LIMIT -15.2 / -5.2 / -0.2 dB, GAIN +12.0 / 0.0 dB", ok);
    set(60, 50, DYN_LIM_DEF + 2, 4);
    frame();
    sig = ui.graph_sig;
    master_rest();
    run(32768.0 * pow(10.0, -3.0 / 20.0), 1000.0, 600);
    frame();
    ok = dyn_mtr[0] > 0u && ui.graph_sig != sig;
    ck("  the meters redraw the panel (the compressor at work)", ok);
    ui.force = 1;
    frame();
    if (dir)
        screen_save(dir, "master");
    master_rest();
}

/* -------------------------------------------------------------- KEPT --- */
static void test_kept(void)
{
    static const int16_t KEEP[DYN_N] = {35, 70, 25, 5};
    project_store_t slot;
    uint32_t sig;
    int ok;
    ui_power_on();
    memcpy(dyn_set, KEEP, sizeof KEEP);
    sig = autosave_sig();
    dyn_set[DYN_GAIN] = 6;
    ok = autosave_sig() != sig;
    dyn_set[DYN_GAIN] = KEEP[DYN_GAIN];
    ok &= project_save_as(1, "DYN") == 0;
    slot = proj_slot[1];
    ui_power_on();
    proj_slot[1] = slot;
    dyn_set[DYN_COMP] = 99;
    project_load(1);
    ok &= msg_is("LOADED") && !memcmp(dyn_set, KEEP, sizeof KEEP);
    ck("KEPT: a project saved with its MASTER settings loads with them; the autosave sees a change", ok);
    xtra_hi_ram[1].dyn[DYN_LIM] = DYN_NLIM;            /* a damaged value (LIMIT past its end): its default */
    xtra_hi_ram[1].dyn[DYN_COMP] = 101;
    project_load(1);
    ok = dyn_set[DYN_LIM] == DYN_LIM_DEF && dyn_set[DYN_COMP] == 0 && dyn_set[DYN_REL] == KEEP[DYN_REL] &&
         dyn_set[DYN_GAIN] == KEEP[DYN_GAIN];
    xtra_slot[1].size = XTRA_V9;                       /* extras of before (no MASTER): the defaults */
    project_load(1);
    ok &= !memcmp(dyn_set, DYN_DEF, sizeof DYN_DEF);
    ck("  a damaged value: its default; extras of before HOME > MASTER: the defaults", ok);
    memcpy(dyn_set, KEEP, sizeof KEEP);
    project_new(4);
    ok = !memcmp(dyn_set, DYN_DEF, sizeof DYN_DEF);
    ck("  a new project: the defaults", ok);
}

int main(int argc, char **argv)
{
    test_before();
    test_curve();
    test_times();
    test_limit();
    test_meters();
    test_page(argc > 1 ? argv[1] : 0);
    test_kept();
    if (bad) {
        printf("dyn: %d FAILED\n", bad);
        return 1;
    }
    printf("dyn: all ok\n");
    return 0;
}
