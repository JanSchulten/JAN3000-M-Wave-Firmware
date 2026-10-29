/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* JAN3000's single-path controls, through the real UI against the host panel. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int wf_bad;
static void wf_check(const char *what, int ok)
{
    printf("workflow: %-100s %s\n", what, ok ? "ok" : "FAIL");
    wf_bad += !ok;
}

static void wf_octaves(void)
{
    step_t before;
    int ok;
    ui_power_on();
    frames(400);
    go_page(GR_ROLL);
    song.octave = 0;
    TSEL->step[0].time = ST_NOTE;
    TSEL->step[0].n = 1;
    TSEL->step[0].note[0] = 60;
    TSEL->step[0].vel = 100;
    cursor_set(0);
    before = TSEL->step[0];
    press(B_OCTUP);
    ok = song.octave == 1 && !memcmp(&before, &TSEL->step[0], sizeof before);
    press(B_OCTDN);
    ok &= song.octave == 0 && !memcmp(&before, &TSEL->step[0], sizeof before);
    wf_check("STEP stopped: OCT+ / OCT- move the octave and preserve every byte of the step", ok);
}

static void wf_mixer(void)
{
    uint32_t i;
    int ok = 1;
    ui_power_on();
    for (i = 0; i < NPAGES; i++) {
        const page_t *p = &PAGES[i];
        if (str_eq(p->title, "VOICE 2"))
            ok &= p->id[0] == P_ALLOC && p->id[1] == P_DETUNE && p->id[2] == P_SPRD && p->id[3] == 0xFFu;
        ok &= !str_eq(p->title, "VOICE 3");
    }
    wf_check("VOICE 2: ALLOC DTUNE SPRD only; no VOICE 3", ok);
    go_title("MIXER");
    int16_t rev = TSEL->p[P_REV];
    turn(EN_K3, 1);
    wf_check("MIXER does not edit REV; the send stays on FX", TSEL->p[P_REV] == rev);
    persist_t p;
    memset(&p, 0, sizeof p);
    settings_export(&p);
    p.favorites.factory[15][26] = HV_LEVELS;
    settings_import(&p, sizeof p);
    wf_check("stored HOME LEVELS migrates to TRACKS", ui_home_view == HV_TRACKS && !home_levels());
}

static void wf_rec_menu(void)
{
    int ok = 1;
    uint32_t i;
    ui_power_on();
    hold(B_HOME);
    turn(EN_ALGO, 1); turn(EN_ALGO, 1);
    for (i = 0; i < mtab_rows(MTAB_AUDIO); i++) {
        ok &= ui.menu_sel != MI_CLICK && ui.menu_sel != MI_CLKLVL && ui.menu_sel != MI_COUNTIN;
        turn(EN_PRESET, 1);
    }
    wf_check("AUDIO menu contains no CLICK, CLICK LEVEL or COUNT-IN; REC owns these settings", ok);
}

static void wf_step_fields(void)
{
    uint16_t rows[EV_ROWS];
    uint32_t i, n;
    int ok = 1;
    ui_power_on();
    step_set_chance(&TSEL->step[0], 50);
    step_set_ratchet(&TSEL->step[0], 3);
    step_set_nudge(&TSEL->step[0], 4);
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60; TSEL->step[0].vel = 80;
    motion_set_lock(TSEL, 0, P_ATK, 70);
    go_auto_top(); ui.ev_vel = 1;
    n = ev_list(rows);
    for (i = 0; i < n; i++) ok &= !ev_skd(rows[i] >> 8);
    ok &= !ev_id_ok(TSEL, EV_CHANCE) && !ev_id_ok(TSEL, EV_RATCH) && !ev_id_ok(TSEL, EV_NUDGE) && !ev_id_ok(TSEL, EV_VEL);
    wf_check("AUTOMATION lists parameter records only, with no step fields or ADD VEL", ok && n == 4u);
}

