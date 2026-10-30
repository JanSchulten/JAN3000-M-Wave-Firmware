/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: HOME > MASTER, the master's dynamics: a compressor, then the limiter (fx.c master_lim, its threshold
 * LIMIT) and the soft clip, after SPEAKER EQ and before the MASTER knob. Included by fx.c after master_out.
 * The whole master runs at the full level (master_block) and MASTER scales what comes out of it, so the knob turns the
 * sound down without changing how it is squeezed (until HOME > MASTER the knob came first: turned down, the limiter
 * hardly worked). USB audio takes the master after the knob as before (USB LEVEL FIXED: before it, audio.c).
 * The settings (params.c dyn_set: COMP RELEASE LIMIT GAIN, the project's) are the main loop's; dyn_poll turns what
 * changed into the ISR's terms (dynx: a few floats and lim_t, written in place: a block that reads half old, half new
 * terms plays a mix of two settings a knob step apart, harmless).
 * The compressor: feed-forward, the stereo pair linked, its detector the block's peak (after SPEAKER EQ, before its own
 * gain) in dBFS, a soft knee 6 dB wide, threshold and ratio both from COMP (0 %: none; 50 %: -12 dB at 3.5:1; 100 %:
 * -24 dB at 6:1), the gain reduction smoothed in dB (the attack while it grows, the release while it shrinks: RELEASE
 * 20 ms .. 1 s, the attack a twentieth of it, 1 .. 50 ms), then GAIN (0 .. +12 dB) into the limiter. The gain is worked
 * once a block (32 samples, 0.73 ms: a log and a power) from the block before and ramped linearly over this one: the
 * attack is a block late, the limiter after it takes what it lets through. COMP 0 and GAIN 0 (the default, every older
 * project) is no compressor at all (dynx.on 0): the master is the limiter and the soft clip as before, at LIMIT's
 * default the threshold of before (DYN_LIM_DEF, fx.c LIM_T).
 * The page's meters (ui_dyn.c): the ISR keeps the deepest reduction (dyn_gr_pk, dB <= 0) and the limiter's highest
 * envelope (dyn_lim_pk) since the UI took them last; the UI turns them into dB and starts them again. */
#define DYN_BLOCK_S (CTL / 44117.647f)                 /* a block's time, s (audio.c: 24 MHz / 544 a frame) */
#define DYN_KNEE 3.0f                                  /* half the knee, dB */
static volatile struct {                               /* main: the settings as the ISR uses them (dyn_poll) */
    float thr, slope, att, rel, mk;                    /* threshold dBFS, 1 / ratio - 1, the smoothing a block (attack,
                                                        * release), GAIN in dB */
    uint8_t on;                                        /* COMP or GAIN not 0: master_dyn */
} dynx;
static struct {                                        /* the ISR's */
    float grs, g;                                      /* the reduction, smoothed (dB <= 0); the gain the block ended on */
    int32_t pk;                                        /* the peak of the block before */
} dyna = {0.0f, 1.0f, 0};
static volatile float dyn_gr_pk;                       /* ISR: the deepest grs since the UI took it (dB <= 0) */
static volatile int32_t dyn_lim_pk;                    /* ISR: the limiter's highest lim_env since the UI took it */
static int16_t dyn_was[DYN_N] = {-1, -1, -1, -1};      /* main: the settings dyn_poll handed over last */
static uint8_t dyn_mtr[2];                             /* main: the page's meters, the compressor's and the limiter's
                                                        * reduction in half dB, a peak falling 0.5 dB a pass */

/* log2 x for x > 0 to ~0.005 (0.03 dB; a quadratic on the mantissa: ui_eq.c eq_log2's, without its offset of 1, which
 * cancels there in a difference) */
FM1_FPU static inline float dyn_log2(float x)
{
    union { float f; uint32_t u; } v = {x};
    float e = (float)((int32_t)((v.u >> 23) & 255u) - 127), m;
    v.u = (v.u & 0x007FFFFFu) | 0x3F800000u;
    m = v.f;
    return e + (-0.34484843f * m + 2.02466578f) * m - 1.67487759f;
}
/* the smoothing a block for a time constant of ms */
FM1_FPU static float dyn_coef(float ms) { return 1.0f - acid_exp2(-DYN_BLOCK_S * 1000.0f * 1.44269504f / ms); }
/* LIMIT's step k as the limiter's threshold: LIM_T x 10^((k - DYN_LIM_DEF) / 40), LIM_T itself at the default */
FM1_FPU static int32_t dyn_lim_t(int32_t k)
{
    return (int32_t)((float)LIM_T * acid_exp2((float)(clamp(k, 0, DYN_NLIM - 1) - DYN_LIM_DEF) * 0.5f * 0.16609640f)
                     + 0.5f);
}

/* main loop, every ui_input pass: the meters from what the ISR kept (and that started again); the settings changed to the
 * ISR's terms */
