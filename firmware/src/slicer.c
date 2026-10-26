/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* SLICER: a per-track insert, a tempo-synced 16-step gate / stutter. It works on the track's dry
 * mono signal after DIST and before LEVEL / PAN / the sends (fx.c mix_part), so the sends follow
 * the chopped sound. M-Vave-OP1: at the START or at the END of the chain. START (GATE, STUT, PUMP): per
 * track, first: sound -> SLICER -> DIST -> INSERT -> LEVEL / PAN / the sends. END (GATE END, PUMP END:
 * SL_GEND and after): on the whole stereo sum after the reverb (dry + wet), before MASTER and the FX
 * layer's punch-ins (fx.c slicer_end), driven by that track's PAT / RATE / DEPTH and step clock; several
 * tracks on END: the lowest drives it (the others' clocks run on). A track on END is not sliced at the start.
 *   P_SLCR    OFF / GATE / STUT / PUMP / GATE END / PUMP END (N_SLCR's names: GT END, PM END)
 *   P_SLPAT   one of SL_NPAT patterns of 16 steps ('x' live / open, '.' gated / repeated); PUMP: the release curve
 *   P_SLRATE  the step: 1/8, 1/16, 1/32, 8T, 16T, 32T, (M-Vave-OP1) 1/4
 *   P_SLDEPTH GATE: how far a '.' step closes (100 % = silent); STUT: the level of the repeat; PUMP: the duck
 * GATE: the gain moves at most 1 / SL_RAMP per sample (2.9 ms from open to closed): a closing
 *   ramp ends on the step boundary, an opening one starts on it.
 * STUT: an 'x' step plays live and is recorded (22.05 kHz, 16 bit, SL_LEN samples a track); a '.'
 *   step plays that recording from its start, looped at the step length (halved until it fits the
 *   recording), cross-faded with the live sound by DEPTH. Every pass of the loop is windowed
 *   (SL_RAMP at both ends and at the step end), the cross-fade moves as the gate does.
 * PUMP (M-Vave-OP1): ducking in time, as a sidechain compressor keyed by a kick would (without the kick): at every step
 *   the level drops by DEPTH (ramped over SL_RAMP: no click) and comes back along the curve PAT picks (sl_pump_r):
 *   PAT 1..4 SMOOTH (an S curve), 5..8 PUNCH (quick, then easing in), 9..12 LINEAR, 13..16 HOLD (down, then back late),
 *   each over the whole step, 3/4, 1/2 or 1/4 of it (after the drop). At the START it ducks the track, at the END the mix.
 * The step clock: per track, always running (also with the SLICER OFF, so switching it on lands
 * in time), restarted with the transport (seq_start -> slicer_start: step 0 starts with the
 * sequencer's step 0), BPM and the track's + the global SWING as the sequencer has them (seq.c
 * step_samples); sample exact. With the SLICER OFF and no ramp left, the signal is not touched. */
#define SL_NPAT 16
#define SL_LEN 4096u                    /* recording, 22.05 kHz samples a track: 186 ms, 8 KB */
#define SL_RAMP_LOG2 7
#define SL_RAMP (1 << SL_RAMP_LOG2)     /* 128 samples, 2.9 ms */
#define SL_SLOPE (32768 / SL_RAMP)
enum { SL_OFF, SL_GATE, SL_STUT, SL_PUMP, SL_GEND, SL_PEND, SL_NMODE };   /* P_SLCR (params.c N_SLCR; stored:
                                                                             * append-only) */

/* bit k = step k: 1 = 'x' (open / live), 0 = '.' (closed / repeat) */
static const uint16_t SL_PAT[SL_NPAT] = {
    0x5555,   /*  1 x.x.x.x.x.x.x.x. */
    0xB6DB,   /*  2 xx.xx.xx.xx.xx.x */
    0x5249,   /*  3 x..x..x..x..x.x. */
    0xB777,   /*  4 xxx.xxx.xxx.xx.x */
    0x6D6D,   /*  5 x.xx.xx.x.xx.xx. */
    0x0F0F,   /*  6 xxxx....xxxx.... */
    0x1111,   /*  7 x...x...x...x... */
    0x54A5,   /*  8 x.x..x.x..x.x.x. */
    0x5333,   /*  9 xx..xx..xx..x.x. */
    0xA929,   /* 10 x..x.x..x..x.x.x */
    0xAAFF,   /* 11 xxxxxxxx.x.x.x.x */
    0xD501,   /* 12 x.......x.x.x.xx */
    0xAAAA,   /* 13 .x.x.x.x.x.x.x.x */
    0xAB6B,   /* 14 xx.x.xx.xx.x.x.x */
    0xBB5D,   /* 15 x.xxx.x.xx.xxx.x */
    0x7597,   /* 16 xxx.x..xx.x.xxx. */
};
#define SL_NDIV 7
static const uint8_t SL_DEN[SL_NDIV] = {2, 4, 8, 3, 6, 12, 1};   /* P_SLRATE (N_SLDIV): a step = 1 / DEN beats (1/4:
                                                                 * M-Vave-OP1, appended: the PUMP's four on the floor) */

static int16_t sl_buf[NTRK][SL_LEN] __attribute__((section(".pool")));
typedef struct {
    uint32_t pos, len;           /* samples into the step, its length */
    uint32_t base;               /* the step without swing */
    uint32_t rp, loop;           /* repeat: read position, loop length (44.1 kHz samples), 0 = none */
    uint32_t rec;                /* recorded (22.05 kHz samples) */
    int32_t gc;                  /* gate closure, Q15: 0 = open */
    int32_t w;                   /* STUT cross-fade, Q15: 0 = live */
    int32_t half;                /* recording: the first sample of a pair */
    uint8_t idx;                 /* step 0..15 */
    uint8_t bit;                 /* this step's pattern bit (latched at its start) */
    uint8_t rec_on;              /* recording this step */
    uint8_t psh;                 /* PUMP: this step's curve (0 SMOOTH, 1 PUNCH, 2 LINEAR, 3 HOLD) */
    uint32_t pu, pinc;           /* .. the release's phase (Q8 of Q15: 0 .. 1 << 23) and its step a sample */
} sl_t;
static sl_t sl[NTRK];
static uint8_t sl_lent;          /* perform.c has borrowed sl_buf: STUT plays live, records nothing */
/* the END's gate (fx.c slicer_end): one for the whole sum, its closure moving as a track's gate does; the track driving
 * it this block (k + 1: the first on END in mix_block's order, the lowest; 0: none), its closure per sample */
static struct {
    int32_t gc;                  /* closure, Q15: 0 = open */
    uint8_t k;                   /* the track driving it this block + 1 */
    int32_t g[CTL];              /* this block's closure, sample by sample */
} sle;

static void slicer_start(void)   /* seq_start: the next block starts step 0 of every track */
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        sl[k].idx = 15;
        sl[k].pos = sl[k].len = 0;
    }
}

static uint32_t sl_pattern(const track_t *t) { return SL_PAT[(uint32_t)(t->p[P_SLPAT] - 1) % SL_NPAT]; }
static int sl_pump_mode(uint32_t mode) { return mode == SL_PUMP || mode == SL_PEND; }

/* PUMP's release: what is left of the drop at u (Q15, 0 .. 1 through the release) for curve sh, Q15 (1 at 0, 0 at 1).
 * The ISR's and the SLICER page's (ui_graph.c) */
static inline int32_t sl_pump_r(uint32_t sh, int32_t u)
{
    int32_t v = 32768 - u;
    switch (sh) {
    case 0:                                         /* SMOOTH: 1 - (3 u^2 - 2 u^3) */
        return 32768 - (int32_t)(((uint32_t)((u * u) >> 15) * (uint32_t)(98304 - 2 * u)) >> 15);
    case 1:                                         /* PUNCH: (1 - u)^2, quick then easing in */
        return (v * v) >> 15;
    case 2:                                         /* LINEAR */
        return v;
    default:                                        /* HOLD: 1 - u^2, down, then back late */
        return 32768 - ((u * u) >> 15);
    }
}
/* PUMP: the step's curve and the release's rate (sl_enter, at a step's start: one divide a step, out of line) */
static __attribute__((noinline)) void sl_pump_enter(const track_t *t, sl_t *s)
{
    uint32_t p = (uint32_t)(t->p[P_SLPAT] - 1) % SL_NPAT, r = s->len * (4u - (p & 3u)) / 4u;
    r = r > (uint32_t)SL_RAMP + 64u ? r - (uint32_t)SL_RAMP : 64u;   /* (after the drop) */
    s->psh = (uint8_t)(p >> 2);
    s->pu = 0;
    s->pinc = (32768u << 8) / r;
}

/* the step clock enters the next step */
static void sl_enter(const track_t *t, sl_t *s)
{
    uint32_t mode = (uint32_t)t->p[P_SLCR];
    s->idx = (uint8_t)((s->idx + 1u) & 15u);
    s->base = (uint32_t)FS * 60u / (uint32_t)song.g[G_BPM] / SL_DEN[(uint32_t)t->p[P_SLRATE] % SL_NDIV];
    s->len = swing_step_len(t, s->base, s->idx);   /* core.h, as seq.c step_samples */
    s->pos = 0;
    s->bit = (uint8_t)((sl_pattern(t) >> s->idx) & 1u);
    s->rp = 0;
    s->loop = 0;
    s->rec_on = 0;
    if (sl_pump_mode(mode))
        sl_pump_enter(t, s);
    if (mode != SL_STUT || sl_lent) {
        s->rec = 0;                                 /* nothing old to repeat when STUT comes on */
    } else if (s->bit) {
        s->rec = 0;                                 /* a live step: record it */
        s->rec_on = 1;
    } else if (s->rec >= 2u * SL_RAMP) {            /* a repeat: the last live step, looped */
        uint32_t l = s->base;
        while (l > 2u * SL_LEN)
            l >>= 1;                                /* (a long step: a half, a quarter .. of it) */
        s->loop = l < 2u * s->rec ? l : 2u * s->rec;
    }
}

/* m samples of one step (no boundary inside). sd: SPREAD's side signal (fx.c mix_spread) or 0: it gets the gate and
 * the live sound's share of the cross-fade (the repeat is the mono recording: on PAN). Inlined into both callers:
 * slicer_track's (sd 0) is the code it always was */
static inline __attribute__((always_inline)) void sl_seg(const track_t *t, sl_t *s, int16_t *buf, int32_t *b,
                                                          int32_t *sd, uint32_t m)
{
    uint32_t mode = (uint32_t)t->p[P_SLCR], j, nbit = (sl_pattern(t) >> ((s->idx + 1u) & 15u)) & 1u;
    int32_t depth = t->p[P_SLDEPTH] * 258;          /* Q15, 0..32766 */
    int32_t tc = mode == SL_GATE && !s->bit ? depth : 0;           /* gate closure of this step */
    int32_t tn = mode == SL_GATE && !nbit ? depth : 0;             /* .. of the next one */
    int32_t tw = mode == SL_STUT && s->loop ? depth : 0;           /* repeat level */
    for (j = 0; j < m; j++) {
        int32_t x = b[j], y, tg = tc;
        uint32_t left = s->len - s->pos - j;        /* samples to the step end, >= 1 */
        if (left <= (uint32_t)SL_RAMP && tn > tg)
            tg = tn;                                /* close by the boundary */
        s->gc += clamp(tg - s->gc, -SL_SLOPE, SL_SLOPE);
        s->w += clamp(tw - s->w, -SL_SLOPE, SL_SLOPE);
        y = s->gc ? x - mulq16(x, (uint32_t)s->gc << 1) : x;
        if (sd) {
            int32_t z = sd[j];
            if (s->gc)
                z -= mulq16(z, (uint32_t)s->gc << 1);
            if (s->w)
                z -= mulq16(z, (uint32_t)s->w << 1);
            sd[j] = z;
        }
        if (s->rec_on) {                            /* 2:1, the pair's mean; Q15 >> 2 (the mix's headroom) */
            if ((s->pos + j) & 1u) {
                buf[s->rec] = (int16_t)clamp((s->half + x) >> 3, -32768, 32767);
                if (++s->rec >= SL_LEN)
                    s->rec_on = 0;
            } else {
                s->half = x;
            }
        }
        if (s->loop) {                              /* the repeat, windowed, read between samples */
            uint32_t rp = s->rp, e = s->loop - 1u - rp;
            int32_t r = buf[rp >> 1];
            if (rp & 1u)
                r = (r + buf[((rp >> 1) + 1u) & (SL_LEN - 1u)]) >> 1;
            r <<= 2;
            if (rp < e)
                e = rp;
            if (left - 1u < e)
                e = left - 1u;
            if (e < (uint32_t)SL_RAMP)
                r = (r * (int32_t)e) >> SL_RAMP_LOG2;
            if (++s->rp >= s->loop)
                s->rp = 0;
            y += mulq16(r - y, (uint32_t)s->w << 1);
        } else if (s->w) {
            y -= mulq16(y, (uint32_t)s->w << 1);    /* (the repeat ended on a boundary: it is 0 there) */
        }
        b[j] = y;
    }
}

/* PUMP: the closure of m samples of the step (no boundary inside) into g, on the closure *gcp: the drop towards DEPTH
 * over the step's first SL_RAMP samples, then the release along the curve; every move at most SL_SLOPE a sample (DEPTH or
 * PAT turned mid-step: no jump) */
static __attribute__((noinline)) void sl_pump(const track_t *t, sl_t *s, int32_t *gcp, int32_t *g, uint32_t m)
{
    uint32_t j, pos = s->pos, sh = s->psh, pu = s->pu, inc = s->pinc;
    int32_t depth = t->p[P_SLDEPTH] * 258, gc = *gcp;
    for (j = 0; j < m; j++, pos++) {
        int32_t tg = depth;
        if (pos >= (uint32_t)SL_RAMP) {             /* the release */
            pu = pu + inc < (32768u << 8) ? pu + inc : 32768u << 8;
            tg = (depth * sl_pump_r(sh, (int32_t)(pu >> 8))) >> 15;
        }
        gc += clamp(tg - gc, -SL_SLOPE, SL_SLOPE);
        g[j] = gc;
    }
    s->pu = pu;
    *gcp = gc;
}
/* PUMP at the START: the track's m samples (and SPREAD's side, sd != 0) ducked */
static __attribute__((noinline)) void sl_pump_seg(const track_t *t, sl_t *s, int32_t *b, int32_t *sd, uint32_t m)
{
    int32_t g[CTL];
    uint32_t j;
    sl_pump(t, s, &s->gc, g, m);
    for (j = 0; j < m; j++) {
        b[j] -= mulq16(b[j], (uint32_t)g[j] << 1);
        if (sd)
            sd[j] -= mulq16(sd[j], (uint32_t)g[j] << 1);
    }
}

/* the END's closure over m samples of track t's step (at o in the block): as sl_seg's GATE, on sle.gc */
static __attribute__((noinline)) void sl_end_seg(const track_t *t, const sl_t *s, uint32_t o, uint32_t m)
{
    uint32_t j, nbit = (sl_pattern(t) >> ((s->idx + 1u) & 15u)) & 1u;
    int32_t depth = t->p[P_SLDEPTH] * 258, tc = s->bit ? 0 : depth, tn = nbit ? 0 : depth, gc = sle.gc;
    for (j = 0; j < m; j++) {
        int32_t tg = tc;
        if (s->len - s->pos - j <= (uint32_t)SL_RAMP && tn > tg)
            tg = tn;                                /* close by the boundary */
        gc += clamp(tg - gc, -SL_SLOPE, SL_SLOPE);
        sle.g[o + j] = gc;
    }
    sle.gc = gc;
}

/* the step clock over n samples and the SLICER on b (0: the clock only, the track is silent). A track at the START
 * (GATE STUT PUMP, or a ramp still running) slices b; the lowest on END gives the END its closure for the block */
static void slicer_track(const track_t *t, int32_t *b, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk), i = 0, mode = (uint32_t)t->p[P_SLCR];
    sl_t *s = &sl[k];
    int act = b && (mode - 1u < SL_GEND - 1u || s->gc || s->w), end = mode >= SL_GEND && !sle.k;
    if (end)
        sle.k = (uint8_t)(k + 1u);
    while (i < n) {
        uint32_t m;
        if (s->pos >= s->len)
            sl_enter(t, s);
        m = s->len - s->pos;
        if (m > n - i)
            m = n - i;
        if (act) {
            if (mode == SL_PUMP)
                sl_pump_seg(t, s, b + i, 0, m);
            else
                sl_seg(t, s, sl_buf[k], b + i, 0, m);
        }
        if (end) {
            if (mode == SL_PEND)
                sl_pump(t, s, &sle.gc, sle.g + i, m);
            else
                sl_end_seg(t, s, i, m);
        }
        s->pos += m;
        i += m;
    }
}

