/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SEQ held > SONG (M-Vave-OP1, firmware/src/scene.c ssong, ui_layer.c), on the firmware's own sources (ui_test.c's
 * harness):
 *   LEVEL    SEQ held: SCENES; SELECT turns it to SONG ([SEQ] SONG) and back, kept for the next hold; the SEQ pages stay
 *            STEP DETAIL AUTOMATION. SONG: the white keys add their scene to the chain's end (the last one's again: a
 *            pass more), KNOB 1 the cell, 2 its scene, 3 its REPS, 4 END; OCT- / OCT+ the cell before / after; EDIT
 *            deletes the cell; F#3 CLEAR (the dialog); PLAY with no cell: ADD A SCENE FIRST; the LEDs
 *   PLAY     the rows one after another, each its REPS passes of its scene (a pass: the loop of its longest track),
 *            the switch on the block the pass ends in (the remainder carried); a row on the scene playing needs no swap;
 *            END STOP stops after the last row, END LOOP goes on with the first; MENU > SCENE CHANGE NEXT BAR does not
 *            change it (always at the scene's end)
 *   OFF      a pick by hand (SEQ + a key) ends the song there (the pick waits as ever); STOP ends it, nothing switches
 *   EDIT     a row's REPS changed while it plays: taken (live edits allowed)
 *   KEPT     SAVE / LOAD keep the song (the extras' tail, XTRA_V8); extras of before (XTRA_V7): none; NEW: none
 *   With a directory: the page's screen.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("ssong: %-96s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        bad++;
    return ok;
}
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }
static void melody(track_t *t, uint32_t base, uint32_t len)   /* 1/16 notes on every other step, LEN len */
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        step_clear(&t->step[i]);
    for (i = 0; i < len; i += 2u) {
        uint8_t n = (uint8_t)(base + i % 12u);
        put_step(t, i, 1, &n, ST_NOTE, 0);
    }
    t->p[P_SLEN] = (int16_t)len;
    t->p[P_SDIV] = 2;
    t->p[P_SSWING] = 0;
}
static void seq_pick(uint32_t s)                     /* SEQ held (SCENES) + white key s, let go */
{
    btn_down(B_SEQ);
    frame();
    if (ui.seq_lv)
        turn(EN_SELECT, -1);
    key_down(white(s));
    frame();
    key_up(white(s));
    btn_up(B_SEQ);
    frame();
    frames(400);                                      /* (two SEQ taps within 300 ms lock its layer) */
}
/* scenes 1, 2, 3: T1 16, 8 and 32 steps of 1/16 (its notes from 60, 72, 48); scene 1 playing, stopped */
static void three_scenes(void)
{
    ui_power_on();
    stop_transport();
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    melody(&trk[0], 60, 16);
    seq_pick(1);
    melody(&trk[0], 72, 8);
    seq_pick(2);
    melody(&trk[0], 48, 32);
    seq_pick(0);
    ssong_clear();
}
static void song_rows(const uint8_t *sc, const uint8_t *rep, uint32_t n, uint32_t loop)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        ssong.scene[i] = sc[i];
        ssong.repeat[i] = rep[i];
    }
    ssong.count = (uint8_t)n;
    ssong.flags = loop ? SSG_LOOP : 0u;
}
/* the audio and the UI on, a UI frame every 22 blocks (16 ms), until the scene playing is `want` or the transport
 * stopped (want 0xFF: until it stops); the block it changed in, max none */
static uint32_t run_until(uint32_t want, uint32_t max)
{
    uint32_t b;
    for (b = 0; b < max; b++) {
        blk();
        if ((want != 0xFFu && scn.cur == want) || (want == 0xFFu && !song.playing && transport_req != 1u))
            return b;
        if (b % 22u == 21u)
            frame();
    }
    return max;
}
static uint32_t pass_blocks(uint32_t steps) { return steps * div_samples(2) / CTL; }   /* (a pass, about) */

static void ltap(uint32_t k) { key_down(k); frame(); key_up(k); frame(); }   /* a key in the layer (the ISR's scan) */
static void song_level(void)                          /* SEQ held (it stays down), the SONG level */
{
    btn_down(B_SEQ);
    frame();
    if (!ui.seq_lv)
        turn(EN_SELECT, 1);
    frame();
}
static void seq_let_go(void) { btn_up(B_SEQ); frame(); frames(400); }
static void song_play(void) { song_level(); press(B_PLAY); seq_let_go(); }   /* SEQ held > SONG + PLAY */

