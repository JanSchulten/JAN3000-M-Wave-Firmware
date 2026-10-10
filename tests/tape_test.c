/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* TAPE (firmware/src/tape.c), on the real sequencer and UI sources (with the stubs of tests/ui_test.c), the audio ISR's
 * part driven block by block (seq.c events_block):
 *   CORE     power-on empty; a note recorded into an armed track while the transport runs (its on and off where they
 *            were played, not quantised); nothing into a track not armed, nor stopped; playback plays it into the track's
 *            engine there and ends it there; overdub; the order of the events (sorted, the gap at the head) after
 *            every operation; STOP ends a held note where the head stands; PLAY goes on from the head.
 *   LOOP     LOOP bars from the bar the head was in, over and over; a key held over the wrap: off at the loop's end,
 *            on again at its start; RESTART (GLO + PLAY) from the loop's start.
 *   SPEED    the head at 2x / 0.5x; the notes it plays moved by SPEED; a note recorded at a SPEED plays at its pitch there.
 *   DIR      backwards: a note sounds from its off to its on; recorded backwards it plays forwards as it was heard.
 *   ERASE    a track's events gone, the others kept; FULL: a note that would not fit is not taken, every on keeps its off.
 *   END      the head stops at the tape's end, the notes end.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void tape_power_on(void)
{
    ui_power_on();
    memset(&tape, 0, sizeof tape);
    memset(tape_ev, 0, sizeof tape_ev);
    trk[0].p[P_AMODE] = 0;                        /* (no arp: the keys play) */
    trk[0].p[P_VOICE] = V_POLY;
}
/* the transport running until the head reaches tick (a guard of 2 x the time it takes at 0.5x) */
static int run_to(uint32_t tick)
{
    uint32_t guard = 0, lim = 2u * (tick + TAPE_BAR) * (tape_beat() / TAPE_PPQN + 1u) / CTL + 64u;
    while ((tape.dir ? tape.head > tick : tape.head < tick) && guard++ < lim)
        events_block(CTL);
    return tape.dir ? tape.head <= tick : tape.head >= tick;
}
static void play(void) { transport_req = 1; blocks(1); }
static void stop(void) { transport_req = 2; blocks(1); }
static void seek(uint32_t tick) { tape.req_seek = tick + 1u; blocks(1); }

/* every event, in tick order (before the gap, then after it); n returned */
static uint32_t tape_list(uint32_t *out, uint32_t max)
{
    uint32_t i, n = 0;
    for (i = 0; i < tape.g0 && n < max; i++)
        out[n++] = tape_ev[i];
    for (i = TAPE_G1; i < TAPE_EV && n < max; i++)
        out[n++] = tape_ev[i];
    return n;
}
/* sorted by tick, the head between the two halves of the gap */
static int tape_sane(void)
{
    uint32_t i, last = 0;
    if (tape.g0 + tape.na > TAPE_EV)
        return 0;
    for (i = 0; i < tape.g0; i++) {
        if (TE_TICK(tape_ev[i]) < last || TE_TICK(tape_ev[i]) > tape.head)
            return 0;
        last = TE_TICK(tape_ev[i]);
    }
    for (i = TAPE_G1; i < TAPE_EV; i++) {
        if (TE_TICK(tape_ev[i]) < last || TE_TICK(tape_ev[i]) < tape.head)
            return 0;
        last = TE_TICK(tape_ev[i]);
    }
    return 1;
}
/* every on of track k has its off later (a note at a time: notes as sorted) */
static int tape_paired(uint32_t k)
{
    static uint32_t ev[TAPE_EV];
    uint32_t n = tape_list(ev, TAPE_EV), i, held[4] = {0};
    for (i = 0; i < n; i++) {
        uint32_t note = TE_NOTE(ev[i]), bit = 1u << (note & 31u), w = note >> 5;
        if (TE_TRK(ev[i]) != k)
            continue;
        if (ev[i] & TE_OFF) {
            if (!(held[w] & bit))
                return 0;
            held[w] &= ~bit;
        } else {
            if (held[w] & bit)
                return 0;
            held[w] |= bit;
        }
    }
    return !(held[0] | held[1] | held[2] | held[3]);
}
static int sounding(const track_t *t, uint32_t note)   /* a gated voice of t plays note */
{
    uint32_t i;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].gate && t->v[i].note == note)
            return 1;
    return 0;
}
static int near(uint32_t a, uint32_t b) { return a + 1u >= b && a <= b + 1u; }