static void wf_clear_routes(void)
{
    uint32_t i;
    int ok = 1;
    ui_power_on(); frames(400);
    for (i = 0; i < NPAGES; i++) ok &= PAGES[i].graph != GR_TOOLS;
    go_page(GR_ROLL);
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60;
    motion_set_lock(TSEL, 0, P_ATK, 70);
    btn_down(B_SEQ); frames(500);
    key_down(1); frame(); key_up(1); frame();
    ok &= TSEL->step[0].n == 1 && motion_count(TSEL) == 1;
    btn_up(B_SEQ); frame(); frames(400);
    btn_down(B_REC); frames(500);
    key_down(1); frame(); key_up(1); frame(); key_down(3); frame(); key_up(3); frame();
    ok &= !ui.confirm && TSEL->step[0].n == 1 && motion_count(TSEL) == 1;
    key_down(0); frame(); key_up(0); frame();
    ok &= TSEL->step[0].n == 0 && !motion_count(TSEL);
    btn_up(B_REC); frame();
    wf_check("whole-pattern CLEAR is REC + F3 only, clears notes and automation; no SAVE TOOLS", ok);
    go_page(GR_FXT);
    fxt_lane[0].step[0] = 1; fxt_lane[1].step[0] = 1; ui.fxt_sel = 0;
    press(B_EDIT);
    ok = ui.confirm == CF_FXT_LANE;
    press(B_OCTUP);
    wf_check("FX TRACKS: EDIT confirms clearing only the selected lane", ok && !fxt_used(0) && fxt_used(1));
    go_page(GR_SONG);
    chain_config.count = 1; chain_config.row[0] = chain_row_of(0, 1); ui.song_row = 0;
    tap_key(1); ok = ui.confirm == CF_DEL_ROW; press(B_OCTDN);
    tap_key(3); ok &= ui.confirm == CF_CLEAR_SONG; press(B_OCTDN);
    wf_check("SONG owns section delete (F#3) and song clear (G#3), both with confirmation", ok);
}

static void wf_navigation(void)
{
    uint32_t f, sign;
    int ok = 1;
    int16_t bpm;
    ui_power_on(); frames(400); press_raw = 1;
    bpm = song.g[G_BPM];
    turn(EN_SELECT, 1); ok &= !ui.home && cur_page()->graph == GR_TRK;
    turn(EN_SELECT, 1); ok &= !ui.home && str_eq(cur_page()->title, "CLOCK");
    turn(EN_SELECT, 1); ok &= !ui.home && str_eq(cur_page()->title, "EQ");   /* (M-Vave-OP1: HOME > EQ) */
    turn(EN_SELECT, 1); ok &= ui.home;
    turn(EN_SELECT, -1); ok &= str_eq(cur_page()->title, "EQ") && !ui.home;
    press(B_HOME); ok &= ui.home;
    for (f = FAM_ENV; f < FAM_TRK; f++) {
        go_home(); frames(400);
        press(FAM_BTN[f]); frames(400);
        ok &= !ui.home && cur_page()->fam == f;
        press(FAM_BTN[f]); ok &= ui.home;
        frames(400); press(FAM_BTN[f]); press(FAM_BTN[f]);
        ok &= ui.home && !ui.lock;
    }
    for (sign = 0; sign < 2; sign++) {
        if (sign) ui_prefs |= PREF_BPM_LOCK;
        for (f = 0; f < 12; f++) turn(EN_SELECT, 1);
        ok &= song.g[G_BPM] == bpm;
    }
    ok &= !menu_visible(MI_BPMLOCK);
    press_raw = 0;
    wf_check("SELECT pages every family including HOME/MIXER/CLOCK/EQ, never BPM; every page button toggles HOME", ok);
}

