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

int main(void)
{
    wf_octaves();
    wf_mixer();
    wf_rec_menu();
    wf_step_fields();
    printf(wf_bad ? "WORKFLOW TESTS FAILED\n" : "WORKFLOW TESTS PASSED\n");
    return wf_bad != 0;
}
