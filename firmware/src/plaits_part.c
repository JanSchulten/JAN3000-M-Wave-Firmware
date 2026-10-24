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
/* Plaits' particle model (plaits/dsp/engine/particle_engine.cc) with its particles (plaits/dsp/noise/particle.h), the
 * diffuser after them (plaits/dsp/fx/diffuser.h on fx_engine.h, 12-bit delay memory) and stmlib's state-variable
 * filter, random numbers and pitch ratios, as C in float for the FPU (core.h FM1_FPU on every function here). Bit for
 * bit Plaits' own output on an IEEE single FPU with PDF_FULL (Plaits' delay lengths; tools/plaits_proto/ref_part.cc);
 * the firmware's diffuser has its lengths at 7/10 (it fits a part's engine memory: docs/PLAITS_PLAN.md). The PRTCL
 * engine (eng_part.c) is built around it: the particles per voice, the diffuser once per part (Plaits has one voice).
 *   6 particles: each sample a particle fires with the probability density (TIMBRE), an impulse through its band-pass
 *   at the note, detuned at random up to spread semitones (HARMONICS), of resonance q (MORPH above 0.5); OUT their sum
 *   through a low-pass at the note, AUX the raw impulses; the diffuser (MORPH below 0.5) smears OUT.
 * The random numbers: stmlib's generator, its state the caller's (Plaits has one for all). Uses plaits_ws.c's
 * helpers. */
#include "stmlib_tables.h"

#define PPA_N 6                                /* particles a voice (kNumParticles) */
#define PPA_PI 3.14159265358979323846          /* (M_PI: the filter's constants as stmlib computes them) */

