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
 *   CUT RES DRV KTR  the filter; these values, the filter and a voice's state are every Plaits engine's
 *          (plaits_voice.c)
 * State: the model per voice in the part's engine memory (wsh_mem_t, 56 bytes a voice: its two oscillators and the
 * ramps). A voice from rest starts at its note (plaits_ws.c pws_start); a retrigger of a sounding voice keeps the
 * model running.
 * Cost on the unit (2026-10-24, INFO's CPU): ~6.3 % of the audio's time a voice with AUX at 0 or not (the model 4.3 %:
 * the FPU probe), before AUX 0 left Plaits' second output out (pws_render need 1: OUT alike, ~40 % less model). */
#include "plaits_ws.c"
#include "plaits_voice.c"

typedef struct {
    pws_t v[NVOICE];                           /* the model of each of the part's voices (voice_t index) */
} wsh_mem_t;
ENG_MEM_FITS("WSHAPE", sizeof(wsh_mem_t));

/* the model s's n samples of voice v into plv_o; its float side, on the FPU */
FM1_FPU __attribute__((noinline)) static void wsh_model(const track_t *t, voice_t *v, pws_t *s, uint32_t n,
                                                        const vmod_t *m)
{
    plv_par_t q;
    plv_params(t, m, &q);
    if (v->s[4]) {
        pws_start(s, plv_phase(v), q.f0, q.harm, q.timb, q.morph);
        v->s[4] = 0;
    }
    pws_render(s, q.f0, q.harm, q.timb, q.morph, plv_out, plv_aux, n, q.need);
    plv_blend(&q, n);
}

static void wsh_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    if (n > CTL)
        n = CTL;
    wsh_model(t, v, &((wsh_mem_t *)ENG_MEM((uint32_t)(t - trk)))->v[(uint32_t)(v - t->v) % NVOICE], n, m);
    plv_post(t, v, out, n, m);
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
    .edit = PLV_EDIT(64, 30, 64),
    .presets = WSHAPE_PRESETS,
    .npresets = NELEM(WSHAPE_PRESETS),
    .note_on = plv_note_on,
    .render = wsh_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(wsh_mem_t),
};
