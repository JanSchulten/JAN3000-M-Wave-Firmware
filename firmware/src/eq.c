/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: the track EQ (HOME > EQ), four bands a track after its INSERT, before LEVEL, PAN and the sends (fx.c
 * mix_part, mix_spread: a SPREAD part's side signal too, its own state). Included by fx.c.
 * A band is an RBJ cookbook biquad (R. Bristow-Johnson, "Cookbook formulae for audio EQ biquad filter coefficients"):
 * LOW / HIGH SHELF (slope 1), BELL (Q 1), LP / HP (Q its RESO), on the chip's FPU in single float (transposed direct form
 * II; a float's 24 bits hold a 20 Hz shelf, the integer SVF's Q12 would not). The settings are the main loop's (params.c
 * eq_set); eq_poll turns what changed into coefficients (the table terms of gen_tables.py: no sine, power or root) and
 * hands a track's set to the ISR through eqx.req as the scenes do theirs: the main loop writes eqx.next[k] only while
 * req[k] is 0, the ISR takes it at a block's start and clears req[k]. The ISR glides each coefficient a quarter of the
 * way a block (~3 ms) to what it took, so a knob turned does not click; a band flat (a shelf or a bell at 0 dB) and
 * there is skipped, a track with every band flat (the default, every older project) costs one load a block. A part
 * that falls silent (fx.c: its tail run out) drops its state at once: the next note starts from rest, as INSERT's does */
typedef struct { float b0, b1, b2, a1, a2; } eq_coef_t;
typedef struct {
    eq_coef_t c[EQ_NB];
    uint8_t on;                                  /* bit b: band b is not flat */
} eq_trk_t;
static volatile struct {
    uint8_t req[NTRK];                           /* main: next[k] waits; ISR: taken (0) */
    eq_trk_t next[NTRK];                         /* main: a track's coefficients as the settings say */
} eqx;
static struct {                                  /* the ISR's */
    eq_trk_t tgt[NTRK];                          /* what it glides to */
    eq_coef_t cur[NTRK][EQ_NB];                  /* what it plays */
    float z[NTRK][2][EQ_NB][2];                  /* the state: the part's mono sum, a SPREAD part's side */
    uint8_t run[NTRK];                           /* bit b: band b plays (not flat, or gliding back to flat) */
    uint8_t live[NTRK];                          /* the state holds something (dropped when the part falls silent) */
} eqa;
static eq_band_t eq_was[NTRK][EQ_NB];            /* main: the settings eq_poll handed over last */
static uint8_t eq_sent;                          /* main: bit k: track k's handed over once (power-on: none) */

static const eq_coef_t EQ_FLAT = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
/* band b's flat: a shelf or a bell at 0 dB */
static int eq_flat(const eq_band_t *b) { return !eq_pass(b) && !b->g; }
/* the coefficients of band b (main loop; a0 normalised to 1) */
FM1_FPU static void eq_coef(eq_coef_t *c, const eq_band_t *b)
{
    int32_t f = clamp(b->f, 0, EQ_NF - 1), g = clamp(b->g, -24, 24) + 24;
    float cs = EQ_COS[f], sn = EQ_SIN[f], a = EQ_A[g], sa = EQ_SQA[g], al, a0, k;
    if (eq_flat(b)) {
        *c = EQ_FLAT;
        return;
    }
    switch (b->ty) {
    case EQT_LP:
    case EQT_HP:
        al = sn / (2.0f * EQ_QV[clamp(b->q, 0, EQ_NQ - 1)]);
        a0 = 1.0f / (1.0f + al);
        k = b->ty == EQT_LP ? (1.0f - cs) * 0.5f : (1.0f + cs) * 0.5f;
        c->b0 = k * a0;
        c->b1 = (b->ty == EQT_LP ? 2.0f * k : -2.0f * k) * a0;
        c->b2 = c->b0;
        c->a1 = -2.0f * cs * a0;
        c->a2 = (1.0f - al) * a0;
        break;
    case EQT_BELL:
        al = sn * 0.5f;                          /* (Q 1) */
        a0 = 1.0f / (1.0f + al / a);
        c->b0 = (1.0f + al * a) * a0;
        c->b1 = -2.0f * cs * a0;
        c->b2 = (1.0f - al * a) * a0;
        c->a1 = c->b1;
        c->a2 = (1.0f - al / a) * a0;
        break;
    default: {                                   /* the shelves, slope 1: 2 sqrt(A) alpha = sqrt(A) sin w0 sqrt 2 */
        float am = a - 1.0f, ap = a + 1.0f, s2 = 1.41421356f * sa * sn;
        if (b->ty == EQT_LSHF) {
            a0 = 1.0f / (ap + am * cs + s2);
            c->b0 = a * (ap - am * cs + s2) * a0;
            c->b1 = 2.0f * a * (am - ap * cs) * a0;
            c->b2 = a * (ap - am * cs - s2) * a0;
            c->a1 = -2.0f * (am + ap * cs) * a0;
            c->a2 = (ap + am * cs - s2) * a0;
        } else {
            a0 = 1.0f / (ap - am * cs + s2);
            c->b0 = a * (ap + am * cs + s2) * a0;
            c->b1 = -2.0f * a * (am + ap * cs) * a0;
            c->b2 = a * (ap + am * cs - s2) * a0;
            c->a1 = 2.0f * (am - ap * cs) * a0;
            c->a2 = (ap - am * cs - s2) * a0;
        }
        break;
    }
    }
}
/* main loop, every pass (ui_input.c, and a project loaded): a track whose settings changed since it was handed over
 * gets its coefficients to the ISR, once it took the last ones */
