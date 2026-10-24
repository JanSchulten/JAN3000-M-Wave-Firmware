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
/* Plaits' additive model (plaits/dsp/engine/additive_engine.cc) with its harmonic oscillator
 * (plaits/dsp/oscillator/harmonic_oscillator.h) and stmlib's parameter ramps, as C in float for the FPU (core.h FM1_FPU
 * on every function here). Bit for bit Plaits' own output on an IEEE single FPU at lp 0.001 (tools/plaits_proto/
 * ref_add.cc). The HRMNC engine (eng_harm.c) is built around it.
 *   OUT  24 sine partials 1..24 (two batches of 12, each a Chebyshev recurrence from one or two table sines)
 *   AUX  the organ's 8: partials 1 2 3 4 6 8 10 12
 *   the spectrum: a peak at TIMBRE (the centroid) of a width from MORPH (the slope), HARMONICS adding bumps beside it;
 *   smoothed a block at a time (lp: Plaits' 0.001 a 12-sample block; a caller with longer blocks raises it), then
 *   normalized to a sum of 1
 * Plaits' three oscillators run on the same frequency from the same start, so their phases are one (phase, freq: the
 * same sums in the same order: still Plaits' samples). Units as Plaits': f0 the note's frequency over the sample rate,
 * HARMONICS TIMBRE MORPH 0..1, the output about +-1. Uses plaits_ws.c's tables (lut_sine) and helpers. */

#define PAD_N 24                               /* OUT's partials */
#define PAD_NA 12                              /* AUX's oscillator: partials 1..12, 8 of them sounding */

typedef struct {
    float phase, freq;                         /* the oscillators' phase 0..1 and their frequency ramp's end */
    float amp[PAD_N + PAD_NA];                 /* their partials' amplitude ramps' ends: 0..23 OUT, 24..35 AUX */
    float spec[PAD_N + PAD_NA];                /* the smoothed spectra (Plaits' amplitudes_) */
} pad_t;                                       /* 296 bytes */

static const uint8_t PAD_ORGAN[8] = {0, 1, 2, 3, 5, 7, 9, 11};

FM1_FPU static void pad_init(pad_t *s)
{
    uint32_t i;
    s->phase = s->freq = 0.0f;
    for (i = 0; i < PAD_N + PAD_NA; i++)
        s->amp[i] = s->spec[i] = 0.0f;
}

FM1_FPU static inline float pad_sine_nowrap(float phase)   /* plaits SineNoWrap: phase 0 .. 1.25 */
{
    int32_t ii;
    float f;
    phase *= 512.0f;
    ii = (int32_t)phase;
    f = phase - (float)ii;
    return lut_sine[ii] + (lut_sine[ii + 1] - lut_sine[ii]) * f;
}

/* Plaits' UpdateAmplitudes: the spectrum of n partials (their slots idx in a) towards the peak, smoothed by lp,
 * normalized */
FM1_FPU static void pad_spectrum(float centroid, float slope, float bumps, float *a, const uint8_t *idx, uint32_t n,
                                 float lp)
{
    const float nf = (float)n - 1.0f;
    const float margin = (1.0f / slope - 1.0f) / (1.0f + bumps);
    const float center = centroid * (nf + margin) - 0.5f * margin;
    float sum = 0.001f;
    uint32_t i;
    for (i = 0; i < n; i++) {
        float order = pws_abs((float)i - center) * slope;
        float gain = 1.0f - order;
        float b = 0.25f + order * bumps;
        const uint32_t j = idx ? idx[i] : i;
        gain += pws_abs(gain);
        gain *= gain;
        gain *= 1.0f + pws_sine(b);
        gain *= gain;
        gain *= gain;
        a[j] += lp * (gain - a[j]);
        sum += a[j];
    }
    sum = 1.0f / sum;
    for (i = 0; i < n; i++)
        a[idx ? idx[i] : i] *= sum;
}

/* the spectra's values for HARMONICS TIMBRE MORPH, smoothed by lp: once a block */
FM1_FPU static void pad_update(pad_t *s, float harmonics, float timbre, float morph, float lp)
{
    const float raw_slope = (1.0f - 0.6f * harmonics) * morph;
    const float slope = 0.01f + 1.99f * raw_slope * raw_slope * raw_slope;
    const float bumps = 16.0f * harmonics * harmonics;
    pad_spectrum(timbre, slope, bumps, s->spec, 0, PAD_N, lp);
    pad_spectrum(timbre, slope, bumps, s->spec + PAD_N, PAD_ORGAN, 8u, lp);
}

