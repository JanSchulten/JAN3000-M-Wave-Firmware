/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* VDRUM on the device (M-Vave-OP1; firmware/src/ui_vdrum.c, eng_vdrum.c), through the UI harness (tests/ui_test.c):
 *   PAGES    EDIT > LAYER, LAYER PITCH, LAYER ENV, PART, PART 2 on a VDRUM track only; HOME's KNOB 1..4 are TYPE DECAY
 *            BODY TUNE; a key struck by hand picks the part the pages edit (the title), the sequencer's notes do not.
 *   EDIT     a detent writes one byte of the part: a factory kit becomes the track's own (KIT OWN), the rest of it as
 *            it was; LYR L1 / L2 / L1+L2 (both layers move); SRC MOD EG step through their names and stay in range;
 *            the panel redraws (graph_signature) with every value; SAVE held undoes the last knob gesture, again redoes it.
 *   STORE    a project keeps a VDRUM track's own kit (in its FM6 patch's place) and an FM6 track's patch side by side;
 *            a record that is no kit loads INIT; a user preset of a VDRUM sound keeps its kit (up_fm6.c), the editor's
 *            FM6_GET finds none in it, an FM6 user preset still keeps its patch.
 *   SCREENS  with a directory argument: the five pages and HOME (grey palette) as PPM.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("vdrum ui: %-90s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

static void vd_track(uint32_t preset)
{
    ui_power_on();
    set_engine_of(TSEL, ENGI_VDRUM);
    apply_preset_to(TSEL, preset);
    TSEL->engine = TSEL->eng_req;
    eng_mem_claim(TSEL);
    frames(400);
}

static uint32_t key_of(uint32_t note)                /* the black key of note's part (M-Vave-OP1: the drum keys, seq.c */
{                                                    /* kit_key; Felucca's keys played the GM map) */
    int32_t st;
    return key_at(1, vd_part_note(note, &st));
}
static void strike(uint32_t note)                   /* a key down for a block, then up */
{
    uint32_t k = key_of(note);
    fm1_in.notes |= 1u << k;
    host_notes |= 1u << k;
    frame();
    events_block(CTL);
    fm1_in.notes &= ~(1u << k);
    frame();
    events_block(CTL);
}

static int page_shown(uint32_t graph)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == graph)
            return page_visible(i);
    return 0;
}

static void pages_test(void)
{
    uint32_t g, vis = 1, hid = 1;
    vd_track(0);
    for (g = GR_VDL; g <= GR_VDP2; g++)
        vis &= page_shown(g) != 0;
    ck("EDIT > LAYER .. PART 2 shown on a VDRUM track", vis);
    ui_power_on();                                   /* track 1: ANALOG */
    for (g = GR_VDL; g <= GR_VDP2; g++)
        hid &= !page_shown(g);
    ck(".. and not on an ANALOG track", hid);
    vd_track(0);
    ck("HOME: KNOB 1..4 are TYPE DECAY BODY TUNE", ENGINES[ENGI_VDRUM]->knob[0] == P_E0 && ENGINES[ENGI_VDRUM]->knob[3] == P_E3);
    go_page(GR_VDL);
    frame();
    ck("no key struck yet: part 1 (KICK)", vdu_part() == 0u);
    strike(39);
    ck("CLAP struck by hand: part 3", vdu_part() == 2u);
    strike(46);
    ck("HAT OP struck: part 5", vdu_part() == 4u);
    trk_note_on(TSEL, 36, 100);                      /* (as the sequencer plays a note: not input_on) */
    events_block(CTL);
    ck("a note of the sequencer does not move the part", vdu_part() == 4u);
}

static void edit_test(void)
{
    uint8_t before[VK_SIZE];
    uint32_t sig0, sig1, b, l2;
    int ok = 1;
    vd_track(0);
    strike(38);                                      /* SNARE: two layers in INIT */
    go_page(GR_VDLP);
    frame();
    sig0 = graph_signature();
    memcpy(before, VD_FACTORY[0], VK_SIZE);
    turn(EN_K2, 3);                                   /* PITCH of layer 1 up */
    b = VK_PART(1) + VK_LPIT;
    ck("a detent: KIT OWN, the own kit holds the factory kit", TSEL->p[P_E4] == VD_OWN && vd_own[0][0] == VK_TAG);
    ck(".. with layer 1's PITCH up", vd_own[0][b] > before[b]);
    {
        uint32_t i;
        for (i = 0; i < VK_SIZE; i++)
            ok &= i == b || vd_own[0][i] == before[i];
    }
    ck(".. and every other byte as it was", ok);
    sig1 = graph_signature();
    ck("the panel redraws (graph_signature)", sig0 != sig1);
    go_page(GR_VDLE);
    frame();
    turn(EN_K1, 2);                                   /* LYR: L1+L2 */
    ck("LYR: L1+L2", vdu_lyr == 2u);
    l2 = vd_own[0][VK_PART(1) + 7u + VK_LLVL];
    b = vd_own[0][VK_PART(1) + VK_LLVL];
    turn(EN_K4, -1);
    ck("L1+L2: LVL moves both layers", vd_own[0][VK_PART(1) + VK_LLVL] < b && vd_own[0][VK_PART(1) + 7u + VK_LLVL] < l2);
    turn(EN_K1, -1);                                  /* L2 */
    b = vd_own[0][VK_PART(1) + VK_LLVL];
    turn(EN_K4, -1);
    ck("L2: only layer 2", vd_own[0][VK_PART(1) + VK_LLVL] == b);
    go_page(GR_VDL);
    frame();
    turn(EN_K1, -1);                                  /* L1 */
    {
        uint32_t i, inrange = 1;
        for (i = 0; i < 8u; i++) {
            turn(EN_K2, 1);
            turn(EN_K3, 1);
            turn(EN_K4, 1);
            inrange &= vdu_get(vd_own[0], 1, 0, VU_SRC) <= 4u && vdu_get(vd_own[0], 1, 0, VU_MOD) <= 2u &&
                       vdu_get(vd_own[0], 1, 0, VU_EG) <= 2u;
        }
        ck("SRC MOD EG step to their last names and stay there", inrange && vdu_get(vd_own[0], 1, 0, VU_SRC) == VS_BPN &&
           vdu_get(vd_own[0], 1, 0, VU_EG) == VE_MULT);
    }
    frames(400);
    memcpy(before, vd_own[0], VK_SIZE);
    go_page(GR_VDLP);
    frame();
    turn(EN_K3, 2);                                   /* one more gesture: MAMT */
    ck("MAMT moved", memcmp(before, vd_own[0], VK_SIZE) != 0);
    frames(400);
    hold(B_SAVE);
    ck("SAVE held: the last gesture undone (the kit as before it)", !memcmp(before, vd_own[0], VK_SIZE));
    hold(B_SAVE);
    ck(".. again: redone", memcmp(before, vd_own[0], VK_SIZE) != 0);
}

