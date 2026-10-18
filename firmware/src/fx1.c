/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* FX1 (M-Vave-OP1): the delay bus as the OP-Z's FX1, of a TYPE (fx1s.type, the FX1 page): DELAY (Felucca's, fx.c
 * fx_buses, untouched) or CLOUD, a granular cloud after Mutable Instruments' Clouds, in the delay's own line (dly_buf:
 * 1.49 s of the send, no RAM of its own). Every track sends into it with its DLY.
 *
 * CLOUD: the send goes into the line as the delay's does (half its level). Up to CL_NG grains read it back: each a
 * piece of SIZE (10 ms .. 1 s, exponential) at PITCH (-24..+24 semitones) through a Hann window (the sine table),
 * between samples (linear), DENS of them a second (0.5 .. 64, exponential; RAND jitters the spacing). SPREAD: where they
 * come from (0: the newest sound, 100: anywhere in the line) and how far apart they sit in the stereo field (as a mid /
 * side pair: the wet bus is mono, the side goes with HALL's to mix_block's rev_side_mix). RAND: each grain detuned at
 * random (quadratic, up to +-1 octave at 100) and up to half of them backwards. The overlap (SIZE x DENS, at most
 * CL_NG) sets the level, 1 / sqrt(overlap), so a dense cloud is as loud as a sparse one; MIX the return. FDBK: the
 * cloud's own sound back into the line (0 .. 0.9, softened by a low-pass): with PITCH, every pass climbs (a shimmer);
 * ECHO THROW lifts it to 0.7 as it does the delay's. CLOUD FREEZE (perform.c PF_CFRZ, a key of the FX layer or a step of
 * an FX TRACK): nothing new goes in while held, the grains go on in what is there.
 * A grain never reads where the line is being written, nor past what has been written since the cloud started
 * (cld.fill): its start is placed between the bounds its length, rate and direction leave (cl_spawn).
 * The TYPE changes in steps a block each: the old type's last block fades out (DELAY: its echoes; CLOUD: its grains),
 * then CL_CLEAR blocks clear the line (2048 samples a block: 23 ms of FX1 silent), then the new type starts in an
 * empty line: no echo of the other one, no click.
 * Cost: CLOUD's work goes by grain and sample (~25 instructions a grain); off (DELAY) only the test in fx_buses. Runs in
 * the audio ISR (fx_buses); the UI writes fx1s. */
#define CL_NG 8u                                 /* grains at most */
#define CL_CLEAR (DLY_LEN / 2048u)               /* blocks a TYPE change clears the line in */
#define CL_EDGE 64u                              /* samples a grain keeps from the write head */
typedef struct {
    uint32_t pos;                                /* where it reads, Q16 samples: wraps with the line (65536 x 65536) */
    int32_t inc;                                 /* .. its step a sample, Q16 (< 0: backwards) */
    uint32_t ph, dph;                            /* its window's phase (a full turn: the grain), its step */
    int32_t p;                                   /* its place in the stereo field, Q15, -0.5 .. 0.5 */
    uint8_t on;
} cl_grain_t;
static struct {
    cl_grain_t g[CL_NG];
    uint8_t type;                                /* the TYPE running (fx1s.type changed: the change, fx1_block) */
    uint8_t busy;                                /* blocks of clearing still to come, 0 none */
    uint8_t side;                                /* sd holds this block's stereo (cloud_side) */
    uint8_t live;                                /* grains sounding (the FX1 / CLOUD pages draw them) */
    uint32_t fill;                               /* samples written since the line was cleared (at most DLY_LEN - 1) */
    uint32_t next;                               /* samples to the next grain */
    uint32_t len, gap;                           /* this block's grain length, the spacing between grains (samples) */
    int32_t lp;                                  /* the feedback's low-pass */
    uint32_t rs;                                 /* its own random numbers (the drums' noise stays as it was) */
    int32_t sd[CTL];                             /* the block's side (left - right) / 2 */
} cld __attribute__((section(".pool")));
static uint32_t cl_rng(void)                     /* xorshift32 (0: started from a fixed seed) */
{
    uint32_t x = cld.rs ? cld.rs : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return cld.rs = x;
}