static void test_core(void)
{
    track_t *t = &trk[0];
    uint32_t ev[16], n;
    tape_power_on();
    bad += check("power-on: the tape empty, the head at its start, nothing armed",
                 tape_free() == TAPE_EV && tape.head == 0u && !tape.arm && tape_sane());
    play();
    input_on(t, 60, 100);
    run_to(96);
    input_off(t, 60);
    bad += check("not armed: nothing recorded", tape_free() == TAPE_EV);
    stop();
    tape.arm = 1;
    input_on(t, 60, 100);
    input_off(t, 60);
    bad += check("armed, stopped: nothing recorded", tape_free() == TAPE_EV);
    seek(0);
    play();
    run_to(96);
    input_on(t, 60, 100);
    run_to(192);
    input_off(t, 60);
    run_to(260);
    stop();
    n = tape_list(ev, 16);
    bad += check("armed and playing: the note's on and off where they were played (beat 2, beat 3)",
                 n == 2u && !(ev[0] & TE_OFF) && (ev[1] & TE_OFF) && TE_NOTE(ev[0]) == 60u && TE_NOTE(ev[1]) == 60u &&
                 TE_TRK(ev[0]) == 0u && near(TE_TICK(ev[0]), 96) && near(TE_TICK(ev[1]), 192) && tape_sane());
    bad += check("  its velocity kept (on and off)", TE_VEL(ev[0]) >= 99u && TE_VEL(ev[0]) <= 101u &&
                 TE_VEL(ev[1]) == TE_VEL(ev[0]));
    bad += check("STOP: the head stays where it stopped", near(tape.head, 260));
    tape.arm = 0;
    seek(0);
    play();
    run_to(90);
    bad += check("playback: silent before the note", !sounding(t, 60));
    run_to(100);
    bad += check("  the note sounds from its on", sounding(t, 60));
    run_to(186);
    bad += check("  .. held to its off", sounding(t, 60));
    run_to(200);
    bad += check("  .. and ends there", !sounding(t, 60) && tape_free() == TAPE_EV - 2u && tape_sane());
    stop();
    seek(0);
    tape.arm = 1;
    play();
    run_to(144);
    input_on(t, 64, 80);
    run_to(240);
    input_off(t, 64);
    run_to(300);
    stop();
    n = tape_list(ev, 16);
    bad += check("overdub: the new note in among the old, in order",
                 n == 4u && TE_NOTE(ev[0]) == 60u && TE_NOTE(ev[1]) == 64u && TE_NOTE(ev[2]) == 60u &&
                 TE_NOTE(ev[3]) == 64u && near(TE_TICK(ev[1]), 144) && near(TE_TICK(ev[3]), 240) && tape_sane() &&
                 tape_paired(0));
    play();
    input_on(t, 67, 100);
    run_to(400);
    stop();
    bad += check("STOP with a recorded note held: its off where the head stopped", tape_paired(0) &&
                 tape_free() == TAPE_EV - 6u && sounding(t, 67));
    input_off(t, 67);
    bad += check("  (the key let go after: nothing more written)", tape_free() == TAPE_EV - 6u);
    tape.arm = 0;
    play();
    bad += check("PLAY again: on from the head", near(tape.head, 400) && song.playing);
    stop();
    tape.arm = 2;
    seek(0);
    play();
    run_to(50);
    input_on(&trk[1], 72, 100);
    run_to(80);
    input_off(&trk[1], 72);
    stop();
    bad += check("track 2 armed: its notes on its own lane, track 1 untouched",
                 tape_count(1) == 2u && tape_count(0) == 6u && tape_paired(1) && tape_paired(0) && tape_sane());
    /* the keys: through the keyboard, as played */
    tape_power_on();
    tape.arm = 1;
    play();
    run_to(48);
    {
        uint32_t k = 7, note = kb_map(t, k);
        fm1_in.notes = 1u << k;
        run_to(120);
        fm1_in.notes = 0;
        run_to(130);
        stop();
        n = tape_list(ev, 16);
        bad += check("a key held: recorded as played (keyboard_block -> input_on / input_off)",
                     n == 2u && TE_NOTE(ev[0]) == note && near(TE_TICK(ev[0]), 48) && near(TE_TICK(ev[1]), 120));
    }
}

