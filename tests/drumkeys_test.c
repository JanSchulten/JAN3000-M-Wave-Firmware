/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The drum keys (firmware/src/seq.c kit_key, eng_drum.c kit_sel / kit_kp, M-Vave-OP1), through the UI harness:
 *   LANES    on a DRUM and a VDRUM track (HOME, not the grid) the black keys are the grid's lanes: a black key strikes its
 *            lane as designed and picks it (each track its own; KICK at power-on); the black keys past the lanes (DRUM
 *            9..11, VDRUM 7..11) are silent and pick nothing
 *   PITCH    a white key strikes the lane picked a semitone a key from F4 (F3 -7 .. G5 +8), OCT -/+ an octave each; the
 *            hit's pitch reaches the drum: DRUM's oscillator a factor 2^(st / 12) faster (past TUNE's range too: a
 *            snare +8), VDRUM's part st x 16 in 1/16 semitones (every part, the TOM its note's own on top)
 *   AGAIN    with the lane's black key still held, each white key strikes again (no note held twice is skipped)
 *   OTHERS   a hit that is no key's (the sequencer's, MIDI's) plays as designed after a pitched one
 *   REC      live recording writes the lane's hit, not the pitch
 *   MESSAGE  the lane and its pitch in the header: "SNARE", "SNARE +3", "SNARE -7", "KICK 0"
 *   SOUNDS   EDIT > SOUNDS: a black key picks its lane, a white key the lane picked
 *   GRID     SEQ > STEP's grid as before: a black key its lane, the white keys the steps (silent)
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <math.h>

