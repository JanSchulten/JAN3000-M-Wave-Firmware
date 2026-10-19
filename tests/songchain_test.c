/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SONG as the OP-Z's pattern chains (M-Vave-OP1: song_chain.c chain_next_row, ui.c chain_live, ui_input.c
 * song_chain_key), on the real sequencer, project and UI sources (the stubs of tests/ui_test.c):
 *   TYPE     REC on SONG types a chain with A..D: the first key a new chain (LOOP on), the same key again its repeats,
 *            another one a section; the REC LED; REC again, or another page, ends it.
 *   LOOP     a LOOP song starts over at its end; END on + ADD's KNOB 4 sets LOOP / STOP; kept with the project.
 *   LIVE     playing: a new chain starts at the end of the loop playing (its repeats not waited for); keys typed before
 *            then are its sections; an added section plays after the ones before; a cell edited plays when its
 *            section comes; a slot not saved plays silent; STOP drops what waits; the FX TRACKS play on.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
#define SLOT_NOTE(s) (60u + 5u * (s))
/* the slots A..C saved: every track 4 steps of a 1/16, step 1 a note of its own; D not saved */
static void four_patterns(void)
{
    uint32_t s, i;
    ui_power_on();
    song.g[G_BPM] = 120;
    for (s = 0; s < 3u; s++) {
        for (i = 0; i < NTRK; i++) {
            track_defaults_steps(&trk[i]);
            trk[i].p[P_SLEN] = 4;
            trk[i].p[P_SDIV] = 2;
            trk[i].p[P_SSWING] = 0;
            trk[i].step[0] = (step_t){{(uint8_t)SLOT_NOTE(s), 0, 0, 0}, 1, ST_NOTE, 0, 96, 0, 0};
        }
        project_save(s);
    }
    chain_defaults(&chain_config);
    fm1_ms += 1000u;
    go_page(GR_SONG);
    frames(400);
}
static uint32_t akey(uint32_t s) { return key_at(0, 2u + s); }    /* A3 B3 C4 D4 */
static uint32_t playing(void) { return chain.lane[0]; }             /* the slot track 1 plays */
static int row_is(uint32_t r, uint32_t s, uint32_t rep)
{
    chain_row_t w = chain_row_of(s, rep);
    return !memcmp(&chain_config.row[r], &w, sizeof w);
}
/* on until the next loop of a section starts (track 1 back on its step 1: every lane is 4 steps); the slot it plays */
static uint32_t next_section(void)
{
    uint32_t guard = 0;
    while (chain.running && trk[0].seq_idx == 0u && guard++ < 100000u)
        events_block(CTL);
    while (chain.running && trk[0].seq_idx != 0u && guard++ < 100000u)
        events_block(CTL);
    return playing();
}
static void blocks_n(uint32_t n) { while (n--) events_block(CTL); }
/* REC / PLAY tapped as a hand does, a while after the last tap (two quick taps of REC lock its layer: #83) */
static void rec(void) { frames(400); press(B_REC); }
static void chain_rec_on(void) { if (ui.song_rec) rec(); rec(); }       /* a new chain's typing */
static void chain_rec_off(void) { if (ui.song_rec) rec(); }
static void play_btn(void) { frames(400); press(B_PLAY); }
static void stop_play(void) { if (song.playing || chain.running) { play_btn(); events_block(CTL); } }

