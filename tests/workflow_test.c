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

int main(void)
{
    wf_octaves();
    printf(wf_bad ? "WORKFLOW TESTS FAILED\n" : "WORKFLOW TESTS PASSED\n");
    return wf_bad != 0;
}
