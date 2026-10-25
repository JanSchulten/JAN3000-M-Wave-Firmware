/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* A new project's sounds, drawn at random (firmware/src/newproj.c, M-Vave-OP1), through the UI harness:
 *   NEW      SAVE > PROJECT's F#5 asks START A NEW PROJECT? (UNSAVED MUSIC IS LOST); OCT- keeps the music; OCT+
 *            makes a new project in no slot: T1 a factory BASS, T2 the power-on pad (FM6 PAD), T3 a LEAD, T4 a kit
 *            (one of DRUM's six, named after it: "66 KIT"; or a VDRUM kit, not its INIT); no steps on any
 *            track, no song, no FX TRACKS, no KEY, CSET off, Felucca's chorus, the globals' defaults (BPM).
 *   SLOT     OCT- on an empty slot asks NEW PROJECT IN SLOT C?; OCT+ makes the new project there (SAVE goes to it).
 *   BOOT     power-on with nothing restored (main.c: project_boot_new): felucca_init's project, the sounds drawn;
 *            boots with other stirs draw other sounds.
 *   DRAWS    900 draws from different states: every BASS and LEAD factory preset of category.c's list (no alias, no
 *            SAMPLE set this build lacks), every DRUM kit and every VDRUM kit but INIT comes up, nothing else.
 * jan_test.c checks the page's keys, LOAD? and OVERWRITE?. Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("newproj: %-99s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

static int kit_ok(const track_t *t)
{
    uint32_t i;
    if (t->eng_req != ENGI_DRUM)
        return np_fits(t->eng_req, t->preset, NP_DRUMS);
    for (i = 0; i < NELEM(NP_KITS); i++)
        if (t->p[P_E0] == NP_KITS[i])
            return 1;
    return 0;
}
/* T1 a BASS, T2 the power-on pad, T3 a LEAD, T4 a kit */
static int sounds_ok(void)
{
    return preset_cat(trk[0].eng_req, trk[0].preset) == CAT_BASS && np_fits(trk[0].eng_req, trk[0].preset, CAT_BASS) &&
           trk[1].eng_req == TRK_DEF[1][0] && trk[1].preset == TRK_DEF[1][1] &&
           preset_cat(trk[2].eng_req, trk[2].preset) == CAT_LEAD && np_fits(trk[2].eng_req, trk[2].preset, CAT_LEAD) &&
           kit_ok(&trk[3]);
}
static int empty_ok(void)
{
    uint32_t k, l, ok = chain_config.count == 0u && !keyt.on && !cset_en && chos.type == CHO_CHORUS && !chos.fdbk &&
                        song.g[G_BPM] == GP[G_BPM].def && !song.playing && motion.count == 0u;
    for (k = 0; k < NTRK; k++)
        ok &= seq_is_empty(&trk[k]);
    for (l = 0; l < FXT_N; l++)
        for (k = 0; k < NSTEP; k++)
            ok &= !fxt_lane[l].step[k];
    return (int)ok;
}
/* music worth keeping: steps, a tempo, a song row, an FX TRACK step, KEY, CSET, the chorus a flanger */
static void some_music(void)
{
    trk[0].step[0].time = ST_NOTE;
    trk[0].step[0].note[0] = 45;
    trk[0].step[0].n = 1;
    trk[2].step[4].time = ST_NOTE;
    trk[2].step[4].note[0] = 72;
    trk[2].step[4].n = 1;
    song.g[G_BPM] = 97;
    chain_config.count = 1;
    fxt_lane[0].step[2] = 3;
    keyt.on = 1;
    cset_en = 1;
    chos.type = CHO_FLANGER;
}
static void tap(uint32_t k) { key_down(k); frame(); key_up(k); frame(); }

static void test_new(void)
{
    char a[24], b[24];
    uint32_t i;
    ui_power_on();
    frames(320);
    for (i = 0; i < 4u; i++)
        memset(&proj_slot[i], 0, sizeof proj_slot[i]);
    go_title("PROJECT");
    some_music();
    frame();
    tap(SLOT_KEY_NEW);
    confirm_text(a, b);
    ck("PROJECT's F#5: START A NEW PROJECT? UNSAVED MUSIC IS LOST", ui.confirm == CF_NEW_PROJ && !ui.confirm_trk &&
       str_eq(a, "START A NEW PROJECT?") && str_eq(b, "UNSAVED MUSIC IS LOST"));
    press(B_OCTDN); frame();
    ck("  OCT-: the music kept", !ui.confirm && !seq_is_empty(&trk[0]) && song.g[G_BPM] == 97 && keyt.on);
    np_rng = 12345u;
    tap(SLOT_KEY_NEW);
    press(B_OCTUP); frames(50);
    ck("  OCT+: a new project, NEW PROJECT, in no slot", !ui.confirm && msg_is("NEW PROJECT") && proj_cur == PROJ_NO_SLOT);
    ck("  T1 a BASS, T2 the power-on pad, T3 a LEAD, T4 a kit", sounds_ok());
    ck("  nothing of the old music: no steps, song, FX TRACKS, KEY, CSET; the chorus, BPM as at power-on", empty_ok());
    {   /* a DRUM track's name is its kit's (ui.c preset_name_of: the footer, HOME TRACKS); 808 as the preset */
        char nm[16];
        set_engine_of(&trk[3], ENGI_DRUM);
        trk[3].p[P_E0] = DK_66;
        preset_name_of(&trk[3], nm, sizeof nm);
        ck("  a DRUM track named after its kit (the footer, HOME TRACKS): 66 KIT", str_eq(nm, "66 KIT"));
        trk[3].p[P_E0] = DK_80;
        preset_name_of(&trk[3], nm, sizeof nm);
        ck("  .. the 808 kit: 808 KIT, as the preset", str_eq(nm, "808 KIT"));
    }
}