/* slicer_track with SPREAD's side signal sd (fx.c mix_spread; b != 0) */
static __attribute__((noinline)) void slicer_track_sd(const track_t *t, int32_t *b, int32_t *sd, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk), i = 0, mode = (uint32_t)t->p[P_SLCR];
    sl_t *s = &sl[k];
    int act = mode - 1u < SL_GEND - 1u || s->gc || s->w, end = mode >= SL_GEND && !sle.k;
    if (end)
        sle.k = (uint8_t)(k + 1u);
    while (i < n) {
        uint32_t m;
        if (s->pos >= s->len)
            sl_enter(t, s);
        m = s->len - s->pos;
        if (m > n - i)
            m = n - i;
        if (act) {
            if (mode == SL_PUMP)
                sl_pump_seg(t, s, b + i, sd + i, m);
            else
                sl_seg(t, s, sl_buf[k], b + i, sd + i, m);
        }
        if (end) {
            if (mode == SL_PEND)
                sl_pump(t, s, &sle.gc, sle.g + i, m);
            else
                sl_end_seg(t, s, i, m);
        }
        s->pos += m;
        i += m;
    }
}

/* a silent part must still be rendered: a repeat is playing or fading (fx.c mix_part) */
static int slicer_busy(const track_t *t)
{
    const sl_t *s = &sl[t - trk];
    return s->w || (t->p[P_SLCR] == SL_STUT && s->loop);
}
