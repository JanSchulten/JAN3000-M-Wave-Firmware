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
/* Plaits' wavetable model (plaits/dsp/engine/wavetable_engine.cc, its Hermite reads and differentiator from
 * plaits/dsp/oscillator/wavetable_oscillator.h), as C in float for the FPU (core.h FM1_FPU on every function here). Bit
 * for bit Plaits' own on an IEEE single FPU with PWT_FULL (Plaits' 16-bit tables: tools/plaits_proto/ref_wt.cc). The
 * firmware's tables are 8 bits (plaits_wt8.h, 25 KB instead of 50: docs/PLAITS_PLAN.md), each wave decoded into a
 * part's cache when a voice needs it (pwc_t). The WTABLE engine (eng_wtable.c) is built around it.
 *   192 integrated waves of 128 samples (+4 for the reads), three banks of 8 x 8 and a fourth of them shuffled (Plaits'
 *   bank D without user data); TIMBRE the row, MORPH the column, HARMONICS the bank: below its middle the waves blend,
 *   above it the banks come back in reverse and the steps between waves grow sharp (quantization)
 *   OUT the blend's derivative (the integrated tables differentiated: their band limit), AUX OUT at 5 bits
 * The parameters' block smoothing (pre_lp: Plaits' 0.2 and 0.05 a 12-sample block) is the caller's (pwt_par_t). */
#define PWT_N 192                              /* waves */
#define PWT_LEN 132                            /* a wave's int16 values (128 + 4) */

typedef struct {
    float phase;
    float xp, yp, zp;                          /* the parameters smoothed once a block (x_pre_lp_ ..) */
    float xl, yl, zl;                          /* .. and every sample (x_lp_ ..) */
    float px, py, pz, pf0;                     /* the ramps' ends (previous_x_ .., previous_f0_) */
    float dlp, dprev;                          /* the differentiator */
} pwt_t;                                       /* 52 bytes */

typedef struct {
    float timbre, morph, harmonics, f0;
    float cxy, cz;                             /* the block smoothing: Plaits' 0.2 (x, y) and 0.05 (z) */
} pwt_par_t;

#ifdef PWT_FULL
/* the reference: Plaits' own 16-bit table (PWT_TABLE, the caller's) */
#define PWT_WAVE(src, w) (PWT_TABLE + (size_t)(w) * PWT_LEN)
#else
#include "plaits_wt8.h"
#define PWC_SLOTS 44                           /* decoded waves a part keeps (its voices use 32 at the most) */
typedef struct {
    uint8_t map[PWT_N];                        /* wave -> its slot + 1, 0 none */
    uint8_t who[PWC_SLOTS];                    /* slot -> its wave + 1, 0 none */
    uint16_t age[PWC_SLOTS];                   /* the block it was last read */
    uint16_t now;                              /* the part's blocks */
    int16_t w[PWC_SLOTS][PWT_LEN];
} pwc_t;

/* wave w decoded (from the cache, or into its stalest slot) */
static const int16_t *pwc_load(pwc_t *c, uint32_t w)
{
    uint32_t s, best = 0, k;
    uint16_t oldest = 0;
    int32_t v;
    for (s = 0; s < PWC_SLOTS; s++) {          /* an empty slot, else the one read longest ago */
        const uint16_t a = c->who[s] ? (uint16_t)(c->now - c->age[s]) : 0xFFFFu;
        if (a > oldest || s == 0) {
            oldest = a;
            best = s;
        }
        if (!c->who[s])
            break;
    }
#ifdef PWC_COUNT
    PWC_COUNT++;                               /* (tests/wtable_test.c: the decodes) */
#endif
    if (c->who[best])
        c->map[c->who[best] - 1u] = 0;
    c->who[best] = (uint8_t)(w + 1u);
    c->map[w] = (uint8_t)(best + 1u);
    c->age[best] = c->now;
    v = pwt_first[w];
    c->w[best][0] = (int16_t)v;
    for (k = 1; k < PWT_LEN; k++) {
        v += PWT_GRID * pwt_steps[w][k - 1];
        c->w[best][k] = (int16_t)v;
    }
    return c->w[best];
}
__attribute__((always_inline)) static inline const int16_t *pwc_get(pwc_t *c, uint32_t w)
{
    const uint32_t s = c->map[w];
    if (!s)
        return pwc_load(c, w);
    c->age[s - 1u] = c->now;
    return c->w[s - 1u];
}
#define PWT_WAVE(src, w) pwc_get((pwc_t *)(src), (w))
#endif