static void store_test(void)
{
    static project_t p;
    uint8_t want[VK_SIZE], fm6[FM6_PACKED], pk[FM6_PACKED];
    int rc;
    vd_track(2);
    set_engine_of(&trk[1], ENGI_FM6);
    apply_preset_to(&trk[1], 3);
    trk[1].engine = trk[1].eng_req;
    memcpy(vd_own[0], VD_FACTORY[4], VK_SIZE);
    vd_own[0][VK_PART(2) + VK_LPIT] = 99;
    TSEL->p[P_E4] = VD_OWN;
    memcpy(want, vd_own[0], VK_SIZE);
    fm6_pack(fm6_patch[1], fm6);
    project_capture(&p);
    ck("a project holds the VDRUM track's own kit", !memcmp(p.fm6[0], want, VK_SIZE));
    ck(".. and the FM6 track's patch", !memcmp(p.fm6[1], fm6, FM6_PACKED));
    memcpy(vd_own[0], VD_FACTORY[1], VK_SIZE);
    TSEL->p[P_E4] = 1;
    apply_preset_to(&trk[1], 0);
    ck("the project loads", !project_restore_runtime(&p));
    fm6_pack(fm6_patch[1], pk);
    ck("the own kit back, KIT OWN", !memcmp(vd_own[0], want, VK_SIZE) && trk[0].p[P_E4] == VD_OWN && trk[0].eng_req == ENGI_VDRUM);
    ck("the FM6 patch back", !memcmp(pk, fm6, FM6_PACKED));
    p.fm6[0][0] = 0x12;                              /* no kit (a wrong tag) */
    p.sum = proj_sum(&p);
    ck("a project whose record is no kit loads", !project_restore_runtime(&p));
    ck(".. with INIT", !memcmp(vd_own[0], VD_FACTORY[0], VK_SIZE));
    vd_track(5);
    memcpy(vd_own[0], VD_FACTORY[6], VK_SIZE);
    vd_own[0][VK_PART(0) + VK_LREL] = 11;
    TSEL->p[P_E4] = VD_OWN;
    memcpy(want, vd_own[0], VK_SIZE);
    rc = up_store(3, "MYKIT");
    ck("a user preset of the VDRUM sound is written", rc == 0 || rc == 3);
    ck(".. holding the kit", !upf_get_of(3, pk, ENGI_VDRUM) && !memcmp(pk, want, VK_SIZE));
    ck(".. the editor's FM6_GET finds no patch in it", upf_get(3, pk) != 0);
    memset(vd_own[0], 0, VK_SIZE);
    TSEL->p[P_E4] = 0;
    ck("the user preset loads", !up_load(3));
    ck(".. its kit back, KIT OWN", !memcmp(vd_own[0], want, VK_SIZE) && TSEL->p[P_E4] == VD_OWN);
    song.sel = 1;
    set_engine_of(TSEL, ENGI_FM6);
    apply_preset_to(TSEL, 2);
    TSEL->engine = TSEL->eng_req;
    fm6_pack(fm6_patch[1], fm6);
    rc = up_store(4, "MYFM");
    ck("an FM6 user preset still keeps its patch", (rc == 0 || rc == 3) && !upf_get(4, pk) && !memcmp(pk, fm6, FM6_PACKED));
    song.sel = 0;
}

static void screens(const char *dir)
{
    static const struct { uint32_t g; const char *n; } S[] = {
        {GR_VDL, "vdrum_layer"}, {GR_VDLP, "vdrum_layer_pitch"}, {GR_VDLE, "vdrum_layer_env"}, {GR_VDP, "vdrum_part"},
        {GR_VDP2, "vdrum_part2"},
    };
    uint32_t i;
    vd_track(1);
    strike(38);
    for (i = 0; i < NELEM(S); i++) {
        go_page(S[i].g);
        frame();
        screen_save(dir, S[i].n);
    }
    ui.home = 1;
    ui.force = 1;
    frame();
    screen_save(dir, "vdrum_home");
}

int main(int argc, char **argv)
{
    pages_test();
    edit_test();
    store_test();
    if (argc > 1)
        screens(argv[1]);
    printf(bad ? "vdrum ui: %d FAIL\n" : "vdrum ui: all ok\n", bad);
    return bad != 0;
}
