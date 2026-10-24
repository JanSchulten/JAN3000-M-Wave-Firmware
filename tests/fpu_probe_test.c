/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The FPU probe (M-Vave-OP1; firmware/src/fpu_probe.c, plaits_ws.c) on the host, through the UI harness (tests/ui_test.c):
 *   DSP      the exact checks pass in plain float; the waveshaper's 16-block hash is FPB_WS_HASH (the unit compares its
 *            own with it: a change to plaits_ws.c or its tables must update the constant, which this test prints);
 *            the waveshaper stays bounded and sounds over the keyboard and its corners.
 *   GUARD    INFO's OCT+ while the boot guard still counts the boot (the first 30 s) runs nothing and says so.
 *   PAGE     after it, OCT+ runs the probe once a press: the verdict OK, the rows drawn (graph signature), OCT+ breathing,
 *            OCT- back to the tabs; with a directory argument the page as PPM.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("fpu probe: %-90s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

static void dsp_test(void)
{
    static volatile const float IN[6] = {1.5f, 2.25f, 0.125f, 1.0f, 3.0f, -2.75f};
    static volatile const int32_t IV[1] = {7};
    uint32_t h = fpb_ws_hash(), b, i, k, ok = 1;
    float peak = 0.0f, rms = 0.0f;
    ck("the exact results (multiply-add, divide, float <-> int)", fpb_check(IN, IV));
    printf("fpu probe: waveshaper hash 0x%08X (FPB_WS_HASH 0x%08X)\n", (unsigned)h, (unsigned)FPB_WS_HASH);
    ck("the waveshaper's 16 blocks hash to FPB_WS_HASH", h == FPB_WS_HASH);
    for (k = 0; k < 8u; k++) {                         /* notes C1 .. C8 at the corners of HARMONICS TIMBRE MORPH */
        float f0 = 440.0f * powf(2.0f, ((float)(24 + 12 * k) - 69.0f) / 12.0f) / 44100.0f;
        float hm = (k & 1u) ? 1.0f : 0.0f, tb = (k & 2u) ? 1.0f : 0.0f, mo = (k & 4u) ? 1.0f : 0.0f;
        double e = 0.0;
        pws_init(&fpb_ws);
        for (b = 0; b < 200u; b++) {
            pws_render(&fpb_ws, f0, hm, tb, mo, fpb_out, fpb_aux, 32u);
            for (i = 0; i < 32u; i++) {
                float a = fpb_out[i] < 0.0f ? -fpb_out[i] : fpb_out[i];
                ok &= fpb_out[i] == fpb_out[i] && fpb_aux[i] == fpb_aux[i];   /* (no NaN) */
                peak = a > peak ? a : peak;
                e += (double)fpb_out[i] * fpb_out[i];
            }
        }
        rms = (float)sqrt(e / (200.0 * 32.0));
        ok &= rms > 0.01f;
    }
    printf("fpu probe: waveshaper peak %.3f over the keyboard and the corners\n", peak);
    ck("the waveshaper is bounded (|out| <= 1.5), no NaN, and sounds at every corner", ok && peak <= 1.5f);
}

static void page_test(const char *dir)
{
    uint32_t sig;
    ui_power_on();
    go_home();
    frames(100);
    ui.menu = 1;                                       /* (MENU at SYSTEM > INFO, as ui_test.c sets it) */
    ui.menu_sel = MI_INFO;
    frame();
    press(B_OCTUP);
    ck("INFO opens, OCT- lit and OCT+ breathing", ui.menu == 3 && oct_leds() == (1u | OCT_BREATH));
    bootguard.pending = 1;
    frame();
    sig = info_sig();
    press(B_OCTUP);
    frame();
    ck("OCT+ in the first 30 s: nothing run, the message says when", fpb.runs == 0 && fpb.state == FPB_NONE &&
       msg_is("FPU TEST AFTER 30 S") && ui.menu == 3);
    bootguard.pending = 0;
    press(B_OCTUP);
    frame();
    ck("after it: one run, the verdict OK", fpb.runs == 1u && fpb.state == FPB_OK && msg_is("FPU OK"));
    ck("the rows redraw with it (INFO's signature)", info_sig() != sig);
    if (dir)
        screen_save(dir, "info_fpu");
    press(B_OCTUP);
    frame();
    ck("each press runs it again", fpb.runs == 2u);
    press(B_OCTDN);
    ck("OCT- back to the tabs", ui.menu == 1 && ui.menu_sel == MI_INFO);
}

int main(int argc, char **argv)
{
    dsp_test();
    page_test(argc > 1 ? argv[1] : 0);
    printf(bad ? "fpu probe: FAILED\n" : "fpu probe: all ok\n");
    return bad != 0;
}