static const uint8_t PWT_SHUF[64] = {          /* bank D: wave i (192 .. 255) is i * 101 % 192 (LoadUserData) */
    0, 101, 10, 111, 20, 121, 30, 131, 40, 141, 50, 151, 60, 161, 70, 171,
    80, 181, 90, 191, 100, 9, 110, 19, 120, 29, 130, 39, 140, 49, 150, 59,
    160, 69, 170, 79, 180, 89, 190, 99, 8, 109, 18, 119, 28, 129, 38, 139,
    48, 149, 58, 159, 68, 169, 78, 179, 88, 189, 98, 7, 108, 17, 118, 27,
};
__attribute__((always_inline)) static inline uint32_t pwt_index(int32_t x, int32_t y, int32_t z)   /* wave_map_ */
{
    const uint32_t i = (uint32_t)(x + y * 8 + z * 64);
    return i < PWT_N ? i : PWT_SHUF[(i - PWT_N) & 63u];
}

FM1_FPU static inline float pwt_hermite(const int16_t *t, int32_t i, float f)   /* InterpolateWaveHermite */
{
    const float xm1 = (float)t[i], x0 = (float)t[i + 1], x1 = (float)t[i + 2], x2 = (float)t[i + 3];
    const float c = (x1 - xm1) * 0.5f, v = x0 - x1, w = c + v, a = w + v + (x2 - x0) * 0.5f, b_neg = w + a;
    return (((a * f) - b_neg) * f + c) * f + x0;
}
FM1_FPU static inline float pwt_clamp(float x, float amount)
{
    x = x - 0.5f;
    x *= amount;
    x = x < -0.5f ? -0.5f : x > 0.5f ? 0.5f : x;
    return x + 0.5f;
}

/* the 8 waves around the integer parts xi yi zi (wave_map_, bank D mirrored back from zi 4 up) into w */
FM1_FPU static inline void pwt_waves(void *src, int32_t xi, int32_t yi, int32_t zi, const int16_t **w)
{
    int32_t z0 = zi, z1 = zi + 1;
    if (z0 >= 4)
        z0 = 7 - z0;
    if (z1 >= 4)
        z1 = 7 - z1;
    w[0] = PWT_WAVE(src, pwt_index(xi, yi, z0));
    w[1] = PWT_WAVE(src, pwt_index(xi + 1, yi, z0));
    w[2] = PWT_WAVE(src, pwt_index(xi, yi + 1, z0));
    w[3] = PWT_WAVE(src, pwt_index(xi + 1, yi + 1, z0));
    w[4] = PWT_WAVE(src, pwt_index(xi, yi, z1));
    w[5] = PWT_WAVE(src, pwt_index(xi + 1, yi, z1));
    w[6] = PWT_WAVE(src, pwt_index(xi, yi + 1, z1));
    w[7] = PWT_WAVE(src, pwt_index(xi + 1, yi + 1, z1));
}

/* the trilinear blend of the 8 waves w at the fractions xf yf zf, at the table position p (Plaits' order) */
FM1_FPU static inline float pwt_blend(const int16_t *const *w, float xf, float yf, float zf, int32_t pi, float pf)
{
    float a, b, xy0, xy1, xyz0, xyz1;
    a = pwt_hermite(w[0], pi, pf);
    b = pwt_hermite(w[1], pi, pf);
    xy0 = a + (b - a) * xf;
    a = pwt_hermite(w[2], pi, pf);
    b = pwt_hermite(w[3], pi, pf);
    xy1 = a + (b - a) * xf;
    xyz0 = xy0 + (xy1 - xy0) * yf;
    a = pwt_hermite(w[4], pi, pf);
    b = pwt_hermite(w[5], pi, pf);
    xy0 = a + (b - a) * xf;
    a = pwt_hermite(w[6], pi, pf);
    b = pwt_hermite(w[7], pi, pf);
    xy1 = a + (b - a) * xf;
    xyz1 = xy0 + (xy1 - xy0) * yf;
    return xyz0 + (xyz1 - xyz0) * zf;
}

FM1_FPU static inline float pwt_mix(void *src, float x, float y, float z, int32_t pi, float pf)   /* at x y z */
{
    const int32_t xi = (int32_t)x, yi = (int32_t)y, zi = (int32_t)z;
    const int16_t *w[8];
    pwt_waves(src, xi, yi, zi, w);
    return pwt_blend(w, x - (float)xi, y - (float)yi, z - (float)zi, pi, pf);
}

/* the block's targets of x y z (Plaits' quantization above the middle of HARMONICS), from the smoothed parameters */
FM1_FPU static inline void pwt_targets(const pwt_t *s, float *tx, float *ty, float *tz, float *quant)
{
    const float x = s->xp, y = s->yp, z = s->zp;
    const float q = z - 3.0f < 0.0f ? 0.0f : z - 3.0f > 1.0f ? 1.0f : z - 3.0f;
    const int32_t xi = (int32_t)x, yi = (int32_t)y, zi = (int32_t)z;
    float xf = x - (float)xi, yf = y - (float)yi, zf = z - (float)zi;
    xf += q * (pwt_clamp(xf, 16.0f) - xf);
    yf += q * (pwt_clamp(yf, 16.0f) - yf);
    zf += q * (pwt_clamp(zf, 16.0f) - zf);
    *tx = (float)xi + xf;
    *ty = (float)yi + yf;
    *tz = (float)zi + zf;
    *quant = q;
}