static void test_slot(void)
{
    char a[24], b[24];
    uint32_t i;
    ui_power_on();
    frames(320);
    for (i = 0; i < 4u; i++)
        memset(&proj_slot[i], 0, sizeof proj_slot[i]);
    go_title("PROJECT");
    some_music();
    frame();
    tap(4);                                             /* A3: C (empty) */
    press(B_OCTDN); frame();
    ck("OCT- on an empty slot says EMPTY SLOT and preserves the music", song.g[G_SLOT] == 3 && !ui.confirm &&
       msg_is("EMPTY SLOT") && !empty_ok());
    tap(SLOT_KEY_NEW);
    confirm_text(a, b);
    ck("F#5 is the only NEW path and asks before discarding live music", ui.confirm == CF_NEW_PROJ &&
       str_eq(a, "START A NEW PROJECT?") && str_eq(b, "UNSAVED MUSIC IS LOST"));
    np_rng = 777u;
    press(B_OCTUP); frames(50);
    ck("  OCT+ starts an unsaved project, leaving the selected empty slot unwritten",
       !ui.confirm && msg_is("NEW PROJECT") && proj_cur == PROJ_NO_SLOT && !project_used(2));
    ck("  its sounds drawn, nothing of the old music", sounds_ok() && empty_ok());
}

static void test_boot(void)
{
    uint32_t i, diff = 0;
    uint8_t first[3] = {0, 0, 0};
    ui_power_on();
    project_boot_new(42u);
    ck("power-on, nothing restored: felucca_init's project with the sounds drawn", sounds_ok() && empty_ok());
    for (i = 0; i < 8u; i++) {                          /* other boots: other stirs, other sounds */
        ui_power_on();
        project_boot_new(1000u + i * 7919u);
        if (!i) {
            first[0] = (uint8_t)(trk[0].preset ^ trk[0].eng_req << 4);
            first[1] = (uint8_t)(trk[2].preset ^ trk[2].eng_req << 4);
            first[2] = (uint8_t)(trk[3].p[P_E0] ^ trk[3].eng_req << 4);
        } else {
            diff |= first[0] != (uint8_t)(trk[0].preset ^ trk[0].eng_req << 4) ||
                    first[1] != (uint8_t)(trk[2].preset ^ trk[2].eng_req << 4) ||
                    first[2] != (uint8_t)(trk[3].p[P_E0] ^ trk[3].eng_req << 4);
        }
    }
    ck("  boots with other stirs draw other sounds", diff);
}

static void test_draws(void)
{
    static uint8_t seen[3][NENGINES][32];
    uint32_t i, e, k, want = 0, got = 0, wrong = 0, kits = 0, dwant = 0, dgot = 0;
    char what[200];
    memset(seen, 0, sizeof seen);
    ui_power_on();
    for (i = 0; i < 900u; i++) {
        np_rng = i * 2654435761u + 17u;
        np_sounds();
        wrong += !sounds_ok();
        seen[0][trk[0].eng_req][trk[0].preset % 32u] = 1;
        seen[1][trk[2].eng_req][trk[2].preset % 32u] = 1;
        if (trk[3].eng_req == ENGI_DRUM)
            seen[2][ENGI_DRUM][trk[3].p[P_E0] % 32] = 1;
        else
            seen[2][trk[3].eng_req][trk[3].preset % 32u] = 1;
    }
    for (e = 0; e < NENGINES; e++)
        for (k = 0; k < ENGINES[e]->npresets && k < 32u; k++) {
            if (!eng_ok(e))
                continue;
            if (np_fits(e, k, CAT_BASS)) {
                want++;
                got += seen[0][e][k];
            }
            if (np_fits(e, k, CAT_LEAD)) {
                want++;
                got += seen[1][e][k];
            }
            if (np_fits(e, k, NP_DRUMS)) {
                dwant++;
                dgot += seen[2][e][k];
            }
        }
    for (k = 0; k < NELEM(NP_KITS); k++)
        kits += seen[2][ENGI_DRUM][NP_KITS[k]];
    snprintf(what, sizeof what, "900 draws: %u of %u BASS / LEAD presets, %u of 6 DRUM kits, %u of %u VDRUM kits (DBOX "
             "retired), %u off the lists", got, want, kits, dgot, dwant, wrong);
    ck(what, want >= 15u && got == want && kits == 6u && dwant == 7u && dgot == dwant && !wrong);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    test_new();
    test_slot();
    test_boot();
    test_draws();
    printf(bad ? "newproj: %d FAILED\n" : "newproj: all passed\n", bad);
    return bad != 0;
}
