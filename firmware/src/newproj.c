/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: a new project's sounds, drawn at random (included by project.c before project_new). T1 a bass, T3 a lead,
 * T4 a drum kit; T2 keeps its pad (TRK_DEF's FM6 PAD):
 *   bass, lead  a factory preset of category BASS or LEAD (category.c, any engine; never an alias, never a SAMPLE set
 *               this build lacks: it would play a sine);
 *   drums       a kit: one of DRUM's six (STD 808 10 66 55 77; eng_drum.c's others are aliases), or a factory kit of
 *               VDRUM (its INIT, the plain kit to start an edit from, left out). Not the single drums
 *               (PHYS HAND DRUM, TOMS, ANALOG2's NOISE HAT).
 * Where: project_new (SAVE > PROJECT: F#5 NEW, OCT- on an empty slot), and power-on with nothing restored
 * (RESTORE LAST OFF, the first start, after a crash: main.c, web/emu: project_boot_new).
 * Random numbers: xorshift32, its state in .noinit (after power-on the SRAM's own pattern, after a reset where it
 * was), stirred with the TIMER4 ticks of each draw (when the key went down) and at power-on with the boot count and
 * the battery's ADC (np_stir). Main loop only: the sounds load as the PRESETS knob loads them (ui.c set_engine_of,
 * apply_preset_to), with no undo copy */
static uint32_t np_rng __attribute__((section(".noinit")));
static void np_stir(uint32_t x) { np_rng = (np_rng ^ x) * 2654435761u + 0x9E3779B9u; }
static uint32_t np_rand(uint32_t n)                    /* 0 .. n - 1 */
{
    uint32_t s = np_rng ^ fm1_ticks();
    if (!s)
        s = 0x2468ACEu;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    np_rng = s;
    return n ? s % n : 0u;
}

static const uint8_t NP_KITS[6] = {DK_STD, DK_80, DK_10, DK_66, DK_55, DK_77};
#define NP_DRUMS 0xFFu                                 /* np_pick: the drum kits (not a category) */

/* factory preset k of engine e can be drawn as cat (NP_DRUMS: a VDRUM kit, not its INIT): its category, not an
 * alias (SAMPLE's retired TRANH), its samples there */
static int np_fits(uint32_t e, uint32_t k, uint32_t cat)
{
    const engine_t *en = ENGINES[e];
    if (cat == NP_DRUMS)
        return e == ENGI_VDRUM && preset_cat(e, k) == CAT_DRUM && !str_eq(en->presets[k].name, "INIT");
    if (preset_cat(e, k) != cat || preset_orig(en, k) != k)
        return 0;
    return en != &ENG_SAMPLE || !smp_set_missing((uint32_t)en->presets[k].e[0] % SMP_NALL);
}
/* a factory sound of cat at random: its engine and preset (NP_DRUMS: DRUM's kits come first, *pre the kit, as if
 * DRUM had a preset each); 0 none (each has several) */
static int np_pick(uint32_t cat, uint32_t *eng, uint32_t *pre)
{
    uint32_t pass, n = 0, want = 0, e, k, nk = cat == NP_DRUMS ? NELEM(NP_KITS) : 0u;
    for (pass = 0; pass < 2u; pass++) {                 /* count them, then take the one drawn */
        if (pass) {
            if (!n)
                return 0;
            want = np_rand(n);
            if (want < nk) {
                *eng = ENGI_DRUM;
                *pre = NP_KITS[want];
                return 1;
            }
        }
        n = nk;
        for (e = 0; e < NENGINES; e++) {
            if (!eng_ok(e))
                continue;
            for (k = 0; k < ENGINES[e]->npresets; k++) {
                if (!np_fits(e, k, cat))
                    continue;
                if (pass && n == want) {
                    *eng = e;
                    *pre = k;
                    return 1;
                }
                n++;
            }
        }
    }
    return 0;
}

/* T1 a bass, T3 a lead, T4 a drum kit, drawn */
static void np_sounds(void)
{
    static const uint8_t TR[3] = {0, 2, 3}, CAT[3] = {CAT_BASS, CAT_LEAD, NP_DRUMS};
    uint32_t j, e, k;
    undo_depth++;                                       /* (a new project: no undo copy of its sounds) */
    for (j = 0; j < 3u; j++) {
        track_t *t = &trk[TR[j]];
        if (!np_pick(CAT[j], &e, &k))
            continue;
        set_engine_of(t, e);
        if (e == ENGI_DRUM)                             /* (its preset, 808 KIT; the kit drawn: a halfword the ISR */
            t->p[P_E0] = (int16_t)k;                    /* reads whole) */
        else
            apply_preset_to(t, k);
    }
    undo_depth--;
}

/* power-on, nothing restored (main.c, web/emu): felucca_init's project, its sounds drawn; stir: what varies from boot
 * to boot (the ticks of a boot vary little) */
static void project_boot_new(uint32_t stir)
{
    np_stir(stir);
    np_sounds();
}
