/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* HRMNC (M-Vave-OP1, engine 18; docs/PLAITS_PLAN.md): Plaits' additive model as a voice, as Behringer's Grind plays
 * it: the model, then a filter, then the ADSR. The model is plaits_add.c (MIT, Emilie Gillet: bit for bit Plaits' own,
 * in float on the chip's FPU); the values, the filter and a voice's state are those of every Plaits engine
 * (plaits_voice.c). TRIO's successor: a sound stored with the retired TRIO (6) loads as this engine's first preset
 * (core.h eng_successor).
 *   HARM   HARMONICS: bumps beside the spectrum's peak (and a narrower peak): from one lobe to a comb of them
 *   TIMB   TIMBRE: where the peak sits, from the fundamental (0) to partial 24 (127); the SHP routings move it
 *   MORPH  MORPH: the peak's width, from a wide spread (0: bright, saw-like) to one partial (127: a sine)
 *   AUX    the blend into Plaits' AUX: the same spectrum on an organ's partials (1 2 3 4 6 8 10 12: drawbars)
 * The spectrum is smoothed once a block (HAR_LP: Plaits' 0.001 a 12-sample block at 48 kHz, held to the same time at
 * 32 samples at 44.1 kHz). A voice from rest starts at its note, the partials fading in over its first block; a
 * retrigger of a sounding voice keeps the model running. State: the model per voice in the part's engine memory
 * (har_mem_t, 296 bytes a voice: the phase, the partials' amplitudes and the spectra). */
#include "plaits_add.c"

typedef struct {
    pad_t v[NVOICE];                           /* the model of each of the part's voices (voice_t index) */
} har_mem_t;
ENG_MEM_FITS("HRMNC", sizeof(har_mem_t));

#define HAR_LP 0.0029f                         /* 1 - 0.999 ^ ((48000 / 12) / (44100 / 32)) */

/* the model s's n samples of voice v into plv_o; its float side, on the FPU */
FM1_FPU __attribute__((noinline)) static void har_model(const track_t *t, voice_t *v, pad_t *s, uint32_t n,
                                                        const vmod_t *m)
{
    plv_par_t q;
    plv_buf_t *w = PLV(t);
    plv_params(t, m, &q);
    if (v->s[4]) {
        pad_start(s, plv_phase(v), q.f0);
        v->s[4] = 0;
    }
    pad_update(s, q.harm, q.timb, q.morph, HAR_LP);
    pad_render(s, q.f0, w->out, w->aux, n, q.need);
    plv_blend(&q, v, n, 0.8f, 0.8f, w);                   /* (Plaits' levels: voice.cc) */
}

static void har_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    if (n > CTL)
        n = CTL;
    har_model(t, v, &((har_mem_t *)ENG_MEM((uint32_t)(t - trk)))->v[(uint32_t)(v - t->v) % NVOICE], n, m);
    plv_post(t, v, out, n, m);
}

/* {HARM TIMB MORPH AUX, CUT RES DRV KTR} */
static const preset_t HARMON_PRESETS[] = {
    {"DRAWBARS", {0, 15, 30, 127, 120, 0, 0, 64}, {0, 80, 127, 30}, 0, 0, PAT(6)},
    {"AIR PAD", {20, 40, 40, 30, 90, 10, 0, 32}, {80, 90, 110, 95}, 10, 0, PAT(5)},
    {"BRIGHT LEAD", {0, 50, 10, 0, 110, 20, 50, 64}, {4, 70, 100, 50}, 15, 1, PAT(4)},
    {"SUB BASS", {0, 0, 100, 0, 70, 30, 30, 64}, {0, 60, 80, 30}, 20, 1, PAT(2)},
    {"GLASS BELL", {100, 90, 60, 0, 127, 0, 0, 64}, {0, 85, 0, 70}, 0, 0, PAT(7)},
    {"VOWEL", {40, 30, 70, 20, 100, 30, 0, 64}, {10, 70, 100, 50}, 0, 1, PAT(4)},
    {"COMB KEYS", {80, 30, 50, 40, 95, 10, 0, 48}, {2, 80, 30, 70}, 0, 0, PAT(6)},
    {"SPECTRAL", {60, 64, 60, 0, 100, 40, 0, 64}, {40, 80, 100, 70}, 40, 0, PAT(5)},
};

static const engine_t ENG_HARMON = {
    .name = "HRMNC",
    .page_title = {"SPECT", "FLT"},
    .edit = PLV_EDIT(0, 15, 30),
    .presets = HARMON_PRESETS,
    .npresets = NELEM(HARMON_PRESETS),
    .note_on = plv_note_on,
    .render = har_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(har_mem_t),
};
