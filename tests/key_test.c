/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* GLO > KEY (firmware/src/key.c, M-Vave-OP1), on the firmware's own sources through the UI harness:
 *   MOVE     key_shift_to in C major: Am F C G on C E G (degrees 6 4 1 5, the nearer way: A below, F above), a passing
 *            note kept above its degree, a root outside the scale chromatically (the nearer way), A minor's own
 *            degrees, CHR as its ROOT's major; a kit never; KEY OFF, the transport stopped or a bar with no root: as
 *            written.
 *   PLAYING  a pattern of C4 on step 1, KEY A F over 2 bars: A3 in bar 1, F4 in bar 2, A3 again (the run round); each
 *            note's off its on (nothing left sounding after STOP); LEN 1: one bar; the arp's notes moved, not while
 *            the track records; the keys played live as they are.
 *   PAGE     GLO opens KEY; KNOB 1 the bar, 2 its ROOT (- C .. B), 3 LEN (the cursor kept in it), 4 ON; a key sets the
 *            cursor bar's root and moves on (round at LEN), only on the page; the cards and the chord names in the
 *            selected track's scale; screens into DIR (optional).
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("key: %-103s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }
static void ktap(uint32_t k) { key_down(k); blk(); frame(); key_up(k); blk(); frame(); }   /* (the ISR takes the key) */

/* -------------------------------------------------------------- MOVE --- */
static void test_move(void)
{
    track_t *t = &trk[0];
    char b[8];
    ui_power_on();
    t->p[P_ROOT] = 0;
    t->p[P_SCALE] = 1;                              /* C major */
    ck("C E G under A: A3 C4 E4 (degree 6, down)", key_shift_to(t, 60, 9) == 57u && key_shift_to(t, 64, 9) == 60u &&
       key_shift_to(t, 67, 9) == 64u);
    ck("  under F: F4 A4 C5 (degree 4, up); under C: as written",
       key_shift_to(t, 60, 5) == 65u && key_shift_to(t, 64, 5) == 69u && key_shift_to(t, 67, 5) == 72u &&
       key_shift_to(t, 60, 0) == 60u && key_shift_to(t, 71, 0) == 71u);
    ck("  under G: G3 B3 D4 (degree 5, down); under D: D4 F4 A4",
       key_shift_to(t, 60, 7) == 55u && key_shift_to(t, 64, 7) == 59u && key_shift_to(t, 67, 7) == 62u &&
       key_shift_to(t, 60, 2) == 62u && key_shift_to(t, 64, 2) == 65u && key_shift_to(t, 67, 2) == 69u);
    ck("  a passing note (C#4) kept a semitone over its degree's move: A#3 under A",
       key_shift_to(t, 61, 9) == 58u);
    ck("  a root outside the scale (A#, Eb): the notes chromatically, the nearer way (-2, +3)",
       key_shift_to(t, 60, 10) == 58u && key_shift_to(t, 64, 3) == 67u);
    t->p[P_ROOT] = 9;
    t->p[P_SCALE] = 2;                              /* A minor: A C E under C (degree 3) -> C E G */
    ck("A minor's A C E under C: C E G", key_shift_to(t, 57, 0) == 60u && key_shift_to(t, 60, 0) == 64u &&
       key_shift_to(t, 64, 0) == 67u);
    t->p[P_ROOT] = 0;
    t->p[P_SCALE] = 0;                              /* CHR: C major */
    ck("CHR: its ROOT's major (C under A: A3)", key_shift_to(t, 60, 9) == 57u);
    t->p[P_SCALE] = 1;
    key_chord_name(t, 9, b);
    ck("chord names in C major: Am F C G Bdim, a root outside: its name, none: -",
       str_eq(b, "Am") && (key_chord_name(t, 5, b), str_eq(b, "F")) && (key_chord_name(t, 0, b), str_eq(b, "C")) &&
       (key_chord_name(t, 7, b), str_eq(b, "G")) && (key_chord_name(t, 11, b), str_eq(b, "Bdim")) &&
       (key_chord_name(t, 10, b), str_eq(b, "A#")) && (key_chord_name(t, KEY_NONE, b), str_eq(b, "-")));
    keyt.on = 1;
    keyt.root[0] = 9;
    key_bar = 0;
    song.playing = 0;
    ck("stopped: as written", key_shift(t, 60) == 60u);
    song.playing = 1;
    ck("playing, ON: moved", key_shift(t, 60) == 57u);
    keyt.on = 0;
    ck("OFF: as written", key_shift(t, 60) == 60u);
    keyt.on = 1;
    keyt.root[0] = KEY_NONE;
    ck("a bar with no root: as written", key_shift(t, 60) == 60u);
    keyt.root[0] = 9;
    set_engine_of(t, ENGI_DRUM);
    ck("a kit (DRUM): never", key_shift(t, 60) == 60u);
    song.playing = 0;
    key_defaults();
}

/* ----------------------------------------------------------- PLAYING --- */
/* the transport from stop over n bars: the sequencer's note on track 0 each time one starts, into got (its count) */
static uint32_t play_bars(uint32_t bars, uint8_t *got, uint32_t max)
{
    uint32_t i, n = 0, blocks = bars * 16u * div_samples(2) / CTL + 4u;
    uint8_t was = 0;
    transport_req = 1;
    for (i = 0; i < blocks; i++) {
        blk();
        if (trk[0].seq_n && trk[0].seq_notes[0] != was && n < max)
            got[n++] = trk[0].seq_notes[0];
        was = trk[0].seq_n ? trk[0].seq_notes[0] : 0;
    }
    return n;
}
static void pattern_c4(void)
{
    track_t *t = &trk[0];
    memset(t->step, 0, sizeof t->step);
    t->step[0].note[0] = 60;
    t->step[0].n = 1;
    t->step[0].time = ST_NOTE;
    t->step[0].vel = 100;
    t->p[P_SLEN] = 16;
    t->p[P_ROOT] = 0;
    t->p[P_SCALE] = 1;
}
static void test_playing(void)
{
    uint8_t got[8];
    uint32_t n, i, live;
    ui_power_on();
    pattern_c4();
    key_defaults();
    keyt.on = 1;
    keyt.len = 2;
    keyt.root[0] = 9;
    keyt.root[1] = 5;
    n = play_bars(3, got, 8);
    ck("C4 on step 1, KEY A F: A3, F4, A3 again (the run round)", n >= 3u && got[0] == 57u && got[1] == 65u && got[2] == 57u);
    transport_req = 2;
    for (i = 0; i < 8u; i++)
        blk();
    for (i = live = 0; i < NVOICE; i++)
        live += trk[0].v[i].active && trk[0].v[i].gate;
    ck("  STOP: nothing left sounding on the track (each off its on)", !song.playing && !trk[0].seq_n && !live);
    keyt.len = 1;
    n = play_bars(2, got, 8);
    ck("  LEN 1: bar 1's root every bar", n >= 2u && got[0] == 57u && got[1] == 57u);
    transport_req = 2;
    blk();
    keyt.on = 0;
    n = play_bars(1, got, 8);
    ck("  KEY OFF: as written", n >= 1u && got[0] == 60u);
    transport_req = 2;
    blk();
    {   /* the arp: C E G held, UP; KEY on A: the list moved; recording that track: as played */
        uint32_t list[64], cnt, len;
        track_t *t = &trk[0];
        memset(t->step, 0, sizeof t->step);
        keyt.on = 1;
        t->p[P_AMODE] = 1;
        memset(held_key[0], 0, sizeof held_key[0]);
        t->held[0] = 60; t->held[1] = 64; t->held[2] = 67; t->nheld = 3;
        song.playing = 1;
        key_bar = 0;
        len = arp_list(t, list, &cnt);
        n = len == 3u && list[0] == 57u && list[1] == 60u && list[2] == 64u;
        song.rec = 1;
        len = arp_list(t, list, &cnt);
        n &= len == 3u && list[0] == 60u;
        song.rec = 0;
        song.playing = 0;
        len = arp_list(t, list, &cnt);
        n &= list[0] == 60u;
        ck("the arp's notes moved (A C E under A), as played while the track records or stopped", n);
        t->nheld = 0;
        t->p[P_AMODE] = 0;
    }
    {   /* a key played live while it runs: as it is */
        ui_power_on();
        pattern_c4();
        memset(trk[0].step, 0, sizeof trk[0].step);
        keyt.on = 1;
        keyt.len = 1;
        keyt.root[0] = 9;
        transport_req = 1;
        for (i = 0; i < 20u; i++)
            blk();
        key_down(white(4));                         /* C4 (the 5th white key from F3) */
        for (i = 0; i < 4u; i++)
            blk();
        for (i = live = 0; i < NVOICE; i++)
            live |= trk[0].v[i].active && trk[0].v[i].gate && trk[0].v[i].note == 60u;
        key_up(white(4));
        transport_req = 2;
        blk();
        ck("a key played live while KEY runs: as it is (C4)", live != 0u && last_note == 60u);
    }
    key_defaults();
}

/* -------------------------------------------------------------- PAGE --- */
static void test_page(const char *dir)
{
    uint32_t ok;
    ui_power_on();
    frames(320);                                     /* (taps further apart than a double tap) */
    press(B_GLO); frames(320);
    ok = str_eq(cur_page()->title, "KEY") && cur_page()->graph == GR_KEY;
    ck("GLO opens KEY (GLO's first page)", ok);
    ok = keyt.len == 4u && !keyt.on && keyt.root[0] == KEY_NONE && key_cur == 0u;
    turn(EN_K2, 1);                                  /* - -> C */
    ok &= keyt.root[0] == 0u;
    turn(EN_K2, 9);                                  /* -> A */
    ok &= keyt.root[0] == 9u;
    turn(EN_K2, -20);                                /* -> - */
    ok &= keyt.root[0] == KEY_NONE;
    ck("KNOB 2 ROOT: - C .. B, not round", ok);
    turn(EN_K1, 2);
    ok = key_cur == 2u;
    turn(EN_K1, 20);
    ok &= key_cur == 3u;                             /* (LEN 4: bar 4 the last) */
    turn(EN_K3, -2);
    ok &= keyt.len == 2u && key_cur == 1u;           /* (the cursor kept in LEN) */
    turn(EN_K3, 30);
    ok &= keyt.len == KEY_BARS;
    turn(EN_K3, -12);
    ok &= keyt.len == 4u;
    ck("KNOB 1 the bar (in LEN), KNOB 3 LEN 1..16 (the cursor kept in it)", ok);
    turn(EN_K4, 1);
    ok = keyt.on == 1u && msg_is("KEY ON");
    turn(EN_K4, -1);
    ok &= !keyt.on && msg_is("KEY OFF");
    turn(EN_K4, 1);
    ck("KNOB 4 ON / OFF", ok && keyt.on);
    turn(EN_K1, -10);
    ktap(white(9));    /* A4 (the 10th white key from F3): bar 1 A, on to bar 2 */
    ok = keyt.root[0] == 9u && key_cur == 1u;
    ktap(white(7));    /* F4 */
    ktap(white(4));    /* C4 */
    ktap(white(8));    /* G4 */
    ok &= keyt.root[1] == 5u && keyt.root[2] == 0u && keyt.root[3] == 7u && key_cur == 0u;   /* (round at LEN 4) */
    ck("a key sets the cursor bar's root and moves on (Am F C G, round at LEN)", ok);
    if (dir) {
        song.playing = 1;
        key_bar = 2;
        ui.force = 1;
        frame();
        screen_save(dir, "key_page");
        song.playing = 0;
    }
    go_title("ENV");
    ktap(white(4));
    frames(320);
    press(B_GLO); frames(320);
    ok = str_eq(cur_page()->title, "KEY") && keyt.root[0] == 9u && key_cur == 0u;
    ck("a key on another page: no root (KEY's back as it was)", ok);
    key_defaults();
}

int main(int argc, char **argv)
{
    test_move();
    test_playing();
    test_page(argc > 1 ? argv[1] : 0);
    printf("%s\n", bad ? "KEY TEST FAILED" : "key test passed");
    return bad != 0;
}
