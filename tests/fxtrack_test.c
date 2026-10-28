/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* FX TRACKS (firmware/src/fxtrack.c), on the real sequencer, effects (perform.c) and UI sources (the stubs of
 * tests/ui_test.c), the audio ISR driven a block at a time (fx.c mix_block):
 *   PLAY     power-on empty; an effect from its step to the end of its TIEs; a lane's LEN; two lanes of different
 *            lengths; step for step with a 1/16 track (no drift); the same effect twice in a row plays as one; a lane's
 *            REPEAT starts on its step (not a 1/16 later); a mute step mutes; STOP ends everything, PLAY from step 1.
 *   RECORD   only armed and playing; a press early in a step onto it, late onto the next (a tap too); held on: TIEs;
 *            let go: nothing written (overdub); a punch inside an older take gives the rest of it back; FX LATCH; the
 *            lanes' own effects are never recorded; the black keys' mutes.
 *   UI       FX's pages (FX, FX TRACKS, FX TRACKS 2, SLICER ..), KNOB 1..4 (lane, STEP, EFFECT, LEN), REC arms the
 *            selected lane (PLAY starts it), the FX layer's keys record end to end, CLEAR (a lane, both, nothing there);
 *            screens (with a directory argument).
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int32_t fx_out[2 * CTL];
static void fx_power_on(void)
{
    ui_power_on();
    perf_latched = 0;
    perf_latch_on = 0;
    perf_kill = 0;
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    trk[0].p[P_SDIV] = 2;                         /* (a 1/16 track, straight, 16 steps) */
    trk[0].p[P_SSWING] = 0;
    trk[0].p[P_SLEN] = 16;
}
static void blk(void) { mix_block(fx_out, CTL); }
static void play(void) { transport_req = 1; blk(); }
static void stop(void) { transport_req = 2; blk(); }
static uint32_t P16(void) { return div_samples(2); }
/* on until lane l plays step i, frac/100 of the way into it (frac 0: its first block) */
static int run_to(uint32_t l, uint32_t i, uint32_t frac)
{
    uint32_t guard = 0;
    while (!(fxt.idx[l] == i && fxt.pos >= frac * P16() / 100u) && guard++ < 200000u)
        blk();
    return guard < 200000u;
}
static void next_step(void) { uint32_t i = fxt.idx[0]; while (fxt.idx[0] == i) blk(); }   /* to the next step's first block */
static uint8_t C(uint32_t e) { return fxt_code(e); }

static void test_play(void)
{
    uint32_t i, ok, now;
    fx_power_on();
    bad += check("power-on: both lanes empty, 16 steps, nothing held",
                 !fxt_used(0) && !fxt_used(1) && FXT_LEN(&fxt_lane[0]) == 16u && !perf_fxt && !fxt.arm);
    fxt_lane[0].step[0] = C(PF_LPF);
    fxt_lane[0].step[1] = FXT_TIE;
    fxt_lane[0].step[2] = FXT_TIE;
    fxt_lane[0].step[4] = C(PF_HPF);
    play();
    ok = perf_fxt == PF_BIT(PF_LPF) && (perf_act & PF_BIT(PF_LPF));
    run_to(0, 2, 99);
    ok &= perf_fxt == PF_BIT(PF_LPF);
    next_step();
    ok &= fxt.idx[0] == 3u && !perf_fxt && !(perf_act & PF_BIT(PF_LPF));
    next_step();
    ok &= perf_fxt == PF_BIT(PF_HPF) && (perf_act & PF_BIT(PF_HPF));
    next_step();
    ok &= !perf_fxt;
    bad += check("an effect plays from its step to the end of its TIEs; the next effect on its step", ok);
    stop();
    bad += check("STOP: nothing held, the effects let go", !perf_fxt && !(perf_act & (PF_BIT(PF_LPF) | PF_BIT(PF_HPF))));
    play();
    bad += check("PLAY: from step 1 again", fxt.idx[0] == 0u && perf_fxt == PF_BIT(PF_LPF));
    stop();
    /* step for step with a 1/16 track, 3 bars at an odd tempo (the remainder of each step kept, as the tracks') */
    fx_power_on();
    song.g[G_BPM] = 97;
    fxt_lane[0].len = 16;
    play();
    ok = 1;
    for (i = 0; i < 3u * 16u * 6000u / CTL; i++) {
        blk();
        ok &= fxt.idx[0] == trk[0].seq_idx % 16u;
    }
    bad += check("the lane's steps start in the very block a 1/16 track's do (3 bars at 97 BPM: no drift)", ok);
    stop();
    fx_power_on();
    fxt_lane[0].len = 3;
    fxt_lane[1].len = 4;
    play();
    ok = 1;
    for (i = 1; i <= 12u; i++) {
        next_step();
        ok &= fxt.idx[0] == i % 3u && fxt.idx[1] == i % 4u;
    }
    bad += check("LEN: a lane wraps at its length; FX1 3 and FX2 4 steps run on against each other", ok);
    stop();
    fx_power_on();
    fxt_lane[0].step[0] = C(PF_R16);
    fxt_lane[0].step[1] = C(PF_R16);
    fxt_lane[0].step[2] = C(PF_R8);
    play();
    now = perf_ord[PF_R16];
    next_step();
    ok = perf_ord[PF_R16] == now && perf_fxt == PF_BIT(PF_R16) && (perf_act & PF_BIT(PF_R16));
    next_step();
    ok &= perf_fxt == PF_BIT(PF_R8);
    bad += check("the same effect on two steps in a row: one effect, not started again (a REPEAT keeps its loop)", ok);
    stop();
    /* a lane's REPEAT starts in the block of its step; a key pressed there waits for the next 1/16 */
    fx_power_on();
    fxt_lane[0].step[3] = C(PF_R16);
    play();
    run_to(0, 2, 50);
    next_step();
    ok = (perf_act & PF_BIT(PF_R16)) != 0u;
    stop();
    fx_power_on();
    play();
    run_to(0, 3, 0);
    perf_press(PF_R16, 1);
    blk();
    ok &= !(perf_act & PF_BIT(PF_R16));
    perf_press(PF_R16, 0);
    stop();
    bad += check("a lane's REPEAT 1/16 runs from its step's block; a key pressed there waits for the next 1/16", ok);
    fx_power_on();
    fxt_lane[1].step[0] = FXT_MUTE | 1u;
    fxt_lane[1].step[1] = FXT_TIE;
    play();
    blk();
    ok = perf_fxt == PF_BIT(PF_M1 + 1u) && ((perf_act >> (PF_M1 + 1u)) & 1u);
    run_to(1, 2, 0);
    blk();
    ok &= !perf_fxt;
    stop();
    bad += check("a mute step: track 2 muted (as its black key holds it) to the end of its TIE", ok);
}