static void test_loop(void)
{
    track_t *t = &trk[0];
    uint32_t pass, on = 0, was = 0, ok = 1;
    tape_power_on();
    seek(TAPE_BAR + 100u);
    tape_loop_ui(1);                              /* 1 bar, from the bar the head is in */
    bad += check("LOOP 1 BAR: from the bar the head is in", tape.loop_in == TAPE_BAR && tape_loop_out() == 2u * TAPE_BAR);
    tape.arm = 1;
    play();
    run_to(2u * TAPE_BAR - 10u);
    input_on(t, 50, 100);
    run_to(2u * TAPE_BAR - 2u);
    for (pass = 0; pass < 40u && tape.head >= TAPE_BAR + 5u; pass++)
        events_block(CTL);
    bad += check("  at its end the head goes back to its start", tape.head < TAPE_BAR + 5u && tape.head >= TAPE_BAR);
    run_to(TAPE_BAR + 30u);
    input_off(t, 50);
    tape.arm = 0;
    bad += check("  a key held over the wrap: off at the loop's end, on again at its start", tape_paired(0) &&
                 tape_count(0) == 4u && tape_sane());
    for (pass = 0; pass < 3u; pass++) {           /* three more passes: the two notes each time */
        uint32_t guard = 0;
        do {
            uint32_t s = sounding(t, 50);
            on += s && !was;
            was = s;
            events_block(CTL);
            ok &= tape.head >= TAPE_BAR && tape.head < 2u * TAPE_BAR;
        } while (++guard < 20000u && !(tape.head < TAPE_BAR + 3u && guard > 50u));
    }
    bad += check("  over and over: the head stays in the loop, its notes play each pass", ok && on >= 3u && tape_sane());
    stop();
    tape.req_seek = 3u * TAPE_BAR + 1u;
    blocks(1);
    transport_req = 3;                            /* RESTART (GLO + PLAY) */
    blocks(1);
    bad += check("RESTART: from the loop's start", song.playing && near(tape.head, TAPE_BAR));
    stop();
    tape_loop_ui(0);
    transport_req = 3;
    blocks(1);
    bad += check("  LOOP OFF: from the tape's start", song.playing && tape.head < 3u);
    stop();
}

