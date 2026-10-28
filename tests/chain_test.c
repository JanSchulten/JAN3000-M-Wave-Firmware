/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The serial FX chain (firmware/src/fx.c mix_block: CHORUS -> SHIMMER -> DELAY -> REVERB) and FX > MIX 2, its levels
 * (params.c fx_lvl, the DELAY's MIX G_DMIX), on the firmware's own sources (ui_test.c):
 *   FLOW     a track's CHR send alone reaches every stage after the CHORUS (the SHIMMER's, the DELAY's and the
 *            REVERB's lines fill); a REV send alone reaches the REVERB only.
 *   LEVELS   SHR 0: the SHIMMER silent, the CHORUS goes on into the DELAY and the REVERB; CHR 0: nothing of a CHR
 *            send anywhere; REV 0: the REVERB silent; a level turned to 0 while the chain rings: no jump.
 *   STEREO   the CHORUS alone on a centred track: left and right differ (its two reads), their mean is the mono bus;
 *            without the CHORUS left and right are equal.
 *   BOUNDS   four loud tracks, every send and level full: the levels' Q16 multiply keeps the sign and never grows
 *            (chain_scale, chain_reverb); the mix bounded.
 *   STORED   the levels with the project (extras XTRA_V9) and in the autosave's signature; extras of before: full.
 *   PAGE     MIX 2 after the sends: KNOB 1 2 4 the levels, KNOB 3 the DELAY's MIX; its signature follows them.
 * Built and run by tests/run_tests.sh (a directory argument: the screen). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("chain: %-92s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

/* boot state, the buses empty, track 1 a held chord (ANALOG SOFT PAD, centred), every send 0, the others silent */
static void fresh(void)
{
    uint32_t k;
    ui_power_on();
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    memset(&fx, 0, sizeof fx);
    rev_clear();
    memset(&hl, 0, sizeof hl);
    song.g[G_RTYPE] = 0;                                /* (ROOM: its lines are rev_comb) */
    song.master_q12 = 4096;
    lim_env = LIM_T;                                    /* (the master's own states: as at power-on, left and right) */
    dc_l = dc_r = dce_l = dce_r = 0;
    lc_l1 = lc_l2 = lc_r1 = lc_r2 = 0;
    memset(lce, 0, sizeof lce);
    for (k = 0; k < NTRK; k++) {
        trk[k].p[P_CHOR] = trk[k].p[P_SHIM] = trk[k].p[P_DLY] = trk[k].p[P_REV] = 0;
        trk[k].p[P_ITYPE] = 0;
        trk[k].p[P_PAN] = 0;
        trk[k].p[P_SPRD] = 0;                           /* (SPREAD 0: the dry mix itself mono) */
    }
    trk_note_on(&trk[0], 48, 110);
    trk_note_on(&trk[0], 55, 110);
}
static int32_t ob[2 * CTL];
static uint32_t lr_diff;                                /* blocks whose left and right differed */
static void run(uint32_t blocks)
{
    uint32_t b, i;
    for (b = 0; b < blocks; b++) {
        int d = 0;
        mix_block(ob, CTL);
        for (i = 0; i < CTL; i++)
            d |= ob[2u * i] != ob[2u * i + 1u];
        lr_diff += (uint32_t)d;
    }
}
static int any16(const int16_t *x, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if (x[i])
            return 1;
    return 0;
}
#define DLY_ANY() any16(dly_buf, DLY_LEN)
#define REV_ANY() any16(rev_comb, sizeof rev_comb / sizeof rev_comb[0])
#define SHIM_ANY() any16(shim_comb, sizeof shim_comb / sizeof shim_comb[0])

static void test_flow(void)
{
    fresh();
    trk[0].p[P_CHOR] = 100;
    run(FS / CTL);
    ck("FLOW: a CHR send alone fills the SHIMMER's, the DELAY's and the REVERB's lines (the chain after the CHORUS)",
       SHIM_ANY() && DLY_ANY() && REV_ANY());
    fresh();
    trk[0].p[P_REV] = 100;
    run(FS / CTL);
    ck("FLOW: a REV send alone reaches the REVERB only (no CHORUS, SHIMMER or DELAY)",
       REV_ANY() && !SHIM_ANY() && !DLY_ANY() && !any16(cho_buf, CHO_LEN));
}

static void test_levels(void)
{
    uint32_t b, i;
    fresh();
    trk[0].p[P_CHOR] = 100;
    fx_lvl[1] = 0;
    run(FS / CTL);
    ck("LEVELS: SHR 0: the SHIMMER silent, the CHORUS goes on into the DELAY and the REVERB",
       !SHIM_ANY() && !shim_live && DLY_ANY() && REV_ANY());
    fresh();
    trk[0].p[P_CHOR] = 100;
    fx_lvl[0] = 0;
    run(FS / CTL);
    ck("LEVELS: CHR 0: nothing of a CHR send anywhere (CHORUS, SHIMMER, DELAY, REVERB)",
       !any16(cho_buf, CHO_LEN) && !SHIM_ANY() && !DLY_ANY() && !REV_ANY());
    fresh();
    trk[0].p[P_REV] = 100;
    fx_lvl[3] = 0;
    run(FS / CTL);
    ck("LEVELS: REV 0: the REVERB silent with its send up", !REV_ANY());
    /* REV turned to 0 while it rings: the input stops, the tail rings out (no jump on the wet bus) */
    fresh();
    trk[0].p[P_REV] = 127;
    run(FS / CTL);
    {
        int32_t prev = wet[CTL - 1u], mx = 0, own = 0;
        for (b = 0; b < 8u; b++) {                      /* (the bus's own steps, as it rings) */
            mix_block(ob, CTL);
            for (i = 0; i < CTL; i++) {
                int32_t st = abs(wet[i] - prev);
                prev = wet[i];
                own = st > own ? st : own;
            }
        }
        fx_lvl[3] = 0;
        for (b = 0; b < 8u; b++) {
            mix_block(ob, CTL);
            for (i = 0; i < CTL; i++) {
                int32_t st = abs(wet[i] - prev);
                prev = wet[i];
                mx = st > mx ? st : mx;
            }
        }
        {
            char what[160];
            snprintf(what, sizeof what, "LEVELS: REV turned to 0 while it rings: the largest step %d (the bus's own %d), the tail "
                     "still there", mx, own);
            ck(what, own > 0 && mx <= own + own / 4 && REV_ANY());
        }
    }
}

static void test_stereo(void)
{
    fresh();
    trk[0].p[P_CHOR] = 127;
    fx_lvl[1] = 0;                                      /* (the CHORUS alone: SHIMMER 0, the DELAY's MIX 0, REVERB 0) */
    song.g[G_DMIX] = 0;
    fx_lvl[3] = 0;
    song.g[G_CDEPTH] = 100;
    lr_diff = 0;
    run(FS / CTL);
    {
        uint32_t i;
        int side = 0;
        for (i = 0; i < CTL; i++)
            side |= cho_sd[i];
        ck("STEREO: the CHORUS alone on a centred track: left and right differ (its side, the two reads)",
           lr_diff > FS / CTL / 2u && side);
    }
    fresh();
    lr_diff = 0;
    run(FS / CTL);
    ck("STEREO: without the CHORUS (and without HALL) left and right are equal", !lr_diff && !cho_on);
}

static void test_bounds(void)
{
    static const int32_t X[6] = {1 << 20, -(1 << 20), 262143 * 4, -262143 * 4, 70000, -70000};
    int32_t x[6], k;
    int ok = 1;
    uint32_t g, b, i;
    for (g = 0; g <= 127u; g++) {                       /* every level */
        memcpy(x, X, sizeof x);
        fx_lvl[1] = (int16_t)g;
        chain_scale(x, 6u, chain_g(1));
        for (k = 0; k < 6; k++)
            ok &= (X[k] < 0 ? x[k] <= 0 : x[k] >= 0) && abs(x[k]) <= abs(X[k]);
    }
    ck("BOUNDS: a level's Q16 multiply keeps the sign and never grows (4 x the sends' clamp, every level)", ok);
    fresh();
    for (k = 0; k < NTRK; k++) {
        static const uint8_t N[4] = {36, 48, 55, 64};
        trk[k].p[P_CHOR] = trk[k].p[P_SHIM] = trk[k].p[P_DLY] = trk[k].p[P_REV] = 127;
        trk[k].p[P_LEVEL] = 127;
        if (k)
            for (i = 0; i < 4u; i++)
                trk_note_on(&trk[k], N[i] + (uint32_t)k, 127);
    }
    song.g[G_DFDBK] = 120;
    ok = 1;
    for (b = 0; b < 3u * FS / CTL; b++) {
        mix_block(ob, CTL);
        for (i = 0; i < CTL; i++)
            ok &= chain_rev[i] >= -262143 && chain_rev[i] <= 262143 && abs(ob[2u * i]) <= 32767 && abs(ob[2u * i + 1u]) <= 32767;
    }
    ck("BOUNDS: four loud tracks, every send and level full, DLY FDBK 120: the REVERB's input clamped, the mix bounded", ok);
}

static void test_stored(void)
{
    static proj_xtra_t x;
    static xtra_hi_t xh;
    uint32_t sig;
    fresh();
    fx_lvl[0] = 11; fx_lvl[1] = 22; fx_lvl[3] = 33;
    sig = autosave_sig();
    fx_lvl[1] = 23;
    ck("STORED: a level edit changes the autosave's signature", sig != autosave_sig());
    ck("STORED: the project with its extras saves", project_save_as(0, "CHAIN") == 0);
    fx_lvl[0] = fx_lvl[1] = fx_lvl[3] = 99;
    project_load(0);
    ck("STORED: and loads the levels (CHR 11, SHR 23, REV 33)", fx_lvl[0] == 11 && fx_lvl[1] == 23 && fx_lvl[3] == 33);
    xtra_capture(&x, &xh, 7);
    ck("STORED: the extras grow by the levels (XTRA_V9 = XTRA_V8 + 4), each byte 0x80 | the level",
       x.size == XTRA_V9 && XTRA_V9 == XTRA_V8 + 4u && xh.lvl[0] == (0x80u | 11u) && xh.lvl[1] == (0x80u | 23u) &&
       xh.lvl[3] == (0x80u | 33u));
    x.size = XTRA_V8;                                   /* extras of before: the levels full */
    xtra_apply(&x, 7, &xh);
    ck("STORED: extras of before (XTRA_V8): the levels full, as the chain played", fx_lvl[0] == FX_LVL_DEF &&
       fx_lvl[1] == FX_LVL_DEF && fx_lvl[3] == FX_LVL_DEF);
}

static void test_page(const char *dir)
{
    uint32_t sig;
    int16_t d0;
    int ok;
    fresh();
    go_title("MIX");
    turn(EN_SELECT, 1);
    frame();
    ok = str_eq(cur_page()->title, "MIX 2") && cur_page()->graph == GR_FXLVL;
    fx_lvl[0] = fx_lvl[1] = fx_lvl[3] = 64;
    d0 = song.g[G_DMIX];
    frame();
    sig = graph_signature();
    turn(EN_K1, 1); turn(EN_K2, -1); turn(EN_K3, 1); turn(EN_K4, -1);
    ok &= fx_lvl[0] > 64 && fx_lvl[1] < 64 && song.g[G_DMIX] > d0 && fx_lvl[3] < 64 && sig != graph_signature();
    ck("PAGE: MIX 2 after the sends: KNOB 1 2 4 the CHR SHR REV levels, KNOB 3 the DELAY's MIX; redrawn", ok);
    frame();
    if (dir)
        screen_save(dir, "mix2");
    track_select(2);
    ck("PAGE: the levels are every track's", fx_lvl[0] > 64 && fx_lvl[1] < 64);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : 0;
    test_flow();
    test_levels();
    test_stereo();
    test_bounds();
    test_stored();
    test_page(dir);
    printf("chain: %s\n", bad ? "FAILED" : "all ok");
    return bad ? 1 : 0;
}