static void eq_poll(void)
{
    uint32_t k, b;
    for (k = 0; k < NTRK; k++) {
        eq_trk_t *n = (eq_trk_t *)&eqx.next[k];
        if (((eq_sent >> k) & 1u) && !memcmp(eq_was[k], eq_set[k], sizeof eq_was[k]))
            continue;
        if (eqx.req[k])
            continue;                            /* (the ISR has not taken the last set: next pass) */
        n->on = 0;
        for (b = 0; b < EQ_NB; b++) {
            eq_coef(&n->c[b], &eq_set[k][b]);
            n->on |= (uint8_t)(!eq_flat(&eq_set[k][b]) << b);
        }
        memcpy(eq_was[k], eq_set[k], sizeof eq_was[k]);
        eq_sent |= (uint8_t)(1u << k);
        RING_PUBLISH();                          /* (the set written before the flag) */
        eqx.req[k] = 1;
    }
}
/* as at power-on (the host tests' ui_power_on; the target starts so): every track flat, nothing handed over */
static void eq_power_on(void)
{
    eq_defaults();
    eq_ui_band = 0;
    memset((void *)&eqx, 0, sizeof eqx);
    memset(&eqa, 0, sizeof eqa);
    memset(eq_was, 0, sizeof eq_was);
    eq_sent = 0;
}

/* ISR, a block of track k (sounding): a set waiting taken; the coefficients a quarter nearer it; a band done gliding
 * back to flat stops. noinline: the call costs one test a block while the EQ is flat (fx.c) */
FM1_FPU static void eq_glide(uint32_t k)
{
    uint32_t b;
    if (eqx.req[k]) {
        memcpy(&eqa.tgt[k], (const void *)&eqx.next[k], sizeof eqa.tgt[k]);
        eqx.req[k] = 0;
        eqa.run[k] |= eqa.tgt[k].on;
        if (!eqa.live[k])                        /* (from rest: no glide, the set at once) */
            memcpy(eqa.cur[k], eqa.tgt[k].c, sizeof eqa.cur[k]);
    }
    for (b = 0; b < EQ_NB; b++) {
        eq_coef_t *c = &eqa.cur[k][b];
        const eq_coef_t *t = &eqa.tgt[k].c[b];
        float d0, d1, d2, e1, e2;
        if (!((eqa.run[k] >> b) & 1u))
            continue;
        d0 = t->b0 - c->b0, d1 = t->b1 - c->b1, d2 = t->b2 - c->b2, e1 = t->a1 - c->a1, e2 = t->a2 - c->a2;
        if ((d0 < 0 ? -d0 : d0) + (d1 < 0 ? -d1 : d1) + (d2 < 0 ? -d2 : d2) + (e1 < 0 ? -e1 : e1) +
            (e2 < 0 ? -e2 : e2) < 1e-5f) {                         /* there: a band flat at the end of its way stops */
            *c = *t;
            if (!((eqa.tgt[k].on >> b) & 1u)) {
                eqa.run[k] &= (uint8_t)~(1u << b);
                eqa.z[k][0][b][0] = eqa.z[k][0][b][1] = eqa.z[k][1][b][0] = eqa.z[k][1][b][1] = 0.0f;
            }
            continue;
        }
        c->b0 += d0 * 0.25f;
        c->b1 += d1 * 0.25f;
        c->b2 += d2 * 0.25f;
        c->a1 += e1 * 0.25f;
        c->a2 += e2 * 0.25f;
    }
}
/* the bands that run on n samples of x (ch 0: the mono sum, 1: a SPREAD part's side) */
FM1_FPU static void eq_run(uint32_t k, uint32_t ch, int32_t *x, uint32_t n)
{
    uint32_t b, i;
    for (b = 0; b < EQ_NB; b++) {
        const eq_coef_t c = eqa.cur[k][b];
        float z1, z2;
        if (!((eqa.run[k] >> b) & 1u))
            continue;
        z1 = eqa.z[k][ch][b][0];
        z2 = eqa.z[k][ch][b][1];
        for (i = 0; i < n; i++) {
            float in = (float)x[i], y = c.b0 * in + z1;
            z1 = c.b1 * in - c.a1 * y + z2;
            z2 = c.b2 * in - c.a2 * y;
            x[i] = y > 8.0e5f ? 800000 : y < -8.0e5f ? -800000 : (int32_t)y;   /* (mix_part's (b >> 2) * LEVEL fits) */
        }
        eqa.z[k][ch][b][0] = z1;
        eqa.z[k][ch][b][1] = z2;
    }
}
/* fx.c, a sounding part's block after its INSERT: b its mono sum, d a SPREAD part's side (0: none) */
static __attribute__((noinline)) void track_eq(const track_t *t, int32_t *b, int32_t *d, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk);
    eq_glide(k);
    if (!eqa.run[k])
        return;
    eqa.live[k] = 1;
    eq_run(k, 0, b, n);
    if (d)
        eq_run(k, 1, d, n);
}
/* fx.c, a part fallen silent (its tail run out): the state dropped, the coefficients where they were going */
static __attribute__((noinline)) void eq_rest(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk);
    if (eqx.req[k]) {
        memcpy(&eqa.tgt[k], (const void *)&eqx.next[k], sizeof eqa.tgt[k]);
        eqx.req[k] = 0;
    }
    memcpy(eqa.cur[k], eqa.tgt[k].c, sizeof eqa.cur[k]);
    memset(eqa.z[k], 0, sizeof eqa.z[k]);
    eqa.run[k] = eqa.tgt[k].on;
    eqa.live[k] = 0;
}
