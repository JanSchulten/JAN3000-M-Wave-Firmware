/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SCL > ORCHID (firmware/src/orchid.c, the Orchid's keyboard after ChoralRoot FM-1), on the firmware's own sources
 * (ui_test.c's harness):
 *   CHORDS   the types (DIM MIN MAJ SUS) and the extensions (6 m7 M7 9) on a root as ChoralRoot's tables give them; a
 *            root alone or with extensions only: the single note; two types held: the one pressed last; VOICE up / down
 *            (the lowest up / the highest down an octave, a step out of 0..127 stops); the names the Orchid's way.
 *   KEYS     through the real key input (keyboard_block): F3 .. C4 the chord block (silent), D4 .. G5 the roots with the
 *            chord, a key's release ends exactly its notes; a track with ORCHID off and a kit: the keys as ever; the ARP
 *            plays the chord's notes; a one-voice part: the root.
 *   LOCK     B3 latches; MIN held and let go stays; 6 and 9 toggle; MAJ resets to MAJ alone; the only type pressed again
 *            alone goes off when let go (the extensions kept for the next); LOCK off clears the latch; ChoralRoot's
 *            manual's example step by step.
 *   PAGE     SCL > ORCHID (SCL's third page): KNOB 1 ON, KNOB 2 VOICE, KNOB 3 LOCK; the ARP page Felucca's again; the
 *            LEDs (the block down or latched lit, B3 with LOCK); STEP entry writes the chord (the block enters nothing,
 *            a type held: each root its own step); ON and VOICE kept with the project (the extras), the latch not.
 *   With a directory: the ORCHID page's screens.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("orchid: %-100s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static int notes_are(const uint8_t *n, uint32_t c, const uint8_t *want, uint32_t wc)
{
    uint32_t i;
    if (c != wc)
        return 0;
    for (i = 0; i < c; i++)
        if (n[i] != want[i])
            return 0;
    return 1;
}
/* the chord block's key of a role: F3 6, F#3 DIM, G3 m7, G#3 MIN, A3 M7, A#3 MAJ, B3 LOCK, C4 SUS, C#4 9 */
static uint32_t bk(uint32_t role)
{
    uint32_t k;
    for (k = 0; k < ORC_KEYS && ORC_KEY[k] != role; k++)
        ;
    return k;
}
#define D4 9u                                     /* key 9: D4, MIDI 62 */
static void tap(uint32_t k) { key_down(k); key_up(k); }
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }

/* ------------------------------------------------------------ CHORDS --- */
static void test_chords(void)
{
    static const uint8_t MAJ[] = {62, 66, 69}, MIN[] = {62, 65, 69}, DIM[] = {62, 65, 68}, SUS[] = {62, 67, 69};
    static const uint8_t MAJ6[] = {62, 66, 69, 71}, MIN7_9[] = {62, 65, 69, 72, 76}, M7[] = {62, 66, 69, 73};
    static const uint8_t ALL[] = {62, 66, 69, 71, 72, 73, 76}, V1[] = {66, 69, 74}, V2[] = {69, 74, 78},
                         VM1[] = {57, 62, 66}, V3[] = {74, 78, 81};
    uint8_t n[ORC_MAX];
    uint32_t c;
    char b[12], x[12];
    int ok = notes_are(n, orc_chord(62, ORC_MAJ, 0, 0, 0, n), MAJ, 3) && notes_are(n, orc_chord(62, ORC_MIN, 0, 0, 0, n), MIN, 3) &&
             notes_are(n, orc_chord(62, ORC_DIM, 0, 0, 0, n), DIM, 3) && notes_are(n, orc_chord(62, ORC_SUS, 0, 0, 0, n), SUS, 3);
    ck("CHORDS: D MAJ D F# A, MIN D F A, DIM D F G#, SUS D G A (ChoralRoot's types)", ok);
    ok = notes_are(n, orc_chord(62, ORC_MAJ, 1, 0, 0, n), MAJ6, 4) && notes_are(n, orc_chord(62, ORC_MIN, 2 | 8, 0, 0, n), MIN7_9, 5) &&
         notes_are(n, orc_chord(62, ORC_MAJ, 4, 0, 0, n), M7, 4) && notes_are(n, orc_chord(62, ORC_MAJ, 15, 0, 0, n), ALL, 7);
    ck("  extensions stack: +6 B, +m7 C, +M7 C#, +9 E; all four: seven notes", ok);
    c = orc_chord(62, ORC_NONE, 15, 0, 0, n);
    ok = c == 1u && n[0] == 62u && orc_chord(62, ORC_NONE, 0, 5, 0, n) == 1u && n[0] == 62u;
    ok &= orc_chord(62, ORC_MAJ, 15, 0, 1, n) == 1u && n[0] == 62u;
    ck("  no type: the root alone (extensions need a type; VOICE leaves a single note); one voice: the root", ok);
    ok = notes_are(n, orc_chord(62, ORC_MAJ, 0, 1, 0, n), V1, 3) && notes_are(n, orc_chord(62, ORC_MAJ, 0, 2, 0, n), V2, 3) &&
         notes_are(n, orc_chord(62, ORC_MAJ, 0, 3, 0, n), V3, 3) && notes_are(n, orc_chord(62, ORC_MAJ, 0, -1, 0, n), VM1, 3);
    c = orc_chord(120, ORC_MAJ, 0, 12, 0, n);
    ok &= c == 3u && n[2] <= 127u;
    c = orc_chord(2, ORC_MAJ, 0, -12, 0, n);
    ok &= c == 3u && n[0] == 2u;                  /* (D0's top is under 12: nothing moves down) */
    ck("  VOICE +1 F# A D', +2 A D' F#', +3 D' F#' A', -1 A, D F#; a step that leaves 0..127 stops there", ok);
    orc_name(b, x, 62, ORC_MAJ, 0);
    ok = str_eq(b, "DM") && !x[0];
    orc_name(b, x, 62, ORC_MAJ, 2 | 8);
    ok &= str_eq(b, "D") && str_eq(x, "7 9");
    orc_name(b, x, 62, ORC_MIN, 1);
    ok &= str_eq(b, "Dm") && str_eq(x, "6");
    orc_name(b, x, 62, ORC_SUS, 2);
    ok &= str_eq(b, "Dsus") && str_eq(x, "m7");
    orc_name(b, x, 62, ORC_DIM, 1 | 2 | 4);
    ok &= str_eq(b, "Ddim") && str_eq(x, "JAZZ");
    orc_name(b, x, 62, ORC_MAJ, 15);
    ok &= str_eq(x, "WTF");
    orc_name(b, x, 61, ORC_NONE, 15);
    ok &= str_eq(b, "C#") && !x[0];
    ck("  names the Orchid's way: DM, D 7 9, Dm 6, Dsus m7, Ddim JAZZ, WTF, a single C#", ok);
}

/* ------------------------------------------------------------ KEYS --- */
static void chord_track(uint32_t k)
{
    orc_en |= (uint8_t)(1u << k);
    trk[k].p[P_VOICE] = V_POLY;                   /* (track 1 is LEGATO at power-on: the root alone) */
    song.sel = (uint8_t)k;
}
static void test_keys(void)
{
    static const uint8_t DMIN[] = {62, 65, 69}, DMAJ7[] = {62, 66, 69, 73}, EMIN[] = {64, 67, 71};
    int ok;
    uint32_t i;
    ui_power_on();
    stop_transport();
    chord_track(0);
    ok = orc_on(&trk[0]);
    key_down(D4);
    ok &= kb_chn[D4] == 1u && kb_chord[D4][0] == 62u;
    key_up(D4);
    key_down(black(1));                           /* G#3: MIN, held */
    ok &= kb_note[black(1)] == KB_SILENT && ((kb_orc >> black(1)) & 1u) && ((orc.down >> ORC_MIN) & 1u);
    key_down(D4);
    ok &= notes_are(kb_chord[D4], kb_chn[D4], DMIN, 3) && orc.ltype == ORC_MIN && orc.lroot == 62u;
    key_down(D4 + 2u);                            /* E4 while D4 still sounds */
    ok &= notes_are(kb_chord[D4 + 2u], kb_chn[D4 + 2u], EMIN, 3);
    key_up(D4);
    ok &= !kb_chn[D4] && kb_chn[D4 + 2u] == 3u;
    key_up(D4 + 2u);
    key_up(black(1));
    ok &= !orc.down && !kb_orc;
    ck("KEYS: a root alone D, G#3 (MIN) held: D4 Dm, E4 Em; the block silent; a release ends its own notes", ok);
    key_down(black(2));                           /* A#3 MAJ */
    key_down(white(2));                           /* A3 M7 */
    key_down(D4);
    ok = notes_are(kb_chord[D4], kb_chn[D4], DMAJ7, 4);
    key_up(D4);
    key_down(black(0));                           /* F#3 DIM pressed last: it counts */
    key_down(D4);
    ok &= orc.ltype == ORC_DIM && kb_chord[D4][1] == 65u && kb_chord[D4][2] == 68u;
    key_up(D4);
    key_up(black(0));
    key_down(D4);                                 /* (DIM let go: MAJ again) */
    ok &= orc.ltype == ORC_MAJ;
    key_up(D4);
    key_up(white(2));
    key_up(black(2));
    ck("  MAJ + M7 held: Dmaj7; DIM pressed last counts; let go: MAJ again", ok);
    chord_track(1);
    orc_en &= (uint8_t)~2u;                       /* (track 2 with ORCHID off: F3 a note) */
    key_down(0);
    ok = kb_note[0] != KB_SILENT && !kb_orc;
    key_up(0);
    chord_track(3);                               /* (track 4, the DRUM kit: no block) */
    ok &= !orc_on(&trk[3]);
    key_down(black(1));
    ok &= !((kb_orc >> black(1)) & 1u);
    key_up(black(1));
    ck("  a track with ORCHID off, a kit with ORCHID on: the keys as ever (no chord block)", ok);
    chord_track(0);
    trk[0].p[P_AMODE] = 1;                        /* (ARP UP) */
    transport_req = 1;
    for (i = 0; i < 4u; i++)
        blk();
    key_down(black(1));
    key_down(D4);
    ok = trk[0].nheld == 3u && kb_chn[D4] == 3u;
    for (i = 0; i < 400u; i++)
        blk();
    ok &= trk[0].arp_note == 62u || trk[0].arp_note == 65u || trk[0].arp_note == 69u || !trk[0].arp_note;
    key_up(D4);
    key_up(black(1));
    ok &= !trk[0].nheld;
    trk[0].p[P_AMODE] = 0;
    stop_transport();
    ck("  the ARP (its own page again) plays the Orchid's chord: Dm's three notes held, the steps among them", ok);
    trk[0].p[P_VOICE] = V_MONO;
    key_down(black(1));
    key_down(D4);
    ok = kb_chn[D4] == 1u && kb_chord[D4][0] == 62u && orc.ltype == ORC_NONE;
    key_up(D4);
    key_up(black(1));
    trk[0].p[P_VOICE] = V_POLY;
    ck("  a MONO part: the root alone (named so: D, not Dm)", ok);
}

/* ------------------------------------------------------------ LOCK --- */
static uint32_t chord_now(uint8_t *n)             /* D4 pressed and let go: its notes */
{
    uint32_t c;
    key_down(D4);
    c = kb_chn[D4];
    memcpy(n, kb_chord[D4], ORC_MAX);
    key_up(D4);
    return c;
}
static void test_lock(void)
{
    uint8_t n[ORC_MAX];
    int ok;
    ui_power_on();
    stop_transport();
    chord_track(0);
    tap(bk(ORC_LOCK));
    ok = orc.lock == 1u;
    key_down(bk(ORC_MIN));
    key_up(bk(ORC_MIN));
    ok &= chord_now(n) == 3u && n[1] == 65u && orc.latch == (1u << ORC_MIN);
    ck("LOCK: B3 on; MIN held and let go stays latched: D4 Dm", ok);
    tap(bk(ORC_X6));
    ok = chord_now(n) == 4u && orc.lext == 1u;                      /* m6 */
    tap(bk(ORC_X9));
    ok &= chord_now(n) == 5u && orc.lext == (1u | 8u);             /* m6/9 */
    tap(bk(ORC_X9));
    ok &= chord_now(n) == 4u && orc.lext == 1u;                     /* m6 */
    tap(bk(ORC_MAJ));
    ok &= chord_now(n) == 3u && orc.ltype == ORC_MAJ && !orc.lext;  /* major, nothing else */
    ck("  ChoralRoot's example: tap 6: m6, tap 9: m6/9, 9 again: m6, MAJ: major and nothing else", ok);
    key_down(bk(ORC_MAJ));                         /* MAJ and M7 held together, let go: both latched */
    key_down(bk(ORC_XMA7));
    key_up(bk(ORC_XMA7));
    key_up(bk(ORC_MAJ));
    ok = chord_now(n) == 4u;                       /* Dmaj7 */
    tap(bk(ORC_MAJ));                              /* the only type again, alone: off when let go */
    ok &= chord_now(n) == 1u && n[0] == 62u && ((orc.latch >> ORC_XMA7) & 1u) && !(orc.latch & ORC_TOP);
    tap(bk(ORC_MAJ));                              /* a type again: back to the waiting M7 */
    ok &= chord_now(n) == 4u && orc.lext == 4u;
    ck("  the only type pressed again alone goes off (D single, M7 kept lit); MAJ again: Dmaj7", ok);
    key_down(bk(ORC_MAJ));                         /* MAJ pressed again but another key joins: no lone repeat */
    key_down(bk(ORC_X9));
    key_up(bk(ORC_X9));
    key_up(bk(ORC_MAJ));
    ok = (orc.latch & ORC_TOP) == (1u << ORC_MAJ) && ((orc.latch >> ORC_X9) & 1u);
    tap(bk(ORC_LOCK));                             /* LOCK off: the latch goes */
    ok &= !orc.lock && !orc.latch && !orc.down && chord_now(n) == 1u;
    ck("  MAJ again with 9 joining: both stay; LOCK off clears the latch (single notes)", ok);
    tap(bk(ORC_LOCK));
    key_down(bk(ORC_MIN));
    tap(bk(ORC_LOCK));                             /* (LOCK off with MIN still held: it stays down while held) */
    ok = ((orc.down >> ORC_MIN) & 1u) && chord_now(n) == 3u;
    key_up(bk(ORC_MIN));
    ok &= !orc.down;
    ck("  LOCK off with a chord key still held: it plays while held, goes up when let go", ok);
}

/* ------------------------------------------------------------ PAGE --- */
static void test_page(const char *dir)
{
    static const uint8_t DMIN[] = {62, 65, 69};
    uint32_t i, m, br;
    int ok;
    ui_power_on();
    stop_transport();
    song.sel = 0;
    trk[0].p[P_VOICE] = V_POLY;
    go_title("CHORD");
    frames(500);                                   /* (the harness's clock from 0 again: the last tap long past) */
    press(B_SCL);                                  /* (SCL after CHORD: ORCHID) */
    frames(400);                                   /* (a knob right after a page button: a combo, not the page's) */
    ok = str_eq(cur_page()->title, "ORCHID") && cur_page()->fam == FAM_SCL && !orc_on(TSEL);
    turn(EN_K1, 1);
    ok &= orc_on(TSEL) && orc_en == 1u;
    turn(EN_K1, -1);
    ok &= !orc_on(TSEL);
    turn(EN_K1, 1);
    for (i = 0; i < NPAGES && !str_eq(PAGES[i].title, "ARP"); i++)
        ;
    ok &= i < NPAGES && PAGES[i].id[0] == P_AMODE;
    ck("PAGE: SCL > CHORD > ORCHID, KNOB 1 ON / OFF (track 1's); the ARP page Felucca's again", ok);
    turn(EN_K2, 1);
    turn(EN_K2, 1);
    ok = orc_voice[0] == 2;
    for (i = 0; i < 20u; i++)
        turn(EN_K2, -1);
    ok &= orc_voice[0] == -12;
    for (i = 0; i < 12u; i++)
        turn(EN_K2, 1);
    ok &= orc_voice[0] == 0;
    turn(EN_K3, 1);
    blk();
    ok &= orc.lock == 1u;
    turn(EN_K3, -1);
    blk();
    ok &= !orc.lock;
    ck("  KNOB 2 VOICE (-12..+12), KNOB 3 LOCK (taken by the ISR at the next block)", ok);
    if (dir)
        screen_save(dir, "orchid_idle");
    turn(EN_K3, 1);
    blk();
    key_down(bk(ORC_MIN));
    key_up(bk(ORC_MIN));
    tap(bk(ORC_X9));
    turn(EN_K2, 1);
    key_down(D4);
    frame();
    m = key_leds(&br);
    ok = ((m >> bk(ORC_MIN)) & 1u) && ((m >> bk(ORC_X9)) & 1u) && ((m >> bk(ORC_LOCK)) & 1u) && !((m >> bk(ORC_MAJ)) & 1u);
    if (dir)
        screen_save(dir, "orchid");
    key_up(D4);
    ck("  LEDs: the latched MIN and 9 lit, B3 lit with LOCK, MAJ dark; the page shows the chord (Dm 9)", ok);
    /* STEP entry (armed, stopped): the chord on a root, the block enters nothing; a type held, each root its own step */
    tap(bk(ORC_LOCK));                             /* (LOCK off: nothing latched) */
    blk();
    turn(EN_K2, -1);
    track_defaults_steps(TSEL);
    go_page(GR_ROLL);
    cursor_set(0);
    frame();
    song.rec = 1;                                  /* (#133: armed) */
    key_down(bk(ORC_MIN));
    frame();
    key_down(D4);
    frame();
    key_up(D4);
    frame();
    ok = TSEL->step[0].n == 3u && notes_are(TSEL->step[0].note, TSEL->step[0].n, DMIN, 3) && ui.cursor == 1u;
    key_down(D4 + 2u);
    frame();
    key_up(D4 + 2u);
    frame();
    ok &= TSEL->step[1].n == 3u && TSEL->step[1].note[0] == 64u && ui.cursor == 2u;
    key_up(bk(ORC_MIN));
    frame();
    ok &= ui.cursor == 2u && TSEL->step[2].n == 0u;
    song.rec = 0;
    ck("  STEP entry: MIN held, D4 then E4: Dm and Em on steps 1 and 2 (the held chord key enters nothing)", ok);
    /* kept with the project: VOICE (the extras), not the latch */
    {
        project_store_t slot;
        orc_voice[0] = 5;
        orc_voice[2] = -3;
        orc_en = 1u | 4u;
        ok = project_save_as(1, "ORC") == 0;
        slot = proj_slot[1];
        ui_power_on();
        proj_slot[1] = slot;
        orc_en = 1u;
        tap(bk(ORC_LOCK));
        ok &= orc.lock == 1u;
        orc_en = 0;
        project_load(1);
        ok &= orc_voice[0] == 5 && orc_voice[2] == -3 && orc_voice[1] == 0 && orc_en == (1u | 4u) && !orc.lock && !orc.latch;
        ck("  kept with the project: ORCHID ON and each track's VOICE; a load: LOCK off, nothing latched", ok);
    }
}

int main(int argc, char **argv)
{
    test_chords();
    test_keys();
    test_lock();
    test_page(argc > 1 ? argv[1] : 0);
    if (bad) {
        printf("orchid: %d FAILED\n", bad);
        return 1;
    }
    printf("orchid: all ok\n");
    return 0;
}
