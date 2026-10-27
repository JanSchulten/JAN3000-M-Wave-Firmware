/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SCL > CSET (firmware/src/cset.c, the chord sets after the Roland J-6's), on the firmware's own sources (ui_test.c's
 * harness):
 *   DATA     the hundred sets of the J-6 owner's manual's Chord Set List (cset_data.h): set 1's C Cadd9 C3 G3 D4 E4,
 *            the names as listed (C#M9/C, Eb; the list's F#FM7 as its notes are: F#M7; the stacks by key), every chord
 *            1 .. 4 notes ascending in the list's range; KNOB 3's genres: POP, JAZZ, BLUES .. MODERN, stopping at the ends.
 *   CHORDS   a key's chord: F3 .. E4 as listed, an octave up or down for each octave the note is away; TRN moves them;
 *            a one-voice part the lowest note; notes out of 0..127 left out.
 *   KEYS     through the real key input (keyboard_block): every key its chord (a black key under QNT WHITE too), the
 *            octave buttons, a release ends exactly its notes; CSET off and a kit: the keys as ever; the ARP holds the
 *            chord's notes; a MIDI note plays the chord of its key and its release ends it.
 *   PAGE     SCL > CHORD, one page: KNOB 1 OFF .. POW, then SET 1 .. 100 (a set makes the track POLY, LEG kept; back
 *            to a shape gives LEG back); a mono sound loaded while on plays POLY (OFF: its LEG); VOICE set by hand
 *            stays; KNOB 3 the genres, before SET 1 back to POW; SCL held: KNOB 3 the same list, one SAVE undo;
 *            STEP entry and live recording write the chord; ON, SET and the VOICE kept with the project (the extras),
 *            older extras: off.
 *   With a directory: the CSET page's screens.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("cset: %-102s %s\n", what, ok ? "ok" : "FAIL");
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
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }
static uint32_t held_bits(const track_t *t)         /* the notes the track's voices hold (bit n - 32 for 32..95) */
{
    uint32_t i, m = 0;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].gate && t->v[i].note >= 32u && t->v[i].note < 64u)
            m |= 1u << (t->v[i].note - 32u);
    return m;
}
static uint32_t bits(const uint8_t *n, uint32_t c)
{
    uint32_t i, m = 0;
    for (i = 0; i < c; i++)
        if (n[i] >= 32u && n[i] < 64u)
            m |= 1u << (n[i] - 32u);
    return m;
}
static void cset_track(uint32_t k, uint32_t set)
{
    cset_en |= (uint8_t)(1u << k);
    trk[k].p[P_VOICE] = V_POLY;
    cset_set[k] = (uint8_t)set;
    song.sel = (uint8_t)k;
}

/* -------------------------------------------------------------- DATA --- */
static void test_data(void)
{
    static const uint8_t CADD9[] = {48, 55, 62, 64};
    uint32_t s, k, i, ok = CSET_N == 100u, runs = 1;
    char b[16];
    ok &= !memcmp(CSET_NOTE[0][0], CADD9, 4);
    cset_name(b, 0, 0);
    ok &= str_eq(b, "Cadd9");
    cset_name(b, 0, 1);
    ok &= str_eq(b, "C#M9/C");
    cset_name(b, 7, 3);
    ok &= str_eq(b, "Eb");
    cset_name(b, 18, 6);
    ok &= str_eq(b, "F#M7");
    cset_name(b, 13, 10);
    ok &= str_eq(b, "A#oct");
    for (s = 0; s < CSET_N; s++)
        for (k = 0; k < 12u; k++) {
            const uint8_t *c = CSET_NOTE[s][k];
            ok &= c[0] >= 29u && c[0] <= 98u && CSET_NAME[s][k][1] < CSET_NQ;
            for (i = 1; i < 4u; i++)
                ok &= !c[i] || (c[i] > c[i - 1u] && c[i] <= 98u);
        }
    ck("DATA: 100 sets; set 1 C Cadd9 C3 G3 D4 E4, C#M9/C; set 8's Eb; set 19's F#M7; the stacks by key; 1..4 notes up", ok);
    ok = cset_genre_step(0, 1) == 2u && cset_genre_step(2, 1) == 5u && cset_genre_step(3, -1) == 2u &&
         cset_genre_step(2, -1) == 0u && cset_genre_step(0, -1) == 0u && str_eq(CSET_GENRE[CSET_GENRE_OF[2]], "JAZZ");
    for (s = 0; (k = cset_genre_step(s, 1)) != s; s = k)
        runs++;
    ok &= s == 98u && str_eq(CSET_GENRE[CSET_GENRE_OF[s]], "MODERN") && cset_genre_step(99, 1) == 98u && runs >= 30u;
    printf("cset:   %u genre runs\n", runs);
    ck("  KNOB 3: set 1 POP -> 3 JAZZ -> 6 BLUES; back to the run's start, then the previous; MODERN last, the ends stop", ok);
}

