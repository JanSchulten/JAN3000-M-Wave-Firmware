/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* WSHAPE (M-Vave-OP1, engine 17; docs/PLAITS_PLAN.md): Plaits' waveshaping model as a voice, as Behringer's Grind
 * plays it: the Plaits oscillator, then a filter, then the ADSR. The model is plaits_ws.c (MIT, Emilie Gillet: bit for
 * bit Plaits' own, in float on the chip's FPU); this file is the Felucca engine around it. PHASE's successor: a sound
 * stored with the retired PHASE (2) loads as this engine's first preset (core.h eng_successor).
 *   HARM   HARMONICS: the waveshaper, from the inverse tangent through the sine, linear and bump shapes to the double
 *          bump (Plaits morphs between neighbours)
 *   TIMB   TIMBRE: the wavefolder's gain (more folds, brighter) and the AUX output's overtones; the SHP routings move
 *          it (ENV DEST / LFO DEST SHP, the matrix's SHP: 1 step a step), as they move ANALOG's pulse width
 *   MORPH  MORPH: the slope's asymmetry, a triangle at 64 (Plaits 0.5) to a saw-like ramp either way
 *   AUX    the blend of Plaits' two outputs: 0 OUT (the folded slope), 127 AUX (the triangle's sine into the second
 *          folder): a rounder, more hollow colour
 *   CUT RES DRV KTR  the filter, as ANALOG's (dsp.c's trapezoidal SVF low-pass, its soft knee; DRV a soft clip before
 *          it; KTR the cutoff following the note, 64 one octave an octave); ENV / LFO / matrix -> FLT move CUT
 * State: the model per voice in the part's engine memory (wsh_mem_t, 56 bytes a voice: its two oscillators and the
 * ramps), the filter in v->s[0] v->s[1], a DC blocker's mean in v->s[2] (Plaits' shapes carry DC; the
 * module's output is AC-coupled). A voice from rest starts at its note (plaits_ws.c pws_start), its phase from
 * v->ph[0] (UNISON's voices: voice.c spreads them); a retrigger of a sounding voice keeps the model running (v->s[4],
 * set by note_on, is the start flag: .keep 0x10 puts back the 0 of a running voice), the filter and the blocker
 * (.keep 0x07).
 * Cost on the unit (2026-10-24, INFO's CPU): ~6.3 % of the audio's time a voice with AUX at 0 or not (the model 4.3 %:
 * the FPU probe), before AUX 0 left Plaits' second output out (pws_render need 1: OUT alike, ~40 % less model). */
#include "plaits_ws.c"

typedef struct {
    pws_t v[NVOICE];                           /* the model of each of the part's voices (voice_t index) */
} wsh_mem_t;
ENG_MEM_FITS("WSHAPE", sizeof(wsh_mem_t));

static void wsh_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->s[0] = v->s[1] = 0;                            /* filter */
    v->s[2] = 0;                                      /* the DC blocker's mean */
    v->s[4] = 1;                                      /* start the model at the first render */
}

/* the model s's n samples of voice v as Q15 into wsh_o (OUT and AUX blended by AUX); its float side, on the FPU */
static int32_t wsh_o[CTL];
FM1_FPU __attribute__((noinline)) static void wsh_model(const track_t *t, voice_t *v, pws_t *s, uint32_t n,
                                                        const vmod_t *m)
{
    static float out[CTL], aux[CTL];
    const float *b;
    const int16_t *p = t->p;
    const float f0 = (float)m->inc * (1.0f / 4294967296.0f);
    const float harm = (float)p[P_E0] * (1.0f / 127.0f), morph = (float)p[P_E2] * (1.0f / 127.0f);
    const float blend = (float)p[P_E3] * (1.0f / 127.0f);
    int32_t tq = (p[P_E1] << 8) + m->shape - (64 << 8);   /* TIMB and the SHP routings, 0..127 << 8 */
    float timb;
    uint32_t i;
    tq = tq < 0 ? 0 : tq > (127 << 8) ? (127 << 8) : tq;
    timb = (float)tq * (1.0f / (127.0f * 256.0f));
    if (v->s[4]) {
        pws_start(s, (float)(v->ph[0] >> 8) * (1.0f / 16777216.0f), f0, harm, timb, morph);
        v->s[4] = 0;
    }
    /* AUX 0: OUT alone (Plaits' second output not computed, b reads OUT: x is OUT exactly); 127: the first fold not */
    pws_render(s, f0, harm, timb, morph, out, aux, n, !p[P_E3] ? 1u : p[P_E3] >= 127 ? 2u : 3u);
    b = p[P_E3] ? aux : out;
    for (i = 0; i < n; i++) {
        float x = out[i] + (b[i] - out[i]) * blend;
        x = x > 1.0f ? 1.0f : x < -1.0f ? -1.0f : x;
        wsh_o[i] = (int32_t)(x * 32767.0f);
    }
}

