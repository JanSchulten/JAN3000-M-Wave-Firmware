/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The retired engines (M-Vave-OP1; docs/PLAITS_PLAN.md step 4: PHASE 2, VOICE 5, TRIO 6, SLICE 13, DBOX 16), through the
 * UI harness (tests/ui_test.c):
 *   OFFERED  none of them: not eng_ok, not in ENGINE_ORDER nor the PRESETS list, no factory preset, silent (ENG_GONE)
 *   PICKED   set_engine_of (the editor's engine command, an engine switch) on one: its successor's first preset
 *            (core.h eng_successor: DBOX -> DRUM, PHASE -> WSHAPE, the others -> ANALOG); the engine knob steps on from the successor
 *   PROJECT  a project with a track of each: every one loads as its successor's first preset (E values the preset's,
 *            the rest of the sound and the steps as stored, the motion on its E values gone, the other events kept)
 *   USER     a user preset record of one: listed and named as its successor, loads as its successor's first preset
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("retired: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

static const uint32_t RET[5] = {2, 5, 6, ENGI_SLICE, ENGI_DBOX};
static uint32_t want_of(uint32_t e) { return e == ENGI_DBOX ? ENGI_DRUM : e == 2u ? ENGI_WSHAPE : 0u; }
static int e_is_preset0(const track_t *t, uint32_t e)   /* its E values: engine e's first preset's */
{
    uint32_t i;
    for (i = 0; i < 8u; i++)
        if (e != ENGI_FM6 || i != 7u)
            if (t->p[P_E0 + i] != ENGINES[e]->presets[0].e[i])
                return 0;
    return 1;
}

static void offered_test(void)
{
    uint32_t i, k, e, total, ok = 1, listed = 0;
    for (i = 0; i < 5u; i++) {
        e = RET[i];
        ok &= !eng_ok(e) && eng_retired(e) && eng_successor(e) == want_of(e) && ENGINES[e] == &ENG_GONE &&
              !ENGINES[e]->npresets && str_eq(ENGINES[e]->name, "-");
        for (k = 0; k < NENG_SHOWN; k++)
            ok &= eng_vis(k) != e;
    }
    for (e = 0; e < NENGINES; e++)
        ok &= eng_retired(e) == (ENGINES[e] == &ENG_GONE) && eng_successor(e) < NENGINES && eng_ok(eng_successor(e)) ==
              (e != ENGI_DIGITAL || FELUCCA_FM4);
    ui_power_on();
    preset_all_pos(&total);
    for (i = 0; i < total; i++)
        if ((e = preset_all_at(i, &k)) < NENGINES && eng_retired(e))
            listed++;
    ck("none offered: not eng_ok, not shown, no presets, ENG_GONE; the PRESETS list holds none of theirs", ok && !listed);
    {   /* silent: a voice of ENG_GONE renders nothing */
        int32_t o[CTL];
        voice_t v;
        uint32_t j;
        memset(&v, 0, sizeof v);
        memset(o, 0, sizeof o);
        ENG_GONE.note_on(TSEL, &v);
        ENG_GONE.render(TSEL, &v, o, CTL, 0);
        for (j = 0, ok = 1; j < CTL; j++)
            ok &= o[j] == 0;
        ck("ENG_GONE renders silence", ok);
    }
}

static void picked_test(void)
{
    uint32_t i, ok = 1;
    for (i = 0; i < 5u; i++) {
        ui_power_on();
        TSEL->p[P_LEVEL] = 77;
        set_engine_of(TSEL, RET[i]);
        ok &= TSEL->eng_req == want_of(RET[i]) && TSEL->preset == 0u && e_is_preset0(TSEL, want_of(RET[i])) &&
              TSEL->p[P_LEVEL] == 77;
        ok &= eng_step(RET[i], 1) == eng_step(want_of(RET[i]), 1);
    }
    ck("set_engine_of on one: its successor's first preset (DBOX DRUM, PHASE WSHAPE, else ANALOG), the mix kept", ok);
}

static void project_test(void)
{
    static project_t p;
    uint32_t i, k, ok = 1, kept = 0, gone = 0;
    for (i = 0; i < 5u; i++) {
        ui_power_on();
        for (k = 0; k < NTRK; k++) {
            set_engine_of(&trk[k], 0);
            trk[k].p[P_E0] = 5;                        /* (E values no first preset has: WAVE PWM, DTN 98) */
            trk[k].p[P_E0 + 1] = 98;
            trk[k].p[P_LEVEL] = (int16_t)(60 + k);
            trk[k].step[3].time = ST_NOTE;
            trk[k].step[3].n = 1;
            trk[k].step[3].note[0] = (uint8_t)(50 + k);
        }
        project_capture(&p);
        p.t[1].engine = (uint8_t)RET[i];               /* track 2: the retired engine */
        p.motion.count = 2;                            /* an E value's motion on track 2, a LEVEL motion on track 3 */
        p.motion.event[0].place = (uint8_t)(1u << 6 | 4u);
        p.motion.event[0].param = (uint8_t)P_E0;
        p.motion.event[0].value = 10;
        p.motion.event[1].place = (uint8_t)(2u << 6 | 4u);
        p.motion.event[1].param = (uint8_t)P_LEVEL;
        p.motion.event[1].value = 5;
        p.sum = proj_sum(&p);
        ok &= !project_restore_runtime(&p);
        ok &= trk[1].eng_req == want_of(RET[i]) && trk[1].preset == 0u && e_is_preset0(&trk[1], want_of(RET[i])) &&
              trk[1].p[P_LEVEL] == 61 && trk[1].step[3].note[0] == 51u;
        ok &= trk[0].eng_req == 0u && trk[0].p[P_E0] == 5 && trk[2].p[P_E0 + 1] == 98;   /* (the others as stored) */
        for (k = 0; k < motion.count; k++) {
            kept += (motion.event[k].place >> 6) == 2u && MOTION_ID(&motion.event[k]) == P_LEVEL;
            gone += (motion.event[k].place >> 6) == 1u && MOTION_ID(&motion.event[k]) >= P_E0;
        }
    }
    ck("a project's track of each: its successor's first preset (E values), LEVEL and steps as stored, the others "
       "untouched", ok);
    ck("  the motion on its E values dropped, the other track's kept", kept == 5u && !gone);
}

static void user_test(void)
{
    uint32_t i, ok = 1;
    char b[16];
    for (i = 0; i < 5u; i++) {
        int rc;
        ui_power_on();
        set_engine_of(TSEL, 0);
        TSEL->p[P_E0] = 99;
        TSEL->p[P_LEVEL] = 70;
        rc = up_store(11, "OLD SOUND");
        ok &= rc == 0 || rc == 3;
        up_rec(11)->engine = (uint8_t)RET[i];          /* (a record stored by a build that had the engine) */
        ok &= up_engine(11) == want_of(RET[i]);
        up_auto_name(b, RET[i], 11);
        ok &= str_eq(b, want_of(RET[i]) == ENGI_DRUM ? "DRUM 12" : want_of(RET[i]) ? "WSHAPE 12" : "ANALOG 12");
        set_engine_of(TSEL, ENGI_FM6);
        ok &= !up_load(11) && TSEL->eng_req == want_of(RET[i]) && e_is_preset0(TSEL, want_of(RET[i])) &&
              TSEL->p[P_LEVEL] == 70;
    }
    ck("a user preset of one: listed and named as its successor, loads as its first preset (the rest as stored)", ok);
}

int main(void)
{
    offered_test();
    picked_test();
    project_test();
    user_test();
    printf(bad ? "retired: FAILED\n" : "retired: all ok\n");
    return bad != 0;
}