/* SIZE (0..100) -> samples: 10 ms x 100^(v / 100), 441 .. 44100 */
static uint32_t cl_size(uint32_t v) { return (441u * pow2_q16((int32_t)(v > 100u ? 100u : v) * 1276 / 100)) >> 16; }
/* DENS (0..100) -> samples between grains: 44100 / (0.5 x 2^(7 v / 100)), 88200 .. 689 */
static uint32_t cl_gap(uint32_t v)
{
    uint32_t p = pow2_q16((int32_t)(v > 100u ? 100u : v) * 1344 / 100) >> 8;   /* 2^(7 v / 100), Q8 */
    return 22579200u / (p ? p : 1u);
}
/* the level for an overlap of o / 256 grains (1 .. CL_NG): 1 / sqrt(o), Q15 */
static __attribute__((noinline)) int32_t cl_gain(uint32_t o)
{
    uint32_t r = 0, b = 1u << 20, x = o << 8;    /* sqrt(o x 256) = sqrt(o / 256) x 256, Q8; o <= 2^11 */
    while (b) {                                   /* (bit by bit, a power of 4 above x down: 11 rounds) */
        if (x >= r + b) {
            x -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
        b >>= 2;
    }
    return r ? (int32_t)((32768u * 256u) / r < 32767u ? (32768u * 256u) / r : 32767u) : 32767;
}

/* a new grain, if one is free and the line holds enough for it (frozen: the write head stands) */
static __attribute__((noinline)) void cl_spawn(uint32_t frozen)
{
    uint32_t k, r = cl_rng(), L = cld.len, inc, take, dmin, dmax, d, fill = cld.fill, rev;
    int32_t d16 = (int32_t)fx1s.pitch * 16, j;
    cl_grain_t *g = 0;
    j = (int32_t)((r >> 16) & 0xFFFFu) - 32768;   /* the spacing to the next one: RAND jitters it (up to +-90 %) */
    j = (j * (int32_t)fx1s.rnd * 9) / 1000;
    cld.next = cld.gap + (uint32_t)(((int32_t)cld.gap >> 4) * j >> 11);
    cld.next = cld.next ? cld.next : 1u;
    for (k = 0; k < CL_NG && !g; k++)
        if (!cld.g[k].on)
            g = &cld.g[k];
    if (!g)
        return;
    r = cl_rng();
    if (fx1s.rnd) {                              /* RAND: detuned at random, quadratic: mostly a little */
        int32_t u = (int32_t)(r & 0xFFFFu) - 32768;
        d16 += ((u * (u < 0 ? -u : u)) >> 15) * (192 * (int32_t)fx1s.rnd / 100) >> 15;
    }
    inc = pow2_q16(d16 < -384 ? -384 : d16 > 384 ? 384 : d16);   /* (+-2 octaves) */
    rev = ((r >> 16) & 0xFFu) < (uint32_t)fx1s.rnd * 128u / 100u;   /* RAND: up to half of them backwards */
    take = ((L >> 4) * (inc >> 4)) >> 8;         /* the samples it goes through: at most half the line */
    if (take > DLY_LEN / 2u) {
        L = L * (DLY_LEN / 2u) / take;
        take = DLY_LEN / 2u;
    }
    if (rev && !frozen && L + take + 2u * CL_EDGE > DLY_LEN) {   /* backwards, live: the write head comes round */
        L = (uint32_t)(((DLY_LEN - 2u * CL_EDGE) << 8) / ((65536u + inc) >> 8));   /* behind it: L (1 + rate) fits */
        take = ((L >> 4) * (inc >> 4)) >> 8;
    }
    if (!rev) {                                  /* forwards: it must stay behind the write head */
        dmin = frozen ? take + CL_EDGE : take > L ? take - L + CL_EDGE : CL_EDGE;
        dmax = frozen ? fill : fill < DLY_LEN - L - CL_EDGE ? fill : DLY_LEN - L - CL_EDGE;
    } else {                                     /* backwards: away from it, but not past what was written */
        dmin = CL_EDGE;
        dmax = fill > take ? fill - take : 0u;
        if (!frozen && dmax > DLY_LEN - L - take - CL_EDGE)
            dmax = DLY_LEN - L - take - CL_EDGE;
    }
    g->inc = rev ? -(int32_t)inc : (int32_t)inc;
    if (dmax < dmin)
        return;                                  /* (not enough sound in the line yet) */
    d = dmin + (((dmax - dmin) * ((((cl_rng() >> 16) * (uint32_t)fx1s.spread) / 100u) >> 8)) >> 8);   /* SPREAD: how far back */
    g->pos = (fx.dly_w - d) << 16;
    g->ph = 0;
    g->dph = 0xFFFFFFFFu / (L ? L : 1u);
    g->p = (((int32_t)(cl_rng() & 0xFFFFu) - 32768) * (int32_t)fx1s.spread) / 200;   /* SPREAD: and where it sits */
    g->on = 1;
}

/* CLOUD: n samples of the send (in) through the line and the grains; mid added to out, side into cld.sd */
static __attribute__((noinline)) void cloud_render(const int32_t *in, int32_t *out, uint32_t n, int32_t fbk)
{
    uint32_t i, k, frozen = (perf_act >> PF_CFRZ) & 1u, live = 0, o;
    int32_t gain, gm, mix = fx1s.mix * 327;
    cld.len = cl_size(fx1s.size);
    cld.gap = cl_gap(fx1s.dens);
    if (cld.next > cld.gap + (cld.gap >> 1))
        cld.next = cld.gap;                      /* (DENS turned up: the next grain at the new rate, not the old one's) */
    o = (cld.len << 8) / cld.gap;                /* the overlap, Q8: 1 .. CL_NG */
    gain = cl_gain(o < 256u ? 256u : o > CL_NG << 8 ? CL_NG << 8 : o);
    gm = mulq15(gain, mix);
    for (i = 0; i < n; i++) {
        int32_t m = 0, s = 0, y;
        if (cld.next > 1u)
            cld.next--;
        else
            cl_spawn(frozen);
        for (k = 0; k < CL_NG; k++) {
            cl_grain_t *g = &cld.g[k];
            uint32_t ix;
            int32_t a, b, x, w;
            if (!g->on)
                continue;
            ix = g->pos >> 16;
            a = dly_buf[ix];
            b = dly_buf[(ix + 1u) & (DLY_LEN - 1u)];
            x = a + (((b - a) * (int32_t)((g->pos >> 1) & 0x7FFFu)) >> 15);
            w = (32768 - osc_sine(g->ph + 0x40000000u)) >> 1;   /* Hann: 0 .. 1 .. 0 over the grain */
            x = (x * w) >> 15;
            m += x;
            s += mulq15(x, g->p);
            g->pos += (uint32_t)g->inc;
            g->ph += g->dph;
            if (g->ph < g->dph)
                g->on = 0;
        }
        y = mulq16(m, (uint32_t)gain << 1);       /* (the line's level: half the send's; up to CL_NG x a full one) */
        out[i] += mulq16(m << 1, (uint32_t)gm << 1);
        cld.sd[i] = mulq16(s << 1, (uint32_t)gm << 1);
        if (!frozen) {                           /* the send in, and the cloud itself back (FDBK), softened */
            cld.lp += (mulq16(y, (uint32_t)fbk << 1) - cld.lp) >> 2;
            dly_buf[fx.dly_w & (DLY_LEN - 1u)] = (int16_t)clamp((in[i] >> 1) + cld.lp, -32768, 32767);
            fx.dly_w++;
            if (cld.fill < DLY_LEN - 1u)
                cld.fill++;
        }
    }
    for (k = 0; k < CL_NG; k++)
        live += cld.g[k].on;
    cld.live = (uint8_t)live;
    cld.side = 1;
}

/* fx_buses, while FX1 is not the DELAY or its TYPE changes: the chorus (as fx_buses' own loop), then FX1: the CLOUD,
 * or the change (the old type's last block fading out, the line cleared, the new one from silence). fb: the delay's
 * feedback, ECHO THROW's included (the CLOUD's FDBK gets the same lift) */
static __attribute__((noinline)) void fx1_block(const int32_t *cho_in, const int32_t *dly_in, int32_t *wet, uint32_t n,
                                                int32_t fb)
{
    uint32_t i, want = fx1s.type < FX1_NTYPE ? fx1s.type : FX1_DELAY;
    int32_t cfb = fx1s.fdbk * 295;
    cho_k_t ck;
    cho_setup(&ck);
    for (i = 0; i < n; i++)                      /* the chorus (fx_buses' own) */
        wet[i] = cho_tick(&ck, cho_in[i]);
    if (pf.ew1 && cfb < THR_FB)
        cfb += mulq16(THR_FB - cfb, (uint32_t)pf.ew1 << 1);   /* ECHO THROW: into the cloud, and round again */
    if (cld.busy) {                              /* clearing: FX1 silent */
        uint32_t at = (CL_CLEAR - cld.busy) * 2048u;
        memset(&dly_buf[at], 0, 2048u * sizeof dly_buf[0]);
        if (--cld.busy == 0) {
            cld.type = (uint8_t)want;
            cld.fill = 0;
            cld.next = 1;
            cld.lp = 0;
            fx.dly_lp = 0;
            memset(cld.g, 0, sizeof cld.g);
        }
        return;
    }
    if (want == cld.type) {
        cloud_render(dly_in, wet, n, cfb);      /* (want: not the DELAY, or fx_buses would not be here) */
        return;
    }
    {   /* the TYPE changes: this block the old one fades out, then the clearing */
        int32_t t[CTL];
        uint32_t m = n < CTL ? n : CTL;
        memset(t, 0, sizeof t);
        if (cld.type == FX1_CLOUD) {
            cloud_render(dly_in, t, m, cfb);
            cld.side = 0;                        /* (its stereo: not this last block) */
        } else {                                 /* the DELAY, as fx_buses runs it */
            uint32_t dl = delay_samples();
            int32_t col = 2000 + song.g[G_DCOLOR] * 240, dmix = song.g[G_DMIX] * 258;
            for (i = 0; i < m; i++) {
                int32_t x = dly_buf[(fx.dly_w - dl) & (DLY_LEN - 1u)];
                fx.dly_lp += mulq15(x - fx.dly_lp, col);
                fx.dly_w++;
                t[i] = mulq15(x << 1, dmix);
            }
        }
        {
            int32_t g = 32768, d = 32768 / (int32_t)m;   /* (to 0 over the block) */
            for (i = 0; i < m; i++, g -= d)
                wet[i] += mulq16(t[i], (uint32_t)(g > 0 ? g : 0) << 1);
        }
        cld.busy = CL_CLEAR;
        memset(cld.g, 0, sizeof cld.g);
        cld.live = 0;
    }
}

/* after the reverb (fx_buses): the CLOUD's stereo with HALL's, or alone (mix_block adds it left, takes it right) */
static __attribute__((noinline)) void cloud_side(uint32_t n)
{
    uint32_t i;
    if (hl.side)
        for (i = 0; i < n; i++)
            hl.sd[i] += cld.sd[i];
    else
        for (i = 0; i < n; i++)
            hl.sd[i] = cld.sd[i];
    hl.side = 1;
    cld.side = 0;
}