static void test_record(void)
{
    uint32_t i, ok;
    fx_power_on();
    play();
    run_to(0, 2, 10);
    perf_press(PF_LPF, 1);
    perf_press(PF_LPF, 0);
    bad += check("not armed: nothing recorded", !fxt_used(0) && !fxt_used(1));
    stop();
    fxt.arm = 1;
    perf_press(PF_LPF, 1);
    perf_press(PF_LPF, 0);
    bad += check("armed, stopped: nothing recorded", !fxt_used(0));
    play();
    run_to(0, 2, 10);
    perf_press(PF_LPF, 1);
    ok = fxt_lane[0].step[2] == C(PF_LPF);
    run_to(0, 5, 50);
    perf_press(PF_LPF, 0);
    ok &= fxt_lane[0].step[3] == FXT_TIE && fxt_lane[0].step[4] == FXT_TIE && fxt_lane[0].step[5] == FXT_TIE;
    run_to(0, 7, 0);
    ok &= !fxt_lane[0].step[6] && !fxt_lane[0].step[1] && !fxt_lane[1].step[2];
    bad += check("armed FX1: a press early in step 3 onto it, held on: TIEs to the step it is let go in; FX2 untouched", ok);
    run_to(0, 8, 70);
    perf_press(PF_HPF, 1);
    run_to(0, 8, 90);
    perf_press(PF_HPF, 0);
    ok = !fxt_lane[0].step[8];
    next_step();
    ok &= fxt_lane[0].step[9] == C(PF_HPF) && perf_fxt == PF_BIT(PF_HPF);
    next_step();
    ok &= !fxt_lane[0].step[10] && !perf_fxt;
    bad += check("a tap late in step 9: onto step 10 (played there), one step", ok);
    run_to(0, 0, 0);
    run_to(0, 2, 30);
    ok = perf_fxt == PF_BIT(PF_LPF);
    bad += check("the next pass plays the take (LPF steps 3..6) as recorded: overdub, nothing held writes nothing",
                 ok && fxt_used(0) == 5u);
    stop();
    /* a punch inside an older take: the take goes on after it */
    fx_power_on();
    fxt_lane[0].step[0] = C(PF_LPF);
    for (i = 1; i < 8u; i++)
        fxt_lane[0].step[i] = FXT_TIE;
    fxt.arm = 1;
    play();
    run_to(0, 2, 5);
    perf_press(PF_PHS, 1);
    run_to(0, 3, 50);
    perf_press(PF_PHS, 0);
    run_to(0, 5, 0);
    ok = fxt_lane[0].step[2] == C(PF_PHS) && fxt_lane[0].step[3] == FXT_TIE && fxt_lane[0].step[4] == C(PF_LPF) &&
         fxt_lane[0].step[5] == FXT_TIE && perf_fxt == PF_BIT(PF_LPF);
    ok &= fxt_plays(0, 1) == PF_LPF && fxt_plays(0, 3) == PF_PHS && fxt_plays(0, 7) == PF_LPF && fxt_plays(0, 8) == PF_N;
    bad += check("PHASER punched into steps 3..4 of an LPF take: LPF again from step 5 to its end", ok);
    stop();
    /* the lanes' own effects are not recorded; the GLO solos neither */
    fx_power_on();
    fxt_lane[0].step[0] = C(PF_FRZ);
    fxt_lane[0].step[1] = FXT_TIE;
    fxt.arm = 2;
    play();
    run_to(0, 3, 0);
    bad += check("what FX1 plays is not recorded onto an armed FX2", !fxt_used(1));
    stop();
    /* FX LATCH: latched on, recorded on; latched off, it ends */
    fx_power_on();
    perf_latch_on = 1;
    fxt.arm = 1;
    play();
    run_to(0, 1, 5);
    perf_press(PF_TAPE, 1);
    perf_press(PF_TAPE, 0);
    run_to(0, 3, 5);
    perf_press(PF_TAPE, 1);
    run_to(0, 5, 0);
    ok = fxt_lane[0].step[1] == C(PF_TAPE) && fxt_lane[0].step[2] == FXT_TIE && fxt_lane[0].step[3] == FXT_TIE &&
         !fxt_lane[0].step[4];
    bad += check("FX LATCH: latched on records the effect, held on as TIEs; latched off, it ends", ok);
    perf_latch_on = 0;
    perf_latched = 0;
    stop();
    /* the black keys' mutes */
    fx_power_on();
    fxt.arm = 1;
    play();
    run_to(0, 4, 10);
    perf_press(PF_M1 + 2u, 1);
    run_to(0, 5, 50);
    perf_press(PF_M1 + 2u, 0);
    ok = fxt_lane[0].step[4] == (FXT_MUTE | 2u) && fxt_lane[0].step[5] == FXT_TIE && fxt_eff(fxt_lane[0].step[4]) ==
         PF_M1 + 2u;
    stop();
    bad += check("a black key held (track 3's mute): recorded as MUTE T3", ok);
    /* two effects held: the one pressed last; let go of it, the other one again (a new start) */
    fx_power_on();
    fxt.arm = 1;
    play();
    run_to(0, 1, 5);
    perf_press(PF_LPF, 1);
    run_to(0, 2, 5);
    perf_press(PF_FLG, 1);
    run_to(0, 3, 50);
    perf_press(PF_FLG, 0);
    run_to(0, 4, 50);
    perf_press(PF_LPF, 0);
    run_to(0, 6, 0);
    ok = fxt_lane[0].step[1] == C(PF_LPF) && fxt_lane[0].step[2] == C(PF_FLG) && fxt_lane[0].step[3] == FXT_TIE &&
         fxt_lane[0].step[4] == C(PF_LPF) && !fxt_lane[0].step[5];
    stop();
    bad += check("LPF held, FLANGER over it: FLANGER from its step; let go, LPF again (its own start)", ok);
}