FM1_FPU static void pwt_init(pwt_t *s)                    /* Plaits' Init */
{
    s->phase = 0.0f;
    s->xp = s->yp = s->zp = s->xl = s->yl = s->zl = 0.0f;
    s->px = s->py = s->pz = 0.0f;
    s->pf0 = (55.0f / 47872.34f);
    s->dlp = s->dprev = 0.0f;
}

/* a voice from rest: at phase, its parameters at their targets (no sweep into them), the differentiator at the blend
 * (no click from 0) */
FM1_FPU static void pwt_start(pwt_t *s, void *src, float phase, const pwt_par_t *q)
{
    float tx, ty, tz, quant, p;
    int32_t pi;
    pwt_init(s);
    s->phase = phase;
    s->xp = q->timbre * 6.9999f;
    s->yp = q->morph * 6.9999f;
    s->zp = q->harmonics * 6.9999f;
    pwt_targets(s, &tx, &ty, &tz, &quant);
    s->xl = s->px = tx;
    s->yl = s->py = ty;
    s->zl = s->pz = tz;
    s->pf0 = q->f0;
    p = phase * 128.0f;
    pi = (int32_t)p;
    s->dprev = pwt_mix(src, tx, ty, tz, pi, p - (float)pi);
}

/* one block of n samples into out (OUT) and aux (AUX, when need bit 1) */
FM1_FPU static void pwt_render(pwt_t *s, void *src, const pwt_par_t *q, float *out, float *aux, uint32_t n,
                               uint32_t need)
{
    float tx, ty, tz, quant, lpc, ix, iy, iz, if0, vx, vy, vz, vf0;
    const float fn = (float)n;
    const int16_t *w[8];
    int32_t wx = -1, wy = -1, wz = -1;                     /* the waves' integer parts: fetched once a block and when */
    uint32_t i;                                             /* x y z cross one (not 8 lookups a sample) */
    s->xp += q->cxy * (q->timbre * 6.9999f - s->xp);       /* (ONE_POLE) */
    s->yp += q->cxy * (q->morph * 6.9999f - s->yp);
    s->zp += q->cz * (q->harmonics * 6.9999f - s->zp);
    pwt_targets(s, &tx, &ty, &tz, &quant);
    lpc = 2.0f * q->f0 * (4.0f - 3.0f * quant);
    lpc = lpc > 0.01f ? lpc : 0.01f;
    lpc = lpc < 0.1f ? lpc : 0.1f;
    vx = s->px;                                             /* the ramps (stmlib ParameterInterpolator) */
    vy = s->py;
    vz = s->pz;
    vf0 = s->pf0;
    ix = (tx - vx) / fn;
    iy = (ty - vy) / fn;
    iz = (tz - vz) / fn;
    if0 = (q->f0 - vf0) / fn;
    for (i = 0; i < n; i++) {
        float f0, gain, cutoff, p, mix;
        int32_t pi;
        vf0 += if0;
        f0 = vf0;
        gain = (1.0f / (f0 * 131072.0f)) * (0.95f - f0);
        cutoff = 128.0f * f0 < 1.0f ? 128.0f * f0 : 1.0f;
        vx += ix;
        vy += iy;
        vz += iz;
        s->xl += lpc * (vx - s->xl);
        s->yl += lpc * (vy - s->yl);
        s->zl += lpc * (vz - s->zl);
        s->phase += f0;
        if (s->phase >= 1.0f)
            s->phase -= 1.0f;
        p = s->phase * 128.0f;
        pi = (int32_t)p;
        {
            const int32_t xi = (int32_t)s->xl, yi = (int32_t)s->yl, zi = (int32_t)s->zl;
            if (xi != wx || yi != wy || zi != wz) {
                pwt_waves(src, xi, yi, zi, w);
                wx = xi;
                wy = yi;
                wz = zi;
            }
            mix = pwt_blend(w, s->xl - (float)xi, s->yl - (float)yi, s->zl - (float)zi, pi, p - (float)pi);
        }
        s->dlp += cutoff * ((mix - s->dprev) - s->dlp);   /* the differentiator */
        s->dprev = mix;
        mix = s->dlp * gain;
        out[i] = mix;
        if (need & 2u)
            aux[i] = (float)(int32_t)(mix * 32.0f) / 32.0f;
    }
    s->px = vx;
    s->py = vy;
    s->pz = vz;
    s->pf0 = vf0;
}
