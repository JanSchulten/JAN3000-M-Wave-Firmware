/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The Felucca side of the Plaits engines (M-Vave-OP1: WSHAPE, HRMNC, PRTCL; docs/PLAITS_PLAN.md), as Behringer's Grind plays
 * a model: the model's two outputs blended, then a drive, a filter and the ADSR. Every one of them has the same eight
 * values, so the same code reads them:
 *   E0 HARM  E1 TIMB  E2 MORPH  Plaits' HARMONICS, TIMBRE, MORPH (0..127 -> 0..1); the SHP routings (ENV / LFO DEST
 *            SHP, the matrix's SHP: 1 step a step) move TIMBRE, as they move ANALOG's pulse width
 *   E3 AUX   the blend of Plaits' OUT (0) and AUX (127); at 0 the model leaves AUX out, at 127 OUT (its need)
 *   E4..E7   CUT RES DRV KTR: ANALOG's filter (dsp.c's trapezoidal SVF low-pass, its soft knee; DRV a soft clip before
 *            it; KTR the cutoff following the note, 64 one octave an octave); ENV / LFO / matrix -> FLT move CUT
 * A voice's state: the filter in v->s[0] v->s[1], a DC blocker's mean in v->s[2] (Plaits' shapes carry DC; the module's
 * output is AC-coupled), the model's start flag in v->s[4] (plv_note_on sets it, the model's first block clears it:
 * .keep 0x17 keeps a running voice's 0, the filter and the blocker over a retrigger); the model itself in the part's
 * engine memory. */

typedef struct {
    float f0, harm, timb, morph, blend;        /* the model's parameters, Plaits' units */
    uint32_t need;                             /* the outputs to compute: bit 0 OUT, bit 1 AUX */
} plv_par_t;

static float plv_out[CTL], plv_aux[CTL];        /* the model's outputs of a block */
static int32_t plv_o[CTL];                     /* .. blended, Q15 */

static void plv_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->s[0] = v->s[1] = 0;                            /* filter */
    v->s[2] = 0;                                      /* the DC blocker's mean */
    v->s[4] = 1;                                      /* start the model at the first render */
}

/* the model's parameters from the track's values and the voice's modulation; the outputs it needs */
FM1_FPU static inline void plv_params(const track_t *t, const vmod_t *m, plv_par_t *q)
{
    const int16_t *p = t->p;
    int32_t tq = (p[P_E1] << 8) + m->shape - (64 << 8);   /* TIMB and the SHP routings, 0..127 << 8 */
    tq = tq < 0 ? 0 : tq > (127 << 8) ? (127 << 8) : tq;
    q->f0 = (float)m->inc * (1.0f / 4294967296.0f);
    q->harm = (float)p[P_E0] * (1.0f / 127.0f);
    q->morph = (float)p[P_E2] * (1.0f / 127.0f);
    q->blend = (float)p[P_E3] * (1.0f / 127.0f);
    q->timb = (float)tq * (1.0f / (127.0f * 256.0f));
    q->need = !p[P_E3] ? 1u : p[P_E3] >= 127 ? 2u : 3u;
}

/* the voice's phase 0..1 (v->ph[0]: UNISON's voices, voice.c spreads them) */
FM1_FPU static inline float plv_phase(const voice_t *v) { return (float)(v->ph[0] >> 8) * (1.0f / 16777216.0f); }

/* plv_out and plv_aux blended by AUX into plv_o (at AUX 0 b reads OUT: x is OUT exactly; at 127 AUX alone) */
FM1_FPU static inline void plv_blend(const plv_par_t *q, uint32_t n)
{
    const float *b = q->need == 1u ? plv_out : plv_aux;
    const float *a = q->need == 2u ? plv_aux : plv_out;
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t x = (int32_t)((a[i] + (b[i] - a[i]) * q->blend) * 32767.0f);   /* (clamped as an integer: the same */
        plv_o[i] = x > 32767 ? 32767 : x < -32767 ? -32767 : x;                 /* as clamping x to +-1 first) */
    }
}

/* the voice after the model (plv_o): the drive, the filter and its knee (ANALOG's), the DC blocker, the amplitude;
 * integer, as every engine */
static void plv_post(const track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    int32_t drive = 32768 + p[P_E6] * 512, drv = p[P_E6];
    int32_t ic1 = v->s[0], ic2 = v->s[1], dc = v->s[2];
    tsvf_t flt;
    uint32_t i;
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t s = plv_o[i];
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

/* the edit page of every Plaits engine (its defaults: d HARM TIMB MORPH) */
#define PLV_EDIT(h, tb, mo)                                                                                            \
    {                                                                                                                  \
        {"HARM", F_PCT, 0, 127, h, 0, 0}, {"TIMB", F_PCT, 0, 127, tb, 0, 0}, {"MORPH", F_PCT, 0, 127, mo, 0, 0},        \
            {"AUX", F_PCT, 0, 127, 0, 0, 0}, {"CUT", F_CUTOFF, 0, 127, 110, 0, 0}, {"RES", F_PCT, 0, 127, 10, 0, 0},    \
            {"DRV", F_PCT, 0, 127, 0, 0, 0}, {"KTR", F_PCT, 0, 127, 64, 0, 0},                                        \
    }
