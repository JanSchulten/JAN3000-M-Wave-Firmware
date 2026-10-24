/* SPDX-License-Identifier: MIT
 * Copyright 2016 Emilie Gillet (Mutable Instruments Plaits); C port for the FM-1 2026, M-Vave-OP1 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice
 * and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
/* Plaits' waveshaping model (plaits/dsp/engine/waveshaping_engine.cc), with the SLOPE shape of its Oscillator
 * (plaits/dsp/oscillator/oscillator.h) and stmlib's integrated polyBLEP, Hermite interpolation and parameter ramps, as
 * C in float for the FPU (core.h FM1_FPU on every function here). Bit for bit Plaits' own output on an IEEE single FPU
 * (tools/plaits_proto/ref.cc). The WSHAPE engine (eng_wshape.c) is built around it, the FPU probe (fpu_probe.c) runs
 * it; pws_start (not Plaits': a module runs on, a voice starts) begins a voice at its note without Plaits' glide from
 * the last block (docs/PLAITS_PLAN.md).
 * Units as Plaits': f0 the note's frequency over the sample rate, HARMONICS TIMBRE MORPH 0..1, the output about +-1. */
#include "plaits_tables.h"

typedef struct {
    float phase, next, freq, pw;               /* the phase 0..1, the BLEP's carry into the next sample, the ramps' ends */
    int32_t high;                              /* the slope is rising */
} pws_osc_t;
typedef struct {
    pws_osc_t slope, tri;                      /* the shaped slope (MORPH its width) and a triangle for AUX */
    float shape, wf, ot;                       /* the ramps' ends: waveshaper, wavefolder gain, overtone blend */
} pws_t;                                       /* 56 bytes */

static const int16_t *const PWS_SHAPES[6] = {lut_ws_inverse_tan, lut_ws_inverse_sin, lut_ws_linear, lut_ws_bump,
                                             lut_ws_double_bump, lut_ws_double_bump};

FM1_FPU static void pws_osc_init(pws_osc_t *o)
{
    o->phase = 0.5f;
    o->next = 0.0f;
    o->high = 1;
    o->freq = 0.001f;
    o->pw = 0.5f;
}
FM1_FPU static void pws_init(pws_t *s)
{
    pws_osc_init(&s->slope);
    pws_osc_init(&s->tri);
    s->shape = s->wf = s->ot = 0.0f;
}

