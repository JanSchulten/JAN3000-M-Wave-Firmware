/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* JAN3000 v0.8's workflow changes (M-Vave-OP1), on the firmware's own sources through the UI harness:
 *   PAGES    another track (ALGORITHM) opens each family at its first page: FX left on CHORUS, a turn of ALGORITHM, FX
 *            again: SEND (the family shown stays where it is)
 *   UNDO     REC armed takes the track's copy; REC held: OCT- undoes what was recorded since, OCT+ redoes it, each
 *            once in a row (NOTHING TO REDO / ALREADY UNDONE); a clear or CLICK changed in the same hold: OCT- puts
 *            those back as before
 *   NEW      SAVE > PROJECT, KNOB 2 NEW, OCT+ asks, OCT+ again: the power-on sounds (808 kit) with empty steps, the
 *            default BPM, no song, FX TRACKS, KEY or CHORUS TYPE, no slot or name; while playing: STOP FIRST
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("jan: %-103s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }

static void test_pages(void)
{
    int ok;
    ui_power_on();
    frames(320);
    press(B_FX); frames(400);
    press(B_FX); frames(400);
    press(B_FX); frames(400);
    ok = str_eq(cur_page()->title, "CHORUS");
    press(B_ENV); frames(400);
    turn(EN_ALGO, 1);
    frame();
    ok &= song.sel == 1u && str_eq(cur_page()->title, "ENV");   /* (the family shown stays) */
    press(B_FX); frames(400);
    ck("another track: FX opens at its first page (SEND), not where the last track left it (CHORUS)",
       ok && str_eq(cur_page()->title, "FX"));
    press(B_FX); frames(400);
    press(B_FX); frames(400);
    turn(EN_ALGO, -1);
    frame();
    ck("  the family shown when the track changes stays on its page (CHORUS)", str_eq(cur_page()->title, "CHORUS"));
}

static void rec_hold_oct(uint32_t b)                  /* REC held (its layer), OCT- or OCT+, let go */
{
    btn_down(B_REC); frames(500);
    press(b); frame();
    btn_up(B_REC); frames(400);
}
static void test_undo(void)
{
    int ok;
    track_t *t;
    ui_power_on();
    frames(320);
    t = TSEL;
    memset(t->step, 0, sizeof t->step);
    t->step[0].note[0] = 60; t->step[0].n = 1; t->step[0].vel = 100;   /* (what was there before) */
    press(B_REC); frames(400);
    ok = (song.rec & 1u) != 0u && undo.trk == 1u && !undo.undone;
    t->step[4].note[0] = 67; t->step[4].n = 1; t->step[4].vel = 100;   /* (what the recording added) */
    t->step[0].note[0] = 62;
    transport_req = 2;
    blk();
    rec_hold_oct(B_OCTUP);
    ok &= msg_is("NOTHING TO REDO") && t->step[4].n == 1u;
    rec_hold_oct(B_OCTDN);
    ok &= t->step[4].n == 0u && t->step[0].note[0] == 60u && undo.undone;
    ck("REC armed takes the track's copy; REC + OCT- undoes the recording (OCT+ before: nothing to redo)", ok);
    rec_hold_oct(B_OCTDN);
    ok = msg_is("ALREADY UNDONE") && t->step[4].n == 0u;
    rec_hold_oct(B_OCTUP);
    ok &= t->step[4].n == 1u && t->step[4].note[0] == 67u && t->step[0].note[0] == 62u && !undo.undone;
    ck("  REC + OCT- again: already undone; REC + OCT+ redoes it", ok);
}

static void test_new(void)
{
    int ok;
    uint32_t k, notes = 0;
    ui_power_on();
    frames(320);
    trk[0].step[2].note[0] = 64; trk[0].step[2].n = 1; trk[0].step[2].vel = 100;
    song.g[G_BPM] = 150;
    keyt.on = 1; keyt.root[0] = 9;
    chos.type = CHO_FLANGER;
    fxt_lane[0].len = 8; fxt_lane[0].step[1] = 3;
    chain_config.count = 2;
    proj_cur = 1;
    go_title("PROJECT");
    frame();
    transport_req = 1;
    blk();
    turn(EN_K2, 1);
    frame();
    press(B_OCTUP); frame();
    ok = msg_is("STOP FIRST") && !ui.confirm;
    transport_req = 2;
    blk();
    frames(100);
    press(B_OCTUP); frame();
    ok &= ui.confirm == CF_NEW_PROJ;
    press(B_OCTUP); frames(50);
    for (k = 0; k < NTRK; k++)
        notes += !seq_is_empty(&trk[k]);
    ok &= !ui.confirm && msg_is("NEW PROJECT") && !notes && song.g[G_BPM] == GP[G_BPM].def && !keyt.on &&
          keyt.root[0] == KEY_NONE && chos.type == CHO_CHORUS && !fxt_lane[0].len && !chain_config.count &&
          proj_cur == PROJ_NO_SLOT && !proj_name[0] && trk[3].eng_req == ENGI_DRUM && str_eq(N_DRUM_KIT[trk[3].p[P_E0]], "808");
    ck("SAVE > PROJECT > NEW (KNOB 2, OCT+, OCT+): power-on sounds (808), empty steps, default BPM, no song / FX TRACKS / "
       "KEY / CHORUS TYPE, no slot; while playing: STOP FIRST", ok);
}

int main(void)
{
    test_pages();
    test_undo();
    test_new();
    printf("%s\n", bad ? "JAN TEST FAILED" : "jan test passed");
    return bad != 0;
}
