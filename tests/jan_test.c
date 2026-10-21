/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* JAN3000 v0.8's workflow changes (M-Vave-OP1), on the firmware's own sources through the UI harness:
 *   PAGES    another track (ALGORITHM) opens each family at its first page: FX left on CHORUS, a turn of ALGORITHM, FX
 *            again: SEND (the family shown stays where it is)
 *   UNDO     REC armed takes the track's copy; REC held: OCT- undoes what was recorded since, OCT+ redoes it, each
 *            once in a row (NOTHING TO REDO / ALREADY UNDONE); a clear or CLICK changed in the same hold: OCT- puts
 *            those back as before
 *   NEW      SAVE > PROJECT, F#5 (NEW) asks, OCT+: the power-on sounds (808 kit) with empty steps, the
 *            default BPM, no song, FX TRACKS, KEY or CHORUS TYPE, no slot or name; while playing: STOP FIRST
 *   SELECT   on a page SELECT turns its family's pages (wrapping), the tempo stays; the page's button again: HOME;
 *            on HOME (and its MIXER, CLOCK) SELECT is the tempo, HOME's taps go round its pages as before
 *   SPLASH   JAN3000 in bars of its own (no text): booting left to right, its colour copies beside it, torn now and
 *            then early on, still at the end; the version and the credit as text
 *   ANALOG2  engine 14, after ANALOG in the list; EDIT's rows on one screen: WAVE TUNE WAVE2 TUNE2 / CUT RES LFO RATE /
 *            CUT2 RES2 LFO2 RATE2 / VCE GLD GLMOD PRIO; a preset sets WAVE 2's values; each filter and LFO acts on
 *            its own oscillator; NOISE sounds; its favourites leave MIDI LEARN's row alone
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

/* SAVE opens PROJECT: a white key F3..B3 picks A..D (silent, lit), OCT+ saves (NAME, or OVERWRITE?), OCT- loads (LOAD?) */
static void test_slots(void)
{
    int ok;
    uint32_t i;
    ui_power_on();
    frames(320);
    for (i = 0; i < 4u; i++)
        memset(&proj_slot[i], 0, sizeof proj_slot[i]);
    press_raw = 1;
    press(B_SAVE); frames(100);
    ok = !ui.home && cur_page()->graph == GR_SLOTS && song.grid == 5u;
    key_down(2); frame(); key_up(2); frame();         /* G3: B */
    ok &= song.g[G_SLOT] == 2 && !gates() && (key_leds(0) & 0x55u) == 0x55u && ((key_leds(0) >> SLOT_KEY_NEW) & 1u);
    press(B_OCTDN); frame();
    ok &= msg_is("EMPTY SLOT") && !ui.confirm;
    press(B_OCTUP); frame();
    ok &= name_on() && nm.kind == NK_PROJ_SAVE;
    press(B_OCTUP); frames(50);
    ok &= !name_on() && project_used(1);
    ck("SAVE opens PROJECT: G3 picks B (silent, the keys lit); OCT- on an empty one: EMPTY SLOT; OCT+: NAME, saved", ok);
    trk[0].p[P_LEVEL] = 11;
    press(B_OCTDN); frame();
    ok = ui.confirm == CF_LOAD_PROJ && trk[0].p[P_LEVEL] == 11;
    press(B_OCTDN); frame();                            /* (OCT-: no) */
    ok &= !ui.confirm && trk[0].p[P_LEVEL] == 11;
    press(B_OCTDN); frame();
    press(B_OCTUP); frames(50);
    ok &= trk[0].p[P_LEVEL] != 11 && msg_is("LOADED");
    press(B_OCTUP); frame();
    ok &= ui.confirm == CF_OVR_PROJ;
    press(B_OCTDN); frame();
    press_raw = 0;
    ck("  OCT- asks LOAD PROJECT B? (OCT- no, OCT+ loads); OCT+ on a used one: OVERWRITE?", ok && !ui.confirm);
}

