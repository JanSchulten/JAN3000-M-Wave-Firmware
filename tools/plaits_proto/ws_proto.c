/* SPDX-License-Identifier: MIT */
/* Phase-0 prototype: Plaits' waveshaping engine (plaits/dsp/engine/waveshaping_engine.cc, the SLOPE shape of
 * plaits/dsp/oscillator/oscillator.h, stmlib's polyBLEP / Hermite / parameter interpolation) as plain C in float,
 * for the FM-1's FPU (target "fprev1" on these functions only).
 *
 * Original code: Copyright 2016 Emilie Gillet, MIT licence:
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 *   documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
 *   to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above
 *   copyright notice and this permission notice shall be included in all copies or substantial portions of the
 *   Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 *
 * WS_FAST: the slopes' reciprocals 1/pw and 1/(1-pw) interpolated linearly across the block instead of divided
 * once a sample: two divides a sample fewer; pw moves slowly (MORPH), so they differ only while it moves. */
#include <stdint.h>
#include "ws_tables.h"

#ifndef WS_FPU
#define WS_FPU
#endif

typedef struct {
    float phase, next_sample, frequency, pw;
    int high;
} ws_osc_t;
typedef struct {
    ws_osc_t slope, tri;
    float prev_shape, prev_wf, prev_ot;
} ws_t;

static const int16_t *const WS_SHAPES[6] = {lut_ws_inverse_tan, lut_ws_inverse_sin, lut_ws_linear, lut_ws_bump,
                                            lut_ws_double_bump, lut_ws_double_bump};

WS_FPU static void ws_osc_init(ws_osc_t *o)
{
    o->phase = 0.5f;
    o->next_sample = 0.0f;
    o->high = 1;
    o->frequency = 0.001f;
    o->pw = 0.5f;
}
WS_FPU void ws_init(ws_t *s)
{
    ws_osc_init(&s->slope);
    ws_osc_init(&s->tri);
    s->prev_shape = s->prev_wf = s->prev_ot = 0.0f;
}

WS_FPU static inline float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
WS_FPU static inline float absf(float x) { return x < 0.0f ? -x : x; }
WS_FPU static inline float iblep_next(float t)
{
    const float t1 = 0.5f * t, t2 = t1 * t1, t4 = t2 * t2;
    return 0.1875f - t1 + 1.5f * t2 - t4;
}
WS_FPU static inline float iblep_this(float t) { return iblep_next(1.0f - t); }

WS_FPU static void ws_slope(ws_osc_t *o, float frequency, float pw, float *out, uint32_t n)
{
    float f = o->frequency, p = o->pw, fi, pi, next = o->next_sample, phase = o->phase;
    int high = o->high;
#ifdef WS_FAST
    float su, sd, sui, sdi;
#endif
    frequency = clampf(frequency, 0.000001f, 0.25f);
    pw = clampf(pw, absf(frequency) * 2.0f, 1.0f - 2.0f * absf(frequency));
    fi = (frequency - f) / (float)n;
    pi = (pw - p) / (float)n;
#ifdef WS_FAST
    su = 1.0f / p;                                   /* the slopes from the block's start to its end, linearly */
    sd = 1.0f / (1.0f - p);
    sui = (1.0f / pw - su) / (float)n;
    sdi = (1.0f / (1.0f - pw) - sd) / (float)n;
#endif
    while (n--) {
        float this_sample = next;
#ifndef WS_FAST
        float su, sd;
#endif
        next = 0.0f;
        f += fi;
        p += pi;
        phase += f;
#ifndef WS_FAST
        su = 1.0f / p;
        sd = 1.0f / (1.0f - p);
#else
        su += sui;
        sd += sdi;
#endif
        if (high ^ (phase < p)) {
            float t = (phase - p) / f, d = (su + sd) * f;
            this_sample -= iblep_this(t) * d;
            next -= iblep_next(t) * d;
            high = phase < p;
        }
        if (phase >= 1.0f) {
            float t, d;
            phase -= 1.0f;
            t = phase / f;
            d = (su + sd) * f;
            this_sample += iblep_this(t) * d;
            next += iblep_next(t) * d;
            high = 1;
        }
        next += high ? phase * su : 1.0f - (phase - p) * sd;
        *out++ = 2.0f * this_sample - 1.0f;
    }
    o->frequency = f;
    o->pw = p;
    o->next_sample = next;
    o->phase = phase;
    o->high = high;
}

