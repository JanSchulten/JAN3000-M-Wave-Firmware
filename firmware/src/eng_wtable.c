/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* WTABLE (M-Vave-OP1, engine 20; docs/PLAITS_PLAN.md): Plaits' wavetable model as a voice, as Behringer's Grind plays
 * it: the model, then a filter, then the ADSR. The model is plaits_wt.c (MIT, Emilie Gillet: bit for bit Plaits' own
 * with its 16-bit tables, in float on the chip's FPU); the firmware's tables are 8 bits (plaits_wt8.h: 25 KB of flash
 * instead of 50; playing, 49 dB from Plaits' own), each wave decoded into the part's cache when a voice reads it. The
 * values, the filter and a voice's state are those of every Plaits engine (plaits_voice.c). VOICE's successor: a sound
 * stored with the retired VOICE (5) loads as this engine's first preset (core.h eng_successor).
 *   HARM   HARMONICS: the bank (four of 8 x 8 waves, the fourth Plaits' shuffle of the others); from its middle up the
 *          banks come back in reverse and the moves between waves turn to steps
 *   TIMB   TIMBRE: the row of the bank; the SHP routings move it
 *   MORPH  MORPH: the column
 *   AUX    the blend into Plaits' AUX: the same wave at 5 bits
 * The parameters glide to a new wave as Plaits' (smoothed a block at a time, held to Plaits' time at our blocks: WTB_CXY,
 * WTB_CZ; then every sample). A voice from rest starts at its note and on its wave; a retrigger of a sounding voice
 * keeps the model running. State: the model per voice and the cache of decoded waves (wtb_mem_t, the part's engine
 * memory; the cache's clock ticks once a block, engine_t.post). */
#include "plaits_wt.c"

typedef struct {
    pwt_t v[NVOICE];                           /* the model of each of the part's voices (voice_t index) */
    pwc_t c;                                   /* the part's decoded waves */
} wtb_mem_t;
ENG_MEM_FITS("WTABLE", sizeof(wtb_mem_t));

/* Plaits smooths x y z by 0.2 and 0.05 a 12-sample block at 47 872 Hz: 1 - (1 - k) ^ (3989 / 1378) at our blocks */
#define WTB_CXY 0.4758f
#define WTB_CZ 0.1380f

/* the model s's n samples of voice v into plv_o; its float side, on the FPU */
FM1_FPU __attribute__((noinline)) static void wtb_model(const track_t *t, voice_t *v, pwt_t *s, pwc_t *c, uint32_t n,
                                                        const vmod_t *m)
{
    plv_par_t q;
    pwt_par_t w;
    plv_buf_t *pb = PLV(t);
    plv_params(t, m, &q);
    w.timbre = q.timb;
    w.morph = q.morph;
    w.harmonics = q.harm;
    w.f0 = q.f0;
    w.cxy = WTB_CXY;
    w.cz = WTB_CZ;
    if (v->s[4]) {
        pwt_start(s, c, plv_phase(v), &w);
        v->s[4] = 0;
    }
    pwt_render(s, c, &w, pb->out, pb->aux, n, q.need);
    plv_blend(&q, v, n, 0.6f, 0.6f, pb);                   /* (Plaits' levels: voice.cc) */
}

static void wtb_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    wtb_mem_t *M = (wtb_mem_t *)ENG_MEM((uint32_t)(t - trk));
    if (n > CTL)
        n = CTL;
    wtb_model(t, v, &M->v[(uint32_t)(v - t->v) % NVOICE], &M->c, n, m);
    plv_post(t, v, out, n, m);
}

/* after the part's voices: the cache's clock (a wave read this block is not the stalest) */
static int wtb_post(track_t *t, int32_t *out, uint32_t n, int32_t fade)
{
    const uint32_t part = (uint32_t)(t - trk);
    (void)out;
    (void)n;
    (void)fade;
    if (part < NPART)
        ((wtb_mem_t *)ENG_MEM(part))->c.now++;
    return 0;
}

/* {HARM TIMB MORPH AUX, CUT RES DRV KTR} */
static const preset_t WTABLE_PRESETS[] = {
    {"WAVE SCAN", {10, 40, 40, 0, 110, 10, 0, 64}, {10, 80, 100, 70}, 0, 0, PAT(5)},
    {"DIGI BASS", {30, 20, 90, 0, 70, 40, 20, 64}, {0, 60, 70, 30}, 20, 1, PAT(2)},
    {"GLASS KEYS", {50, 80, 20, 0, 100, 10, 0, 48}, {2, 80, 30, 70}, 0, 0, PAT(6)},
    {"SCAN PAD", {20, 64, 64, 0, 90, 10, 0, 32}, {80, 90, 110, 95}, 10, 0, PAT(5)},
    {"BIT LEAD", {40, 60, 100, 127, 110, 20, 10, 64}, {4, 70, 100, 50}, 15, 1, PAT(4)},
    {"STEP TABLE", {100, 50, 50, 0, 110, 10, 0, 64}, {2, 70, 100, 40}, 0, 0, PAT(3)},
    {"HOLLOW", {70, 30, 110, 30, 100, 0, 0, 64}, {10, 80, 90, 60}, 0, 0, PAT(6)},
    {"CHAOS ROW", {120, 90, 30, 0, 110, 20, 0, 64}, {2, 70, 100, 40}, 20, 1, PAT(4)},
};

static const engine_t ENG_WTABLE = {
    .name = "WTABLE",
    .page_title = {"TABLE", "FLT"},
    .edit = PLV_EDIT(10, 40, 40),
    .presets = WTABLE_PRESETS,
    .npresets = NELEM(WTABLE_PRESETS),
    .note_on = plv_note_on,
    .render = wtb_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(wtb_mem_t),
    .post = wtb_post,
};