FM1_FPU static void dyn_poll(void)
{
    float c, rel, gc = -dyn_gr_pk, gl = 0.0f;
    int32_t lp = dyn_lim_pk, k;
    uint32_t m[2];
    dyn_gr_pk = 0.0f;
    dyn_lim_pk = 0;
    if (lp > lim_t)
        gl = 6.02059991f * (dyn_log2((float)lp) - dyn_log2((float)lim_t));
    m[0] = (uint32_t)(gc * 2.0f + 0.5f);
    m[1] = (uint32_t)(gl * 2.0f + 0.5f);
    for (k = 0; k < 2; k++) {
        uint32_t f = dyn_mtr[k] ? dyn_mtr[k] - 1u : 0u;
        m[k] = m[k] > 99u ? 99u : m[k];
        dyn_mtr[k] = (uint8_t)(m[k] > f ? m[k] : f);
    }
    if (!memcmp(dyn_was, dyn_set, sizeof dyn_was))
        return;
    memcpy(dyn_was, dyn_set, sizeof dyn_was);
    c = (float)clamp(dyn_set[DYN_COMP], 0, 100);
    rel = (float)dyn_rel_ms(dyn_set[DYN_REL]);
    dynx.thr = -0.24f * c;
    dynx.slope = 1.0f / (1.0f + 0.05f * c) - 1.0f;
    dynx.att = dyn_coef(rel / 20.0f);
    dynx.rel = dyn_coef(rel);
    dynx.mk = 0.5f * (float)clamp(dyn_set[DYN_GAIN], 0, DYN_GMAX);
    lim_t = dyn_lim_t(dyn_set[DYN_LIM]);
    dynx.on = dyn_set[DYN_COMP] > 0 || dyn_set[DYN_GAIN] > 0;
}
/* power-on (and the host tests): the settings as a new project's, nothing handed over yet */
static void dyn_power_on(void)
{
    dyn_defaults();
    memset((void *)&dynx, 0, sizeof dynx);
    dyna.grs = 0.0f;
    dyna.g = 1.0f;
    dyna.pk = 0;
    dyn_gr_pk = 0.0f;
    dyn_lim_pk = 0;
    dyn_mtr[0] = dyn_mtr[1] = 0;
    memset(dyn_was, 0xFF, sizeof dyn_was);
    lim_t = LIM_T;
}

/* the master with the compressor (dynx.on): the block's gain from the block before's peak, then each sample through the
 * DC blocker and SPEAKER EQ, the gain, the limiter and the soft clip, scaled by post (MASTER) */
FM1_FPU static __attribute__((noinline)) void master_dyn(int32_t *out, uint32_t n, int32_t post)
{
    uint32_t i;
    int32_t pk = 0;
    float g = dyna.g, dg, gr, ov;
    ov = (dyna.pk > 0 ? 6.02059991f * dyn_log2((float)dyna.pk) - 90.3089987f : -120.0f) - dynx.thr;   /* (dBFS: 32768
                                                                                                           * is 0) */
    gr = ov <= -DYN_KNEE ? 0.0f : ov < DYN_KNEE ? dynx.slope * (ov + DYN_KNEE) * (ov + DYN_KNEE) * (0.25f / DYN_KNEE)
                                                : dynx.slope * ov;
    dyna.grs += (gr - dyna.grs) * (gr < dyna.grs ? dynx.att : dynx.rel);
    if (dyna.grs < dyn_gr_pk)
        dyn_gr_pk = dyna.grs;
    dg = (acid_exp2((dyna.grs + dynx.mk) * 0.16609640f) - g) * (1.0f / CTL);   /* (dB / 6.02: a power of 2) */
    for (i = 0; i < n; i++) {
        int32_t l = mix_l[i] + wet[i], r = mix_r[i] + wet[i], al, ar;
        master_pre(&l, &r);
        al = l < 0 ? -l : l;
        ar = r < 0 ? -r : r;
        pk = al > pk ? al : pk;
        pk = ar > pk ? ar : pk;
        g += dg;
        l = (int32_t)((float)l * g);
        r = (int32_t)((float)r * g);
        master_lim(&l, &r);
        out[2u * i] = (l * post) >> 12;
        out[2u * i + 1u] = (r * post) >> 12;
    }
    dyna.g = g;
    dyna.pk = pk;
}

/* the master of a block (fx.c mix_block, perf_master): mix_l / mix_r + wet at the full level through it, MASTER after
 * (USB LEVEL FIXED: the full level out, audio.c scales the DAC's after USB took it). |out| <= 32767 after the soft clip:
 * x MASTER (<= 4096) >> 12 fits */
static void master_block(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t post = fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12;
    if (dynx.on) {
        master_dyn(out, n, post);
    } else {
        for (i = 0; i < n; i++) {
            int32_t l = mix_l[i] + wet[i], r = mix_r[i] + wet[i];
            master_out(&l, &r);
            out[2u * i] = (l * post) >> 12;
            out[2u * i + 1u] = (r * post) >> 12;
        }
        dyna.grs = 0.0f;                               /* (the compressor off: it starts from rest when it comes on) */
        dyna.g = 1.0f;
    }
    if (lim_env > dyn_lim_pk)
        dyn_lim_pk = lim_env;
}