static void test_select(void)
{
    int ok;
    int16_t b0;
    uint32_t n, first;
    ui_power_on();
    frames(320);
    press_raw = 1;                                     /* (the taps as they are: tests/ui_test.c press) */
    b0 = song.g[G_BPM];
    press(B_FX); frames(400);
    first = ui.page;
    ok = str_eq(cur_page()->title, "FX");
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "INSERT") && song.g[G_BPM] == b0;
    turn(EN_SELECT, -1); frame();
    turn(EN_SELECT, -1); frame();
    ok &= str_eq(cur_page()->title, "FX TRACKS");      /* (back past the first: the last, FX's seventh card) */
    for (n = 0; n < 8u && ui.page != first; n++) {
        turn(EN_SELECT, 1); frame();
    }
    ok &= ui.page == first && n == 1u && song.g[G_BPM] == b0 && !ui.home;
    ck("SELECT on a page: the family's next / previous page, wrapping; the tempo stays", ok);
    press(B_FX); frames(400);
    ok = ui.home;
    turn(EN_SELECT, 2); frame();
    ok &= song.g[G_BPM] == b0 + 2;
    ck("  the page's button again: HOME; SELECT on HOME: the tempo", ok);
    press(B_HOME); frames(400);
    ok = !ui.home && str_eq(cur_page()->title, "MIXER");
    turn(EN_SELECT, 1); frame();
    ok &= song.g[G_BPM] == b0 + 3 && str_eq(cur_page()->title, "MIXER");
    press(B_HOME); frames(400);
    ok &= str_eq(cur_page()->title, "CLOCK");
    btn_down(B_GLO); frame();
    turn(EN_SELECT, 3);
    btn_up(B_GLO); frames(400);
    ok &= song.g[G_BPM] == b0 + 3;
    press_raw = 0;
    ck("  HOME's taps go round its pages (MIXER, CLOCK); SELECT there the tempo; GLO + SELECT: nothing", ok);
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
    key_down(SLOT_KEY_NEW); frame(); key_up(SLOT_KEY_NEW); frame();
    ok = msg_is("STOP FIRST") && !ui.confirm;
    transport_req = 2;
    blk();
    frames(100);
    key_down(SLOT_KEY_NEW); frame(); key_up(SLOT_KEY_NEW); frame();
    ok &= ui.confirm == CF_NEW_PROJ;
    press(B_OCTUP); frames(50);
    for (k = 0; k < NTRK; k++)
        notes += !seq_is_empty(&trk[k]);
    ok &= !ui.confirm && msg_is("NEW PROJECT") && !notes && song.g[G_BPM] == GP[G_BPM].def && !keyt.on &&
          keyt.root[0] == KEY_NONE && chos.type == CHO_CHORUS && !fxt_lane[0].len && !chain_config.count &&
          proj_cur == PROJ_NO_SLOT && !proj_name[0] && trk[3].eng_req == ENGI_DRUM && str_eq(N_DRUM_KIT[trk[3].p[P_E0]], "808");
    ck("SAVE > PROJECT > NEW (F#5, OCT+): power-on sounds (808), empty steps, default BPM, no song / FX TRACKS / "
       "KEY / CHORUS TYPE, no slot; while playing: STOP FIRST", ok);
}

static uint32_t splash_px(uint32_t ms, uint16_t c, uint32_t *hash)   /* the name's band: pixels of colour c */
{
    uint32_t x, y, n = 0, h = 2166136261u;
    draw_splash_at(ms);
    for (y = SPL_Y + SPL_NY; y < SPL_Y + SPL_NY + SPL_GH; y++)
        for (x = 0; x < 240u; x++) {
            n += host_screen[y * 240u + x] == swap16(c);
            h = (h ^ host_screen[y * 240u + x]) * 16777619u;
        }
    *hash = h;
    return n;
}
static void test_splash(void)
{
    uint32_t end, early, ghost, h0, h1, h2, ms, torn = 0;
    int ok;
    ui_power_on();
    end = splash_px(SPL_MS, T_TEXT, &h0);
    ghost = splash_px(SPL_MS, T_ACCENT, &h1);
    early = splash_px(150u, T_TEXT, &h2);
    ok = end > 800u && ghost > 40u && early < end / 2u && h1 == h0;
    for (ms = 0; ms < SPL_GLITCH; ms += 20u) {        /* a tear: the frame differs from the same frame untorn */
        uint32_t a, b;
        splash_px(ms, T_TEXT, &a);
        splash_px(ms, T_TEXT, &b);
        ok &= a == b;                                 /* (the same ms, the same frame) */
        torn += ms >= SPL_WIPE && (spl_hash(ms / 60u, 99u) & 3u) == 0u;
    }
    ok &= torn > 0u && splash_px(SPL_MS, T_TEXT, &h2) == end && h2 == h0;
    ck("splash: JAN3000 in bars, booted left to right, colour copies beside it, a tear now and then, still at the end",
       ok);
}