static int rec_lit(void)
{
    uint32_t i;
    for (i = 0; i < 41u; i++)                       /* (the stub key map has none: 4 LEDs a column, as test_breath) */
        led_pos[i] = (uint8_t)((i / 4u) << 3 | (1u + i % 4u));
    ui_leds();
    i = led_pos[panel.btn[B_REC]];
    return i != 0xFFu && ((fm1_led[i >> 3] >> (i & 7u)) & 1u);
}
static void test_type(void)
{
    four_patterns();
    bad += check("SONG: REC dark", !rec_lit());
    rec();
    bad += check("SONG, REC: types a chain (the pattern stays disarmed), REC lit",
                 ui.song_rec && ui.song_new && !song.rec && msg_is("NEW CHAIN: TAP A-D") && rec_lit());
    tap_key(akey(0));
    bad += check("  A: a new chain, every track on A, LOOP", chain_config.count == 1u && row_is(0, 0, 1) &&
                 (chain_config.rsv[0] & CHAIN_LOOP) && msg_is("NEW CHAIN: A"));
    tap_key(akey(0));
    bad += check("  A again: its section twice", chain_config.count == 1u && row_is(0, 0, 2) && msg_is("CHAIN + A x2"));
    tap_key(akey(1));
    tap_key(akey(2));
    bad += check("  B, C: a section each", chain_config.count == 3u && row_is(1, 1, 1) && row_is(2, 2, 1) &&
                 ui.song_row == 2u && msg_is("CHAIN + C"));
    rec();
    bad += check("REC again: done (the keys set the cell again)", !ui.song_rec && msg_is("CHAIN DONE"));
    rec();
    tap_key(akey(1));
    bad += check("REC, B: a new chain replaces the song", chain_config.count == 1u && row_is(0, 1, 1));
    go_title("TAPE");
    go_page(GR_SONG);
    bad += check("another page: the typing ends", !ui.song_rec);
}

static void test_loop(void)
{
    uint32_t ok;
    four_patterns();
    rec();
    tap_key(akey(0));
    tap_key(akey(0));
    tap_key(akey(1));
    tap_key(akey(2));
    rec();
    play_btn();
    events_block(CTL);
    ok = chain.running && playing() == 0u && seq_steps(&trk[0])[0].note[0] == SLOT_NOTE(0);
    ok &= next_section() == 0u && chain.remaining == 1u;
    ok &= next_section() == 1u && seq_steps(&trk[0])[0].note[0] == SLOT_NOTE(1);
    ok &= next_section() == 2u;
    ok &= next_section() == 0u && chain.row == 0u && song.playing;
    bad += check("LOOP: A A B C, then A again (the song goes on)", ok);
    ui.song_row = chain_config.count;                 /* + ADD: KNOB 4 the END */
    turn(EN_K4, -1);
    ok = !(chain_config.rsv[0] & CHAIN_LOOP) && msg_is("END: STOP") && chain.req == CHN_EDIT;
    ok &= next_section() == 0u && next_section() == 1u && next_section() == 2u && chain.running;
    next_section();
    ok &= !song.playing && !chain.running;
    bad += check("END STOP (+ ADD's KNOB 4) while it plays: the song stops at its end", ok);
    turn(EN_K4, 1);
    ok = (chain_config.rsv[0] & CHAIN_LOOP) && msg_is("END: LOOP");
    project_save(0);
    chain_defaults(&chain_config);
    project_load(0);
    bad += check("END LOOP: kept with the project", ok && (chain_config.rsv[0] & CHAIN_LOOP) && chain_config.count == 3u);
}

