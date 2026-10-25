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

int main(void)
{
    wf_octaves();
    wf_mixer();
    printf(wf_bad ? "WORKFLOW TESTS FAILED\n" : "WORKFLOW TESTS PASSED\n");
    return wf_bad != 0;
}