/* ------------------------------------------------------------ CHORDS --- */
static void test_chords(void)
{
    static const uint8_t F3[] = {41, 57, 64, 67}, E4[] = {52, 60, 62, 67}, F4[] = {53, 69, 76, 79}, E3[] = {40, 48, 50, 55};
    static const uint8_t C4[] = {48, 55, 62, 64}, C4T2[] = {50, 57, 64, 66};
    uint8_t n[CHORD_MAX];
    uint32_t c;
    int ok;
    ui_power_on();
    cset_track(0, 0);
    c = cset_make(TSEL, 53, 0, n);
    ok = notes_are(n, c, F3, 4);
    c = cset_make(TSEL, 64, 0, n);
    ok &= notes_are(n, c, E4, 4);
    c = cset_make(TSEL, 65, 0, n);
    ok &= notes_are(n, c, F4, 4);
    c = cset_make(TSEL, 52, 0, n);
    ok &= notes_are(n, c, E3, 4);
    c = cset_make(TSEL, 60, 0, n);
    ok &= notes_are(n, c, C4, 4);
    ck("CHORDS: F3 .. E4 as listed (F3: F2 A3 E4 G4, E4: E3 C4 D4 G4); F4 an octave up, E3 one down; C4 as listed", ok);
    c = cset_make(TSEL, 60, 2, n);
    ok = notes_are(n, c, C4T2, 4);
    TSEL->p[P_VOICE] = V_MONO;
    c = cset_make(TSEL, 60, 0, n);
    ok &= c == 1u && n[0] == 48u;
    TSEL->p[P_VOICE] = V_POLY;
    c = cset_make(TSEL, 125, 0, n);               /* (F9: set 1's F six octaves up, only its F2 + 72 under 128) */
    ok &= c == 1u && n[0] == 41u + 72u;
    ck("  TRN +2 moves them; a MONO part the lowest note (C3); notes past 127 left out", ok);
}

/* -------------------------------------------------------------- KEYS --- */
static void test_keys(void)
{
    static const uint8_t F3[] = {41, 57, 64, 67}, FS3[] = {42, 57, 62, 64};
    uint32_t i;
    int ok;
    ui_power_on();
    stop_transport();
    cset_track(0, 0);
    key_down(0);                                  /* F3 */
    blk();
    ok = notes_are(kb_chord[0], kb_chn[0], F3, 4) && (held_bits(TSEL) & bits(F3, 4)) == bits(F3, 4);
    key_up(0);
    blk();
    ok &= !kb_chn[0] && !held_bits(TSEL);
    ck("KEYS: F3 plays set 1's F (FM9: F2 A3 E4 G4); let go, its notes end", ok);
    TSEL->p[P_QUANT] = QN_WHITE;
    key_down(1);                                  /* F#3: a black key under WHITE, its chord all the same */
    ok = notes_are(kb_chord[1], kb_chn[1], FS3, 4);
    key_up(1);
    TSEL->p[P_QUANT] = 0;
    song.octave = 1;
    key_down(0);
    ok &= kb_chn[0] == 4u && kb_chord[0][0] == 53u;
    key_up(0);
    song.octave = 0;
    TSEL->p[P_TRANS] = 3;
    key_down(0);
    ok &= kb_chn[0] == 4u && kb_chord[0][0] == 44u;
    key_up(0);
    TSEL->p[P_TRANS] = 0;
    ck("  a black key under QNT WHITE plays its chord (F#: Dadd9/F#); OCT+ an octave up; TRN +3 moves the chord", ok);
    cset_en = 0;
    key_down(0);
    ok = kb_chn[0] == 1u && kb_chord[0][0] == 53u;
    key_up(0);
    set_engine_of(&trk[3], ENGI_DRUM);
    trk[3].engine = trk[3].eng_req;
    cset_en = 8u;
    ok &= !cset_on(&trk[3]);
    cset_en = 0;
    ck("  CSET off: F3 the note alone; a kit with CSET on: no chords", ok);
    cset_track(0, 0);
    trk[0].p[P_AMODE] = 1;                        /* (ARP UP) */
    transport_req = 1;
    for (i = 0; i < 4u; i++)
        blk();
    key_down(0);
    ok = trk[0].nheld == 4u;
    for (i = 0; i < 400u; i++)
        blk();
    ok &= !trk[0].arp_note || trk[0].arp_note == 41u || trk[0].arp_note == 57u || trk[0].arp_note == 64u ||
          trk[0].arp_note == 67u;
    key_up(0);
    ok &= !trk[0].nheld;
    trk[0].p[P_AMODE] = 0;
    stop_transport();
    ck("  the ARP holds the chord's four notes and steps among them", ok);
    {                                             /* MIDI IN: the chord of the note's key, its release ends it */
        static const uint8_t C4[] = {48, 55, 62, 64};
        song.g[G_ROUTE] = 0;                      /* (CH1-4: channel 1 plays track 1) */
        midi_note_event(0, 60, 100);
        blk();
        ok = (held_bits(TSEL) & bits(C4, 4)) == bits(C4, 4);
        midi_note_event(0, 60, 0);
        blk();
        ok &= !held_bits(TSEL);
        ck("  a MIDI note (C4 on channel 1) plays set 1's C (Cadd9), its note-off ends it", ok);
    }
}