WS_FPU static inline float tame(float f0, float harmonics, float order)
{
    float max_f = 0.5f / order, a;
    f0 *= harmonics;
    a = clampf(1.0f - (f0 - max_f) / (0.5f - max_f), 0.0f, 1.0f);
    return a * a * a;
}
WS_FPU static inline float hermite(const float *table, float index, float size)
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
WS_FPU static inline float sine_wrap(float phase)
{
    int32_t ii;
    float f;
    phase -= (float)(int32_t)phase;
    phase *= 512.0f;
    ii = (int32_t)phase;
    f = phase - (float)ii;
    return lut_sine[ii] + (lut_sine[ii + 1] - lut_sine[ii]) * f;
}

/* one block: f0 the note's frequency / the sample rate; harmonics, timbre, morph 0..1; out the main output, aux
 * Plaits' AUX (the sine / overtone blend); both n floats */
WS_FPU void ws_render(ws_t *s, float f0, float harmonics, float timbre, float morph, float *out, float *aux, uint32_t n)
{
    const float pw = morph * 0.45f + 0.5f;
    const float slope = 3.0f + absf(morph - 0.5f) * 5.0f;
    const float shape_amount = absf(harmonics - 0.5f) * 2.0f;
    const float shape_att = tame(f0, slope, 16.0f);
    const float wf_att = tame(f0, slope * (3.0f + shape_amount * shape_att * 5.0f), 12.0f);
    const float og = timbre * (2.0f - timbre);
    const float inv_n = 1.0f / (float)n;
    float sh = s->prev_shape, wf = s->prev_wf, ot = s->prev_ot;
    const float sh_i = (0.5f + (harmonics - 0.5f) * shape_att - sh) * inv_n;
    const float wf_i = (0.03f + 0.46f * timbre * wf_att - wf) * inv_n;
    const float ot_i = (og * (2.0f - og) - ot) * inv_n;
    uint32_t i;
    ws_slope(&s->slope, f0, pw, out, n);
    ws_slope(&s->tri, f0, 0.5f, aux, n);
    for (i = 0; i < n; i++) {
        float shape, wsi, x, y, mix, index, fold, fold_2, sine;
        int32_t si, wi;
        const int16_t *s1, *s2;
        sh += sh_i;
        wf += wf_i;
        ot += ot_i;
        shape = sh * 3.9999f;
        si = (int32_t)shape;
        shape -= (float)si;
        s1 = WS_SHAPES[si];
        s2 = WS_SHAPES[si + 1];
        wsi = 127.0f * out[i] + 128.0f;
        wi = (int32_t)wsi;
        wsi -= (float)wi;
        wi &= 255;
        x = (float)s1[wi] * (1.0f / 32768.0f);
        x += ((float)s1[wi + 1] * (1.0f / 32768.0f) - x) * wsi;
        y = (float)s2[wi] * (1.0f / 32768.0f);
        y += ((float)s2[wi + 1] * (1.0f / 32768.0f) - y) * wsi;
        mix = x + (y - x) * shape;
        index = mix * wf + 0.5f;
        fold = hermite(lut_fold + 1, index, 512.0f);
        fold_2 = -hermite(lut_fold_2 + 1, index, 512.0f);
        sine = sine_wrap(aux[i] * 0.25f + 0.5f);
        out[i] = fold;
        aux[i] = sine + (fold_2 - sine) * ot;
    }
    s->prev_shape = sh;
    s->prev_wf = wf;
    s->prev_ot = ot;
}