/* a voice from rest: the oscillators at phase (0..1) and at f0, the spectra as Plaits' Reset left them and then set
 * once (the first block fades the partials in from 0 over its length, as Plaits' first block after a Reset) */
FM1_FPU static void pad_start(pad_t *s, float phase, float f0)
{
    pad_init(s);
    s->phase = phase;
    s->freq = f0 >= 0.5f ? 0.5f : f0;
}

/* one block of n samples into out (OUT) and aux (AUX); need: bit 0 out, bit 1 aux (3 both, as Plaits). Without a bit
 * that output is not computed (left as scratch), its ramps end where they would (the other output stays Plaits' own;
 * the left-out one within rounding of it when it comes back). Call pad_update first */
FM1_FPU static void pad_render(pad_t *s, float f0, float *out, float *aux, uint32_t n, uint32_t need)
{
    float av[PAD_N + PAD_NA], ai[PAD_N + PAD_NA];
    float f = s->freq, fi, phase = s->phase;
    uint32_t i, k;
    if (f0 >= 0.5f)
        f0 = 0.5f;
    fi = (f0 - f) / (float)n;
    if (need & 1u) {
        for (k = 0; k < PAD_N; k++) {
            float fh = f0 * (float)(1u + k), want;
            if (fh >= 0.5f)
                fh = 0.5f;
            want = s->spec[k] * (1.0f - fh * 2.0f);
            av[k] = s->amp[k];
            ai[k] = (want - s->amp[k]) / (float)n;
        }
    }
    for (k = 0; k < PAD_NA; k++) {
        float fh = f0 * (float)(1u + k), want;
        if (fh >= 0.5f)
            fh = 0.5f;
        want = s->spec[PAD_N + k] * (1.0f - fh * 2.0f);
        av[PAD_N + k] = s->amp[PAD_N + k];
        ai[PAD_N + k] = (want - s->amp[PAD_N + k]) / (float)n;
        if (!(need & 2u))
            s->amp[PAD_N + k] = want;                  /* (left out: the ramp's end, within rounding of its sum) */
    }
    if (!(need & 1u))
        for (k = 0; k < PAD_N; k++) {
            float fh = f0 * (float)(1u + k);
            if (fh >= 0.5f)
                fh = 0.5f;
            s->amp[k] = s->spec[k] * (1.0f - fh * 2.0f);   /* (left out: as above) */
        }
    for (i = 0; i < n; i++) {
        float two_x, prev, cur, sum, tmp;
        f += fi;
        phase += f;
        if (phase >= 1.0f)
            phase -= 1.0f;
        two_x = 2.0f * pad_sine_nowrap(phase);
        if (need & 1u) {
            prev = 1.0f;                           /* partials 1..12 */
            cur = two_x * 0.5f;
            sum = 0.0f;
            for (k = 0; k < 12u; k++) {
                av[k] += ai[k];
                sum += av[k] * cur;
                tmp = cur;
                cur = two_x * cur - prev;
                prev = tmp;
            }
            out[i] = sum;
            prev = pws_sine(phase * 12.0f + 0.25f);   /* partials 13..24 */
            cur = pws_sine(phase * 13.0f);
            sum = 0.0f;
            for (k = 12u; k < 24u; k++) {
                av[k] += ai[k];
                sum += av[k] * cur;
                tmp = cur;
                cur = two_x * cur - prev;
                prev = tmp;
            }
            out[i] += sum;
        }
        if (need & 2u) {
            prev = 1.0f;                           /* the organ: partials 1..12 (4 of them at 0, as Plaits') */
            cur = two_x * 0.5f;
            sum = 0.0f;
            for (k = 0; k < 12u; k++) {
                av[PAD_N + k] += ai[PAD_N + k];
                sum += av[PAD_N + k] * cur;
                tmp = cur;
                cur = two_x * cur - prev;
                prev = tmp;
            }
            aux[i] = sum;
        }
    }
    for (k = 0; k < PAD_N; k++)                    /* the ramps end where their sums are (stmlib's interpolator) */
        if (need & 1u)
            s->amp[k] = av[k];
    for (k = PAD_N; k < PAD_N + PAD_NA; k++)
        if (need & 2u)
            s->amp[k] = av[k];
    s->freq = f;
    s->phase = phase;
}
