/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* HOME > EQ (firmware/src/eq.c, params.c eq_set, ui_graph.c graph_eq), on the firmware's own sources (ui_test.c's harness):
 *   FLAT     power-on, a new project, extras of before: every band flat; the ISR runs no band, a block passes untouched
 *   RESPONSE each TYPE's level measured with sines through track_eq against the cookbook's in double: a BELL +6 dB at
 *            1 kHz, a LOW SHELF -12 dB at 100 Hz, a HIGH SHELF +9 dB at 8 kHz, a LP and a HP (Q 0.71 and 2.83), at the
 *            band's FREQ, an octave either side and far away; four bands at once add up
 *   GLIDE    a band changed while it plays moves a quarter a block, no step past what the change asks; flat again: it
 *            stops (run cleared, state 0); a part fallen silent drops its state, the next note starts from rest
 *   PAGE     HOME, SELECT: MIXER, CLOCK, EQ, HOME; KNOB 1 the band, 2 FREQ, 3 GAIN (a LP / HP: RESO, kept apart), 4 TYPE,
 *            the selected track's (ALGORITHM); a sound loaded keeps them; the panel redraws on each
 *   KEPT     saved with the project and loaded back, the autosave told; another project: its own; a damaged band: its
 *            default; a new project: flat
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <math.h>

