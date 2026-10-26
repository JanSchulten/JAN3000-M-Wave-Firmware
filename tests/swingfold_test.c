/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* One SWING (M-Vave-OP1, JAN3000 Phase 2): the global SWG of HOME > CLOCK is gone; a project's global goes into each
 * track's own SWG as it loads (project.c project_restore_runtime: own + global, at most 100, as track_swing summed
 * them), so every track swings as it did. On the firmware's own sources (ui_test.c's harness):
 *   LOAD     a project saved with global 30 (an older firmware's): the tracks' own 80 0 50 10 load as 100 30 80 40,
 *            the global 0; each track's swing as before; the header's slot letter not marked edited.
 *   SCENES   a stored scene's own SWG takes the project's global too (20 -> 50), the scene playing its tracks'.
 *   SONG     each slot plays its own global in its tracks' swing (slot A global 30, slot B 0: 10 -> 40 and 10).
 *   PAGE     CLOCK shows no SWG; MOMENTARY and the editor cannot set the global (its range 0 .. 0).
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("swingfold: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static void own(int a, int b, int c, int d)
{
    trk[0].p[P_SSWING] = (int16_t)a; trk[1].p[P_SSWING] = (int16_t)b;
    trk[2].p[P_SSWING] = (int16_t)c; trk[3].p[P_SSWING] = (int16_t)d;
}
static int owns(int a, int b, int c, int d)
{
    return trk[0].p[P_SSWING] == a && trk[1].p[P_SSWING] == b && trk[2].p[P_SSWING] == c && trk[3].p[P_SSWING] == d;
}

static void test_load(void)
{
    project_store_t slot;
    int ok;
    ui_power_on();
    stop_transport();
    own(80, 0, 50, 10);
    song.g[G_SWING] = 30;                         /* (as a project of firmware before saved it) */
    trk[0].step[0].n = 1; trk[0].step[0].note[0] = 60;
    ok = project_save_as(1, "SWG") == 0;
    slot = proj_slot[1];
    ui_power_on();
    proj_slot[1] = slot;
    project_load(1);
    ok &= song.g[G_SWING] == 0 && owns(100, 30, 80, 40);
    ok &= track_swing(&trk[0]) == 100 && track_swing(&trk[1]) == 30 && track_swing(&trk[2]) == 80 &&
          track_swing(&trk[3]) == 40;
    ck("LOAD: global 30 goes into the tracks' own SWG (80 0 50 10 -> 100 30 80 40), the global 0", ok);
    ok = pat_from[0] == 2u && pat_from_h[0] == pat_from_sig(&trk[0]);
    ck("  the slot's letter not marked edited after the load", ok);
    ok = project_save_as(1, "SWG") == 0 && proj_import(&proj_scratch, &proj_slot[1], sizeof(project_store_t)) &&
         proj_scratch.g[G_SWING] == 0;
    slot = proj_slot[1];
    ui_power_on();
    proj_slot[1] = slot;
    project_load(1);
    ok &= owns(100, 30, 80, 40);
    ck("  saved again: global 0, the own values kept (an older firmware plays it the same)", ok);
}

static void test_scenes(void)
{
    project_store_t slot;
    scn_view_t v;
    uint32_t n, off;
    uint8_t tm[NTRK][5];
    int ok;
    memset(tm, 0, sizeof tm);
    ui_power_on();
    stop_transport();
    own(20, 0, 0, 0);
    trk[0].step[0].n = 1; trk[0].step[0].note[0] = 60;
    scn_view_trk(&v);                             /* scene 2: the tracks with track 1's own 20 */
    n = scn_encode(&v, 0, 0);
    off = scb_resize(1, n);
    scn_encode(&v, scb.data + off, n);
    own(5, 0, 0, 0);                              /* (scene 1, playing: 5) */
    song.g[G_SWING] = 30;
    ok = project_save_as(2, "SCN") == 0;
    slot = proj_slot[2];
    ui_power_on();
    proj_slot[2] = slot;
    project_load(2);
    ok &= scb.len[1] != 0u && scn_decode(scb.data + scb_off(1), scb.len[1], SCN_BUF, tm);
    ok &= SCN_BUF->timing[0][2] == 50 && trk[0].p[P_SSWING] == 35 && song.g[G_SWING] == 0;
    ck("SCENES: a stored scene's SWG takes the global (20 -> 50), the scene playing its tracks' (5 -> 35)", ok);
}

static void test_song(void)
{
    project_store_t a, b;
    int ok;
    ui_power_on();
    stop_transport();
    own(10, 10, 10, 10);
    song.g[G_SWING] = 30;
    ok = project_save_as(0, "A") == 0;
    a = proj_slot[0];
    song.g[G_SWING] = 0;
    ok &= project_save_as(1, "B") == 0;
    b = proj_slot[1];
    ui_power_on();
    proj_slot[0] = a;
    proj_slot[1] = b;
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(0, 1);
    ok &= chain_prepare() == 0;
    ok &= chain.source[0].timing[0][2] == 40 && chain.source[1].timing[0][2] == 10 && chain.source[0].timing[3][2] == 40;
    seq_stop(); chain_config.count = 0; chain.armed = 0;
    ck("SONG: each slot its own global in its tracks' SWG (A 10 + 30 = 40, B 10 + 0 = 10)", ok);
}

static void test_page(void)
{
    uint32_t i;
    int ok = 1;
    ui_power_on();
    go_title("CLOCK");
    frame();
    for (i = 0; i < 4u; i++)
        ok &= cur_page()->id[i] != G_SWING;
    ok &= GP[G_SWING].min == 0 && GP[G_SWING].max == 0;
    song.g[G_SWING] = 0;
    trk[0].p[P_SSWING] = 25;
    ok &= track_swing(&trk[0]) == 25;
    ck("PAGE: CLOCK has no SWG; the global's range 0 .. 0 (the editor, MOMENTARY skip it); a track its own", ok);
}

int main(void)
{
    test_load();
    test_scenes();
    test_song();
    test_page();
    if (bad) {
        printf("swingfold: %d FAILED\n", bad);
        return 1;
    }
    printf("swingfold: all ok\n");
    return 0;
}