/* -------------------------------------------------------------- PAGE --- */
static void test_page(const char *dir)
{
    static const uint8_t F3[] = {41, 57, 64, 67};
    uint32_t i;
    int ok, ok2;
    ui_power_on();
    stop_transport();
    song.sel = 0;
    set_engine_of(TSEL, 0);
    apply_preset_to(TSEL, 0);                     /* (SAW LEAD: a mono sound, LEG) */
    frames(400);
    ok = TSEL->p[P_VOICE] == V_LEGATO;
    for (i = 0, ok2 = 1; i < NPAGES; i++)
        ok2 &= !str_eq(PAGES[i].title, "CSET");
    go_title("CHORD");
    frames(500);
    ok &= ok2 && cur_page()->fam == FAM_SCL && !cset_on(TSEL) && TSEL->p[P_CHRD] == CH_OFF;
    if (dir)
        screen_save(dir, "chord_off");
    for (i = 0; i < 9u; i++)
        turn(EN_K1, 1);
    ok &= TSEL->p[P_CHRD] == CH_POW && !cset_on(TSEL);
    turn(EN_K1, 1);                               /* (past POW: SET 1) */
    ok &= cset_on(TSEL) && cset_set[0] == 0u && TSEL->p[P_CHRD] == CH_POW && TSEL->p[P_VOICE] == V_POLY &&
          cset_voice[0] == V_LEGATO;
    turn(EN_K1, -1);
    ok &= !cset_on(TSEL) && TSEL->p[P_CHRD] == CH_POW && TSEL->p[P_VOICE] == V_LEGATO;
    ck("PAGE: one CHORD page (no CSET page); KNOB 1 OFF .. POW, then SET 1 (POLY, LEG kept), back: POW, LEG", ok);
    turn(EN_K1, 1);
    apply_preset_to(TSEL, 0);                     /* (a mono sound loaded while on: POLY, its LEG kept) */
    ok = TSEL->p[P_VOICE] == V_POLY && cset_voice[0] == V_LEGATO;
    turn(EN_K1, -1);
    ok &= TSEL->p[P_VOICE] == V_LEGATO;
    turn(EN_K1, 1);
    TSEL->p[P_VOICE] = V_MONO;                    /* (VOICE set by hand while on: OFF leaves it) */
    turn(EN_K1, -1);
    ok &= TSEL->p[P_VOICE] == V_MONO;
    TSEL->p[P_VOICE] = V_LEGATO;
    ck("  a mono sound loaded while on plays POLY (OFF: its LEG); VOICE set by hand stays", ok);
    turn(EN_K1, 1);
    turn(EN_K1, 1);
    ok = cset_set[0] == 1u;
    for (i = 0; i < 120u; i++)
        turn(EN_K1, 1);
    ok &= cset_set[0] == 99u && cset_on(TSEL);
    for (i = 0; i < 99u; i++)
        turn(EN_K1, -1);
    ok &= cset_set[0] == 0u && cset_on(TSEL);
    turn(EN_K3, 1);
    ok &= cset_set[0] == 2u;
    turn(EN_K3, 1);
    ok &= cset_set[0] == 5u;
    turn(EN_K3, -1);
    ok &= cset_set[0] == 2u;
    turn(EN_K3, -1);
    ok &= cset_set[0] == 0u && cset_on(TSEL);
    turn(EN_K3, -1);                              /* (before SET 1: the chord shapes again, POW) */
    ok &= !cset_on(TSEL) && TSEL->p[P_CHRD] == CH_POW;
    turn(EN_K3, -1);
    ok &= !cset_on(TSEL) && TSEL->p[P_CHRD] == CH_POW;
    turn(EN_K3, 1);                               /* (from a shape: SET 1) */
    ok &= cset_on(TSEL) && cset_set[0] == 0u;
    frame();
    ok &= help_page() && str_eq(help_page(), "[K1]CHORD[K3]GENRE");
    ck("  KNOB 1 SET 1 .. 100 (stops at the end), KNOB 3 the genres (POP, JAZZ 3, BLUES 6, back, POW); its help line", ok);
    {                                             /* the SCL layer's CHRD: the same list, one SAVE undo */
        go_page(GR_ROLL);
        frames(400);
        cset_switch(TSEL, 0);
        TSEL->p[P_CHRD] = CH_MAJ;
        TSEL->p[P_VOICE] = V_LEGATO;
        frames(400);
        btn_down(B_SCL);
        frames(500);
        for (i = 0; i < 8u; i++)
            turn(EN_K3, 1);                       /* (MAJ .. POW, then SET 1, SET 2) */
        ok = cset_on(TSEL) && cset_set[0] == 1u && TSEL->p[P_VOICE] == V_POLY;
        btn_up(B_SCL);
        frame();
        hold(B_SAVE);
        ok &= !cset_on(TSEL) && TSEL->p[P_CHRD] == CH_MAJ && TSEL->p[P_VOICE] == V_LEGATO;
        hold(B_SAVE);
        ok &= cset_on(TSEL) && cset_set[0] == 1u && TSEL->p[P_VOICE] == V_POLY;
        ck("  SCL held, KNOB 3: the same list into the sets; SAVE held undoes the set and its POLY, again redoes", ok);
        cset_switch(TSEL, 0);
        TSEL->p[P_VOICE] = V_LEGATO;
        cset_set[0] = 0;
        TSEL->p[P_CHRD] = CH_POW;
        go_title("CHORD");
        frames(400);
        turn(EN_K1, 1);                           /* (SET 1, for the screens and the entry below) */
    }
    if (dir) {
        cset_set[0] = 0;
        frame();
        screen_save(dir, "cset_idle");
        key_down(0);
        frame();
        screen_save(dir, "cset");
        key_up(0);
        cset_set[0] = 73;
        key_down(5);                              /* (A#3: set 74's A#, Dm7) */
        frame();
        screen_save(dir, "cset_long");
        key_up(5);
        cset_set[0] = 56;                         /* (set 57: Bb5add9/b13 on A#, the widest name) */
        frame();
        screen_save(dir, "cset_wide");
        cset_set[0] = 0;
    }
    track_defaults_steps(TSEL);                   /* STEP entry (armed, stopped): F3 writes its chord */
    go_page(GR_ROLL);
    cursor_set(0);
    frame();
    song.rec = 1;
    key_down(0);
    frame();
    key_up(0);
    frame();
    ok = notes_are(TSEL->step[0].note, TSEL->step[0].n, F3, 4) && ui.cursor == 1u;
    song.rec = 0;
    ck("  STEP entry: F3 writes the chord's four notes, the cursor moves on", ok);
    track_defaults_steps(TSEL);                   /* live recording (armed, playing): the chord into the step */
    song.rec = 1;
    transport_req = 1;
    for (i = 0; i < 8u; i++)
        blk();
    key_down(0);
    blk();
    key_up(0);
    for (i = 0; i < 4u; i++)
        blk();
    {
        uint32_t s, n = 0;
        for (s = 0; s < 4u; s++)
            n += TSEL->step[s].n == 4u && notes_are(TSEL->step[s].note, 4, F3, 4);
        ok = n == 1u;
    }
    song.rec = 0;
    stop_transport();
    ck("  live recording: the chord's four notes in one step", ok);
    {                                             /* kept with the project (the extras); older extras: off */
        project_store_t slot;
        cset_en = 1u | 4u;
        cset_set[0] = 41;
        cset_set[2] = 7;
        cset_voice[0] = V_LEGATO;
        cset_voice[2] = V_UNISON;
        ok = project_save_as(1, "CSET") == 0;
        slot = proj_slot[1];
        ui_power_on();
        proj_slot[1] = slot;
        project_load(1);
        ok &= cset_en == (1u | 4u) && cset_set[0] == 41u && cset_set[2] == 7u && cset_set[1] == 0u &&
              cset_voice[0] == V_LEGATO && cset_voice[2] == V_UNISON && cset_voice[1] == CSET_VNONE;
        xtra_slot[1].size = XTRA_V5;              /* (as a firmware before CSET wrote them) */
        project_load(1);
        ok &= !cset_en && !cset_set[0] && cset_voice[0] == CSET_VNONE;
        ck("  kept with the project: ON, each track's SET and VOICE before it; extras from before CSET: off", ok);
    }
}
int main(int argc, char **argv)
{
    test_data();
    test_chords();
    test_keys();
    test_page(argc > 1 ? argv[1] : 0);
    if (bad) {
        printf("cset: %d FAILED\n", bad);
        return 1;
    }
    printf("cset: all ok\n");
    return 0;
}
