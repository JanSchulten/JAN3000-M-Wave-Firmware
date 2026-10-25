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
/* Plaits' swarm model (plaits/dsp/engine/swarm_engine.cc, swarm_engine.h: its grain envelopes, band-limited saws and
 * stmlib's FastSineOscillator), as C in float for the FPU (core.h FM1_FPU on every function here). Bit for bit Plaits'
 * own on an IEEE single FPU (tools/plaits_proto/ref_swarm.cc). The SWARM engine (eng_swarm.c) is built around it.
 *   8 oscillators a voice, each a saw (OUT) and a sine (AUX) at the note, spread around it by rank (HARMONICS); each
 *   under a grain envelope whose rate is the density (TIMBRE) and whose size MORPH sets: short grains of their pitch
 *   (MORPH low), or the oscillators held and gliding at random between pitches (MORPH high). burst: a note restarts the
 *   swarm fast, then it slows (Plaits with its trigger patched); otherwise it runs at its rate.
 * The random numbers: stmlib's generator, its state the caller's (plaits_part.c pun_rand). Uses plaits_ws.c's and
 * plaits_part.c's helpers (pws_blep, pun_ratio, pun_rand) and plaits_ws.c's lut_sine. */
#define PSW_N 8                                /* oscillators a voice (kNumSwarmVoices) */

typedef struct {
    float from, interval, phase, fm, amplitude, prev_size, filter;   /* GrainEnvelope */
    float rank;
    float s_phase, s_next, s_freq, s_gain;     /* AdditiveSawOscillator */
    float x, y, eps, amp;                      /* FastSineOscillator */
} psw_osc_t;                                   /* 64 bytes */
typedef struct {
    psw_osc_t o[PSW_N];
} psw_t;                                       /* 512 bytes */

FM1_FPU static void psw_init(psw_t *s)                    /* Init and Reset */
{
    const float n = (float)((PSW_N - 1) / 2);   /* (3: Plaits divides as integers) */
    uint32_t i;
    for (i = 0; i < PSW_N; i++) {
        psw_osc_t *o = &s->o[i];
        o->rank = ((float)i - n) / n;
        o->from = 0.0f;
        o->interval = 1.0f;
        o->phase = 1.0f;
        o->fm = 0.0f;
        o->amplitude = 0.5f;
        o->prev_size = 0.0f;
        o->filter = 0.0f;
        o->s_phase = 0.0f;
        o->s_next = 0.0f;
        o->s_freq = 0.01f;
        o->s_gain = 0.0f;
        o->x = 1.0f;
        o->y = 0.0f;
        o->eps = 0.0f;
        o->amp = 0.0f;
    }
}

FM1_FPU static inline void psw_step(psw_osc_t *o, uint32_t *rng, float rate, int burst, int start)   /* Step */
{
    int randomize = 0;
    if (start) {
        o->phase = 0.5f;
        o->fm = 16.0f;
        randomize = 1;
    } else {
        o->phase += rate * o->fm;
        if (o->phase >= 1.0f) {
            o->phase -= (float)(int32_t)o->phase;
            randomize = 1;
        }
    }
    if (randomize) {
        o->from += o->interval;
        o->interval = pun_rand(rng) - o->from;
        if (burst)
            o->fm *= 0.8f + 0.2f * pun_rand(rng);
        else
            o->fm = 0.5f + 1.5f * pun_rand(rng);
    }
}

FM1_FPU static inline float psw_amplitude(psw_osc_t *o, float size_ratio)
{
    float target = 1.0f;
    if (size_ratio >= 1.0f) {
        float ph = (o->phase - 0.5f) * size_ratio;
        ph = ph < -1.0f ? -1.0f : ph > 1.0f ? 1.0f : ph;
        target = 0.5f * (pws_sine(0.5f * ph + 1.25f) + 1.0f);
    }
    if ((size_ratio >= 1.0f) ^ (o->prev_size >= 1.0f))
        o->filter = 0.5f;
    o->filter *= 0.95f;
    o->prev_size = size_ratio;
    o->amplitude += (0.5f - o->filter) * (target - o->amplitude);
    return o->amplitude;
}

FM1_FPU static inline float psw_rsqrt(float x)            /* stmlib fast_rsqrt_carmack */
{
    union { float f; uint32_t u; } v;
    float y;
    v.f = x;
    v.u = 0x5f3759dfu - (v.u >> 1);
    y = v.f;
    return y * (1.5f - (x * 0.5f * y * y));
}