static void test_live(void)
{
    uint32_t ok;
    four_patterns();
    chain_rec_on();
    tap_key(akey(0));
    tap_key(akey(0));
    tap_key(akey(1));
    chain_rec_off();
    play_btn();
    events_block(CTL);
    blocks_n(100);
    chain_rec_on();
    tap_key(akey(2));
    ok = chain.req == CHN_NEW && playing() == 0u && chain.remaining == 2u && !msg_is("STOP TO EDIT");
    ok &= next_section() == 2u && chain.row == 0u && chain.config.count == 1u && !chain.req;
    bad += check("playing A (x2): REC, C: C starts at the end of A's loop now, not after its repeats", ok);
    tap_key(akey(1));
    ok = chain.req == CHN_EDIT && chain_config.count == 2u;
    ok &= next_section() == 1u && next_section() == 2u;
    bad += check("  B typed while C plays: after C, then C again (the chain C B loops)", ok);
    chain_rec_on();
    tap_key(akey(0));
    tap_key(akey(1));
    tap_key(akey(1));
    ok = chain.req == CHN_NEW;
    ok &= next_section() == 0u && next_section() == 1u && chain.remaining == 2u && next_section() == 1u &&
          next_section() == 0u;
    bad += check("  REC, A B B typed within one loop: a new chain from A when that loop ends", ok);
    chain_rec_off();
    ui.song_row = 1;
    ui.song_trk = 0;
    turn(EN_K3, 1);                                   /* section 2, T1: B -> C */
    ok = chain_config.row[1].slot[0] == 2u && chain.req == CHN_EDIT && !msg_is("STOP TO EDIT");
    while (chain.row != 1u && chain.running)
        next_section();
    ok &= chain.lane[0] == 2u && chain.lane[1] == 1u && seq_steps(&trk[0])[0].note[0] == SLOT_NOTE(2);
    bad += check("a cell turned while the song plays: no STOP TO EDIT, it plays when its section comes", ok);
    chain_rec_on();
    tap_key(akey(3));
    ok = next_section() == 3u && chain.running && !seq_steps(&trk[0])[0].n;
    blocks_n(200);
    ok &= chain.running && !trk[0].seq_n;
    bad += check("a new chain on D (not saved): silent there, the song plays on", ok);
    chain_rec_on();
    tap_key(akey(0));
    chain_rec_off();
    play_btn();
    events_block(CTL);
    ok = !song.playing && !chain.running && !chain.req;
    play_btn();
    events_block(CTL);
    ok &= chain.running && playing() == 0u && chain.config.count == 1u;
    bad += check("STOP drops the chain that waits; PLAY plays the song as typed (A)", ok);
    stop_play();
    /* each section plays the FX TRACKS saved with its slot (opstore.c): A's LPF, B none */
    project_load(0);
    fxt_lane[0].step[0] = fxt_code(PF_LPF);
    fxt_lane[0].step[1] = FXT_TIE;
    fxt_lane[0].len = 4;
    project_save(0);
    memset(fxt_lane, 0, sizeof fxt_lane);
    chain_rec_on();
    tap_key(akey(0));
    tap_key(akey(1));
    chain_rec_off();
    play_btn();
    events_block(CTL);
    blocks_n(10);
    ok = perf_fxt == PF_BIT(PF_LPF) && fxt.idx[0] == 0u;
    next_section();
    blocks_n(10);
    ok &= playing() == 1u && !perf_fxt;
    next_section();
    blocks_n(10);
    ok &= playing() == 0u && perf_fxt == PF_BIT(PF_LPF);
    bad += check("each section plays the FX TRACKS saved with its slot (A: LPF, B: none), from their step 1", ok);
    go_title("FX TRACKS");
    turn(EN_K3, 1);
    ok = msg_is("STOP TO EDIT") && !fxt_lane[0].step[0] && fxt_src(0)->step[0] == fxt_code(PF_LPF);
    frames(400);
    press(B_REC);
    ok &= !fxt.arm && msg_is("STOP TO RECORD");
    stop_play();
    ok &= fxt_src(0) == &fxt_lane[0] && !fxt_lane[0].step[0];
    bad += check("  FX TRACKS in a song: shows the section's lanes, STOP TO EDIT, STOP TO RECORD; stopped: its own", ok);
}

static void test_screens(const char *dir)
{
    four_patterns();
    rec();
    tap_key(akey(0));
    tap_key(akey(0));
    tap_key(akey(1));
    tap_key(akey(2));
    tap_key(akey(1));
    play_btn();
    events_block(CTL);
    next_section();
    blocks_n(300);
    ui.force = 1;
    frame();
    screen_save(dir, "song-chain-playing");
    play_btn();
    events_block(CTL);
    rec();
    ui.song_row = chain_config.count;
    ui.force = 1;
    frame();
    screen_save(dir, "song-chain-end");
}

int main(int argc, char **argv)
{
    test_type();
    test_loop();
    test_live();
    if (argc > 1)
        test_screens(argv[1]);
    if (bad) {
        printf("songchain: %d FAILED\n", bad);
        return 1;
    }
    printf("songchain: all ok\n");
    return 0;
}