/* track 1's ANALOG2 voice at C4 for 64 blocks: the output's energy (>> 8 a sample) and a hash of it */
static uint64_t a2_play(uint32_t *hash)
{
    voice_t v;
    vmod_t m;
    int32_t out[CTL];
    uint64_t e = 0;
    uint32_t b, i, h = 2166136261u;
    memset(&v, 0, sizeof v);
    memset(&m, 0, sizeof m);
    m.pitch16 = v.pitch16 = 60 * 16;
    m.inc = pitch_inc(60 * 16);
    m.amp0 = m.amp1 = 32767;
    m.shape = 64 << 8;
    analog2_note_on(&trk[0], &v);
    for (b = 0; b < 64u; b++) {
        memset(out, 0, sizeof out);
        analog2_render(&trk[0], &v, out, CTL, &m);
        for (i = 0; i < CTL; i++) {
            e += (uint64_t)((int64_t)out[i] * out[i]) >> 8;
            h = (h ^ (uint32_t)out[i]) * 16777619u;
        }
    }
    *hash = h;
    return e;
}
static void test_analog2(void)
{
    static const char *const L[4][4] = {{"WAVE", "TUNE", "WAVE2", "TUNE2"}, {"CUT", "RES", "LFO", "RATE"},
                                        {"CUT2", "RES2", "LFO2", "RATE2"}, {"VCE", "GLD", "GLMOD", "PRIO"}};
    uint8_t rr[4];
    uint32_t n, j, c, h0, h1, h2;
    uint64_t e0, e1;
    int ok;
    track_t *t = &trk[0];
    ui_power_on();
    frames(320);
    ok = ENGINES[14] == &ENG_ANALOG2 && eng_step(0, 1) == 14u && eng_step(14, 1) == ENGI_FM6 && eng_ok(14) &&
         a2_hz(pitch_inc(69 * 16), 10) - pitch_inc(69 * 16) == 973920u && a2_hz(1000u, -50) == 0u;   /* (TUNE: Hz) */
    set_engine_of(t, 14);
    frame();
    ok &= t->eng_req == 14u && t->p[P_A2_CUT2] == A2_OSC2_PRESET[0][0] && t->p[P_A2_RATE2] == A2_OSC2_PRESET[0][3];
    ck("ANALOG2: engine 14 (ANLG2), after ANALOG in the list, TUNE in Hz; its first preset sets WAVE 2's values too", ok);
    go_title("EDIT 1");
    frame();
    n = edit_rows(ui.page, rr);
    ok = n == 4u;
    for (j = 0; ok && j < 4u; j++)
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = page_desc(&PAGES[rr[j]], c, &vp);
            ok &= d && vp && str_eq(d->label, L[j][c]);
        }
    ok &= ok && str_eq(PAGES[rr[2]].title, "WAVE 2") && str_eq(param_desc_of(14, P_A2_RATE2)->label, "RATE2") && str_eq(ENGINES[14]->name, "ANLG2") && str_eq(ENGINES[14]->edit[1].unit, "Hz") &&
          str_eq(track_desc(t, P_A2_CUT2)->label, "CUT2") && track_desc(t, P_A2_CUT2)->fmt == F_CUTOFF;
    ck("  EDIT: one screen of four rows, WAVE TUNE WAVE2 TUNE2 / CUT RES LFO RATE / CUT2 RES2 LFO2 RATE2 / VCE GLD GLMOD "
       "PRIO", ok);
    t->p[P_E0] = A2_SAW; t->p[P_E1] = 0; t->p[P_E2] = A2_SQR; t->p[P_E3] = 0;
    t->p[P_E4] = 100; t->p[P_E5] = 20; t->p[P_E6] = 0; t->p[P_E7] = 60;
    t->p[P_A2_CUT2] = 100; t->p[P_A2_RES2] = 20; t->p[P_A2_LFO2] = 0; t->p[P_A2_RATE2] = 60;
    e0 = a2_play(&h0);
    t->p[P_A2_CUT2] = 10;
    e1 = a2_play(&h1);
    ok = e0 > 0u && e1 < e0 * 3u / 4u;
    t->p[P_A2_CUT2] = 100;
    t->p[P_E6] = 127; t->p[P_E7] = 120;
    a2_play(&h2);
    ok &= h2 != h0;
    t->p[P_E6] = 0;
    t->p[P_A2_LFO2] = 127; t->p[P_A2_RATE2] = 120;
    a2_play(&h1);
    ok &= h1 != h0 && h1 != h2;
    t->p[P_A2_LFO2] = 0;
    t->p[P_E0] = t->p[P_E2] = A2_NOISE;
    ok &= a2_play(&h1) > e0 / 8u;
    ck("  WAVE 2's CUT2 closes its oscillator; each LFO moves its own filter (LFO, LFO2 different); NOISE sounds", ok);
    {
        uint8_t row14[32];
        memcpy(row14, favorites.factory[14], sizeof row14);
        ok = favorite_set(14, 3, 1) && favorite_has(14, 3) && !favorite_has(0, 3) &&
             !memcmp(row14, favorites.factory[14], sizeof row14);
        favorite_set(14, 3, 0);
        ok &= !favorite_has(14, 3);
    }
    ck("  ANALOG2's favourites (ANALOG's row, upper half): MIDI LEARN's row untouched", ok);
}

int main(void)
{
    test_pages();
    test_select();
    test_undo();
    test_new();
    test_slots();
    test_analog2();
    test_splash();
    printf("%s\n", bad ? "JAN TEST FAILED" : "jan test passed");
    return bad != 0;
}