static void test_level(const char *dir)
{
    uint32_t m, br, i, steps = 1;
    three_scenes();
    for (i = 0; i < NPAGES; i++)
        steps &= !str_eq(PAGES[i].title, "SCENE SONG");
    btn_down(B_SEQ);
    frame();
    turn(EN_SELECT, 1);
    ck("LEVEL: SEQ held + SELECT: [SEQ] SONG (the SEQ pages stay the step pages: no SCENE SONG page)",
       ui.layer == LAYER_SEQ && ui.seq_lv == 1u && str_eq(layer_head(), "[SEQ] SONG") && steps && !ssong.count);
    press(B_PLAY);
    ck("  PLAY with no cell: ADD A SCENE FIRST, nothing plays", msg_is("ADD A SCENE FIRST") && !song.playing &&
       transport_req != 1u);
    ltap(white(1));                                /* scene 2 */
    ltap(white(1));                                /* .. again: x2 */
    ltap(white(0));                                /* scene 1 */
    ltap(white(2));                                /* scene 3 */
    ck("  the white keys add their scenes (the last one again: a pass more): 2 x2, 1 x1, 3 x1; the cell on the last",
       ssong.count == 3u && ssong.scene[0] == 1u && ssong.repeat[0] == 2u && ssong.scene[1] == 0u &&
       ssong.scene[2] == 2u && ui.ssg_row == 2u && msg_is("SCENE 3 X1"));
    turn(EN_K3, 2);                                   /* (cell 3: x3) */
    turn(EN_K4, 1);
    press(B_OCTDN);
    turn(EN_K2, 3);                                   /* (cell 2: scene 4) */
    ck("  KNOB 3 the cell's REPS, KNOB 4 END LOOP; OCT- the cell before, KNOB 2 its scene", ssong.repeat[2] == 3u &&
       (ssong.flags & SSG_LOOP) && ui.ssg_row == 1u && ssong.scene[1] == 3u);
    turn(EN_K1, 5);                                   /* (past the last: +) */
    turn(EN_K2, 1);                                   /* (adds the scene playing, that only) */
    ck("  KNOB 1 past the last cell: +; KNOB 2 there adds a cell on the scene playing", ssong.count == 4u &&
       ssong.scene[3] == 0u && ssong.repeat[3] == 1u && ui.ssg_row == 3u);
    fm1_ms = 3u * SEQ_BLINK_MS;
    m = layer_leds(&br);
    ck("  the LEDs: the scene playing blinks, the used ones lit, F#3 CLEAR breathes", ((m >> white(0)) & 1u) &&
       ((m >> white(1)) & 1u) && ((br >> black(0)) & 1u) && !((m >> white(5)) & 1u));
    if (dir) {
        ui.force = 1;
        frame();
        screen_save(dir, "seq-song");
    }
    press(B_EDIT);
    ck("  EDIT deletes the cell selected (the one added): 3 cells", ssong.count == 3u && msg_is("ROW DELETED"));
    seq_let_go();
    btn_down(B_SEQ);
    frame();
    frames(600);                                      /* (a peek: the layer shows) */
    ck("  the next hold opens on SONG again; SELECT back: [SEQ] SCENES", ui.seq_lv == 1u && ui.layer == LAYER_SEQ &&
       (turn(EN_SELECT, -1), ui.seq_lv == 0u) && str_eq(layer_head(), "[SEQ] SCENES"));
    seq_let_go();
    song_level();
    ltap(black(0));
    ck("  F#3: CLEAR THE SCENE SONG? OCT+ clears it", ui.confirm == CF_CLEAR_SSONG);
    btn_up(B_SEQ);
    frame();
    press(B_OCTUP);
    ck("  .. cleared", !ssong.count && msg_is("SONG CLEARED"));
    frames(400);
}