/* the voice: the model, the drive, the filter and its knee (ANALOG's), the amplitude; integer, as every engine */
static void wsh_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    int32_t drive = 32768 + p[P_E6] * 512, drv = p[P_E6];
    int32_t ic1 = v->s[0], ic2 = v->s[1], dc = v->s[2];
    tsvf_t flt;
    uint32_t i;
    if (n > CTL)
        n = CTL;
    wsh_model(t, v, &((wsh_mem_t *)ENG_MEM((uint32_t)(t - trk)))->v[(uint32_t)(v - t->v) % NVOICE], n, m);
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t s = wsh_o[i];
        if (drv)
            s = softclip(((s >> 2) * (drive >> 2)) >> 11);   /* (ANALOG's: up to 3x, no overflow) */
        s = soft_knee(tsvf_lp(&flt, s >> 1, &ic1, &ic2), 16000) << 1;
        dc += ((s << 8) - dc) >> 9;                   /* the mean (about 14 Hz), Q8: the bump shapes, the skewed */
        s -= dc >> 8;                                 /* slopes, their clipped tops and a saturated resonance */
        out[i] += voice_amp(s, m, i) << 1;
    }
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = dc;
}

/* {HARM TIMB MORPH AUX, CUT RES DRV KTR} */
static const preset_t WSHAPE_PRESETS[] = {
    {"SOFT FOLD", {64, 30, 64, 0, 110, 10, 0, 64}, {2, 70, 100, 40}, 0, 0, PAT(3)},
    {"FOLD BASS", {40, 70, 30, 0, 70, 40, 20, 64}, {0, 60, 60, 30}, 30, 1, PAT(2)},
    {"GLASS LEAD", {90, 50, 80, 20, 100, 20, 0, 64}, {4, 70, 100, 50}, 15, 1, PAT(4)},
    {"WARM PAD", {64, 20, 64, 60, 75, 10, 0, 32}, {80, 90, 110, 95}, 10, 0, PAT(5)},
    {"BUZZ SAW", {20, 90, 110, 0, 95, 30, 30, 64}, {2, 70, 90, 40}, 20, 1, PAT(4)},
    {"SOFT KEYS", {70, 15, 50, 40, 90, 0, 0, 48}, {2, 80, 30, 70}, 0, 0, PAT(6)},
    {"METAL PLUCK", {110, 100, 20, 30, 100, 20, 0, 64}, {0, 85, 0, 60}, 40, 0, PAT(3)},
    {"SWEEP", {50, 60, 64, 10, 60, 60, 10, 64}, {30, 80, 100, 70}, 50, 0, PAT(5)},
};

static const engine_t ENG_WSHAPE = {
    .name = "WSHAPE",
    .page_title = {"SHAPE", "FLT"},
    .edit = {
        {"HARM", F_PCT, 0, 127, 64, 0, 0},
        {"TIMB", F_PCT, 0, 127, 30, 0, 0},
        {"MORPH", F_PCT, 0, 127, 64, 0, 0},
        {"AUX", F_PCT, 0, 127, 0, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 110, 0, 0},
        {"RES", F_PCT, 0, 127, 10, 0, 0},
        {"DRV", F_PCT, 0, 127, 0, 0, 0},
        {"KTR", F_PCT, 0, 127, 64, 0, 0},
    },
    .presets = WSHAPE_PRESETS,
    .npresets = NELEM(WSHAPE_PRESETS),
    .note_on = wsh_note_on,
    .render = wsh_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(wsh_mem_t),
};