static void test_speed_dir(void)
{
    track_t *t = &trk[0];
    uint32_t b, h0, ev[8];
    tape_power_on();
    play();
    for (b = 0; b < 400u; b++)
        events_block(CTL);
    h0 = tape.head;
    stop();
    seek(0);
    tape.speed = 12;
    play();
    for (b = 0; b < 400u; b++)
        events_block(CTL);
    bad += check("SPEED +12: the head twice as fast", near(tape.head, 2u * h0));
    stop();
    seek(0);
    tape.speed = -12;
    play();
    for (b = 0; b < 400u; b++)
        events_block(CTL);
    bad += check("SPEED -12: half as fast", near(tape.head * 2u, h0) || near(tape.head * 2u + 1u, h0));
    stop();
    tape.speed = 0;
    seek(0);
    tape.arm = 1;
    play();
    run_to(50);
    input_on(t, 60, 100);
    run_to(150);
    input_off(t, 60);
    stop();
    tape.arm = 0;
    tape.speed = 7;
    seek(0);
    play();
    run_to(100);
    bad += check("  a note recorded at 0 plays 7 up at SPEED +7", sounding(t, 67) && !sounding(t, 60));
    tape.speed = 0;                               /* (SPEED turned while it sounds: its off still ends it) */
    run_to(160);
    bad += check("  .. and ends at its off though SPEED changed meanwhile", !sounding(t, 67) && !sounding(t, 60));
    stop();
    tape_erase(1);
    tape.speed = -12;
    seek(0);
    tape.arm = 1;
    play();
    run_to(20);
    input_on(t, 60, 100);
    run_to(40);
    input_off(t, 60);
    stop();
    tape.arm = 0;
    tape_list(ev, 8);
    bad += check("  recorded at SPEED -12: stored an octave up (it plays as heard at that SPEED)",
                 TE_NOTE(ev[0]) == 72u);
    seek(0);
    play();
    run_to(30);
    bad += check("  .. at that SPEED it sounds as played", sounding(t, 60));
    stop();
    tape.speed = 0;
    /* DIR REV */
    tape_power_on();
    tape.arm = 1;
    play();
    run_to(100);
    input_on(t, 62, 100);
    run_to(200);
    input_off(t, 62);
    stop();
    tape.arm = 0;
    seek(300);
    tape.rev = 1;
    play();
    run_to(205);
    bad += check("DIR REV: silent after the note's end (from above)", !sounding(t, 62));
    run_to(195);
    bad += check("  sounds from its off backwards", sounding(t, 62));
    run_to(105);
    bad += check("  .. to its on", sounding(t, 62));
    run_to(95);
    bad += check("  .. and ends there", !sounding(t, 62) && tape_sane());
    stop();
    tape_erase(1);
    seek(400);
    tape.arm = 1;
    play();
    run_to(300);
    input_on(t, 65, 100);                         /* pressed at 300, let go at 250, backwards */
    run_to(250);
    input_off(t, 65);
    run_to(200);
    stop();
    tape.arm = 0;
    tape_list(ev, 8);
    bad += check("  recorded backwards: forwards it runs from where the key came up to where it went down",
                 tape_count(0) == 2u && !(ev[0] & TE_OFF) && near(TE_TICK(ev[0]), 250) && (ev[1] & TE_OFF) &&
                 near(TE_TICK(ev[1]), 300) && tape_sane());
    tape.rev = 0;
    seek(240);
    play();
    run_to(260);
    bad += check("  .. and plays so", sounding(t, 65));
    run_to(310);
    bad += check("  .. to its end", !sounding(t, 65));
    stop();
    tape.arm = 1;
    seek(0);
    play();
    run_to(20);
    input_on(t, 70, 100);
    run_to(40);
    tape.rev = 1;                                  /* DIR turned with a key held: the note ends there first */
    blocks(1);
    run_to(30);
    input_off(t, 70);
    stop();
    tape.rev = 0;
    blocks(1);
    bad += check("  DIR turned while a key is held: its note closed the way it began", tape_paired(0) && tape_sane());
}