static void test_ui(const char *dir)
{
    uint32_t ok;
    fx_power_on();
    press(B_FX);
    ok = cur_page()->graph == GR_FX;
    frames(400);
    go_title("SLICER");
    frames(400);
    press(B_FX);
    ok &= cur_page()->graph == GR_FXT && fxt_page();
    frames(400);
    press(B_FX);
    ok &= str_eq(cur_page()->title, "MIX") && !fxt_page();
    bad += check("FX: FX first; FX TRACKS the last card (after SLICER), then FX again (M-Vave-OP1's seven)", ok);
    go_title("FX TRACKS");
    turn(EN_K1, 1);
    ok = ui.fxt_sel == 1u;
    turn(EN_K1, 5);
    ok &= ui.fxt_sel == 1u;
    turn(EN_K1, -1);
    ok &= ui.fxt_sel == 0u;
    bad += check("KNOB 1: the lane, FX1 or FX2", ok);
    turn(EN_K2, 5);
    ok = ui.fxt_cur == 5u;
    turn(EN_K2, 40);
    ok &= ui.fxt_cur == 15u;
    bad += check("KNOB 2: the step, up to the lane's LEN", ok);
    turn(EN_K3, 1);
    ok = fxt_lane[0].step[15] == FXT_TIE && msg_is("TIE: THE EFFECT GOES ON");
    turn(EN_K3, 1);
    ok &= fxt_lane[0].step[15] == C(PF_R8) && msg_is("REPEAT 1/8");
    turn(EN_K3, 5);
    ok &= fxt_eff(fxt_lane[0].step[15]) == PF_HPF;
    turn(EN_K3, 40);
    ok &= fxt_lane[0].step[15] == (FXT_MUTE | 3u) && msg_is("MUTE T4");
    turn(EN_K3, -60);
    ok &= fxt_lane[0].step[15] == 0u && msg_is("OFF");
    bad += check("KNOB 3: the step's EFFECT: OFF, TIE, the effects, the mutes (named as turned)", ok);
    turn(EN_K4, -8);
    ok = fxt_lane[0].len == 8u && ui.fxt_cur == 7u;
    turn(EN_K4, 200);
    ok &= fxt_lane[0].len == NSTEP;
    turn(EN_K4, -(int)(NSTEP - 16u));
    ok &= fxt_lane[0].len == 16u;
    bad += check("KNOB 4: LEN 1..NSTEP (the step follows it down)", ok);
    turn(EN_K1, 1);
    press(B_REC);
    ok = fxt.arm == 2u && song.rec == 0u && transport_req == 0u && msg_is("FX2 ARMED: HOLD [FX]");
    press(B_PLAY);                                  /* (M-Vave-OP1: armed only, PLAY starts) */
    ok &= transport_req == 1u;
    blk();
    frames(400);
    press(B_REC);
    ok &= fxt.arm == 0u && msg_is("FX2 DISARMED");
    stop();
    bad += check("REC on FX TRACKS: arms the selected lane (not the pattern), PLAY then starts; again: disarmed", ok);
    /* end to end: FX held, a white key, on an armed lane */
    fx_power_on();
    go_title("FX TRACKS");
    frames(400);
    press(B_REC);
    press(B_PLAY);                                  /* (M-Vave-OP1: REC arms, PLAY starts) */
    frames(100);                                    /* (the UI's frames between the ISR's blocks, as on the unit) */
    blk();
    run_to(0, 1, 5);
    btn_down(B_FX);
    frame();
    key_down(white(4));                             /* C4: LPF */
    frame();
    run_to(0, 3, 40);
    key_up(white(4));
    btn_up(B_FX);
    frame();
    run_to(0, 5, 0);
    ok = fxt_lane[0].step[1] == C(PF_LPF) && fxt_lane[0].step[2] == FXT_TIE && fxt_lane[0].step[3] == FXT_TIE &&
         !fxt_lane[0].step[4] && !gates();
    bad += check("FX held + C4 on an armed FX1: LPF recorded from step 2 over 3 steps, no note sounds", ok);
    if (dir) {
        fxt_lane[0].step[8] = C(PF_R16);
        fxt_lane[0].step[9] = FXT_TIE;
        fxt_lane[0].step[12] = C(PF_ODN);
        fxt_lane[1].len = 12;
        fxt_lane[1].step[0] = C(PF_FLG);
        fxt_lane[1].step[1] = FXT_TIE;
        fxt_lane[1].step[2] = FXT_TIE;
        fxt_lane[1].step[3] = FXT_TIE;
        fxt_lane[1].step[6] = FXT_MUTE | 0u;
        fxt_lane[1].step[8] = C(PF_ETHR);
        fxt_lane[1].step[9] = FXT_TIE;
        run_to(0, 9, 30);
        ui.force = 1;
        frame();
        screen_save(dir, "fxtracks-recording");
    }
    stop();
    press(B_REC);
    frames(400);
    if (dir) {
        ui.force = 1;
        frame();
        screen_save(dir, "fxtracks-stopped");
    }
    /* FX TRACKS owns the selected lane's clear, with confirmation. */
    fxt_lane[1].step[5] = C(PF_REV);
    {
        uint8_t n0 = TSEL->step[0].n;
        go_page(GR_FXT); ui.fxt_sel = 0;
        press(B_EDIT);
        ok = ui.confirm == CF_FXT_LANE && fxt_used(0) && fxt_used(1);
        if (dir) screen_save(dir, "fxtracks-clear-confirm");
        press(B_OCTDN);
        ok &= fxt_used(0) && fxt_used(1);
        press(B_EDIT); press(B_OCTUP);
        ok &= !fxt_used(0) && fxt_used(1) && TSEL->step[0].n == n0;
        ui.fxt_sel = 1;
        press(B_EDIT); press(B_OCTUP);
        ok &= !fxt_used(0) && !fxt_used(1) && TSEL->step[0].n == n0;
        bad += check("FX TRACKS: EDIT clears only the chosen lane; OCT- cancels; notes stay", ok);
        press(B_EDIT);
        bad += check("empty FX lane: NOTHING TO CLEAR, no dialog", !ui.confirm && msg_is("NOTHING TO CLEAR"));
    }
}

int main(int argc, char **argv)
{
    test_play();
    test_record();
    test_ui(argc > 1 ? argv[1] : 0);
    if (bad) {
        printf("fxtrack: %d FAILED\n", bad);
        return 1;
    }
    printf("fxtrack: all ok\n");
    return 0;
}