static int bad;
static void blk_mix(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }
static int ck(const char *what, int ok)
{
    printf("eq: %-104s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
/* the cookbook in double: the level in dB of band b at f Hz */
static double ref_db(const eq_band_t *b, double f)
{
    double w0 = 2 * M_PI * EQ_HZ_X10[b->f] / 10.0 / FS, cs = cos(w0), sn = sin(w0), A = pow(10, b->g / 2.0 / 40.0);
    double q = 0.5 * pow(2, b->q / 4.0), al, b0, b1, b2, a0, a1, a2, sa = sqrt(A);
    double complex_re, complex_im, w = 2 * M_PI * f / FS, nr, ni, dr, di;
    switch (b->ty) {
    case EQT_LP:
    case EQT_HP:
        al = sn / (2 * q);
        b1 = b->ty == EQT_LP ? 1 - cs : -(1 + cs);
        b0 = b2 = b->ty == EQT_LP ? (1 - cs) / 2 : (1 + cs) / 2;
        a0 = 1 + al, a1 = -2 * cs, a2 = 1 - al;
        break;
    case EQT_BELL:
        al = sn / 2;
        b0 = 1 + al * A, b1 = -2 * cs, b2 = 1 - al * A, a0 = 1 + al / A, a1 = -2 * cs, a2 = 1 - al / A;
        break;
    case EQT_LSHF:
        al = sn / 2 * sqrt(2);
        b0 = A * ((A + 1) - (A - 1) * cs + 2 * sa * al), b1 = 2 * A * ((A - 1) - (A + 1) * cs);
        b2 = A * ((A + 1) - (A - 1) * cs - 2 * sa * al), a0 = (A + 1) + (A - 1) * cs + 2 * sa * al;
        a1 = -2 * ((A - 1) + (A + 1) * cs), a2 = (A + 1) + (A - 1) * cs - 2 * sa * al;
        break;
    default:
        al = sn / 2 * sqrt(2);
        b0 = A * ((A + 1) + (A - 1) * cs + 2 * sa * al), b1 = -2 * A * ((A - 1) + (A + 1) * cs);
        b2 = A * ((A + 1) + (A - 1) * cs - 2 * sa * al), a0 = (A + 1) - (A - 1) * cs + 2 * sa * al;
        a1 = 2 * ((A - 1) - (A + 1) * cs), a2 = (A + 1) - (A - 1) * cs - 2 * sa * al;
        break;
    }
    nr = b0 + b1 * cos(w) + b2 * cos(2 * w), ni = -(b1 * sin(w) + b2 * sin(2 * w));
    dr = a0 + a1 * cos(w) + a2 * cos(2 * w), di = -(a1 * sin(w) + a2 * sin(2 * w));
    complex_re = nr * nr + ni * ni, complex_im = dr * dr + di * di;
    return 10 * log10(complex_re / complex_im);
}
/* track 0's EQ as eq_set says (handed over, taken, no glide): from rest */
static void eq_take(void)
{
    eq_poll();
    eq_rest(&trk[0]);
}
/* a sine of f Hz through track 0's EQ: the level in dB of what comes out (the first 0.3 s let settle) */
static double measure(double f)
{
    static int32_t x[CTL];
    double ei = 0, eo = 0, ph = 0;
    uint32_t b, i, nb = (uint32_t)(1.3 * FS / CTL), settle = (uint32_t)(0.3 * FS / CTL);
    eq_rest(&trk[0]);
    for (b = 0; b < nb; b++) {
        int32_t in[CTL];
        for (i = 0; i < CTL; i++, ph += 2 * M_PI * f / FS)
            x[i] = in[i] = (int32_t)(20000.0 * sin(ph));
        track_eq(&trk[0], x, 0, CTL);
        if (b >= settle)
            for (i = 0; i < CTL; i++) {
                ei += (double)in[i] * in[i];
                eo += (double)x[i] * x[i];
            }
    }
    return 10 * log10(eo / ei);
}
static void band(uint32_t n, int16_t f, int16_t g, int16_t ty, int16_t q)
{
    eq_set[0][n] = (eq_band_t){f, g, ty, q};
}
static int flat_all(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if (memcmp(eq_set[k], EQ_DEF, sizeof EQ_DEF))
            return 0;
    return 1;
}

/* ------------------------------------------------------------ FLAT --- */
static void test_flat(void)
{
    static int32_t x[CTL], y[CTL];
    uint32_t i, ok;
    ui_power_on();
    frame();
    for (i = 0; i < CTL; i++)
        x[i] = y[i] = (int32_t)(i * 7919u % 40000u) - 20000;
    track_eq(&trk[0], x, 0, CTL);
    ok = flat_all() && !eqa.run[0] && !eqa.run[1] && !eqa.run[2] && !eqa.run[3] && !memcmp(x, y, sizeof x) &&
         eq_set[0][0].ty == EQT_LSHF && EQ_HZ_X10[eq_set[0][0].f] == 1000u && EQ_HZ_X10[eq_set[0][1].f] == 5000u &&
         EQ_HZ_X10[eq_set[0][2].f] == 20000u && eq_set[0][3].ty == EQT_HSHF && EQ_HZ_X10[eq_set[0][3].f] == 80000u;
    ck("FLAT: power-on: L.SHF 100 Hz, BELL 500 Hz, BELL 2 kHz, H.SHF 8 kHz, 0 dB; no band runs, a block untouched", ok);
}

/* -------------------------------------------------------- RESPONSE --- */
static int near_db(double got, double want, double tol, const char *what)
{
    int ok = fabs(got - want) <= tol;
    if (!ok)
        printf("  %s: %.2f dB, want %.2f\n", what, got, want);
    return ok;
}
static int check_band(const eq_band_t *b, const char *name)
{
    static const double F[] = {30, 60, 120, 250, 500, 1000, 2000, 4000, 8000, 12000, 16000};
    uint32_t i;
    int ok = 1;
    for (i = 0; i < EQ_NB; i++)
        eq_set[0][i] = EQ_DEF[i];
    eq_set[0][0] = *b;
    eq_take();
    for (i = 0; i < NELEM(F); i++) {
        double want = ref_db(b, F[i]), got = measure(F[i]);
        char w[48];
        snprintf(w, sizeof w, "%s at %.0f Hz", name, F[i]);
        ok &= near_db(got, want, want < -30 ? 1.5 : 0.15, w);
    }
    return ok;
}
static void test_response(void)
{
    eq_band_t b;
    double s, want;
    uint32_t i;
    int ok;
    ui_power_on();
    b = (eq_band_t){68, 12, EQT_BELL, 2};              /* BELL +6 dB at 1 kHz */
    ok = check_band(&b, "BELL +6 1k") && near_db(measure(1000), 6.0, 0.1, "BELL at its FREQ");
    ck("RESPONSE: BELL +6 dB at 1 kHz (+6.0 there), as the cookbook's from 30 Hz to 16 kHz", ok);
    b = (eq_band_t){28, -24, EQT_LSHF, 2};             /* LOW SHELF -12 dB at 100 Hz */
    ok = check_band(&b, "L.SHF -12 100") && near_db(measure(30), -11.7, 0.4, "L.SHF far below");
    ck("  L.SHF -12 dB at 100 Hz (-6 at 100 Hz, ~-12 below, 0 above)", ok);
    b = (eq_band_t){104, 18, EQT_HSHF, 2};             /* HIGH SHELF +9 dB at 8 kHz */
    ck("  H.SHF +9 dB at 8 kHz", check_band(&b, "H.SHF +9 8k"));
    b = (eq_band_t){68, 0, EQT_LP, 2};                 /* LP 1 kHz, Q 0.71 */
    ok = check_band(&b, "LP 1k") && near_db(measure(1000), -3.0, 0.1, "LP at its FREQ");
    ck("  LP 1 kHz Q 0.71 (-3 dB there, -12 an octave up per octave)", ok);
    b = (eq_band_t){40, 0, EQT_HP, 10};                /* HP 200 Hz, Q 2.83: a resonance */
    ok = check_band(&b, "HP 200 Q2.83") && near_db(measure(EQ_HZ_X10[40] / 10.0), 9.04, 0.15, "HP's resonance");
    ck("  HP 200 Hz Q 2.83 (+9 dB at its FREQ: the resonance)", ok);
    band(0, 20, -10, EQT_LSHF, 2);                     /* four at once: their levels add */
    band(1, 56, 8, EQT_BELL, 2);
    band(2, 84, -6, EQT_BELL, 2);
    band(3, 110, 12, EQT_HSHF, 2);
    eq_take();
    ok = 1;
    for (i = 0; i < 6u; i++) {
        static const double F[6] = {40, 300, 700, 2500, 6000, 15000};
        uint32_t k;
        for (k = 0, want = 0; k < EQ_NB; k++)
            want += ref_db(&eq_set[0][k], F[i]);
        s = measure(F[i]);
        ok &= near_db(s, want, 0.2, "four bands");
    }
    ck("  four bands at once: their levels add (40 Hz .. 15 kHz)", ok);
}

/* ----------------------------------------------------------- GLIDE --- */
static void test_glide(void)
{
    static int32_t x[CTL];
    uint32_t b, i, n;
    float d0, d;
    int ok;
    ui_power_on();
    band(1, 56, 24, EQT_BELL, 2);                      /* +12 dB at 500 Hz, then while it plays to -12 */
    eq_take();
    memset(x, 0, sizeof x);
    track_eq(&trk[0], x, 0, CTL);
    ok = eqa.run[0] == 2u && eqa.live[0];
    band(1, 56, -24, EQT_BELL, 2);
    eq_poll();
    d0 = fabsf(eqa.cur[0][1].b0 - eqx.next[0].c[1].b0);
    track_eq(&trk[0], x, 0, CTL);
    d = fabsf(eqa.cur[0][1].b0 - eqa.tgt[0].c[1].b0);
    ok &= d > 0.70f * d0 && d < 0.80f * d0;            /* (a quarter of the way in a block) */
    for (n = 1; n < 200u && fabsf(eqa.cur[0][1].b0 - eqa.tgt[0].c[1].b0) > 0.0f; n++)
        track_eq(&trk[0], x, 0, CTL);
    printf("  glide: there after %u blocks (%.1f ms)\n", n, n * 1000.0 * CTL / FS);
    ok &= n > 10u && n < 80u && eqa.run[0] == 2u;
    ck("GLIDE: a band changed while it plays: a quarter of the way a block, there in a few ms", ok);
    band(1, 56, 0, EQT_BELL, 2);                       /* flat again: it glides back, then stops, its state 0 */
    eq_poll();
    for (b = 0; b < 200u && eqa.run[0]; b++) {
        for (i = 0; i < CTL; i++) x[i] = (int32_t)(i * 1000u) - 16000;
        track_eq(&trk[0], x, 0, CTL);
    }
    ok = !eqa.run[0] && b > 5u && eqa.z[0][0][1][0] == 0.0f && eqa.z[0][0][1][1] == 0.0f;
    ck("  flat again: it glides back to flat and stops (no band runs, its state 0)", ok);
    band(2, 80, 12, EQT_BELL, 2);                      /* a part fallen silent: its state dropped (mix_part) */
    eq_take();
    trk_note_on(&trk[0], 60, 100);
    for (b = 0; b < 50u; b++) blk_mix();
    ok = eqa.live[0] && (eqa.z[0][0][2][0] != 0.0f || eqa.z[0][0][2][1] != 0.0f);
    trk_note_off(&trk[0], 60);
    for (b = 0; b < 2000u && eqa.live[0]; b++) blk_mix();
    ok &= !eqa.live[0] && eqa.z[0][0][2][0] == 0.0f && eqa.z[0][0][2][1] == 0.0f && eqa.run[0] == 4u;
    ck("  a part fallen silent (its tail run out): its state dropped, the band still on for the next note", ok);
}

/* ------------------------------------------------------------ PAGE --- */
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
    ok &= str_eq(cur_page()->title, "EQ") && cur_page()->graph == GR_EQ;
    turn(EN_SELECT, 1); frame();
    ok &= ui.home;
    turn(EN_SELECT, -1); frame();
    ok &= str_eq(cur_page()->title, "EQ");
    ck("PAGE: HOME, SELECT: MIXER, CLOCK, EQ, HOME (and back: EQ)", ok);
    sig = ui.graph_sig;
    turn(EN_K1, 2); frame();                           /* band 3 */
    ok = eq_ui_band == 2 && ui.graph_sig != sig;
    turn(EN_K2, 4); frame();                           /* its FREQ 2 kHz -> 2.5 kHz (4 steps) */
    ok &= eq_set[0][2].f == 84 && EQ_HZ_X10[84] == 25000u;
    turn(EN_K3, -6); frame();                          /* GAIN -3 dB */
    ok &= eq_set[0][2].g == -6;
    turn(EN_K4, 2); frame();                           /* TYPE BELL -> LP: KNOB 3 is its RESO now */
    ok &= eq_set[0][2].ty == EQT_LP;
    turn(EN_K3, 4); frame();
    ok &= eq_set[0][2].q == 6 && eq_set[0][2].g == -6;
    turn(EN_K4, -2); frame();                          /* back to BELL: its GAIN as it was */
    ok &= eq_set[0][2].ty == EQT_BELL && eq_set[0][2].g == -6 && eq_set[0][2].q == 6;
    turn(EN_K1, 10); frame();                          /* (the band: 1 .. 4) */
    ok &= eq_ui_band == 3;
    ck("  KNOB 1 the band (1..4), 2 FREQ (R40 steps), 3 GAIN, 4 TYPE; a LP's KNOB 3 its RESO, each kept apart", ok);
    {
        int16_t *vp;
        char v[12];
        const char *u;
#define FMT(c) do { const param_desc_t *d_ = page_desc(cur_page(), (c), &vp); param_format(d_, *vp, v, &u); } while (0)
        eq_ui_band = 2;
        FMT(1);
            ok = str_eq(v, "2.50") && str_eq(u, "kHz");
        FMT(2);
        ok &= str_eq(v, "-3.0") && str_eq(u, "dB");
        eq_set[0][2].f = 3, eq_set[0][2].g = 1;
        FMT(1);
        ok &= str_eq(v, "23.6") && str_eq(u, "Hz");
        FMT(2);
        ok &= str_eq(v, "+0.5");
        eq_set[0][2].f = 120, eq_set[0][2].ty = EQT_HP;
        FMT(1);
        ok &= str_eq(v, "20.0") && str_eq(u, "kHz");
        FMT(2);
        ok &= str_eq(v, "1.41") && str_eq(u, "Q") && str_eq(page_desc(cur_page(), 2, &vp)->label, "RESO");
        eq_set[0][2].q = 0, eq_set[0][2].g = 0, eq_set[0][2].ty = EQT_BELL;
        FMT(1);
        FMT(2);
        ok &= str_eq(v, "0.0") && str_eq(u, "dB");
        eq_set[0][2].g = -1;
        FMT(2);
        ok &= str_eq(v, "-0.5");
        eq_set[0][2] = EQ_DEF[2];
        eq_set[0][2].f = 84, eq_set[0][2].g = -6, eq_set[0][2].q = 6;
#undef FMT
    }
    ck("  the cards: 2.50 kHz, -3.0 dB, 23.6 Hz, +0.5 dB, 20.0 kHz, RESO 1.41 Q, 0.0 dB, -0.5 dB", ok);
    turn(EN_ALGO, 1); frame();                         /* track 2: its own bands */
    eq_ui_band = 0;
    turn(EN_K3, 4); frame();
    ok = song.sel == 1u && eq_set[1][0].g == 4 && eq_set[0][0].g == 0 && eq_set[0][2].g == -6;
    turn(EN_ALGO, -1); frame();
    preset_load_to(&trk[0], 5, 1);                     /* a sound loaded: the EQ stays the track's */
    frame();
    ok &= eq_set[0][2].f == 84 && eq_set[0][2].g == -6;
    ck("  each track its own (ALGORITHM); a sound loaded keeps the track's EQ", ok);
    eq_set[0][0].g = -10;                              /* (the screen: a cut low, a bell, band 3 picked) */
    eq_ui_band = 2;
    ui.force = 1;
    frame();
    if (dir)
        screen_save(dir, "eq");
    blk_mix();                                         /* (a set handed over before: taken; this one next) */
    eq_poll();
    ok = eqx.req[0] != 0u;
    blk_mix();
    ok &= eqa.run[0] == 5u;                            /* (bands 1 and 3: not flat) */
    ck("  what the page set reaches the audio: bands 1 and 3 run", ok);
}

/* ------------------------------------------------------------ KEPT --- */
static void test_kept(void)
{
    project_store_t slot;
    eq_band_t keep[NTRK][EQ_NB];
    uint32_t sig;
    int ok;
    ui_power_on();
    band(1, 60, -7, EQT_BELL, 2);
    eq_set[3][2] = (eq_band_t){100, 0, EQT_LP, 12};
    memcpy(keep, eq_set, sizeof keep);
    sig = autosave_sig();
    eq_set[2][1].g = 3;
    ok = autosave_sig() != sig;
    eq_set[2][1].g = 0;
    ok &= project_save_as(1, "EQS") == 0;
    slot = proj_slot[1];
    ui_power_on();
    proj_slot[1] = slot;
    eq_set[0][0].g = 9;
    project_load(1);
    ok &= msg_is("LOADED") && !memcmp(eq_set, keep, sizeof keep);
    ck("KEPT: a project saved with its EQ loads with it (every track's bands); the autosave sees a change", ok);
    xtra_hi_ram[1].eq[3][2][0] = 200;                  /* a damaged band (FREQ out of range): its default */
    xtra_hi_ram[1].eq[0][1][2] = 9;                    /* .. a TYPE no firmware wrote */
    project_load(1);
    ok = eq_set[3][2].f == EQ_DEF[2].f && eq_set[3][2].ty == EQ_DEF[2].ty && !memcmp(&eq_set[0][1], &EQ_DEF[1], sizeof EQ_DEF[1]) &&
         !memcmp(&eq_set[0][0], &keep[0][0], sizeof keep[0][0]);
    xtra_slot[1].size = XTRA_V8;                       /* extras of before (no EQ): flat */
    project_load(1);
    ok &= flat_all();
    ck("  a damaged band: its default; extras of before the EQ: flat", ok);
    band(0, 30, 6, EQT_LSHF, 2);
    project_new(4);
    ok = flat_all();
    ck("  a new project: flat", ok);
}

int main(int argc, char **argv)
{
    test_flat();
    test_response();
    test_glide();
    test_page(argc > 1 ? argv[1] : 0);
    test_kept();
    if (bad) {
        printf("eq: %d FAILED\n", bad);
        return 1;
    }
    printf("eq: all ok\n");
    return 0;
}