static void wf_storage(void)
{
    uint32_t i;
    int ok = 1;
    ui_power_on(); frames(400); go_page(GR_SLOTS);
    song.g[G_SLOT] = 1; turn(EN_K1, 1); turn(EN_K3, 1); turn(EN_K4, 1); turn(EN_PRESET, 1);
    ok &= song.g[G_SLOT] == 1 && !ui.act;
    key_down(white(3)); frame(); key_up(white(3)); frame();
    ok &= song.g[G_SLOT] == 4;
    go_page(GR_USER);
    for (i = 0; i < 16u; i++) {
        key_down(white(i)); frame(); key_up(white(i)); frame();
        ok &= ui.uslot == i && song.grid == 5u && !gates();
    }
    turn(EN_K1, 1);
    ok &= ui.uslot == 31u;
    for (i = 0; i < 16u; i++) {
        key_down(white(i)); frame(); key_up(white(i)); frame();
        ok &= ui.uslot == i + 16u;
    }
    up_store(31, "SLOT31");
    key_down(1); frame(); key_up(1); frame();
    ok &= name_on() && nm.kind == NK_USER_RENAME && nm.slot == 31u;
    press(B_OCTDN);
    key_down(3); frame(); key_up(3); frame();
    ok &= ui.confirm == CF_ERASE_USER;
    press(B_OCTDN);
    press(B_OCTDN); ok &= ui.confirm == CF_LOAD_USER;
    press(B_OCTUP); ok &= TSEL->user == 32u;
    go_page(GR_SLOTS); song.g[G_SLOT] = 2;
    project_save_as(1, "KEEP");
    key_down(3); frame(); key_up(3); frame();
    ok &= ui.confirm == CF_ERASE_PROJ;
    press(B_OCTDN); ok &= project_used(1);
    key_down(3); frame(); key_up(3); frame();
    press(B_OCTUP); ok &= !project_used(1);
    wf_check("PROJECT and USER: white-key slots, USER two banks on K1; F#3 rename, G#3 delete; no knob actions", ok);
}

static void wf_storage_octaves(void)
{
    uint32_t user;
    for (user = 0; user < 2u; user++) {
        int ok;
        ui_power_on(); frames(400);
        go_page(user ? GR_USER : GR_SLOTS);
        ui.uslot = 0; song.g[G_SLOT] = 1; song.octave = 1;
        press(B_OCTDN);
        ok = song.octave == 1 && !ui.confirm && !name_on() && msg_is("EMPTY SLOT");
        press(B_OCTUP);
        ok &= song.octave == 1 && name_on();
        press(B_OCTDN);
        ok &= song.octave == 1 && !name_on();
        press(B_OCTUP); press(B_OCTUP); /* empty slot: NAME, then write */
        ok &= song.octave == 1 && !name_on() && (user ? up_used(0) : project_used(0));
        press(B_OCTDN);
        ok &= song.octave == 1 && ui.confirm == (user ? CF_LOAD_USER : CF_LOAD_PROJ);
        press(B_OCTDN); /* cancel the load */
        ok &= song.octave == 1 && !ui.confirm;
        press(B_OCTUP);
        ok &= song.octave == 1 && ui.confirm == (user ? CF_OVR_USER : CF_OVR_PROJ);
        press(B_OCTDN); /* cancel the overwrite */
        ok &= song.octave == 1 && !ui.confirm;
        press(B_OCTUP); press(B_OCTUP); press(B_OCTUP); /* overwrite, NAME, write */
        ok &= song.octave == 1 && !name_on() && !ui.confirm;
        press(B_OCTDN); press(B_OCTUP); /* load, confirm */
        ok &= song.octave == 1 && !ui.confirm;
        wf_check(user ? "USER: empty/load/save/overwrite and their cancellations preserve the keyboard octave"
                      : "PROJECT: empty/load/save/overwrite and their cancellations preserve the keyboard octave", ok);
        go_home(); press(B_OCTUP); ok = song.octave == 2;
        press(B_OCTDN);
        wf_check("after leaving storage, OCT+ / OCT- still transpose the keyboard", ok && song.octave == 1);
    }
}

