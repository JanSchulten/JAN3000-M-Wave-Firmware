/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SWARM (M-Vave-OP1, engine 21; docs/PLAITS_PLAN.md): Plaits' swarm model as a voice, as Behringer's Grind plays it: the
 * model, then a filter, then the ADSR. The model is plaits_swarm.c (MIT, Emilie Gillet: bit for bit Plaits' own, in
 * float on the chip's FPU); the values, the filter and a voice's state are those of every Plaits engine
 * (plaits_voice.c). SLICE's successor: a sound stored with the retired SLICE (13) loads as this engine's first preset
 * (core.h eng_successor).
 *   HARM   HARMONICS: the spread of the 8 oscillators around the note (0 all on it, 127 far apart)
 *   TIMB   TIMBRE: the grains' rate, from a slow drift to a buzz; the SHP routings move it
 *   MORPH  MORPH: the grains' size: low, short grains each at its pitch; high, the oscillators held, gliding at random
 *   AUX    the blend into Plaits' AUX: the same swarm of sines
 * A note starts a burst (Plaits with its trigger patched, as the Grind's keys): the swarm restarts fast, then settles
 * slower. A voice from rest starts quiet; a retrigger of a sounding voice keeps it running and bursts again. The random
 * numbers: one generator a part (in its memory). State: the 8 oscillators of each voice (swm_mem_t, the part's engine
 * memory). */
#include "plaits_swarm.c"

typedef struct {
    psw_t v[NVOICE];                           /* the swarm of each of the part's voices (voice_t index) */
    uint32_t rng;                              /* the part's random numbers (stmlib Random) */
} swm_mem_t;
ENG_MEM_FITS("SWARM", sizeof(swm_mem_t));

static void swm_note_on(track_t *t, voice_t *v)
{
    plv_note_on(t, v);
    v->s[5] = 1;                                      /* a burst at the note's first block */
}

/* the model s's n samples of voice v into plv_o; its float side, on the FPU */
FM1_FPU __attribute__((noinline)) static void swm_model(const track_t *t, voice_t *v, psw_t *s, uint32_t *rng,
                                                        uint32_t n, const vmod_t *m)
{
    plv_par_t q;
    float density, spread, size_ratio;
    plv_params(t, m, &q);
    if (v->s[4]) {
        psw_init(s);
        v->s[4] = 0;
    }
    psw_params(q.timb, q.morph, q.harm, n, PRT_RATE, &density, &spread, &size_ratio);
    psw_render(s, rng, q.f0, density, spread, size_ratio, 1, v->s[5] != 0, plv_out, plv_aux, n, q.need);
    v->s[5] = 0;
    plv_blend(&q, v, n, -3.0f, 1.0f);                  /* (Plaits' levels: voice.cc) */
}

static void swm_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    swm_mem_t *M = (swm_mem_t *)ENG_MEM((uint32_t)(t - trk));
    if (n > CTL)
        n = CTL;
    swm_model(t, v, &M->v[(uint32_t)(v - t->v) % NVOICE], &M->rng, n, m);
    plv_post(t, v, out, n, m);
}

/* {HARM TIMB MORPH AUX, CUT RES DRV KTR} */
static const preset_t SWARM_PRESETS[] = {
    {"SAW CLOUD", {40, 60, 80, 0, 110, 10, 0, 64}, {20, 80, 110, 80}, 10, 0, PAT(5)},
    {"SUPERSAW", {70, 30, 110, 0, 110, 10, 0, 64}, {2, 70, 110, 50}, 15, 1, PAT(4)},
    {"GRAIN BUZZ", {50, 90, 20, 0, 100, 20, 0, 64}, {2, 70, 100, 40}, 0, 0, PAT(3)},
    {"DRONE", {30, 20, 100, 60, 90, 10, 0, 32}, {80, 90, 120, 100}, 0, 0, PAT(5)},
    {"SINE SWARM", {60, 50, 90, 127, 120, 0, 0, 64}, {30, 80, 110, 80}, 0, 0, PAT(5)},
    {"BURST PLUCK", {80, 100, 40, 0, 100, 20, 0, 64}, {0, 75, 0, 50}, 30, 0, PAT(7)},
    {"BEE HIVE", {100, 110, 60, 0, 90, 30, 10, 64}, {10, 80, 110, 70}, 0, 0, PAT(5)},
    {"CHORUS BASS", {20, 30, 120, 0, 70, 30, 20, 64}, {0, 60, 80, 30}, 20, 1, PAT(2)},
};

static const engine_t ENG_SWARM = {
    .name = "SWARM",
    .page_title = {"SWARM", "FLT"},
    .edit = PLV_EDIT(40, 60, 80),
    .presets = SWARM_PRESETS,
    .npresets = NELEM(SWARM_PRESETS),
    .note_on = swm_note_on,
    .render = swm_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(swm_mem_t),
};