/* one block of n samples into out (the saws) and aux (the sines, when need bit 1); f0, density (TIMBRE), spread
 * (HARMONICS), size_ratio (MORPH) as Plaits computes them (psw_params) */
FM1_FPU static void psw_render(psw_t *s, uint32_t *rng, float f0, float density, float spread, float size_ratio,
                               int burst, int start, float *out, float *aux, uint32_t n, uint32_t need)
{
    const float fn = (float)n;
    uint32_t i, k;
    for (i = 0; i < n; i++)
        out[i] = 0.0f;
    if (need & 2u)
        for (i = 0; i < n; i++)
            aux[i] = 0.0f;
    for (k = 0; k < PSW_N; k++) {
        psw_osc_t *o = &s->o[k];
        float amplitude, expo, f, lin;
        psw_step(o, rng, density, burst, start);
        amplitude = psw_amplitude(o, size_ratio) * (1.0f / (float)PSW_N);
        expo = size_ratio < 1.0f ? 2.0f * (o->from + o->interval * o->phase) - 1.0f : o->from;
        f = f0 * pun_ratio(48.0f * expo * spread * o->rank);
        lin = o->rank * (o->rank + 0.01f) * spread * 0.25f;
        f *= 1.0f + lin;
        {   /* the saw */
            float fr = f >= 0.25f ? 0.25f : f, vf = o->s_freq, vg = o->s_gain, next = o->s_next, ph = o->s_phase;
            const float fi = (fr - vf) / fn, gi = (amplitude - vg) / fn;
            for (i = 0; i < n; i++) {
                float this_sample = next;
                next = 0.0f;
                vf += fi;
                ph += vf;
                if (ph >= 1.0f) {
                    float t;
                    ph -= 1.0f;
                    t = ph / vf;
                    this_sample -= 0.5f * t * t;                        /* ThisBlepSample */
                    next -= -0.5f * (1.0f - t) * (1.0f - t);           /* NextBlepSample */
                }
                next += ph;
                vg += gi;
                out[i] += (2.0f * this_sample - 1.0f) * vg;
            }
            o->s_freq = vf;
            o->s_gain = vg;
            o->s_next = next;
            o->s_phase = ph;
        }
        {   /* the sine (stmlib FastSineOscillator, ADDITIVE); its state runs on without AUX as well */
            float fr = f, am = amplitude, fp, x = o->x, y = o->y, ve = o->eps, va = o->amp, ei, ai, norm;
            if (fr >= 0.25f) {
                fr = 0.25f;
                am = 0.0f;
            } else {
                am *= 1.0f - fr * 4.0f;
            }
            fp = fr * (float)PPA_PI;
            ei = (fp * (2.0f - (2.0f * 0.96f / 6.0f) * fp * fp) - ve) / fn;   /* Fast2Sin */
            ai = (am - va) / fn;
            norm = x * x + y * y;
            if (norm <= 0.5f || norm >= 2.0f) {
                const float sc = psw_rsqrt(norm);
                x *= sc;
                y *= sc;
            }
            if (need & 2u) {
                for (i = 0; i < n; i++) {
                    ve += ei;
                    x += ve * y;
                    y -= ve * x;
                    va += ai;
                    aux[i] += va * x;
                }
            } else {
                for (i = 0; i < n; i++) {
                    ve += ei;
                    x += ve * y;
                    y -= ve * x;
                }
                va = am;                           /* (its ramp's end, within rounding) */
            }
            o->x = x;
            o->y = y;
            o->eps = ve;
            o->amp = va;
        }
        size_ratio *= 0.97f;
    }
}

/* Plaits' parameters: density from TIMBRE (rate_k: the module's sample rate over ours, the grains a second), spread
 * from HARMONICS, size_ratio from MORPH, for blocks of n */
FM1_FPU static inline void psw_params(float timbre, float morph, float harmonics, uint32_t n, float rate_k,
                                      float *density, float *spread, float *size_ratio)
{
    *density = pun_note(timbre * 120.0f) * 0.025f * (float)n * rate_k;
    *spread = harmonics * harmonics * harmonics;
    *size_ratio = 0.25f * pun_ratio((1.0f - morph) * 84.0f);
}