static void wf_undo_groups(void)
{
    int ok;
    int16_t level, scale;
    step_t before[NSTEP];
    ui_power_on(); frames(400);
    level = trk[0].p[P_LEVEL];
    btn_down(B_GLO); frames(500);
    turn(EN_K1, -2); turn(EN_K2, -3); slot_key(black(1));
    press(B_OCTDN);
    ok = trk[0].p[P_LEVEL] == level - 2 && trk[1].p[P_MUTE] && song.octave == -1;
    hold(B_SAVE);
    ok &= trk[0].p[P_LEVEL] == level && !trk[1].p[P_MUTE];
    hold(B_SAVE);
    ok &= trk[0].p[P_LEVEL] == level - 2 && trk[1].p[P_MUTE];
    btn_up(B_GLO); frame();
    wf_check("GLO: OCT- only shifts octave; SAVE held inside the layer undoes/redoes its whole mix", ok);
    ui_power_on(); frames(400); scale = TSEL->p[P_SCALE];
    btn_down(B_SCL); frames(500); turn(EN_K2, 1); turn(EN_K3, 1);
    btn_up(B_SCL); frame(); hold(B_SAVE);
    ok = TSEL->p[P_SCALE] == scale && TSEL->p[P_CHRD] == TP[P_CHRD].def;
    hold(B_SAVE); ok &= TSEL->p[P_SCALE] == scale + 1;
    wf_check("SCL: all changes in one opening are one SAVE undo/redo", ok);
    ui_power_on(); track_defaults_steps(TSEL); TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60; memcpy(before, TSEL->step, sizeof before);
    btn_down(B_SEQ); frames(500); slot_key(black(1)); slot_key(black(2));
    btn_down(B_EDIT); frame(); btn_up(B_EDIT); frame(); ok = memcmp(before, TSEL->step, sizeof before) != 0;
    btn_up(B_SEQ); frame(); hold(B_SAVE);
    ok &= !memcmp(before, TSEL->step, sizeof before);
    wf_check("SEQ: EDIT does not reset; multiple tools in one opening undo together", ok);
    ui_power_on(); frames(400); go_page(GR_ROLL); track_defaults_steps(TSEL); TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60; memcpy(before, TSEL->step, sizeof before);
    press(B_REC); frames(400);
    TSEL->step[0].note[0] = 62; TSEL->step[4].note[0] = 74;
    btn_down(B_REC); frames(500); slot_key(black(LY_INIT_B)); press(B_OCTDN); press(B_OCTUP);
    ok = TSEL->step[0].note[0] == 62;
    btn_up(B_REC); frame(); hold(B_SAVE);
    ok &= !memcmp(before, TSEL->step, sizeof before);
    hold(B_SAVE); ok &= TSEL->step[0].note[0] == 62;
    wf_check("recording is one SAVE undo/redo; REC BACK and REC OCT undo routes are absent", ok);
}

/* Exercise the sounding and saved values separately: a snapshot must not stop motion, and a swap must
 * refresh its playback base before another step runs. The host audio clock advances explicitly here. */
static void wf_undo_automation(void)
{
    int ok;
    uint8_t original, loaded;
    int16_t loaded_atk;
    undo_t prior;
    ui_power_on(); frames(400);
    TSEL->p[P_ATK] = 23; original = TSEL->preset;
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60;
    motion_set_event(TSEL, 0, P_ATK, 92);
    seq_start(); seq_tick(TSEL, CTL);
    btn_down(B_EDIT); frames(500);
    ok = TSEL->p[P_ATK] == 92 && motion_base_value(TSEL, P_ATK) == 23;
    turn(EN_K2, 1);
    loaded = TSEL->preset; loaded_atk = motion_base_value(TSEL, P_ATK);
    btn_up(B_EDIT); frame(); hold(B_SAVE);
    ok &= TSEL->preset == original && motion_base_value(TSEL, P_ATK) == 23;
    motion_step(TSEL, 0, &motion, 1); seq_stop();
    ok &= TSEL->p[P_ATK] == 23 && motion_count(TSEL) == 1;
    wf_check("EDIT sound undo during automation restores the patch base, including after another step and STOP", ok);
    seq_start(); seq_tick(TSEL, CTL); hold(B_SAVE);
    ok = TSEL->preset == loaded && loaded != original && motion_base_value(TSEL, P_ATK) == loaded_atk;
    motion_step(TSEL, 0, &motion, 1); seq_stop();
    wf_check("sound redo during automation restores the loaded patch base through playback and STOP",
             ok && TSEL->p[P_ATK] == loaded_atk && motion_count(TSEL) == 1);

    ui_power_on(); frames(400); turn(EN_PRESET, 1);
    motion_set_event(TSEL, 0, P_ATK, 92); motion_set_event(TSEL, 1, P_ATK, 37);
    seq_start(); seq_tick(TSEL, CTL); prior = undo;
    btn_down(B_EDIT); frames(500); turn(EN_K4, 1); /* unused: an input, but no persistent edit */
    motion_step(TSEL, 1, &motion, 1);
    btn_up(B_EDIT); frame();
    wf_check("automation advancing during an unchanged EDIT opening preserves the preceding undo",
             !memcmp(&prior, &undo, sizeof undo));
    seq_stop();
}

