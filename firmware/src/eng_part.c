/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* PRTCL (M-Vave-OP1, engine 19; docs/PLAITS_PLAN.md): Plaits' particle model as a voice, as Behringer's Grind plays it:
 * the model, then a filter, then the ADSR; Plaits' diffuser once for the part, after its voices (engine_t.post). The
 * model is plaits_part.c (MIT, Emilie Gillet: bit for bit Plaits' own, in float on the chip's FPU, the diffuser's
 * delays at 7/10 to fit the part's memory); the values, the filter and a voice's state are those of every Plaits engine
 * (plaits_voice.c).
 *   HARM   HARMONICS: how far each particle's band-pass is detuned from the note at random (0: all at the note, 127:
 *          +-4 octaves)
 *   TIMB   TIMBRE: the density, from a sparse crackle to a dense hiss; the SHP routings move it
 *   MORPH  MORPH: below 64 the diffuser (a smear of short echoes, the most at 0), above 64 the band-passes' resonance
 *          (ringing pings, the most at 127); 64 neither
 *   AUX    the blend into Plaits' AUX: the raw impulses, unfiltered (dust)
 * A note fires every particle at the first sample of its second block (Plaits' trigger; the first block's amplitude
 * ramps up from 0), a voice from rest starts with quiet filters; a
 * retrigger of a sounding voice keeps them ringing. The random numbers: one generator a part (in its memory: a part
 * from rest plays its notes alike). State: the particles per voice and the diffuser (prt_mem_t, in the part's engine
 * memory). */
#include "plaits_part.c"

typedef struct {
    ppa_t v[NVOICE];                           /* the particles of each of the part's voices (voice_t index) */
    uint32_t rng;                              /* the part's random numbers (stmlib Random) */
    uint8_t on;                                /* the diffuser set up */
    uint8_t live;                              /* .. and still sounding */
    pdf_t d;                                   /* the part's diffuser */
} prt_mem_t;
ENG_MEM_FITS("PRTCL", sizeof(prt_mem_t));

#define PRT_RATE (47872.34f / 44100.0f)        /* the module's sample rate over ours: the particles a second */

static void prt_note_on(track_t *t, voice_t *v)
{
    plv_note_on(t, v);
    v->s[5] = 2;                                      /* fire the particles at the note's second block (its first, the
                                                       * amplitude's ramp from 0, would hide the impulse) */
}

/* the model s's n samples of voice v into plv_o; its float side, on the FPU */
FM1_FPU __attribute__((noinline)) static void prt_model(const track_t *t, voice_t *v, ppa_t *s, uint32_t *rng,
                                                        uint32_t n, const vmod_t *m)
{
    plv_par_t q;
    ppa_par_t pq;
    plv_buf_t *w = PLV(t);
    plv_params(t, m, &q);
    if (v->s[4]) {
        ppa_init(s);
        v->s[4] = 0;
    }
    ppa_params(&pq, q.timb, q.morph, q.harm, PRT_RATE);
    ppa_render(s, rng, v->s[5] == 1, &pq, q.f0, w->out, w->aux, n, q.need);
    if (v->s[5])
        v->s[5]--;
    plv_blend(&q, v, n, -2.0f, 1.0f, w);                   /* (Plaits' levels: voice.cc) */
}

static void prt_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    prt_mem_t *M = (prt_mem_t *)ENG_MEM((uint32_t)(t - trk));
    if (n > CTL)
        n = CTL;
    prt_model(t, v, &M->v[(uint32_t)(v - t->v) % NVOICE], &M->rng, n, m);
    plv_post(t, v, out, n, m);
}

/* the part after its voices: the diffuser (MORPH below 64), scaled by an engine switch's fade; 1 while its tail sounds
 * with no voice. The part's sum to float at 1 / 65536 (a voice's peak about 0.7: the delay memory holds +-8) */
FM1_FPU __attribute__((noinline)) static int prt_post(track_t *t, int32_t *out, uint32_t n, int32_t fade)
{
    float io[CTL];                                 /* (on the stack: each core its own, docs/DUALCORE_PLAN.md) */
    const uint32_t part = (uint32_t)(t - trk);
    prt_mem_t *M;
    const float mo = (float)t->p[P_E2] * (1.0f / 127.0f), rd = 2.0f * pws_abs(mo - 0.5f);
    const float diff = mo < 0.5f ? rd * rd : 0.0f;
    float amount = 0.8f * diff * diff * ((float)fade * (1.0f / 32767.0f));
    int32_t peak = 0;
    uint32_t i, any = 0;
    if (part >= NPART)
        return 0;
    M = (prt_mem_t *)ENG_MEM(part);
    if (n > CTL)
        n = CTL;
    if (!M->on) {
        pdf_init(&M->d, 44100.0f);
        M->on = 1;
    }
    if (amount <= 0.0f) {                          /* (no diffusion: the dry part, as Plaits' amount 0) */
        M->live = 0;
        return 0;
    }
    for (i = 0; i < n; i++)
        any |= (uint32_t)out[i];
    if (!any && !M->live)                          /* silent in, the tail gone: nothing to smear */
        return 0;
    for (i = 0; i < n; i++)
        io[i] = (float)out[i] * (1.0f / 65536.0f);
    pdf_process(&M->d, amount, 0.5f * diff + 0.25f, io, n);
    for (i = 0; i < n; i++) {
        const int32_t y = (int32_t)(io[i] * 65536.0f);
        out[i] = y;
        peak |= y < 0 ? -y : y;
    }
    M->live = peak > 16;                           /* (about -72 dB of a voice) */
    return !any && M->live;
}

/* {HARM TIMB MORPH AUX, CUT RES DRV KTR} */
static const preset_t PRTCL_PRESETS[] = {
    {"DUST CLOUD", {60, 70, 20, 0, 110, 10, 0, 64}, {10, 80, 100, 70}, 0, 0, PAT(5)},
    {"RAIN", {100, 95, 50, 0, 100, 0, 0, 32}, {30, 90, 110, 90}, 0, 0, PAT(5)},
    {"RESO PING", {0, 40, 110, 0, 127, 0, 0, 64}, {0, 80, 60, 60}, 0, 0, PAT(3)},
    {"CRACKLE", {40, 80, 64, 127, 90, 20, 30, 0}, {0, 90, 110, 60}, 0, 0, PAT(5)},
    {"GLITTER PAD", {30, 100, 10, 0, 110, 10, 0, 64}, {80, 90, 110, 95}, 10, 0, PAT(5)},
    {"WOOD TAPS", {10, 25, 90, 0, 110, 0, 0, 64}, {0, 70, 0, 50}, 0, 0, PAT(7)},
    {"GEIGER", {127, 15, 64, 60, 120, 0, 10, 0}, {0, 90, 127, 40}, 0, 0, PAT(5)},
    {"SHIMMER", {20, 110, 0, 0, 100, 10, 0, 64}, {60, 90, 110, 100}, 10, 0, PAT(5)},
};

static const engine_t ENG_PRTCL = {
    .name = "PRTCL",
    .page_title = {"DUST", "FLT"},
    .edit = PLV_EDIT(60, 70, 20),
    .presets = PRTCL_PRESETS,
    .npresets = NELEM(PRTCL_PRESETS),
    .note_on = prt_note_on,
    .render = prt_render,
    .knob = {P_E0, P_E1, P_E2, P_E4},                 /* HOME: HARM TIMB MORPH, as the Grind's three, and CUT */
    .keep = 0x17,                                     /* the filter, the DC blocker; the start flag (a running 0) */
    .mem = sizeof(prt_mem_t),
    .post = prt_post,
};