static void test_erase_full_end(void)
{
    track_t *t = &trk[0];
    uint32_t i, ok = 1;
    tape_power_on();
    tape.arm = 3;
    play();
    for (i = 0; i < 20u; i++) {
        input_on(&trk[i & 1u], 60u + i, 100);
        run_to(tape.head + 10u);
        input_off(&trk[i & 1u], 60u + i);
        run_to(tape.head + 10u);
    }
    stop();
    tape.req_clear = 1;
    blocks(1);
    bad += check("ERASE track 1: its events gone, track 2's kept, in order",
                 tape_count(0) == 0u && tape_count(1) == 20u && tape_sane() && tape_paired(1));
    tape_power_on();
    tape.arm = 1;
    play();
    for (i = 0; i < TAPE_EV; i++) {                /* far more notes than fit */
        input_on(t, 40u + i % 40u, 100);
        events_block(CTL);
        input_off(t, 40u + i % 40u);
        events_block(CTL);
        if (tape.head > TAPE_END - 100u)
            break;
    }
    ok = tape.full && tape_free() <= 1u && tape_paired(0) && tape_sane();
    stop();
    bad += check("FULL: the notes that do not fit are not taken; every on has its off", ok && tape_paired(0));
    tape_power_on();
    tape.arm = 1;
    seek(TAPE_END - 50u);
    play();
    input_on(t, 60, 100);
    run_to(TAPE_END);
    blocks(10);
    bad += check("END: the head stops at the tape's end, the recorded note closed there", tape.head == TAPE_END &&
                 tape_paired(0) && tape_count(0) == 2u && tape_sane());
    input_off(t, 60);
    input_on(t, 61, 100);
    bad += check("  no recording at the end", tape_count(0) == 2u);
    input_off(t, 61);
    tape.rev = 1;
    blocks(40);
    bad += check("  DIR REV there: it runs back", tape.head < TAPE_END);
    stop();
}