static void wf_record_domains(void)
{
    int ok;
    uint8_t key_before[sizeof keyt];
    ui_power_on(); frames(400);
    trk[0].step[0].note[0] = 60; trk[1].step[0].note[0] = 65;
    press(B_REC); frames(400); turn(EN_ALGO, 1); press(B_REC); frames(400);
    trk[0].step[0].note[0] = 72; trk[1].step[0].note[0] = 77;
    motion_set_lock(&trk[0], 0, P_ATK, 23);
    motion_set_lock(&trk[3], 1, P_ATK, 45);
    hold(B_SAVE);
    ok = trk[0].step[0].note[0] == 60 && trk[1].step[0].note[0] == 65 && !motion_count(&trk[0]) && motion_count(&trk[3]) == 1;
    hold(B_SAVE);
    ok &= trk[0].step[0].note[0] == 72 && trk[1].step[0].note[0] == 77 && motion_count(&trk[0]) == 1;
    wf_check("one recording undo spans all armed tracks and their motion; unarmed tracks stay intact", ok);
    ui_power_on(); frames(400); go_page(GR_FXT);
    fxt_lane[0].step[0] = 1; fxt_lane[1].step[1] = 2;
    press(B_REC); frames(400); turn(EN_K1, 1); press(B_REC); frames(400);
    fxt_lane[0].step[0] = 3; fxt_lane[1].step[1] = 4;
    hold(B_SAVE);
    ok = fxt_lane[0].step[0] == 1 && fxt_lane[1].step[1] == 2;
    hold(B_SAVE); ok &= fxt_lane[0].step[0] == 3 && fxt_lane[1].step[1] == 4;
    wf_check("FX recording: both armed lanes undo and redo as one take", ok);
    ui_power_on(); frames(400); go_page(GR_KEY);
    memcpy(key_before, (const void *)&keyt, sizeof keyt);
    press(B_REC); frames(400); song.playing = 1; slot_key(white(3)); song.playing = 0;
    ok = memcmp(key_before, (const void *)&keyt, sizeof keyt) != 0;
    hold(B_SAVE); ok &= !memcmp(key_before, (const void *)&keyt, sizeof keyt) && !key_own;
    hold(B_SAVE); ok &= memcmp(key_before, (const void *)&keyt, sizeof keyt) != 0 && key_own;
    wf_check("KEY recording: roots, ON and scene ownership undo and redo together", ok);
    ui_power_on(); frames(400); go_page(GR_SONG); chain_defaults(&chain_config);
    press(B_REC); frames(400); slot_key(4); slot_key(6);
    ok = chain_config.count == 2;
    hold(B_SAVE); ok &= !chain_config.count;
    hold(B_SAVE); ok &= chain_config.count == 2;
    wf_check("SONG REC chain entry is one take for SAVE undo and redo", ok);
}

