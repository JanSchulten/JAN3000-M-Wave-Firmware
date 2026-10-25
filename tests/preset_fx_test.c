/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* A factory preset brings the sound, not the effects (M-Vave-OP1; firmware/src/ui.c preset_fx, preset_load_to),
 * through the UI harness (tests/ui_test.c):
 *   FACTORY  every factory preset of every engine, loaded as the PRESETS list loads it (apply_preset_to), keeps the
 *            track's sends (DIST CHORUS DELAY REVERB) and its INSERT (TYPE A B C MIX) as they were, and still sets
 *            its sound (the EDIT values, the envelope) and turns the matrix off as before; so do the PRESETS knob,
 *            an engine switch (set_engine_of) and a MIDI Program Change (midi_pc_load).
 *   USER     a user preset brings the sends and the INSERT it was saved with (upreset.c up_load).
 *   POWER-ON the power-on sounds load with their sends as ever (preset_load_to with sends: main.c, project.c's
 *            format-1 tracks): every track's sends are its preset's (or 0 24 28 36), the INSERT OFF.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("preset fx: %-90s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

/* the track's effects set to values no preset or default has */
static const int16_t FX_MINE[4] = {11, 22, 33, 44};
static const int16_t INS_MINE[5] = {2, 17, 29, 41, 90};
static void fx_mine(track_t *t)
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        t->p[P_DIST + i] = FX_MINE[i];
    for (i = 0; i < 5u; i++)
        t->p[P_ITYPE + i] = INS_MINE[i];
}
static int fx_is_mine(const track_t *t)
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        if (t->p[P_DIST + i] != FX_MINE[i])
            return 0;
    for (i = 0; i < 5u; i++)
        if (t->p[P_ITYPE + i] != INS_MINE[i])
            return 0;
    return 1;
}
static int preset_fx_set(const preset_t *pr)        /* the preset names sends of its own */
{
    return pr->fx[0] || pr->fx[1] || pr->fx[2] || pr->fx[3];
}

static void factory_test(void)
{
    uint32_t e, k, i, loads = 0, kept = 0, sound = 0, own = 0, matrix = 0;
    ui_power_on();
    for (e = 0; e < NENGINES; e++) {
        const engine_t *en = ENGINES[e];
        if (!eng_ok(e) || !en->npresets)
            continue;
        set_engine_of(TSEL, e);
        for (k = 0; k < en->npresets; k++) {
            uint32_t pi = preset_orig(en, k), ok = 1;
            if (e == ENGI_SAMPLE && k == SMP_SET_PERC) {
                set_engine_of(TSEL, e);              /* (PERC loads as DRUM: back to SAMPLE) */
                continue;
            }
            fx_mine(TSEL);
            TSEL->p[P_M1SRC] = 1;                    /* a matrix slot on: a factory preset turns it off */
            apply_preset_to(TSEL, k);
            loads++;
            kept += fx_is_mine(TSEL);
            own += preset_fx_set(&en->presets[pi]);
            for (i = 0; i < 8u; i++)
                ok &= e == ENGI_FM6 && i == 7u ? 1 : TSEL->p[P_E0 + i] == en->presets[pi].e[i];   /* (FM6 SLOT: the patch) */
            ok &= TSEL->p[P_ATK] == en->presets[pi].env[0] && TSEL->p[P_REL] == en->presets[pi].env[3];
            sound += ok;
            matrix += TSEL->p[P_M1SRC] == 0;
            if (TSEL->eng_req != e)
                set_engine_of(TSEL, e);
        }
    }
    printf("preset fx: %u factory loads, %u of them presets with sends of their own\n", (unsigned)loads, (unsigned)own);
    ck("every factory preset keeps the track's sends and INSERT", loads > 50u && kept == loads);
    ck("a preset with sends of its own among them (the test reaches the old path)", own > 0u);
    ck("every factory preset still sets its sound (EDIT values, envelope)", sound == loads);
    ck("every factory preset still turns the matrix off", matrix == loads);

    fx_mine(TSEL);
    set_engine_of(TSEL, ENGI_FM6);
    ck("an engine switch keeps them", fx_is_mine(TSEL) && TSEL->eng_req == ENGI_FM6);
    set_engine_of(TSEL, 0);
    fx_mine(TSEL);
    midi_pc_load(TSEL, 2);
    ck("a MIDI Program Change keeps them", fx_is_mine(TSEL) && TSEL->preset == preset_orig(ENGINES[0], 2));
    go_home();
    frames(100);
    fx_mine(TSEL);
    turn(EN_PRESET, 1);
    frames(100);
    turn(EN_PRESET, 1);
    frames(100);
    ck("the PRESETS knob keeps them", fx_is_mine(TSEL));
}

static void user_test(void)
{
    int rc;
    uint32_t i;
    ui_power_on();
    set_engine_of(TSEL, 0);
    apply_preset_to(TSEL, 3);
    fx_mine(TSEL);
    rc = up_store(9, "WETONE");
    ck("a user preset saved with the track's effects", rc == 0 || rc == 3);
    for (i = 0; i < 4u; i++)
        TSEL->p[P_DIST + i] = 0;
    for (i = 0; i < 5u; i++)
        TSEL->p[P_ITYPE + i] = TP[P_ITYPE + i].def;
    apply_preset_to(TSEL, 0);
    ck("it loads with them (sends and INSERT as saved)", !up_load(9) && fx_is_mine(TSEL));
}

static void power_on_test(void)
{
    static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
    uint32_t i, k, ok = 1;
    ui_power_on();
    for (k = 0; k < NTRK; k++) {
        const engine_t *en = ENGINES[TRK_DEF[k][0]];
        const preset_t *pr = &en->presets[preset_orig(en, TRK_DEF[k][1])];
        for (i = 0; i < 4u; i++)
            ok &= trk[k].p[P_DIST + i] == (pr->fx[i] ? pr->fx[i] - 1 : FX_DEF[i]);
        ok &= trk[k].p[P_ITYPE] == TP[P_ITYPE].def;
    }
    ck("the power-on sounds load with their sends, the INSERT off (as ever)", ok);
}

int main(void)
{
    factory_test();
    user_test();
    power_on_test();
    printf(bad ? "preset fx: FAILED\n" : "preset fx: all ok\n");
    return bad != 0;
}