/* GLO > TAPE and TAPE 2 on the UI: the pages, the knobs, REC, ERASE, the LED; screens into dir (when given) */
static void record_demo(void)                     /* a few notes on tracks 1, 2 and 4, a loop, the head in the middle */
{
    static const uint8_t N1[] = {48, 55, 60, 63, 67, 70, 72, 67};
    uint32_t i;
    tape.arm = 0xB;
    seek(0);
    play();
    for (i = 0; i < 8u; i++) {
        run_to(i * 48u + 4u);
        input_on(&trk[0], N1[i], 100);
        if (!(i & 1u))
            input_on(&trk[1], 60u + i, 90);
        if (!(i & 3u))
            input_on(&trk[3], 36u + (i & 4u) / 2u, 120);
        run_to(i * 48u + 36u);
        input_off(&trk[0], N1[i]);
        if (!(i & 1u))
            input_off(&trk[1], 60u + i);
        if (!(i & 3u))
            input_off(&trk[3], 36u + (i & 4u) / 2u);
    }
    for (i = 0; i < 12u; i++) {
        run_to(TAPE_BAR + i * 96u);
        input_on(&trk[1], 62u + i % 5u, 100);
        run_to(TAPE_BAR + i * 96u + 80u);
        input_off(&trk[1], 62u + i % 5u);
    }
    stop();
    tape.arm = 0;
}
static void test_ui(const char *dir)
{
    uint32_t n;
    tape_power_on();
    press(B_GLO);
    bad += check("GLO: TAPE first", !ui.home && cur_page()->graph == GR_TAPE && tape_page());
    frames(400);                                    /* (not a double tap: that locks GLO's layer) */
    press(B_GLO);
    bad += check("  again: TAPE 2", cur_page()->graph == GR_TAPE2);
    frames(400);
    press(B_GLO);
    bad += check("  again: SONG", cur_page()->graph == GR_SONG && !tape_page());
    frames(400);
    press(B_GLO);
    bad += check("  .. and round to TAPE", cur_page()->graph == GR_TAPE && !ui.layer);
    frames(400);
    turn(EN_K1, 1);
    blocks(1);
    bad += check("TAPE KNOB 1: POS a beat a detent", tape.head == TAPE_PPQN);
    turn(EN_K1, 3);
    blocks(1);
    bad += check("  turned fast: a bar a detent", tape.head == TAPE_PPQN + 3u * TAPE_BAR);
    turn(EN_K1, -40);
    blocks(1);
    bad += check("  never before the start", tape.head == 0u);
    turn(EN_K1, 1);
    turn(EN_K1, 1);
    turn(EN_K1, 1);
    turn(EN_K1, 1);
    turn(EN_K1, 1);
    blocks(1);
    turn(EN_K2, 2);
    bad += check("KNOB 2: LOOP 2 BARS from the bar the head is in", tape.loop == 2u && TAPE_LOOP_BARS[tape.loop] == 2u &&
                 tape.loop_in == TAPE_BAR);
    turn(EN_K2, -5);
    bad += check("  back to OFF", tape.loop == 0u);
    turn(EN_K3, 3);
    bad += check("KNOB 3: SPEED in semitones", tape.speed == 3);
    turn(EN_K3, 30);
    bad += check("  .. to +12 at most", tape.speed == 12);
    turn(EN_K3, -12);
    turn(EN_K4, 1);
    bad += check("KNOB 4: DIR REV", tape.rev == 1u);
    turn(EN_K4, -1);
    bad += check("  .. FWD", tape.rev == 0u);
    track_select(1);
    press(B_REC);
    bad += check("REC on TAPE: arms the tape for the selected track, not its pattern; PLAY starts",
                 tape.arm == 2u && song.rec == 0u && transport_req == 1u);
    stop();
    frames(400);
    press(B_REC);
    bad += check("  REC again: disarmed", tape.arm == 0u);
    tape_power_on();
    record_demo();
    n = tape_count(0);
    track_select(0);
    open_family(FAM_GLO);
    frames(400);
    press(B_GLO);                                   /* TAPE 2 */
    bad += check("TAPE 2: the ERASE actions", cur_page()->graph == GR_TAPE2 && act_cols() == 3u);
    turn(EN_K1, 1);
    press(B_OCTUP);
    bad += check("  KNOB 1 picks ERASE T1, OCT+ asks first", ui.confirm == CF_TAPE_TRK && tape_count(0) == n);
    if (dir)
        screen_save(dir, "tape-erase-confirm");
    press(B_OCTUP);
    blocks(1);
    bad += check("  OCT+ again: track 1's notes erased, the others kept",
                 tape_count(0) == 0u && tape_count(1) != 0u && tape_count(3) != 0u && tape_sane());
    turn(EN_K2, 1);
    press(B_OCTUP);
    press(B_OCTUP);
    blocks(1);
    bad += check("  KNOB 2 picks ERASE ALL: the whole tape", tape_free() == TAPE_EV);
    turn(EN_K1, 1);
    press(B_OCTUP);
    bad += check("  nothing there: NOTHING TO ERASE, no dialog", !ui.confirm && msg_is("NOTHING TO ERASE"));
    if (!dir)
        return;
    tape_power_on();
    record_demo();
    open_family(FAM_GLO);
    ui.force = 1;
    frame();
    screen_save(dir, "tape-stopped");
    tape_loop_ui(0);
    seek(TAPE_BAR / 2u);
    tape_loop_ui(2);
    tape.arm = 4;
    track_select(2);
    play();
    run_to(TAPE_BAR + 150u);
    input_on(&trk[2], 72, 100);
    run_to(TAPE_BAR + 220u);
    frame();
    screen_save(dir, "tape-recording");
    input_off(&trk[2], 72);
    stop();
    tape.arm = 0;
    tape.speed = -5;
    tape.rev = 1;
    frame();
    screen_save(dir, "tape-rev-speed");
    tape.speed = 0;
    tape.rev = 0;
    blocks(1);
    frames(400);
    press(B_GLO);
    screen_save(dir, "tape2");
}

int main(int argc, char **argv)
{
    test_core();
    test_loop();
    test_speed_dir();
    test_erase_full_end();
    test_ui(argc > 1 ? argv[1] : 0);
    if (bad) {
        printf("tape: %d FAILED\n", bad);
        return 1;
    }
    printf("tape: all ok\n");
    return 0;
}