static void wf_layer_settings(void)
{
    int ok;
    uint8_t map[20];
    ui_power_on(); frames(400);
    btn_down(B_EDIT); frames(500); turn(EN_K3, 1); btn_up(B_EDIT); frame();
    ok = preset_favorite(); hold(B_SAVE); ok &= !preset_favorite(); hold(B_SAVE); ok &= preset_favorite();
    wf_check("EDIT favorite changes participate in the layer's undo transaction", ok);
    memcpy(map, favorites.factory[15], 10); memcpy(map + 10, favorites.factory[15] + 14, 10);
    btn_down(B_FX); frames(500); key_down(white(1)); frame(); turn(EN_PRESET, 1); key_up(white(1)); frame(); btn_up(B_FX); frame();
    ok = memcmp(map + 10, favorites.factory[15] + 14, 10) != 0;
    hold(B_SAVE); ok &= !memcmp(map + 10, favorites.factory[15] + 14, 10);
    hold(B_SAVE); ok &= memcmp(map + 10, favorites.factory[15] + 14, 10) != 0;
    wf_check("FX key-map settings undo and redo via SAVE, independently of temporary punch-ins", ok);
}

static void wf_empty_transaction(void)
{
    int ok;
    step_t before[NSTEP];
    ui_power_on(); frames(400);
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60;
    memcpy(before, TSEL->step, sizeof before);
    turn(EN_PRESET, 1);
    uint8_t previous = undo.preset, selected = TSEL->preset;
    btn_down(B_SEQ); frames(500); slot_key(black(1)); slot_key(black(1)); btn_up(B_SEQ); frame();
    ok = !memcmp(before, TSEL->step, sizeof before);
    hold(B_SAVE);
    ok &= TSEL->preset == previous && TSEL->preset != selected;
    wf_check("a layer changed back to its opening state preserves the preceding undo", ok);
}

static void wf_capture(const char *dir)
{
    ui_power_on(); frames(400); ui.msg_t = 0;
    screen_save(dir, "home");
    go_page(GR_TRK); ui.msg_t = 0; frame(); screen_save(dir, "mixer");
    go_title("CLOCK"); ui.msg_t = 0; frame(); screen_save(dir, "clock");
    go_title("VOICE 2"); ui.msg_t = 0; frame(); screen_save(dir, "voice2");
    btn_down(B_REC); frames(500); ui.msg_t = 0; ui.force = 1; frame(); screen_save(dir, "recording"); btn_up(B_REC); frame();
    btn_down(B_SEQ); frames(500); ui.msg_t = 0; ui.force = 1; frame(); screen_save(dir, "seq-tools"); btn_up(B_SEQ); frame();
    go_title("DETAIL"); ui.msg_t = 0; frame(); screen_save(dir, "detail");
    motion_set_lock(TSEL, 0, P_ATK, 70); go_auto_top(); ui.msg_t = 0; frame(); screen_save(dir, "automation");
    go_page(GR_USER); ui.uslot = 31; up_store(31, "TEST SOUND"); ui.msg_t = 0; frame(); screen_save(dir, "user-bank2");
    go_page(GR_SLOTS); ui.msg_t = 0; frame(); screen_save(dir, "project");
    go_page(GR_FXT); ui.fxt_sel = 0; fxt_lane[0].step[0] = 1; confirm_open(CF_FXT_LANE, 0); frame(); screen_save(dir, "fx-clear");
    ui.confirm = 0; go_page(GR_SONG); confirm_open(CF_CLEAR_SONG, 0); frame(); screen_save(dir, "song-clear");
}

int main(int argc, char **argv)
{
    wf_octaves();
    wf_mixer();
    wf_rec_menu();
    wf_step_fields();
    wf_clear_routes();
    wf_navigation();
    wf_storage();
    wf_storage_octaves();
    wf_undo_groups();
    wf_undo_automation();
    wf_record_domains();
    wf_layer_settings();
    wf_empty_transaction();
    if (argc > 1) wf_capture(argv[1]);
    printf(wf_bad ? "WORKFLOW TESTS FAILED\n" : "WORKFLOW TESTS PASSED\n");
    return wf_bad != 0;
}