static int bad;
static int ck(const char *what, int ok)
{
    printf("drumkeys: %-100s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

/* track 1 on engine e, settled (its fade done), HOME shown */
static track_t *kit_home(uint32_t e)
{
    int32_t o[2u * CTL];
    uint32_t i;
    ui_power_on();
    track_select(0);
    set_engine_of(TSEL, e);
    for (i = 0; i < 256u && (TSEL->xf_on || TSEL->eng_req != TSEL->engine); i++)
        mix_block(o, CTL);
    go_home();
    frame();
    return TSEL;
}
static void key_hit(uint32_t k) { key_down(k); key_up(k); }   /* (ui_test.c: key_down, key_up, black, white) */
static drum_lane_t *lane(track_t *t, uint32_t l) { return &drum_kit_of(t)[l]; }
static void render(void) { int32_t o[2u * CTL]; mix_block(o, CTL); }

/* ------------------------------------------------------------- DRUM --- */
static void test_drum(void)
{
    track_t *t = kit_home(ENGI_DRUM);
    uint32_t inc0, inc12, p, n0;
    int ok;
    ok = kit_sel[0] == 0u && kit_track(t) == NLANE;
    key_hit(black(1));                                 /* SNARE */
    ok &= kit_sel[0] == 1u && lane(t, DV_SNARE)->owner && lane(t, DV_SNARE)->kp == 0 && kit_kp == 0;
    frame();
    ok &= msg_is("SNARE");
    ck("LANES: DRUM, black key 2 strikes SNARE as designed and picks it (KICK at power-on); its name said", ok);
    n0 = kit_ev.n;
    for (p = 8; p < 11u; p++)
        key_hit(black(p));
    ok = kit_sel[0] == 1u && kit_ev.n == n0;
    ck("  black keys 9..11 (no lane): silent, the lane picked stays", ok);

    key_hit(white(7));                                 /* F4: as designed */
    render();
    inc0 = lane(t, DV_SNARE)->c.inc[0];
    ok = lane(t, DV_SNARE)->kp == 0;
    frame();
    ok &= msg_is("SNARE 0");
    key_hit(white(10));                                /* +3 */
    ok &= lane(t, DV_SNARE)->kp == 3 && kit_kp == 0;
    frame();
    ok &= msg_is("SNARE +3");
    key_hit(white(0));                                 /* F3: -7 */
    ok &= lane(t, DV_SNARE)->kp == -7;
    frame();
    ok &= msg_is("SNARE -7");
    key_hit(white(15));                                /* G5: +8 (past TUNE's range: the snare's goes to +6.4) */
    render();
    ok &= lane(t, DV_SNARE)->kp == 8 && fabs((double)lane(t, DV_SNARE)->c.inc[0] / inc0 - pow(2.0, 8.0 / 12.0)) < 0.01;
    printf("drumkeys:   (SNARE +8: the oscillator x%.4f, 2^(8/12) = %.4f)\n", (double)lane(t, DV_SNARE)->c.inc[0] / inc0,
           pow(2.0, 8.0 / 12.0));
    ck("PITCH: white keys strike SNARE a semitone a key from F4 (0, +3, F3 -7, G5 +8 at 2^(8/12): past TUNE's range)", ok);
    song.octave = 1;
    key_hit(white(7));
    render();
    inc12 = lane(t, DV_SNARE)->c.inc[0];
    ok = lane(t, DV_SNARE)->kp == 12 && fabs((double)inc12 / inc0 - 2.0) < 0.01;
    song.octave = -2;
    key_hit(white(3));                                 /* B3: -4 - 24 */
    ok &= lane(t, DV_SNARE)->kp == -28;
    song.octave = 0;
    ck("  OCT+: F4 an octave up (the oscillator x2); OCT- twice: B3 -28", ok);

    key_hit(black(0));                                 /* KICK picked */
    key_hit(white(12));                                /* +5 */
    ok = kit_sel[0] == 0u && lane(t, DV_KICK)->kp == 5;
    frame();
    ok &= msg_is("KICK +5");
    ck("  black key 1 picks KICK: the white keys strike it (+5)", ok);

    key_down(black(1));                                /* SNARE held .. */
    drum_lit[0] = 0;
    key_hit(white(9));                                 /* .. a white key strikes it again */
    ok = (drum_lit[0] >> DV_SNARE) & 1u;
    drum_lit[0] = 0;
    key_down(white(11));
    key_down(white(4));                                /* (two white keys at once: both strike) */
    ok &= (drum_lit[0] >> DV_SNARE) & 1u && lane(t, DV_SNARE)->kp == -3;
    key_up(white(11));
    key_up(white(4));
    key_up(black(1));
    ck("AGAIN: SNARE's black key held, white keys strike it again (one at a time, two at once)", ok);

    trk_note_on(t, 38, 100);                           /* (the sequencer's or MIDI's: as designed) */
    ok = lane(t, DV_SNARE)->kp == 0;
    trk_note_on(t, 41, 100);                           /* (a low tom: the GM map's own pitch, no key's) */
    ok &= lane(t, DV_TOM)->kp == 0 && lane(t, DV_TOM)->st == -7;
    ck("OTHERS: a hit that is no key's plays as designed (a snare), a GM note at its own pitch (a low tom -7)", ok);

    song.rec = 1u << 0;                                /* REC: the hit, not the pitch */
    song.playing = 1;
    t->seq_idx = 5;
    t->seq_pos = 0;
    key_hit(white(14));                                /* SNARE +7 */
    ok = t->step[5].hit == 1u << DV_SNARE && !t->step[5].n;
    song.rec = 0;
    song.playing = 0;
    ck("REC: a pitched white key writes SNARE's hit (no pitch, no note)", ok);
}

/* ------------------------------------------------------------ VDRUM --- */
static void test_vdrum(void)
{
    track_t *t = kit_home(ENGI_VDRUM);
    vd_mem_t *M = vd_mem_of(t);
    uint32_t n0, p;
    int ok;
    ok = kit_track(t) == VD_NP;
    key_hit(black(3));                                 /* HATCL */
    ok &= kit_sel[0] == 3u && M->part[3].owner && M->part[3].st16 == 0;
    key_hit(white(9));
    ok &= M->part[3].st16 == 2 * 16;
    frame();
    ok &= msg_is("HATCL +2");
    ck("LANES / PITCH: VDRUM, black key 4 strikes HATCL and picks it; a white key +2 (st16 32)", ok);
    n0 = kit_ev.n;
    for (p = 6; p < 11u; p++)
        key_hit(black(p));
    ok = kit_sel[0] == 3u && kit_ev.n == n0;
    ck("  black keys 7..11 (six parts): silent, the part picked stays", ok);
    key_hit(black(5));                                 /* TOM: its own note 45 at 0 */
    key_hit(white(6));
    ok = M->part[5].st16 == -16;
    key_hit(black(1));
    song.octave = 1;
    key_hit(white(8));
    ok &= M->part[1].st16 == 13 * 16;
    song.octave = 0;
    trk_note_on(t, 38, 100);
    ok &= M->part[1].st16 == 0;
    trk_note_on(t, 41, 100);                           /* (a low tom: 4 below the TOM's 45) */
    ok &= M->part[5].st16 == -4 * 16;
    ck("  TOM -1, SNARE with OCT+ +13; the sequencer's snare as designed, a GM low tom its own -4", ok);
    track_select(1);                                   /* (each track its own lane picked) */
    ok = kit_sel[1] == 0u && kit_sel[0] == 1u;
    ck("  each track its own lane picked", ok);
}

/* ----------------------------------------------------- SOUNDS, GRID --- */
static void test_pages(void)
{
    track_t *t = kit_home(ENGI_DRUM);
    uint32_t k;
    int ok;
    go_title("SOUNDS");
    ok = cur_page()->graph == GR_DLANE;
    tap_key(black(4));                                 /* HATOP */
    ok &= ui.lane == DV_HATO;
    kit_sel[0] = 2;                                    /* (CLAP picked by the keys) */
    tap_key(white(3));
    ok &= ui.lane == 2u;
    ck("SOUNDS: a black key picks its lane (HATOP), a white key the lane picked (CLAP)", ok);
    go_home();
    frame();
    fam_next(FAM_SEQ);
    frame();
    ok = grid_on();
    for (k = 0; k < 16u; k++)
        step_clear(&t->step[k]);
    tap_key(black(1));                                 /* the grid: a black key its lane .. */
    ok &= ui.lane == DV_SNARE;
    drum_lit[0] = 0;
    tap_key(white(2));                                 /* .. a white key toggles step 3, silent */
    keyboard_block();
    ok &= (t->step[2].hit >> DV_SNARE) & 1u && !drum_lit[0];
    ck("GRID: SEQ > STEP as before: a black key picks its lane, a white key toggles its step (silent)", ok);
}

int main(void)
{
    test_drum();
    test_vdrum();
    test_pages();
    if (bad) {
        printf("drumkeys: %d FAILED\n", bad);
        return 1;
    }
    printf("drumkeys: all ok\n");
    return 0;
}