FM1_FPU static inline float pun_ratio(float semitones)   /* stmlib SemitonesToRatio: -128 .. 127 */
{
    float pitch = semitones + 128.0f;
    const int32_t pi = (int32_t)pitch;
    const float pf = pitch - (float)pi;
    return lut_pitch_ratio_high[pi] * lut_pitch_ratio_low[(int32_t)(pf * 256.0f)];
}
FM1_FPU static inline float pun_note(float note)          /* plaits NoteToFrequency (the module's sample rate) */
{
    note -= 9.0f;
    note = note < -128.0f ? -128.0f : note > 127.0f ? 127.0f : note;
    return (55.0f / 47872.34f) * 0.25f * pun_ratio(note);
}
FM1_FPU static inline float pun_rand(uint32_t *st)         /* stmlib Random::GetFloat */
{
    uint32_t w = *st = *st * 1664525u + 1013904223u;
    /* (float)w / 2^32 without an unsigned conversion: the halves exactly, their sum rounded once, as (float)w */
    return ((float)(int32_t)(w >> 16) * 65536.0f + (float)(int32_t)(w & 0xFFFFu)) * (1.0f / 4294967296.0f);
}
/* sqrtf of a positive normal float, rounded to nearest as IEEE's (no libm here): the mantissa's integer root */
static float pun_sqrt(float x)
{
    union { float f; uint32_t u; } v;
    uint64_t a, r = 0, b;
    int32_t e;
    v.f = x;
    e = (int32_t)(v.u >> 23) - 127;
    a = (uint64_t)((v.u & 0x7FFFFFu) | 0x800000u);
    if (e & 1) {
        a <<= 1;
        e--;
    }
    a <<= 23;                                  /* sqrt(a) in [2^23, 2^25): the result's 24 bits and one more */
    for (b = (uint64_t)1 << 48; b > a; b >>= 2)
        ;
    for (; b; b >>= 2) {
        if (a >= r + b) {
            a -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
    }
    if (a > r)                                 /* the remainder past half an ulp: up (no exact half exists) */
        r++;
    if (r >= (1u << 24)) {
        r >>= 1;
        e += 2;
    }
    v.u = (uint32_t)((e / 2 + 127) << 23) | ((uint32_t)r & 0x7FFFFFu);
    return v.f;
}

typedef struct {
    float g, r, h, s1, s2;                     /* stmlib Svf: coefficients, state */
} psvf_t;
FM1_FPU static inline void psvf_set(psvf_t *f, float fr, float q)   /* set_f_q<FREQUENCY_DIRTY> */
{
    const float a = (float)(3.736e-01f * PPA_PI * PPA_PI * PPA_PI);
    f->g = fr * ((float)PPA_PI + a * fr * fr);
    f->r = 1.0f / q;
    f->h = 1.0f / (1.0f + f->r * f->g + f->g * f->g);
}
FM1_FPU static inline void psvf_init(psvf_t *f)
{
    psvf_set(f, 0.01f, 100.0f);
    f->s1 = f->s2 = 0.0f;
}

typedef struct {
    float pre_gain;
    psvf_t f;
} ppt_t;
typedef struct {
    ppt_t p[PPA_N];
    psvf_t post;                               /* the low-pass at the note */
} ppa_t;                                       /* 164 bytes */

typedef struct {
    float density, gain, q, spread, sd;        /* sd: sqrt(density) */
    float diffusion;                           /* 0 .. 1 (MORPH below 0.5) */
} ppa_par_t;

FM1_FPU static void ppa_init(ppa_t *s)
{
    uint32_t i;
    for (i = 0; i < PPA_N; i++) {
        s->p[i].pre_gain = 0.0f;
        psvf_init(&s->p[i].f);
    }
    psvf_init(&s->post);
}

/* Plaits' parameters for TIMBRE MORPH HARMONICS; rate_k scales the density (1: Plaits' own; the firmware's sample rate
 * against the module's keeps the particles a second) */
FM1_FPU static void ppa_params(ppa_par_t *q, float timbre, float morph, float harmonics, float rate_k)
{
    const float ds = pun_note(60.0f + timbre * timbre * 72.0f);
    const float qs = pun_ratio(morph >= 0.5f ? (morph - 0.5f) * 120.0f : 0.0f);
    const float rd = 2.0f * pws_abs(morph - 0.5f);
    q->density = ds * ds * (1.0f / (float)PPA_N) * rate_k;
    q->gain = 1.0f / q->density;
    q->q = 0.5f + qs * qs;
    q->spread = 48.0f * harmonics * harmonics;
    q->diffusion = morph < 0.5f ? rd * rd : 0.0f;
    q->sd = pun_sqrt(q->density);
}

/* one block of n samples into out (OUT before the diffuser) and aux (AUX); sync: every particle fires at the first
 * sample (Plaits' trigger); need bit 1: aux (without it aux is left alone) */
FM1_FPU static void ppa_render(ppa_t *s, uint32_t *rng, int sync, const ppa_par_t *q, float f0, float *out, float *aux,
                               uint32_t n, uint32_t need)
{
    uint32_t i, k;
    for (i = 0; i < n; i++)
        out[i] = 0.0f;
    if (need & 2u)
        for (i = 0; i < n; i++)
            aux[i] = 0.0f;
    for (k = 0; k < PPA_N; k++) {
        ppt_t *pt = &s->p[k];
        float u = pun_rand(rng), g = pt->f.g, r = pt->f.r, h = pt->f.h, s1 = pt->f.s1, s2 = pt->f.s2;
        int can = 1;
        if (sync)
            u = q->density;
        for (i = 0; i < n; i++) {
            float x = 0.0f, hp, bp;
            if (u <= q->density) {
                x = u * q->gain;
                if (can) {
                    const float uu = 2.0f * pun_rand(rng) - 1.0f;
                    float fr = pun_ratio(q->spread * uu) * f0;
                    fr = fr < 0.25f ? fr : 0.25f;
                    pt->pre_gain = 0.5f / pun_sqrt(q->q * fr * q->sd);
                    psvf_set(&pt->f, fr, q->q);
                    g = pt->f.g;
                    r = pt->f.r;
                    h = pt->f.h;
                    can = 0;
                }
            }
            if (need & 2u)
                aux[i] += x;
            hp = (pt->pre_gain * x - r * s1 - g * s1 - s2) * h;   /* the band-pass */
            bp = g * hp + s1;
            s1 = g * hp + bp;
            s2 = g * bp + (g * bp + s2);
            out[i] += bp;
            u = pun_rand(rng);
        }
        pt->f.s1 = s1;
        pt->f.s2 = s2;
    }
    psvf_set(&s->post, f0 < 0.49f ? f0 : 0.49f, 0.5f);   /* the low-pass at the note */
    {
        float g = s->post.g, r = s->post.r, h = s->post.h, s1 = s->post.s1, s2 = s->post.s2;
        for (i = 0; i < n; i++) {
            float hp = (out[i] - r * s1 - g * s1 - s2) * h, bp = g * hp + s1, lp;
            s1 = g * hp + bp;
            lp = g * bp + s2;
            s2 = g * bp + lp;
            out[i] = lp;
        }
        s->post.s1 = s1;
        s->post.s2 = s2;
    }
}

/* the diffuser: four all-passes, a modulated one, a delay with a low-pass and two more all-passes in its loop. Its
 * delay memory: 12-bit values (x 4096, in 16 bits), a ring of PDF_SIZE */
#ifdef PDF_FULL
#define PDF_L(n) (n)                           /* (Plaits' lengths: the reference test) */
#define PDF_SIZE 8192
#define PDF_K 1.0f
#else
#define PDF_L(n) ((n) * 7 / 10)
#define PDF_K 0.7f
#endif
#define PDF_B1 0
#define PDF_B2 (PDF_B1 + PDF_L(126) + 1)
#define PDF_B3 (PDF_B2 + PDF_L(180) + 1)
#define PDF_B4 (PDF_B3 + PDF_L(269) + 1)
#define PDF_BA (PDF_B4 + PDF_L(444) + 1)
#define PDF_BB (PDF_BA + PDF_L(1653) + 1)
#define PDF_BD (PDF_BB + PDF_L(2010) + 1)
#ifndef PDF_SIZE
#define PDF_SIZE (PDF_BD + PDF_L(3411) + 1)    /* 5670 */
#endif
typedef char pdf_fits[PDF_BD + PDF_L(3411) + 1 <= PDF_SIZE ? 1 : -1];   /* (the lines fit the ring) */

typedef struct {
    int32_t wp;                                /* the write position, counting down */
    float lp;                                  /* the loop's low-pass */
    float y1, y0, c;                           /* the LFO (stmlib CosineOscillator, approximate), 0.3 Hz */
    float lfo;                                 /* its value */
    int16_t buf[PDF_SIZE];
} pdf_t;

/* a cleared diffuser; fs the sample rate (the LFO's 0.3 Hz) */
FM1_FPU static void pdf_init(pdf_t *d, float fs)
{
    float fr = 0.3f / fs * 32.0f, sign = 16.0f;
    uint32_t i;
    fr -= 0.25f;
    if (fr < 0.0f) {
        fr = -fr;
    } else if (fr > 0.5f) {
        fr -= 0.5f;
    } else {
        sign = -16.0f;
    }
    d->c = sign * fr * (1.0f - 2.0f * fr);
    d->y1 = d->c * 0.25f;
    d->y0 = 0.5f;
    d->lfo = 0.0f;
    d->lp = 0.0f;
    d->wp = 0;
    for (i = 0; i < PDF_SIZE; i++)
        d->buf[i] = 0;
}

FM1_FPU static inline float pdf_rd(const pdf_t *d, int32_t wp, int32_t at)   /* (wp + at) on the ring */
{
    int32_t i = wp + at;
    if (i >= PDF_SIZE)
        i -= PDF_SIZE;
    return (float)d->buf[i] / 4096.0f;
}
FM1_FPU static inline void pdf_wr(pdf_t *d, int32_t wp, int32_t at, float x)
{
    int32_t i = wp + at, w = (int32_t)(x * 4096.0f);
    if (i >= PDF_SIZE)
        i -= PDF_SIZE;
    d->buf[i] = (int16_t)(w < -32768 ? -32768 : w > 32767 ? 32767 : w);
}
FM1_FPU static inline float pdf_interp(const pdf_t *d, int32_t wp, int32_t base, float offset)
{
    const int32_t oi = (int32_t)offset;
    const float of = offset - (float)oi, a = pdf_rd(d, wp, oi + base), b = pdf_rd(d, wp, oi + base + 1);
    return a + (b - a) * of;
}

/* n samples of io through the diffuser, mixed in by amount; rt its loop's gain */
FM1_FPU static void pdf_process(pdf_t *d, float amount, float rt, float *io, uint32_t n)
{
    const float kap = 0.625f, klp = 0.75f;
    float lp = d->lp, lfo = d->lfo;
    int32_t wp = d->wp;
    while (n--) {
        float acc, prev, wet;
        if (--wp < 0)
            wp += PDF_SIZE;
        if ((wp & 31) == 0) {                  /* the LFO, every 32 samples (FxEngine::Start) */
            const float y = d->y0;
            d->y0 = d->c * d->y0 - d->y1;
            d->y1 = y;
            lfo = y + 0.5f;
        } else {
            lfo = d->y1 + 0.5f;
        }
        acc = *io;
        prev = pdf_rd(d, wp, PDF_B1 + PDF_L(126) - 1);   /* ap1 .. ap3 */
        acc += prev * kap;
        pdf_wr(d, wp, PDF_B1, acc);
        acc = acc * -kap + prev;
        prev = pdf_rd(d, wp, PDF_B2 + PDF_L(180) - 1);
        acc += prev * kap;
        pdf_wr(d, wp, PDF_B2, acc);
        acc = acc * -kap + prev;
        prev = pdf_rd(d, wp, PDF_B3 + PDF_L(269) - 1);
        acc += prev * kap;
        pdf_wr(d, wp, PDF_B3, acc);
        acc = acc * -kap + prev;
        prev = pdf_interp(d, wp, PDF_B4, 400.0f * PDF_K + 43.0f * PDF_K * lfo);   /* ap4, modulated */
        acc += prev * kap;
        pdf_wr(d, wp, PDF_B4, acc);
        acc = acc * -kap + prev;
        acc += pdf_interp(d, wp, PDF_BD, 3070.0f * PDF_K + 340.0f * PDF_K * lfo) * rt;   /* the loop's delay */
        lp += klp * (acc - lp);
        acc = lp;
        prev = pdf_rd(d, wp, PDF_BA + PDF_L(1653) - 1);   /* dapa, dapb */
        acc += prev * -kap;
        pdf_wr(d, wp, PDF_BA, acc);
        acc = acc * kap + prev;
        prev = pdf_rd(d, wp, PDF_BB + PDF_L(2010) - 1);
        acc += prev * kap;
        pdf_wr(d, wp, PDF_BB, acc);
        acc = acc * -kap + prev;
        pdf_wr(d, wp, PDF_BD, acc);
        wet = acc * 2.0f;                      /* (Write(del, 2.0f): the wet output after the gain) */
        *io += amount * (wet - *io);
        io++;
    }
    d->lp = lp;
    d->lfo = lfo;
    d->wp = wp;
}