FM1_FPU static inline float pws_clamp(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
FM1_FPU static inline float pws_abs(float x) { return x < 0.0f ? -x : x; }
FM1_FPU static inline float pws_blep_next(float t)  /* stmlib NextIntegratedBlepSample */
{
    const float t1 = 0.5f * t, t2 = t1 * t1, t4 = t2 * t2;
    return 0.1875f - t1 + 1.5f * t2 - t4;
}
FM1_FPU static inline float pws_blep_this(float t) { return pws_blep_next(1.0f - t); }

/* n samples of the band-limited slope: up for pw of the cycle, down for the rest; frequency and pw ramp from the last
 * block's values (stmlib ParameterInterpolator). Two divides a sample (the slopes) */
FM1_FPU static void pws_slope(pws_osc_t *o, float freq, float pw, float *out, uint32_t n)
{
    float f = o->freq, p = o->pw, fi, pi, next = o->next, phase = o->phase;
    int32_t high = o->high;
    freq = pws_clamp(freq, 0.000001f, 0.25f);
    pw = pws_clamp(pw, pws_abs(freq) * 2.0f, 1.0f - 2.0f * pws_abs(freq));
    fi = (freq - f) / (float)n;
    pi = (pw - p) / (float)n;
    while (n--) {
        float this_sample = next, su, sd;
        next = 0.0f;
        f += fi;
        p += pi;
        phase += f;
        su = 1.0f / p;
        sd = 1.0f / (1.0f - p);
        if (high ^ (phase < p)) {                  /* the top: up turns down */
            float t = (phase - p) / f, d = (su + sd) * f;
            this_sample -= pws_blep_this(t) * d;
            next -= pws_blep_next(t) * d;
            high = phase < p;
        }
        if (phase >= 1.0f) {                       /* the wrap: down turns up */
            float t, d;
            phase -= 1.0f;
            t = phase / f;
            d = (su + sd) * f;
            this_sample += pws_blep_this(t) * d;
            next += pws_blep_next(t) * d;
            high = 1;
        }
        next += high ? phase * su : 1.0f - (phase - p) * sd;
        *out++ = 2.0f * this_sample - 1.0f;
    }
    o->freq = f;
    o->pw = p;
    o->next = next;
    o->phase = phase;
    o->high = high;
}

FM1_FPU static inline float pws_tame(float f0, float harmonics, float order)   /* less shaping as the partials near Nyquist */
{
    float max_f = 0.5f / order, a;
    f0 *= harmonics;
    a = pws_clamp(1.0f - (f0 - max_f) / (0.5f - max_f), 0.0f, 1.0f);
    return a * a * a;
}
FM1_FPU static inline float pws_hermite(const float *table, float index, float size)
{
    int32_t ii;
    float f, xm1, x0, x1, x2, c, v, w, a, b_neg;
    index *= size;
    ii = (int32_t)index;
    f = index - (float)ii;
    xm1 = table[ii - 1];
    x0 = table[ii];
    x1 = table[ii + 1];
    x2 = table[ii + 2];
    c = (x1 - xm1) * 0.5f;
    v = x0 - x1;
    w = c + v;
    a = w + v + (x2 - x0) * 0.5f;
    b_neg = w + a;
    return (((a * f) - b_neg) * f + c) * f + x0;
}
FM1_FPU static inline float pws_sine(float phase)    /* plaits Sine: lut_sine, wrapped */
{
    int32_t ii;
    float f;
    phase -= (float)(int32_t)phase;
    phase *= 512.0f;
    ii = (int32_t)phase;
    f = phase - (float)ii;
    return lut_sine[ii] + (lut_sine[ii + 1] - lut_sine[ii]) * f;
}

/* the waveshaper's, the wavefolder gain's and the overtone blend's values for f0 HARMONICS TIMBRE MORPH (the ends of a
 * block's ramps) */
FM1_FPU static void pws_ends(float f0, float harmonics, float timbre, float morph, float *sh, float *wf, float *ot)
{
    const float slope = 3.0f + pws_abs(morph - 0.5f) * 5.0f;
    const float shape_amount = pws_abs(harmonics - 0.5f) * 2.0f;
    const float shape_att = pws_tame(f0, slope, 16.0f);
    const float wf_att = pws_tame(f0, slope * (3.0f + shape_amount * shape_att * 5.0f), 12.0f);
    const float og = timbre * (2.0f - timbre);
    *sh = 0.5f + (harmonics - 0.5f) * shape_att;
    *wf = 0.03f + 0.46f * timbre * wf_att;
    *ot = og * (2.0f - og);
}

/* a voice from rest: both oscillators at phase (0..1) and at f0 and their widths, the ramps at their ends: the first
 * block plays the note, not a glide into it */
FM1_FPU static void pws_start(pws_t *s, float phase, float f0, float harmonics, float timbre, float morph)
{
    float f = pws_clamp(f0, 0.000001f, 0.25f), pw = morph * 0.45f + 0.5f;
    pws_init(s);
    s->slope.phase = s->tri.phase = phase;
    s->slope.freq = s->tri.freq = f;
    s->slope.pw = pws_clamp(pw, f * 2.0f, 1.0f - 2.0f * f);
    s->tri.pw = pws_clamp(0.5f, f * 2.0f, 1.0f - 2.0f * f);
    s->slope.high = phase < s->slope.pw;
    s->tri.high = phase < s->tri.pw;
    pws_ends(f0, harmonics, timbre, morph, &s->shape, &s->wf, &s->ot);
}

/* one block of n samples into out (the model) and aux (Plaits' AUX: the triangle's sine blended into the second fold) */
FM1_FPU static void pws_render(pws_t *s, float f0, float harmonics, float timbre, float morph, float *out, float *aux,
                               uint32_t n)
{
    const float pw = morph * 0.45f + 0.5f;
    float sh = s->shape, wf = s->wf, ot = s->ot, sh_e, wf_e, ot_e, sh_i, wf_i, ot_i;
    uint32_t i;
    pws_ends(f0, harmonics, timbre, morph, &sh_e, &wf_e, &ot_e);
    sh_i = (sh_e - sh) / (float)n;
    wf_i = (wf_e - wf) / (float)n;
    ot_i = (ot_e - ot) / (float)n;
    pws_slope(&s->slope, f0, pw, out, n);
    pws_slope(&s->tri, f0, 0.5f, aux, n);
    for (i = 0; i < n; i++) {
        float shape, wsi, x, y, mix, index, fold, fold_2, sine;
        int32_t si, wi;
        const int16_t *s1, *s2;
        sh += sh_i;
        wf += wf_i;
        ot += ot_i;
        shape = sh * 3.9999f;                      /* between two of the five shapes */
        si = (int32_t)shape;
        shape -= (float)si;
        s1 = PWS_SHAPES[si];
        s2 = PWS_SHAPES[si + 1];
        wsi = 127.0f * out[i] + 128.0f;            /* the slope through both shapes */
        wi = (int32_t)wsi;
        wsi -= (float)wi;
        wi &= 255;
        x = (float)s1[wi] / 32768.0f;
        x += ((float)s1[wi + 1] / 32768.0f - x) * wsi;
        y = (float)s2[wi] / 32768.0f;
        y += ((float)s2[wi + 1] / 32768.0f - y) * wsi;
        mix = x + (y - x) * shape;
        index = mix * wf + 0.5f;                   /* then through the folders */
        fold = pws_hermite(lut_fold + 1, index, 512.0f);
        fold_2 = -pws_hermite(lut_fold_2 + 1, index, 512.0f);
        sine = pws_sine(aux[i] * 0.25f + 0.5f);
        out[i] = fold;
        aux[i] = sine + (fold_2 - sine) * ot;
    }
    s->shape = sh;
    s->wf = wf;
    s->ot = ot;
}