static void test_play(void)
{
    static const uint8_t SC[3] = {0, 1, 2}, REP[3] = {2, 1, 1};
    uint32_t b, want, t0 = 0;
    three_scenes();
    song_rows(SC, REP, 3, 0);
    ui.ssg_row = 0;
    song_play();
    ck("PLAY: SEQ held > SONG + PLAY starts the song on cell 1 (scene 1), SONG: SCENE 1", ssp.on && ssp.row == 0u &&
       ssp.left == 2u && scn.cur == 0u && msg_is("SONG: SCENE 1"));
    b = run_until(1, 20000);
    want = 2u * pass_blocks(16);                      /* scene 1 twice (16 steps), then scene 2 */
    printf("  scene 2 after %u blocks (two passes of scene 1: %u)\n", b, want);
    ck("  scene 1 x2, then scene 2: on the block its second pass ends, step 1 there", b + 1u >= want && b <= want + 1u &&
       trk[0].seq_idx == 0u && trk[0].step[0].note[0] == 72u && ssp.row == 1u);
    t0 = b;
    b = run_until(2, 20000);
    printf("  scene 3 after %u more blocks (a pass of scene 2: %u)\n", b, pass_blocks(8));
    ck("  then scene 3 after one pass of scene 2", b + 1u >= pass_blocks(8) && b <= pass_blocks(8) + 1u && ssp.row == 2u);
    b = run_until(0xFFu, 40000);
    ck("  END STOP: after scene 3's pass the transport stops, the song over", b + 2u >= pass_blocks(32) &&
       b <= pass_blocks(32) + 2u && !ssp.on && scn.cur == 2u);
    (void)t0;
    /* LOOP: back to row 1; MENU > SCENE CHANGE NEXT BAR does not move the switches */
    three_scenes();
    song_rows(SC, REP, 2, 1);
    menu_put(MI_SCENE, 1);
    frame();
    ui.ssg_row = 0;
    song_play();
    b = run_until(1, 20000);
    ck("LOOP, NEXT BAR set: scene 2 after the two passes of scene 1 all the same (not at the bar)",
       b + 1u >= 2u * pass_blocks(16) && b <= 2u * pass_blocks(16) + 1u);
    b = run_until(0, 20000);
    ck("  END LOOP: after scene 2 the first row again (scene 1)", b + 1u >= pass_blocks(8) && b <= pass_blocks(8) + 1u &&
       ssp.on && ssp.row == 0u && ssp.left == 2u);
    menu_put(MI_SCENE, 0);
    /* a row on the scene playing: no swap, its passes counted on */
    {
        static const uint8_t S2[2] = {0, 0}, R2[2] = {1, 2};
        uint32_t n;
        three_scenes();
        song_rows(S2, R2, 2, 0);
        ui.ssg_row = 0;
        song_play();
        for (n = 0; n < pass_blocks(16) + 4u; n++) {
            blk();
            if (n % 22u == 21u)
                frame();
        }
        ck("  a row on the scene playing (scene 1 x1, scene 1 x2): no swap, row 2 counts on", ssp.row == 1u &&
           ssp.left == 2u && scn.cur == 0u && scn.req == SCN_IDLE);
        transport_req = 2;
        blk();
        frame();
    }
}

static void test_off(void)
{
    static const uint8_t SC[2] = {0, 1}, REP[2] = {4, 4};
    uint32_t b;
    three_scenes();
    song_rows(SC, REP, 2, 1);
    ui.ssg_row = 0;
    song_play();
    for (b = 0; b < 300u; b++) {
        blk();
        if (b % 22u == 21u)
            frame();
    }
    song_level();
    ui.ssg_row = 0;
    turn(EN_K3, -3);                                  /* (edited while it plays: cell 1 now x1) */
    seq_let_go();
    b = run_until(1, 20000);
    ck("EDIT: row 1's REPS turned down to x1 while it plays: scene 2 after its pass now", ssp.on && ssp.row == 1u &&
       b < pass_blocks(16));
    seq_pick(2);
    ck("OFF: a pick by hand (SEQ + G3) ends the song, the pick waits (NEXT: SCENE 3)", !ssp.on && scn.req == SCN_END &&
       scn.next == 2u && msg_is("NEXT: SCENE 3"));
    transport_req = 2;
    blk();
    frame();
    three_scenes();
    song_rows(SC, REP, 2, 1);
    ui.ssg_row = 0;
    song_play();
    for (b = 0; b < 4u * pass_blocks(16) - 30u; b++) {   /* (into row 1's last pass: scene 2 waiting) */
        blk();
        if (b % 22u == 21u)
            frame();
    }
    song_level();
    press(B_PLAY);                                    /* STOP */
    seq_let_go();
    blk();
    frame();
    frames(100);
    ck("  STOP in the last pass (the next scene waiting): the song over, nothing switches", !ssp.on && !song.playing &&
       scn.cur == 0u && scn.req == SCN_IDLE);
}

static void test_kept(void)
{
    static const uint8_t SC[3] = {2, 0, 1}, REP[3] = {3, 1, 16};
    ssong_t keep;
    three_scenes();
    song_rows(SC, REP, 3, 1);
    keep = ssong;
    project_save(1);
    ssong_clear();
    project_load(1);
    ck("KEPT: SAVE / LOAD: the song as it was (rows, REPS, END LOOP)", !memcmp(&ssong, &keep, sizeof keep));
    xtra_slot[1].size = XTRA_V7;                      /* (extras of before the scene song) */
    project_load(1);
    ck("  extras of before (XTRA_V7): no song", !ssong.count && !ssong.flags);
    ssong = keep;
    project_new(0xFFu);
    ck("  NEW PROJECT: no song", !ssong.count);
}

int main(int argc, char **argv)
{
    test_level(argc > 1 ? argv[1] : 0);
    test_play();
    test_off();
    test_kept();
    if (bad) {
        printf("ssong: %d FAILED\n", bad);
        return 1;
    }
    printf("ssong: all ok\n");
    return 0;
}
