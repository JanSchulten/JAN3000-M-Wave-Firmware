/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: the second core's start and its end, the application's side (docs/DUALCORE_PLAN.md step 4; after
 * Felucca 1.6.1's main.c). The core itself is hal/fm1_cpu1.h (from felucca-bsp: Melodee's start, mailbox and fault
 * path); fx.c mix_block hands it some of the parts each block.
 *   c1_boot   at power-on, before the audio and the timers, interrupts off (started later it never reaches its
 *             entry): only with MENU > SYSTEM > 2ND CORE ON, and never on a start that follows a failed one (the boot
 *             guard counted a crash or a hang in the last start's first 30 s: that start runs on one core and the
 *             option turns itself OFF, so a unit the second core cannot start on is back on one core after one
 *             restart). Then core 1's FPU: the probe's results (fpu_probe.c: four exact ones and the waveshaper's
 *             hash) computed there. Right: every part may go to it. Wrong: the parts that compute on the FPU stay on
 *             core 0 (fx.c mix_fpu_parts). A fault of core 1's on the way retires it (hal/fm1_cpu1.h, the fallback)
 *   c1_poll   the main loop: core 1 misread, faulted, was late or did not answer (hal: held from then on, one core
 *             until power-off), or the start was skipped: the option OFF, saved, and said once */
#if FELUCCA_CPU2
enum { C1B_NONE, C1B_START, C1B_OFF };
/* what a start does about the second core: failed = the boot guard's count of failed starts before this one */
static uint32_t c1_boot_plan(uint32_t failed, uint32_t on) { return !on ? C1B_NONE : failed ? C1B_OFF : C1B_START; }
static uint32_t c1_set(void) { return (ui_prefs2 & (PREF_CPU2_ON >> 16)) != 0u; }
enum { C1N_NONE, C1N_GAVE_UP, C1N_BAD_START };
static uint8_t c1_note;                         /* what c1_poll still has to say (C1N_*) */
static const char *c1_note_text(uint32_t n) { return n == C1N_BAD_START ? "2ND CORE OFF: BAD START" : "2ND CORE MISREAD: OFF"; }
/* the option OFF and saved, and the reason on the screen (ui.msg: 23 characters at most); once a power-on (ON set
 * again afterwards stays, for the next one) */
static void c1_tell(uint32_t n)
{
    if (c1_set()) {
        ui_prefs2 &= (uint8_t)~(PREF_CPU2_ON >> 16);
        settings_later();
    }
    ui_message(c1_note_text(n));
}

#ifdef C1_HW
static volatile uint8_t c1_fpu_res;             /* core 1's answer: 1 right, 2 wrong */
static void c1_fpu_job(uint32_t arg)            /* on core 1 */
{
    static volatile const float IN[6] = {1.5f, 2.25f, 0.125f, 1.0f, 3.0f, -2.75f};   /* (fpu_probe's) */
    static volatile const int32_t IV[1] = {7};
    (void)arg;
    c1_fpu_res = (uint8_t)(fpb_check(IN, IV) && fpb_ws_hash() == FPB_WS_HASH ? 1u : 2u);
    fm1_cpu1_sync();
}
static void c1_boot(void)
{
    uint32_t plan = c1_boot_plan(bootguard.failed, c1_set());
    if (plan == C1B_OFF)
        c1_note = C1N_BAD_START;
    if (plan != C1B_START || fm1_cpu1_start())
        return;
    if (fm1_cpu1_run(c1_fpu_job, 0) && !fm1_cpu1_wait()) {
        fm1_cpu1_sync();
        c1_fpu_ok = c1_fpu_res == 1u;
    }
}
static void c1_poll(void)
{
    static uint8_t told;
    uint32_t n = c1_note ? c1_note : *(volatile uint8_t *)&fm1_cpu1_gave_up ? C1N_GAVE_UP : C1N_NONE;   /* (the ISR's) */
    if (!n || told)
        return;
    told = 1;
    c1_tell(n);
}
#endif
#endif
