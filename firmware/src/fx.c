/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Effects: per track the DRIVE (the INSERT of 1.5), then sends into four shared buses, one serial chain (M-Vave-OP1):
 * CHORUS -> SHIMMER -> DELAY -> REVERB. Each stage takes the tracks' sends to it and what every stage before it gives
 * (chain_feed, chain_reverb); every stage's wet goes to the mix as well. MIX 2's levels (params.c fx_lvl; the DELAY's
 * is its MIX, G_DMIX) scale a stage's input, so a stage turned down rings out as a send would and passes nothing on.
 * Mono buses (the CHORUS, HALL, SHIMMER and the CLOUD add a side), stereo dry mix. */
#define DLY_LEN 65536u           /* 1.49 s: 1/4 fits from 41 BPM, 1/4D from 61, 1/2 from 81 */
/* M-Vave-OP1: the delay bus is FX1, of a TYPE (fx1.c): DELAY, or CLOUD (granular, in the delay's line). Its settings:
 * the main loop's (FX1 and CLOUD pages), kept with the project's extras (opstore.c); bytes the ISR reads a block at a time */
enum { FX1_DELAY, FX1_CLOUD, FX1_NTYPE };
static volatile struct {
    uint8_t type;                        /* FX1_* */
    uint8_t size, dens, spread;          /* CLOUD: the grains' length, rate and spread, 0..100 */
    int8_t pitch;                        /* .. their pitch, -24..+24 semitones */
    uint8_t rnd, fdbk, mix;              /* .. RAND (detune, reversed), FDBK, MIX, 0..100 */
} fx1s = {FX1_DELAY, 50, 50, 30, 0, 10, 0, 80};
/* M-Vave-OP1: the chorus bus's TYPE and FDBK (the CHORUS page; kept with the project's extras, opstore.c; the ISR reads
 * them a block at a time). TYPE CHORUS with FDBK 0 is Felucca's chorus bit for bit */
enum { CHO_CHORUS, CHO_FLANGER, CHO_VIBRATO, CHO_NTYPE };
static volatile struct {
    uint8_t type;                        /* CHO_* */
    uint8_t fdbk;                        /* 0..100 */
} chos;
static int32_t cho_wet[CTL], cho_sd[CTL], shim_wet[CTL], chain_rev[CTL];
static uint8_t cho_on;                   /* the CHORUS gave something this block (cho_bus): it feeds the stages after it */
/* MIX 2's level of stage k (0 CHORUS, 1 SHIMMER, 3 REVERB; params.c fx_lvl), Q15 */
static inline int32_t chain_g(uint32_t k) { return fx_lvl[k & 3u] >= 127 ? 32768 : fx_lvl[k & 3u] <= 0 ? 0 : fx_lvl[k & 3u] * 258; }
#define CHO_LEN 2048u
static int16_t dly_buf[DLY_LEN] __attribute__((section(".pool")));
static int16_t cho_buf[CHO_LEN] __attribute__((section(".pool")));
static const uint16_t REV_COMB[4] = {1116, 1188, 1277, 1356};
static const uint16_t REV_AP[2] = {556, 441};
static int16_t rev_comb[1116 + 1188 + 1277 + 1356] __attribute__((section(".pool")));
static union {                          /* ROOM's allpasses; SPRING's allpass chain (int32: no clamps) */
    int16_t ap[556 + 441];
    int32_t sp[(556 + 441) / 2];
} rev_u __attribute__((section(".pool")));
#define rev_ap (rev_u.ap)
static struct {
    uint32_t dly_w, cho_w, cho_ph;
    int32_t dly_lp;
    uint16_t comb_i[4], ap_i[2];
    int32_t comb_lp[4];
    uint8_t rtype;                       /* the reverb model running (G_RTYPE: 0 ROOM, 1 SPRING, 2 HALL; 3, the retired
                                          * SHIMMER, runs as 2) */
    uint16_t sp_w;                       /* SPRING: the loop's write index (SP_MASK) */
    int32_t sp_lp, sp_hp, sp_he, sp_size;   /* .. its loop low-pass, low cut (and its remainder), the loop
                                             * length (Q8, glides) */
    uint32_t sp_ph;                      /* .. the output tap's wobble */
    uint32_t dl_cur, dl_new;             /* M-Vave-OP1: the delay's tap (0: none yet), the one fading in (dly_glide) */
    int32_t dxf;                         /* .. samples of that fade still to go (0: none) */
} fx;

/* the chorus bus: a short line read through a sine, between samples, the read fed back (FDBK). By TYPE: CHORUS
 * Felucca's (M-Vave-OP1: read twice, stereo: cho_bus), 9.1 .. 26 ms (DEPTH 6 samples a step), feedback up to 0.49; FLANGER 0.5 .. 6.3 ms (2 a step: the comb
 * swept through the audio band), up to 0.87 (the jet); VIBRATO 1 .. 9.6 ms (3 a step), up to 0.49: the copy's pitch
 * swings (the bus is all wet: a track with its CHO send up and its LEVEL down is a vibrato alone). Its constants a
 * block (cho_setup), a sample (cho_tick: read first, the line's shortest read 22 samples back, so the order does not
 * change Felucca's) */
typedef struct { int32_t base, cd, g; uint32_t inc; } cho_k_t;
static inline void cho_setup(cho_k_t *k)
{
    uint32_t ty = chos.type, fb = chos.fdbk > 100u ? 100u : chos.fdbk;
    k->inc = LFO_INC[song.g[G_CRATE] & 127] / CTL;
    k->base = (ty == CHO_FLANGER ? 22 : ty == CHO_VIBRATO ? 44 : 400) << 8;
    k->cd = song.g[G_CDEPTH] * (ty == CHO_FLANGER ? 2 : ty == CHO_VIBRATO ? 3 : 6);
    k->g = (int32_t)fb * (ty == CHO_FLANGER ? 285 : 160);   /* Q15 */
}
static inline int32_t cho_tick(const cho_k_t *k, int32_t in)
{
    int32_t r, f, c0, c1, c;
    uint32_t ri;
    fx.cho_ph += k->inc;
    r = k->base + ((osc_sine(fx.cho_ph) + 32768) * k->cd >> 8);   /* Q8 delay: read between samples */
    ri = (uint32_t)r >> 8;
    f = r & 255;
    c0 = cho_buf[(fx.cho_w - ri) & (CHO_LEN - 1u)];
    c1 = cho_buf[(fx.cho_w - ri - 1u) & (CHO_LEN - 1u)];
    c = c0 + (((c1 - c0) * f) >> 8);
    c1 = (c * k->g + (c < 0 ? 32767 : 0)) >> 15;     /* (the feedback rounded towards zero: no hum left after it) */
    cho_buf[fx.cho_w & (CHO_LEN - 1u)] = (int16_t)clamp((in >> 1) + c1, -32768, 32767);
    fx.cho_w++;
    return c << 1;
}
/* M-Vave-OP1: the chorus's second read, for the right (cho_bus), after cho_tick wrote this sample: its sine a quarter turn
 * on (ph), the same depth */
static inline int32_t cho_tap(const cho_k_t *k, uint32_t ph)
{
    int32_t r = k->base + ((osc_sine(ph) + 32768) * k->cd >> 8), f = r & 255, c0, c1;
    uint32_t ri = (uint32_t)r >> 8;
    c0 = cho_buf[(fx.cho_w - 1u - ri) & (CHO_LEN - 1u)];
    c1 = cho_buf[(fx.cho_w - 2u - ri) & (CHO_LEN - 1u)];
    return (c0 + (((c1 - c0) * f) >> 8)) << 1;
}
/* M-Vave-OP1: the CHORUS, the chain's first stage, a block: in (the sends, CHORUS's level on them) through the line,
 * read twice (cho_tick's read the left, cho_tap's a quarter turn on the right): cho_wet their mean (the mono bus, what
 * the stages after take), cho_sd half their difference (mix_block: added left, taken right). cho_on: any of it not 0 */
static __attribute__((noinline)) void cho_bus(const int32_t *in, uint32_t n)
{
    cho_k_t k;
    uint32_t i;
    int32_t g = chain_g(0), any = 0;
    cho_setup(&k);
    for (i = 0; i < n; i++) {
        int32_t a = cho_tick(&k, g == 32768 ? in[i] : mulq16(in[i], (uint32_t)g << 1)), b = cho_tap(&k, fx.cho_ph + 0x40000000u);
        cho_wet[i] = (a + b) >> 1;
        cho_sd[i] = (a - b) >> 1;
        any |= a | b;
    }
    cho_on = any != 0;
}

/* SPRING (G_RTYPE 1): one spring of a spring tank, mono like the other buses, in the ROOM's own buffers (no
 * RAM of its own): the input and the loop's return -> a low cut (~110 Hz: a spring carries little bass) ->
 * SP_N stretched first-order allpasses, (a + z^-4) / (1 + a z^-4) (after Valimaki, Parker and Abel: below
 * fs / 8 = 5.5 kHz the group delay rises with frequency, the chirp; each pass round the loop adds more of
 * it: the "boing", the drips) -> the loop's delay line (rev_comb, SP_LEN) -> back through a one-pole
 * low-pass (DAMP) and the decay gain (SIZE). The output: the spring's far end, half way along the loop
 * (the first sound 15 .. 30 ms after the send: the tank's own pre-delay; a slow wobble of a sample or two
 * on it), plus a second, quieter pickup at three quarters (a shorter spring beside it: denser). SIZE sets
 * the loop's length (30 .. 60 ms) and its decay; DAMP the loop's low-pass. The allpasses' states: 4
 * samples each (int32), in rev_ap's memory. Changing the model fades the old one's block out and clears both buffers. */
#define SP_LEN 4096u                     /* the loop's line in rev_comb (4937 samples) */
#define SP_MASK (SP_LEN - 1u)
#define SP_N 10u                         /* allpass stages */
#define SP_A 2867                        /* their coefficient, Q12 (0.7: Q12 keeps (x - o) * a in 32 bits up to
                                          * |x - o| < 749000, far past any peak the chain reaches) */
_Static_assert(sizeof rev_comb / 2u >= SP_LEN && sizeof rev_u.sp / 4u >= 4u * (SP_N + 1u), "SPRING in ROOM's buffers");

/* HALL (G_RTYPE 2, 1.2): a smooth, dense, stereo reverb, again in ROOM's buffers (its few states and one block
 * of work shared with SHIMMER). ROOM's 4 combs each ring on their own (the metallic tone); here every echo
 * goes round 8 lines that all feed each other, and they run at half the rate (22.05 kHz: the same memory holds
 * twice the time, so twice the resonances, at half the work; the tail stops near 10 kHz, as a hall's air
 * does). The send (every other sample of it, low-passed 1 2 1; ROOM's level) -> a low cut (~110 Hz: no boom
 * in a long tail) -> 2 allpass diffusers (rev_ap: a drum's click smeared into a burst) -> into the 8 lines
 * (rev_comb, mutually prime lengths, 21 .. 34 ms). Lines 1, 2, 4, 7 are read through slow sines (+-HL_MOD
 * samples, between samples; 0.37 or 0.74 Hz, each in its own phase): the resonances drift instead of ringing.
 * Lines 0, 3, 5, 6, read at their ends, go through a one-pole low-pass (DAMP: the mixing takes every echo
 * through them); every
 * line through its gain (from its length: all decay alike; RT60 below the damping 0.5 s at SIZE 0, 4 s at
 * 90, 9 s at 127; ~2.9 s heard at 90 with DAMP 60, M-Vave-OP1: 2.4 before HL_IN, the quiet lines' rounding cut it); then the 8 x 8 mixing: a conference matrix (Paley,
 * order 8: every line feeds the 7 others, +-1 each, none itself: no flutter at a line's own length), with
 * the send added. Stereo: left lines 0 3 4 -7, right 1 2 5 6 (sums spread over the matrix's rows); the wet
 * bus (mono) gets left + right, rev_side left - right, which mix_block adds to the left and takes from the
 * right (rev_side_mix). M-Vave-OP1: down to 22.05 kHz and back up through a half-band low-pass (hall_down, hall_up:
 * Felucca's 1 2 1 down and its line between two samples up let what folds over at 11.025 kHz through, a hiss in the
 * tail's highs). Rounding: a loud line's gain rounds to the nearest
 * (the tail decays as it should), a quiet one's towards zero (it ends in exact silence: no limit cycle), and
 * the low-passes keep their steps' remainders (no offset). */
#define HL_NL 8u                         /* lines */
#define HL_NAP 2u                        /* diffusers */
#define HL_NC (CTL / 2u)                 /* a block at half the rate */
#define HL_MOD 8                         /* the taps' swing, samples (read 2 .. 18 samples short of the end) */
#define HL_LPM 0x69u                     /* the lines with a low-pass */
#define HL_KD(d) (32767 - (d) * 180)     /* its coefficient (Q15) for DAMP d: none at 0 .. ~1 kHz at 127 */
#define HL_LOUD 32                       /* a line this loud (two samples' sum) rounds its gain to the nearest */
#define HL_IN 5160                       /* the send into the lines, Q15 (M-Vave-OP1: ROOM's 2580 x 2, +6 dB: the lines'
                                          * int16 carried the tail 11 .. 23 dB below full scale in the demos, its last
                                          * second in the rounding's noise; the loudest demo now peaks 5 dB below) */
#define HL_OUT 2                         /* the outputs' gain: the level of ROOM's tail (M-Vave-OP1: 4 / 2, for HL_IN) */
static const uint16_t HL_LEN[HL_NL] = {467, 509, 547, 593, 631, 673, 709, 743};   /* (+1 each: the mirror) */
static const uint16_t HL_AP[HL_NAP] = {131, 307};
static const int16_t HL_APG[HL_NAP] = {11469, 10240};               /* Q14: 0.7, 0.625 */
_Static_assert(467 + 509 + 547 + 593 + 631 + 673 + 709 + 743 + HL_NL <= sizeof rev_comb / 2u &&
               131 + 307 <= sizeof rev_u.ap / 2u, "HALL in ROOM's buffers");
_Static_assert(CTL % 2u == 0u && HL_NC + 2u * HL_MOD + 2u < 467u && HL_NC <= 131u, "HALL: one wrap a block at most");
/* M-Vave-OP1: HALL's rate change, a half-band low-pass of 31 taps (Kaiser, beta 7): flat to 8 kHz, -0.4 dB at 9 kHz,
 * -6 dB at 11.025 kHz where the two rates fold over, 51 dB down and more from 14 kHz. The 1 2 1 down and the line between
 * two samples up it replaces let a 9 kHz tone ring in the tail with a mirror at 13 kHz 5 dB below it, and brought one of
 * 15 kHz back as 7 kHz (reverb_test: HALL ALIAS). A half-band's every other tap is 0 and the rest pair up, so its work
 * is 8 multiplies a sample down and 8 for each of mid and side up, on the FPU (single float: the send's 19 bits and the
 * lines' sums fit its 24). HB_K: the taps 0, 2 .. 14 of 31 (30, 28 .. 16 the same), scaled so they sum to 0.25 (the
 * centre, 15, is 0.5: unity gain at 0 Hz, both phases up). Its delay: 15 samples down, 15 up (0.7 ms; the first echo
 * comes 21 ms after the send). The histories are the bus's (hall_t: a block's samples in front of them in hb) */
#define HB_D 15u                         /* the centre tap: the delay, samples */
#define HB_N 8u                          /* the taps of one half that are not 0 */
static const float HB_K[HB_N] = {-0.000125854f, 0.001064443f, -0.003772257f, 0.009803523f, -0.021590671f, 0.043986472f,
                                 -0.093085237f, 0.313719582f};
typedef struct {                         /* persistent state, one per HALL / SHIMMER bus */
    uint16_t i[HL_NL], d[HL_NAP];        /* the lines' write indices, the diffusers' */
    int32_t lp[HL_NL], le[HL_NL];        /* the low-passes and their steps' remainders */
    int32_t x, m, s, hp, he;             /* the send's last sample, its low cut and that step's remainder; the
                                          * last mid and side out (half rate) */
    float hin[2u * HB_D];                /* M-Vave-OP1: the send's last 30 samples (hall_down) */
    float hm[HB_D], hs[HB_D];            /* .. the last 15 mid and side out (hall_up) */
    int16_t g[HL_NL], sz;                /* the lines' gains / sqrt(7) (Q14), for SIZE sz - 1 (0: none yet) */
    uint32_t ph;                         /* the taps' drift */
    uint8_t side;                        /* sd holds this block's stereo difference (HALL running) */
    int32_t sd[CTL];                     /* the block's left minus right (mix_block: rev_side_mix) */
    int32_t sh[HL_NC], shlp;             /* SHIMMER: its octave going into the next block's send; its low-pass */
    uint32_t shp;                        /* fractional pitch-shifter phase of the independent bus */
    uint16_t shw, shd;                   /* .. its line's write index, the first tap's distance */
} hall_t;
/* Small states and block scratch in RAM leave the pool's 8 KiB reserve intact; every long line stays in the pool. */
static hall_t hl, shim_hl;
static struct {                         /* shared scratch: the two buses render sequentially */
    int32_t bx[HL_NC], l[HL_NC], r[HL_NC];
    int16_t y[HL_NL][HL_NC];
} hw;
static struct {                         /* M-Vave-OP1: hall_down / hall_up's history and block, in a row */
    float x[2u * HB_D + CTL], m[HB_D + HL_NC], s[HB_D + HL_NC];
} hb;
static int16_t shim_shb[1024] __attribute__((section(".pool")));
static int16_t shim_comb[4872 + HL_NL] __attribute__((section(".pool")));
static int16_t shim_ap[131 + 307] __attribute__((section(".pool")));
#define rev_side (hl.sd)

/* master: peak limiter in front of the soft clipper. Fast attack (~0.1 ms),
 * ~150 ms release, threshold where tanh is still nearly linear, so chords
 * get quieter instead of crushed. M-Vave-OP1: the threshold is HOME > MASTER's LIMIT (lim_t, dyn.c dyn_poll; the main
 * loop writes the word, the ISR reads it), LIM_T its default: the threshold as it always was */
#define LIM_T 18000
static int32_t lim_env = LIM_T;
static int32_t lim_t = LIM_T;
/* MENU > USB LEVEL FIXED (for #42: record over USB with the speaker turned down): the mix goes to master_out at the
 * full MASTER level, USB audio takes that (audio.c uac_tap), and only then does MASTER scale what the DAC gets
 * (usb_fixed_dac). MASTER (0, the default): MASTER before master_out, as always (USB follows the knob) */
static volatile uint8_t fx_usb_fixed;
#define MASTER_FULL 4096                /* main.c: the MASTER knob's top, Q12 */
/* the MASTER pot (ADC 0..1023, smoothed in main.c) to its level, square law: 0 .. 4088 (Q12) */
static inline uint32_t master_of_pot(uint32_t k10) { return (k10 * k10) >> 8; }
static __attribute__((noinline)) void usb_fixed_dac(int32_t *out, uint32_t n)   /* audio ISR, after uac_tap */
{
    uint32_t i;
    int32_t m = (int32_t)song.master_q12;
    for (i = 0; i < 2u * n; i++)
        out[i] = (out[i] * m) >> 12;                /* (|out| <= 32767 after the soft clip: fits) */
}
static volatile uint8_t fx_lowcut;     /* settings: 1 LOWCUT 12 dB/oct ~110 Hz, 2 BASS+ (the small speaker):
                                        * 12 dB/oct ~220 Hz plus the harmonics of the bass (spk_bass) */
static int32_t lc_l1, lc_l2, lc_r1, lc_r2, dc_l, dc_r, dce_l, dce_r;

/* DC blocker (~2 Hz), always on: a leaky integrator of the input (Q6 state) subtracted from it.
 * The >> 12 step keeps its remainder (error feedback, 0..4095) and adds it to the next one, so no
 * part of the step is lost: the state follows the input exactly, down to 0 after the sound stops.
 * (A rounded step of (x - dc) / 4096 would stop moving at |x - dc| < 2048 and leave an offset of up
 * to +-31 at the output after silence.) */
static inline int32_t dc_block(int32_t x, int32_t *dc, int32_t *err)
{
    int32_t e = (x << 6) - *dc + *err, d = e >> 12;
    *err = e - (d << 12);
    *dc += d;
    return x - ((*dc + 32) >> 6);
}

static int32_t lce[4];
static inline int32_t lowcut1(int32_t x, int32_t *lc, int32_t *err, uint32_t sh)   /* x minus its one-pole low-pass */
{
    int32_t e = x - *lc + *err, d = e >> sh;
    *err = e - (d << sh);
    *lc += d;
    return x - *lc;
}

/* BASS+: what the speaker cannot play, heard through its harmonics. The bass below ~150 Hz is clipped at its
 * own envelope (a level-following trapezoid: odd harmonics), then band-passed ~220 Hz..1 kHz and added.
 * The low-pass has 4 poles (#42: with 2, the trapezoid rebuilt the 300 .. 600 Hz of the mix itself, late,
 * and cancelled up to 6 dB of it; now under 0.5 dB) */
static int32_t sb_lp1, sb_lp2, sb_lp3, sb_lp4, sb_env, sb_h1, sb_h2, sb_hl;
static inline int32_t spk_bass(int32_t m)
{
    int32_t a, t, u;
    sb_lp1 += ((m - sb_lp1) * 692) >> 15;
    sb_lp2 += ((sb_lp1 - sb_lp2) * 692) >> 15;
    sb_lp3 += ((sb_lp2 - sb_lp3) * 692) >> 15;
    sb_lp4 += ((sb_lp3 - sb_lp4) * 692) >> 15;
    a = sb_lp4 < 0 ? -sb_lp4 : sb_lp4;
    if (a > sb_env)
        sb_env += (a - sb_env) >> 2;
    else if (sb_env > 0)
        sb_env -= (sb_env >> 11) + 1;
    t = clamp(sb_lp4 * 8, -sb_env, sb_env);
    sb_h1 += (t - sb_h1) >> 5;
    u = t - sb_h1;
    sb_h2 += (u - sb_h2) >> 5;
    u -= sb_h2;
    sb_hl += (u - sb_hl) >> 3;
    return sb_hl + (sb_hl >> 1);      /* x1.5 (#180: x3 peaked at 1.5 .. 3.5 x a full-scale kick, the limiter */
}                                     /* pulled the mix down up to 16 dB on each hit and the buzz took over) */

/* the master's first half: the DC blocker, SPEAKER EQ */
static inline void master_pre(int32_t *l, int32_t *r)
{
    *l = dc_block(*l, &dc_l, &dce_l);
    *r = dc_block(*r, &dc_r, &dce_r);
    if (fx_lowcut) {                  /* two one-pole high-passes, error feedback as dc_block (the */
        uint32_t sh = fx_lowcut == 2u ? 5u : 6u;    /* rounded step stopped at |x - lc| < 32: an offset) */
        int32_t b = fx_lowcut == 2u ? spk_bass((*l + *r) >> 1) : 0;
        *l = lowcut1(*l, &lc_l1, &lce[0], sh);
        *l = lowcut1(*l, &lc_l2, &lce[1], sh) + b;
        *r = lowcut1(*r, &lc_r1, &lce[2], sh);
        *r = lowcut1(*r, &lc_r2, &lce[3], sh) + b;
    }
}
/* .. its second: the limiter (at lim_t), the soft clip. HOME > MASTER's compressor goes between the two (dyn.c) */
static inline void master_lim(int32_t *l, int32_t *r)
{
    int32_t al, ar, a, lt = lim_t;
    al = *l < 0 ? -*l : *l;
    ar = *r < 0 ? -*r : *r;
    a = al > ar ? al : ar;
    if (a > lim_env)
        lim_env += (a - lim_env) >> 2;
    else if (lim_env > lt)
        lim_env -= ((lim_env - lt) >> 12) + 1;
    if (lim_env > lt) {
        int32_t g = (int32_t)(((uint32_t)lt << 15) / (uint32_t)lim_env);   /* < 32768 */
        *l = ((*l >> 4) * g) >> 11;                      /* >> 4 first: |l| may be far above Q15 */
        *r = ((*r >> 4) * g) >> 11;
    }
    *l = softclip(*l);
    *r = softclip(*r);
}
static inline void master_out(int32_t *l, int32_t *r)
{
    master_pre(l, r);
    master_lim(l, r);
}
/* length of one division (N_DIV order) in samples at the song tempo */
static const uint8_t DIV_DEN[6] = {1, 2, 4, 8, 3, 6};    /* original IDs stay fixed; slow rates append */
static uint32_t midi_beat_samples;                    /* zero until an external clock has a measured tempo */
static uint32_t beat_samples(void)
{
    return song.g[G_CLOCK] && midi_beat_samples ? midi_beat_samples : (uint32_t)FS * 60u / (uint32_t)song.g[G_BPM];
}
static uint32_t div_samples(uint32_t div)
{
    uint32_t quarter = beat_samples();
    return div < 6u ? quarter / DIV_DEN[div] : div < 10u ? quarter << (div - 5u) : quarter / DIV_DEN[div % 6u];
}

#include "perform.c"                                 /* the FX hold layer's effects (the master) */
#include "click.c"                                   /* the metronome's click (after the master: audio.c) */
static uint32_t delay_samples(void);
#include "fx1.c"                                     /* M-Vave-OP1: FX1's CLOUD, the TYPE switch (the delay's line) */
#include "eq.c"                                      /* M-Vave-OP1: HOME > EQ, each track's four bands */

/* M-Vave-OP1: the delay's feedback, rounded towards zero (as ROOM's: fx.c rev_room). Floored, a low-pass left at -2
 * fed -1 back for ever: an endless -1 in the line, which the chain (its echoes into the REVERB) made a tail that never
 * ended */
static inline int32_t dly_fb(int32_t lp, int32_t fb)
{
    int32_t f = lp * fb;
    return (f + ((f >> 31) & 32767)) >> 15;
}

/* the delay's TIME (params.c N_DTIME): N_DIV's ten (div_samples), then 1.5.1's dotted 1/8D 1/16D 1/4D (10, 11, 12: 3/4,
 * 3/8, 3/2 of a quarter); at most DLY_LEN - 1 samples (1.486 s): 1/4D clamps at 60 BPM and below (1/2 below 81) */
static const uint8_t DIV_DOT[3] = {2, 3, 1};           /* 3 quarters >> this */
static uint32_t delay_samples(void)
{
    uint32_t d = (uint32_t)song.g[G_DTIME];
    uint32_t s = d - 10u < 3u ? (beat_samples() * 3u) >> DIV_DOT[d - 10u] : div_samples(d);
    return s < 16u ? 16u : s >= DLY_LEN ? DLY_LEN - 1u : s;
}

/* ROOM (G_RTYPE 0): 4 damped combs + 2 allpasses (Freeverb-like, mono), added to out */
static __attribute__((noinline)) void rev_room(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, k;
    int32_t size = 25000 + song.g[G_RSIZE] * 50, damp = 32767 - song.g[G_RDAMP] * 200;
    for (i = 0; i < n; i++) {
        int32_t a = 0;
        int16_t *c = rev_comb;
        int32_t in = mulq15(rev_in[i], 2580);           /* 1/8 at -4 dB: level as before the allpass fix */
        for (k = 0; k < 4u; k++) {
            int32_t o = c[fx.comb_i[k]], f = (fx.comb_lp[k] - o) * (32767 - damp);
            fx.comb_lp[k] = o + ((f + ((f >> 31) & 32767)) >> 15);
            f = fx.comb_lp[k] * size;                   /* the low-pass and the feedback rounded towards zero
                                                         * (floored, at DAMP 0 a pattern of small negatives went
                                                         * round for ever, a hum of 32 .. 40 Hz near -60 dBFS, on
                                                         * USB too; at other DAMPs an offset): the tail ends in 0 */
            c[fx.comb_i[k]] = (int16_t)clamp(in + ((f + ((f >> 31) & 32767)) >> 15), -32768, 32767);
            if (++fx.comb_i[k] >= REV_COMB[k])
                fx.comb_i[k] = 0;
            a += o;
            c += REV_COMB[k];
        }
        c = rev_ap;
        for (k = 0; k < 2u; k++) {
            int32_t o = c[fx.ap_i[k]];
            int32_t v = a + ((o + (int32_t)((uint32_t)o >> 31)) >> 1);   /* (halved towards zero: a -1 floored
                                                                          * stayed for ever) */
            c[fx.ap_i[k]] = (int16_t)clamp(v, -32768, 32767);
            a = o - a;                                  /* Freeverb: out = buf - in (o - v is a notch comb) */
            if (++fx.ap_i[k] >= REV_AP[k])
                fx.ap_i[k] = 0;
            c += REV_AP[k];
        }
        out[i] += a;
    }
}

/* SPRING (see the top), added to out */
static __attribute__((noinline)) void rev_spring(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, k, s = (uint32_t)song.g[G_RSIZE];
    int32_t g = 19661 + (int32_t)s * 85;                /* the loop's gain: 0.6 .. 0.93 */
    int32_t kl = 26000 - song.g[G_RDAMP] * 160;         /* its low-pass: ~9 kHz .. ~1.3 kHz */
    int32_t len = (int32_t)(1323u + ((s * 1323u) >> 7)) << 8, L, L2, L3, f, w;
    int16_t *ln = rev_comb;
    int32_t *ap = rev_u.sp;
    if (!fx.sp_size)
        fx.sp_size = len;
    fx.sp_size += clamp(len - fx.sp_size, -256, 256);   /* SIZE glides (a sample a block at most) */
    L = fx.sp_size >> 8;
    fx.sp_ph += 2u * LFO_INC[24];                       /* the wobble: a slow sine, 1.5 samples deep */
    w = (fx.sp_size >> 1) + ((osc_sine(fx.sp_ph) * 3) >> 8);   /* the far end, Q8 */
    L2 = w >> 8;
    f = w & 255;
    L3 = (L * 3) >> 2;
    for (i = 0; i < n; i++) {
        uint32_t wp = fx.sp_w, j = (wp & 3u) * (SP_N + 1u);
        int32_t x = mulq15(rev_in[i], 2580), r = ln[(wp - (uint32_t)L) & SP_MASK], p, o;
        int32_t t0 = ln[(wp - (uint32_t)L2) & SP_MASK], t1 = ln[(wp - (uint32_t)L2 - 1u) & SP_MASK];
        fx.sp_lp += mulq15(r - fx.sp_lp, kl);
        o = fx.sp_lp * g;
        x += (o + ((o >> 31) & 32767)) >> 15;           /* towards 0: a loop of floors would hold an offset */
        o = x - fx.sp_hp + fx.sp_he;                    /* the low cut, its step's remainder kept (as */
        fx.sp_he = o & 63;                              /* dc_block): no dead band to hold an offset in the loop */
        fx.sp_hp += o >> 6;
        x -= fx.sp_hp;
        p = ap[j];                                      /* the chain: ap[j + k], stage k's output 4 samples ago */
        ap[j] = x;
        for (k = 1; k <= SP_N; k++) {                   /* (lossless: bounded by the loop's input, no clamp) */
            int32_t v = (x - ap[j + k]) * SP_A;         /* towards 0, as the loop's gain: floors would feed */
            o = ap[j + k];                              /* the loop a little offset and noise for ever */
            x = ((v + ((v >> 31) & 4095)) >> 12) + p;
            p = o;
            ap[j + k] = x;
        }
        ln[wp & SP_MASK] = (int16_t)clamp(x, -32768, 32767);
        fx.sp_w = (uint16_t)(wp + 1u);
        out[i] += (t0 + (((t1 - t0) * f) >> 8)) * 4 + ln[(wp - (uint32_t)L3) & SP_MASK] * 2;
    }
}

/* HALL's helpers: m / 2^sh rounded towards zero (a product that feeds back must not floor: a loop of floors
 * holds an offset and a little noise for ever) */
static inline int32_t sh_tz(int32_t m, uint32_t sh) { return (m + ((m >> 31) & ((1 << sh) - 1))) >> sh; }
/* 2^-x, x Q16 (0 .. 16): Q15 (32768 at x = 0). 2^-x = 2^u / 2^(i + 1), u = 1 - frac(x); 2^u by a cubic (1e-4) */
static uint32_t exp2_neg(uint32_t x)
{
    uint32_t i = x >> 16, u = 32768u - ((x & 0xFFFFu) >> 1), p = 2579u;
    p = ((p * u) >> 15) + 7409u;
    p = ((p * u) >> 15) + 22780u;
    p = ((p * u) >> 15) + 32768u;
    return i < 15u ? p >> (i + 1u) : 0u;
}
/* the lines' gains for SIZE s: RT60 = 0.5 s * 2^(s / 30); a line of D samples (22.05 kHz) loses 60 dB in
 * RT60 * 22050 / D passes: g = 2^(-D * 3 log2(10) / (22050 * RT60)) = 2^(-D * 59.2 / 65536 * 2^(-s / 30)),
 * at most 0.99, over sqrt(7) (the matrix's rows have 7 entries of +-1: / sqrt(7) makes it orthogonal) */
static __attribute__((noinline)) void hall_gains(hall_t *h, int32_t s)
{
    uint32_t q = exp2_neg((uint32_t)s * 65536u / 30u), k, g;
    for (k = 0; k < HL_NL; k++) {
        g = exp2_neg((HL_LEN[k] * 59u * q) >> 15);
        h->g[k] = (int16_t)(((g > 32440u ? 32440u : g) * 6193u) >> 15);
    }
    h->sz = (int16_t)(s + 1);
}

/* an allpass diffuser over x[0 .. n) from q (its next samples, no wrap): its state rounded towards zero */
static inline void hall_ap(int16_t *q, int32_t g, int32_t *x, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t y = q[i], v = x[i] + sh_tz(y * g, 14);
        q[i] = (int16_t)clamp(v, -32768, 32767);
        x[i] = y - ((v * g + 8192) >> 14);
    }
}
/* a line's end over y[0 .. n) from b (no wrap; b[n] is still the line's: its mirror at the end): between b[i]
 * and b[i + 1], f / 256 of the way (rounded to the nearest: it stays between the two), times its gain g (Q14).
 * The gain's product: rounded to the nearest while the line is loud (lo 0: the tail decays as it should; a
 * truncation would make it die early, the sooner the quieter), towards zero once it is quiet (lo 1: the last
 * steps of the tail, which then ends in exact silence) */
static inline void hall_end(const int16_t *b, int32_t f, int16_t *y, uint32_t n, int32_t g, int lo)
{
    uint32_t i;
    if (lo)
        for (i = 0; i < n; i++)
            y[i] = (int16_t)sh_tz((b[i] + (((b[i + 1u] - b[i]) * f + 128) >> 8)) * g, 14);
    else
        for (i = 0; i < n; i++)
            y[i] = (int16_t)(((b[i] + (((b[i + 1u] - b[i]) * f + 128) >> 8)) * g + 8192) >> 14);
}
/* a line's end over y[0 .. n) from b (no wrap), through the low-pass (kd, Q15; the state *lp and its step's
 * remainder *le), times its gain g (rounded as hall_end's) */
static inline void hall_end_lp(const int16_t *b, int16_t *y, uint32_t n, int32_t g, int lo, int32_t *lp, int32_t *le,
                               int32_t kd)
{
    uint32_t i;
    int32_t l = *lp, r = *le, e;
    if (lo)
        for (i = 0; i < n; i++) {
            e = (b[i] - l) * kd + r, r = e & 32767, l += e >> 15;
            y[i] = (int16_t)sh_tz(l * g, 14);
        }
    else
        for (i = 0; i < n; i++) {
            e = (b[i] - l) * kd + r, r = e & 32767, l += e >> 15;
            y[i] = (int16_t)((l * g + 8192) >> 14);
        }
    *lp = l;
    *le = r;
}
/* the block's HL_NC samples of a circular buffer of len from index d in at most two runs, no wrap inside them:
 * body with s (the run's start in the block) and k (its length); d moves on */
#define HL_RUNS(d, len, k, s, body)                                                         \
    for (s = 0; s < HL_NC; s += k) {                                                        \
        k = (len) - (d) < HL_NC - s ? (len) - (d) : HL_NC - s;                              \
        body;                                                                               \
        d = (d) + k >= (len) ? 0u : (d) + k;                                                \
    }

/* HALL (see the top): a block of the send (n = CTL samples: audio.c) -> the mid added to out, the side into
 * rev_side. At half the rate (HL_NC samples) in passes over the block, each with few states (every line is
 * longer than a block: what the lines give in this block was written before it): the send, every other
 * sample, low cut, diffusers (hl.bx); the lines' ends at their drifting taps, low-passed, scaled (hl.y); the
 * outputs and the matrix; back into the lines; the outputs back to 44.1 kHz. */
#define HL_LFO ((uint32_t)(0.37 * CTL / FS * 4294967296.0))   /* the drift: 0.37 Hz (even lines), 0.74 Hz (odd) */
/* the SHIMMER bus's octave (M-Vave-OP1; shimmer_pitch): the HALL's tail ((left + right) / 2, at the half rate) -> a
 * line of SH_LEN read by two taps half a line apart, each through a triangle (0 where it jumps back) -> a one-pole
 * low-pass -> the next block's send, before its low cut. (It was REVERB TYPE SHIMMER, G_RTYPE 3, too: retired, a
 * stored 3 plays HALL; the SHIMMER is the chain's own stage.) */
#define SH_LEN 1024u                     /* 46 ms at 22.05 kHz: the window the octave is read in */
#define SH_MASK (SH_LEN - 1u)
#define SH_G 12000                       /* Q15: the octave's level going back in (0.37: 16000 still dies away, slowly) */
#define SH_LP 20000                      /* Q15: its low-pass */
#define SH_IN 1                          /* the tail into the line: (left + right) / 2, the lines' sum over 4 */
_Static_assert(sizeof shim_shb / 2u == SH_LEN, "SHIMMER's line");
/* The independent bus's two windowed taps at a variable rate (-12 .. +24 semitones). At +12 and FDBK
 * 127 this is the legacy octave bit for bit. FDBK 100 % is the proven SH_G ceiling, not unity loop gain. */
static const uint32_t SH_RATE[37] = {32768u, 34716u, 36781u, 38968u, 41285u, 43740u, 46341u, 49097u, 52016u, 55109u, 58386u, 61858u, 65536u, 69433u, 73562u, 77936u, 82570u, 87480u, 92682u, 98193u, 104032u, 110218u, 116772u, 123715u, 131072u, 138866u, 147123u, 155872u, 165140u, 174960u, 185364u, 196386u, 208064u, 220436u, 233544u, 247431u, 262144u};
static __attribute__((noinline)) void shimmer_pitch(hall_t *h, int16_t *pitch)
{
    uint32_t i, w = h->shw, phase = h->shp, rate = SH_RATE[clamp(shim_p[2], -12, 24) + 12];
    int32_t lp = h->shlp, gain = SH_G * clamp(shim_p[3], 0, 127) / 127;
    if (!shim_p[2]) gain /= 32;                 /* no spectral shift: leave gain margin for the hall's resonances */
    for (i = 0; i < HL_NC; i++) {
        uint32_t d = phase >> 16, f = phase & 65535u, e = (d + SH_LEN / 2u) & SH_MASK;
        uint32_t a = (w - d) & SH_MASK, b = (w - e) & SH_MASK;
        int32_t g0 = (int32_t)(d < SH_LEN / 2u ? d : SH_LEN - d), y, u, v;
        pitch[w] = (int16_t)clamp((hw.l[i] + hw.r[i]) >> SH_IN, -32768, 32767);
        u = pitch[a] + mulq16(pitch[(a - 1u) & SH_MASK] - pitch[a], f);
        v = pitch[b] + mulq16(pitch[(b - 1u) & SH_MASK] - pitch[b], f);
        y = ((u * g0 + v * ((int32_t)SH_LEN / 2 - g0)) >> 9) - lp;
        lp += (y * SH_LP + (y < 0 ? 32767 : 0)) >> 15;
        h->sh[i] = (lp * gain + (lp < 0 ? 32767 : 0)) >> 15;
        w = (w + 1u) & SH_MASK;
        phase = (phase + 65536u - rate) & ((SH_LEN << 16) - 1u);
    }
    h->shw = (uint16_t)w; h->shp = phase; h->shlp = lp;
}

/* M-Vave-OP1: the send (CTL samples) to half the rate: y[i] the half-band's output at the pair i's second sample */
FM1_FPU static __attribute__((noinline)) void hall_down(hall_t *h, const int32_t *in, int32_t *y)
{
    float *x = hb.x;
    uint32_t i, j;
    memcpy(x, h->hin, sizeof h->hin);
    for (i = 0; i < CTL; i++)
        x[2u * HB_D + i] = (float)in[i];
    for (i = 0; i < HL_NC; i++) {
        const float *p = x + 2u * i + 1u;               /* p[30]: the pair's second sample; p[0] 30 before it */
        float a = 0.5f * p[HB_D];
        for (j = 0; j < HB_N; j++)
            a += HB_K[j] * (p[2u * j] + p[2u * HB_D - 2u * j]);
        y[i] = (int32_t)a;
    }
    memcpy(h->hin, x + CTL, sizeof h->hin);
}
/* .. and the lines' mid and side (hw.l, hw.r) back to 44.1 kHz: the even samples the taps', the odd ones the centre's
 * (the sample 7 before at half the rate) */
FM1_FPU static __attribute__((noinline)) void hall_up(hall_t *h, int32_t *out)
{
    float *m = hb.m, *s = hb.s;
    uint32_t i, j;
    memcpy(m, h->hm, sizeof h->hm);
    memcpy(s, h->hs, sizeof h->hs);
    for (i = 0; i < HL_NC; i++) {
        m[HB_D + i] = (float)((hw.l[i] + hw.r[i]) * HL_OUT);
        s[HB_D + i] = (float)((hw.l[i] - hw.r[i]) * HL_OUT);
    }
    for (i = 0; i < HL_NC; i++) {
        const float *pm = m + i, *ps = s + i;           /* pm[15]: this half-rate sample; pm[0] 15 before it */
        float a = 0.0f, b = 0.0f;
        for (j = 0; j < HB_N; j++) {
            a += HB_K[j] * (pm[j] + pm[HB_D - j]);
            b += HB_K[j] * (ps[j] + ps[HB_D - j]);
        }
        out[2u * i] += (int32_t)(2.0f * a);
        out[2u * i + 1u] += (int32_t)pm[HB_D / 2u + 1u];
        h->sd[2u * i] = (int32_t)(2.0f * b);
        h->sd[2u * i + 1u] = (int32_t)ps[HB_D / 2u + 1u];
    }
    memcpy(h->hm, m + HL_NC, sizeof h->hm);
    memcpy(h->hs, s + HL_NC, sizeof h->hs);
}
static __attribute__((noinline)) void hall_block(hall_t *h, int16_t *comb, int16_t *ap, int16_t *pitch,
                                              const int32_t *rev_in, int32_t *out, int32_t size, int32_t damp, int shimmer)
{
    int16_t *b = comb, *a = ap;
    uint32_t i, k, s, m, d;
    int32_t kd = HL_KD(damp);
    int32_t xp = h->x, hp = h->hp, he = h->he, mp = h->m, sp = h->s;
    if (h->sz != size + 1)
        hall_gains(h, size);
    hall_down(h, rev_in, hw.bx);                        /* (M-Vave-OP1: half the rate, the half-band's) */
    xp = rev_in[CTL - 1u];                              /* (the shimmer bus: still fed) */
    for (i = 0; i < HL_NC; i++) {
        int32_t x = hw.bx[i], v;
        x = mulq15(clamp(x, -262143, 262143), HL_IN) + h->sh[i];   /* (ROOM's level x 2; SHIMMER's octave, 0 in a HALL:
                                                                     * through the low cut too, no rumble round it) */
        v = x - hp + he;                                /* the low cut, its step's remainder kept */
        he = v & 31;
        hp += v >> 5;
        hw.bx[i] = x - hp;
    }
    h->x = xp, h->hp = hp, h->he = he;
    for (k = 0; k < HL_NAP; k++) {                      /* the diffusers */
        d = h->d[k];
        HL_RUNS(d, HL_AP[k], m, s, hall_ap(a + d, HL_APG[k], hw.bx + s, m));
        h->d[k] = (uint16_t)d;
        a += HL_AP[k];
    }
    h->ph += HL_LFO;
    for (k = 0; k < HL_NL; k++) {                       /* the lines' ends */
        int32_t g = h->g[k], f = 0, lo;
        if ((HL_LPM >> k) & 1u)
            d = h->i[k];                                /* (low-passed: the line's end) */
        else {                                          /* (drifting: HL_MOD + 2 +- HL_MOD samples short of it) */
            uint32_t r = (uint32_t)(((HL_MOD + 2) << 8) +
                                    ((osc_sine(h->ph * (1u + (k & 1u)) + k * 0x2C000000u) * HL_MOD) >> 7));
            f = (int32_t)(r & 255u);
            d = h->i[k] + (r >> 8);
            d = d >= HL_LEN[k] ? d - HL_LEN[k] : d;
        }
        s = d + HL_LEN[k] / 2u;                         /* quiet? (two samples half a line apart) */
        s = s >= HL_LEN[k] ? s - HL_LEN[k] : s;
        lo = (b[d] < 0 ? -b[d] : b[d]) + (b[s] < 0 ? -b[s] : b[s]) < HL_LOUD;
        if ((HL_LPM >> k) & 1u) {
            HL_RUNS(d, HL_LEN[k], m, s, hall_end_lp(b + d, hw.y[k] + s, m, g, lo, &h->lp[k], &h->le[k], kd));
        } else {
            HL_RUNS(d, HL_LEN[k], m, s, hall_end(b + d, f, hw.y[k] + s, m, g, lo));
        }
        b += HL_LEN[k] + 1u;
    }
    for (i = 0; i < HL_NC; i++) {
        int32_t y0 = hw.y[0][i], y1 = hw.y[1][i], y2 = hw.y[2][i], y3 = hw.y[3][i];
        int32_t y4 = hw.y[4][i], y5 = hw.y[5][i], y6 = hw.y[6][i], y7 = hw.y[7][i], x = hw.bx[i];
        int32_t t, p0, p1, p2, p3, p4, p5, p6;
        hw.l[i] = y0 + y3 + y4 - y7;                    /* the outputs */
        hw.r[i] = y1 + y2 + y5 + y6;
        /* the matrix: C = [0 1; -1 Q], Q (7 x 7) = chi(j - i), the quadratic character mod 7 (+1 at distances
         * 1 2 4, -1 at 3 5 6): row 1 + i of Q y = 2 (the three at +1) - (the sum of the 7) + y_1+i */
        t = y1 + y2 + y3 + y4 + y5 + y6 + y7;
        p0 = y2 + y3 + y5, p1 = y3 + y4 + y6, p2 = y4 + y5 + y7, p3 = y5 + y6 + y1;
        p4 = y6 + y7 + y2, p5 = y7 + y1 + y3, p6 = y1 + y2 + y4;
        y0 += t;
        hw.y[0][i] = (int16_t)clamp(t + x, -32768, 32767);   /* (+ the send: signs spread over the rows) */
        hw.y[1][i] = (int16_t)clamp(2 * p0 + y1 - y0 + x, -32768, 32767);
        hw.y[2][i] = (int16_t)clamp(2 * p1 + y2 - y0 + x, -32768, 32767);
        hw.y[3][i] = (int16_t)clamp(2 * p2 + y3 - y0 + x, -32768, 32767);
        hw.y[4][i] = (int16_t)clamp(2 * p3 + y4 - y0 + x, -32768, 32767);
        hw.y[5][i] = (int16_t)clamp(2 * p4 + y5 - y0 + x, -32768, 32767);
        hw.y[6][i] = (int16_t)clamp(2 * p5 + y6 - y0 + x, -32768, 32767);
        hw.y[7][i] = (int16_t)clamp(2 * p6 + y7 - y0 - x, -32768, 32767);
    }
    b = comb;
    for (k = 0; k < HL_NL; k++) {                       /* back into the lines (and the mirror after a wrap) */
        d = h->i[k];
        HL_RUNS(d, HL_LEN[k], m, s, for (i = 0; i < m; i++) b[d + i] = hw.y[k][s + i]; if (!d) b[HL_LEN[k]] = b[0]);
        h->i[k] = (uint16_t)d;
        b += HL_LEN[k] + 1u;
    }
    hall_up(h, out);                                    /* back to 44.1 kHz (M-Vave-OP1: the half-band's) */
    mp = (hw.l[HL_NC - 1u] + hw.r[HL_NC - 1u]) * HL_OUT;   /* (the last mid and side: the shimmer bus's still-sounding test) */
    sp = (hw.l[HL_NC - 1u] - hw.r[HL_NC - 1u]) * HL_OUT;
    h->m = mp, h->s = sp;
    h->side = 1;
    if (shimmer)
        shimmer_pitch(h, pitch);                        /* (the SHIMMER bus's octave) */
}

static __attribute__((noinline)) void rev_hall(const int32_t *in, int32_t *out, uint32_t n)
{
    (void)n;
    hall_block(&hl, rev_comb, rev_ap, 0, in, out, song.g[G_RSIZE], song.g[G_RDAMP], 0);
}

/* the reverb's buffers and states to silence (the model changed) */
static void rev_clear(void)
{
    uint32_t i;
    for (i = 0; i < sizeof rev_comb / 2u; i++)
        rev_comb[i] = 0;
    for (i = 0; i < sizeof rev_u.ap / 2u; i++)          /* (int16: 997 of them, an odd count the int32 view misses one of) */
        rev_u.ap[i] = 0;
    for (i = 0; i < 4u; i++)
        fx.comb_lp[i] = 0;
    for (i = 0; i < HL_NL; i++)
        hl.lp[i] = hl.le[i] = 0;
    fx.sp_lp = fx.sp_hp = fx.sp_he = 0;
    hl.x = hl.m = hl.s = hl.hp = hl.he = 0;
    memset(hl.hin, 0, sizeof hl.hin);                   /* (M-Vave-OP1: the half-band's histories) */
    memset(hl.hm, 0, sizeof hl.hm);
    memset(hl.hs, 0, sizeof hl.hs);
    for (i = 0; i < HL_NC; i++)                         /* (the octave's of the retired SHIMMER TYPE: 0) */
        hl.sh[i] = 0;
    hl.shlp = 0;
}

#if FELUCCA_CPU2
/* the dry mix and the sends, as each core adds its parts into them (after Felucca 1.6.1 / Jangada: mx[1] is the
 * second core's, mix_block), a part's own block (b; mx[0].b also the fade of a reverb model change, after the parts)
 * and whether a part fed the SHIMMER (shl: shimmer_send, taken into shim_live after the parts). In the pool: the same
 * SRAM as .bss, its budget has the room */
typedef struct {
    int32_t l[CTL], r[CTL], c[CTL], d[CTL], v[CTL], sh[CTL], b[CTL];
    uint8_t shl;
} mixsum_t;
static mixsum_t mx[2] __attribute__((section(".pool")));
#define mix_l (mx[0].l)
#define mix_r (mx[0].r)
#define send_c (mx[0].c)
#define send_d (mx[0].d)
#define send_r (mx[0].v)
#define send_sh (mx[0].sh)
#define part_buf (mx[0].b)
#define SM_DECL , mixsum_t *sm                          /* mix_part, mix_spread, shimmer_send: the sums they add into */
#define SM_ARG(x) , (x)
#define SM(f) (sm->f)
#else
/* one core: the arrays of before, the same code (SM(l) = mix_l, ...; send_c .. mix_r are declared below) */
static int32_t part_buf[CTL];                            /* a part's block (mix_part); the fade of a model change */
#define SM_DECL
#define SM_ARG(x)
#define SM(f) SM1_##f
#define SM1_l mix_l
#define SM1_r mix_r
#define SM1_c send_c
#define SM1_d send_d
#define SM1_v send_r
#define SM1_sh send_sh
#define SM1_b part_buf
#define SM1_shl shim_live
#endif

/* the model changed (to rt): the old one's block fades out (HALL's stereo difference with it), its buffers are
 * cleared, the new one starts from silence */
static __attribute__((noinline)) void rev_switch(const int32_t *rev_in, int32_t *wet, uint32_t n, int32_t rt)
{
    int32_t *t = part_buf, g = 65536, d = 65536 / (int32_t)n;
    uint32_t i;
    for (i = 0; i < n; i++)
        t[i] = 0;
    if (fx.rtype >= 2u)
        rev_hall(rev_in, t, n);
    else if (fx.rtype)
        rev_spring(rev_in, t, n);
    else
        rev_room(rev_in, t, n);
    for (i = 0; i < n; i++, g -= d) {
        wet[i] += mulq16(t[i], (uint32_t)g);
        rev_side[i] = mulq16(rev_side[i], (uint32_t)g);   /* (HALL only: hl.side set) */
    }
    rev_clear();
    fx.rtype = (uint8_t)rt;
}

/* M-Vave-OP1: the delay's TIME changed (a detent, the tempo, an external clock's new estimate, which moves by ~176
 * samples a ms of its 1 ms timestamps): the read tap jumped there at once and every echo clicked, under an external
 * clock about every quarter note. Now the old tap fades into the new one over DLY_XF samples (23 ms, linear: no
 * pitch bend, no click), in the feedback as in the output; a change during a fade waits for its end. The rest as
 * fx_buses' own loop, which stays as it was while the tap holds still (this one out of line) */
#define DLY_XF_LOG2 10
#define DLY_XF (1 << DLY_XF_LOG2)
static __attribute__((noinline)) void dly_glide(const int32_t *dly_in, int32_t *wet, uint32_t n, int32_t fb, uint32_t dl)
{
    uint32_t i;
    int32_t col = 2000 + song.g[G_DCOLOR] * 240, dmix = song.g[G_DMIX] * 258;
    if (!fx.dl_cur)
        fx.dl_cur = dl;                                 /* (the first block: nothing to fade from) */
    if (!fx.dxf && dl != fx.dl_cur) {
        fx.dl_new = dl;
        fx.dxf = DLY_XF;
    }
    for (i = 0; i < n; i++) {
        int32_t y = cho_wet[i], x = dly_buf[(fx.dly_w - fx.dl_cur) & (DLY_LEN - 1u)];
        if (fx.dxf) {
            int32_t xb = dly_buf[(fx.dly_w - fx.dl_new) & (DLY_LEN - 1u)];
            x += ((xb - x) * (DLY_XF - fx.dxf)) >> DLY_XF_LOG2;   /* (|xb - x| < 2^16: fits) */
            if (!--fx.dxf)
                fx.dl_cur = fx.dl_new;
        }
        fx.dly_lp += mulq15(x - fx.dly_lp, col);
        dly_buf[fx.dly_w & (DLY_LEN - 1u)] =
            (int16_t)clamp((dly_in[i] >> 1) + dly_fb(fx.dly_lp, fb), -32768, 32767);
        fx.dly_w++;
        y += mulq15(x << 1, dmix);
        wet[i] = y;
    }
}

/* the REVERB's input in the chain: its sends, the SHIMMER's wet and out (the CHORUS's wet and the DELAY's echoes: fx_buses'
 * wet so far), at the REVERB's level (MIX 2). Clamped before the reverb's Q15 multiply: several loud tracks plus the
 * stages before must not wrap */
static __attribute__((noinline)) const int32_t *chain_reverb(const int32_t *in, const int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t g = chain_g(3);
    for (i = 0; i < n; i++)
        chain_rev[i] = mulq16(clamp(in[i] + shim_wet[i] + out[i], -262143, 262143), (uint32_t)g << 1);   /* (Q16: the sum
                                                         * of four loud tracks would wrap a Q15 multiply) */
    return chain_rev;
}

/* the DELAY and the REVERB for one block, after the CHORUS (cho_bus) and the SHIMMER: their inputs in, wet (the
 * CHORUS's, the DELAY's, the REVERB's) stereo-equal out */
static void fx_buses(const int32_t *dly_in, const int32_t *rev_in, int32_t *wet, uint32_t n)
{
    uint32_t i, dl = delay_samples();
    int32_t fb = song.g[G_DFDBK] * 230, col = 2000 + song.g[G_DCOLOR] * 240;
    int32_t dmix = song.g[G_DMIX] * 258;
    int32_t rt;
    if (pf.ew1 && fb < THR_FB)
        fb += mulq16(THR_FB - fb, (uint32_t)pf.ew1 << 1);   /* ECHO THROW (perform.c): more feedback while held, 0.7
                                                             * at most (the loop's gain below 1: it never runs away) */
    if (fx1s.type | cld.type | cld.busy)
        fx1_block(dly_in, wet, n, fb);                  /* M-Vave-OP1: FX1 not the DELAY, or changing (fx1.c) */
    else if ((dl != fx.dl_cur) | fx.dxf)
        dly_glide(dly_in, wet, n, fb, dl);              /* M-Vave-OP1: TIME changed, the taps crossfade */
    else
    for (i = 0; i < n; i++) {
        int32_t y = cho_wet[i], x;                      /* the chorus's (cho_bus) */
        /* delay with a low-passed feedback */
        x = dly_buf[(fx.dly_w - dl) & (DLY_LEN - 1u)];
        fx.dly_lp += mulq15(x - fx.dly_lp, col);
        dly_buf[fx.dly_w & (DLY_LEN - 1u)] =
            (int16_t)clamp((dly_in[i] >> 1) + dly_fb(fx.dly_lp, fb), -32768, 32767);
        fx.dly_w++;
        y += mulq15(x << 1, dmix);
        wet[i] = y;
    }
    rev_in = chain_reverb(rev_in, wet, n);
    rt = song.g[G_RTYPE];
    rt = rt == 1 ? 1 : rt == 2 || rt == 3 ? 2 : 0;     /* (3: the retired SHIMMER, a HALL now) */
    hl.side = 0;
    if (rt != fx.rtype)
        rev_switch(rev_in, wet, n, rt);
    else if (rt >= 2)
        rev_hall(rev_in, wet, n);
    else if (rt)
        rev_spring(rev_in, wet, n);
    else
        rev_room(rev_in, wet, n);
    if (cld.side)
        cloud_side(n);                                  /* (fx1.c: the CLOUD's stereo, with HALL's or alone) */
}

/* after the buses: HALL's stereo difference to the left and from the right of the dry mix (the wet bus adds the
 * same to both) */
static __attribute__((noinline)) void rev_side_mix(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        l[i] += rev_side[i];
        r[i] -= rev_side[i];
    }
}

/* DRIVE (the INSERT of 1.5, Discussions #78 and #177; M-Vave-OP1: the track's one drive): after the SLICER, before
 * LEVEL / PAN / the sends. TYPE (P_ITYPE, N_ITYPE; stored, append-only) and its three values A B C (P_IA .. P_IC, what
 * they mean: params.c ins_desc):
 *   SOFT HARD FOLD FUZZ  DRIVE (1x .. ~13x, squared), TONE (two one-pole low-passes at F_CUTOFF), LEVEL (F_DB, as the
 *                        track's LEVEL: 112 is 0 dB). A slow low cut in front (~27 Hz), a DC blocker after (FUZZ is
 *                        lopsided). SOFT tanh, HARD a flat clip at a voice's level, FOLD a triangle folder (the wave
 *                        reflected at +-16384: every fold more harmonics), FUZZ a hard top and a soft, halved bottom;
 *   CRUSH                BITS (1 .. 16, of a voice's full scale), RATE (a held sample, 689 Hz .. 44.1 kHz, INS_HOLD:
 *                        aliasing as the old samplers), LPF (two one-pole low-passes at F_CUTOFF after them, #177).
 * M-Vave-OP1: the swept types (PHASR FLANG CHOR, stored 6 .. 8) are retired and play as OFF: the CHORUS bus has its
 * CHORUS / FLANGER / VIBRATO, the FX layer its PHASER and FLANGER. So is the old pre-drive DIST (P_DIST, kept in the
 * stored formats; the power-on sounds that had it get a SOFT drive instead, ui.c preset_load_to).
 * MIX (P_IMIX, no longer on a page: 100 % by default; the editor and automation still reach it) blends dry and wet
 * linearly; MIX 0 or TYPE OFF: the track bit for bit as before (nothing runs). MIX glides (2.9 ms, SL_SLOPE); a TYPE
 * change fades the old one out, clears the state, fades the new one in; so does a start from MIX 0 (no stale filter).
 * With SPREAD (mix_spread) the drive takes the mono sum b; the side passes it by.
 * Order (M-Vave-OP1): SLICER -> DRIVE: the SLICER chops the sound first (the drive colours the gated sound, a STUT
 * repeats the sound before it). State in the pool, per track (ins[]); noinline, its own loop: the audio ISR's inlined
 * path gains one test per part (track_t ins_run) */
enum { IT_OFF, IT_SOFT, IT_HARD, IT_FOLD, IT_FUZZ, IT_CRUSH, IT_N };   /* (6 .. 8, PHASR FLANG CHOR: retired, OFF) */
static const uint8_t INS_HOLD[16] = {64, 48, 40, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1};   /* CRUSH RATE */
typedef struct {
    int32_t w;                          /* the wet share now, Q15 */
    uint8_t type;                       /* the type running */
    uint8_t cnt;                        /* CRUSH: samples left of the held one */
    int32_t hp, lp1, lp2, dc, hold;     /* low cut (Q8), the two low-passes, DC blocker (Q8), CRUSH's held sample */
} ins_t;
static ins_t ins[NTRK] __attribute__((section(".pool")));

static inline int32_t ins_mixq(int32_t m) { return m >= 127 ? 32768 : m <= 0 ? 0 : m * 258; }   /* MIX -> Q15 */
/* the one-pole low-pass (topology-preserving) coefficient of cutoff index c, Q16: G = g / (1 + g), g = tan(pi fc / FS) */
static int32_t ins_lpk(int32_t c)
{
    uint32_t g = SVF_G[clamp(c, 0, 127)];
    return (int32_t)((g << 16) / (4096u + g));
}
/* the drives' and CRUSH's shape of v (a voice's full scale 32768; v already through DRIVE): the curve the INSERT
 * graph draws too */
static inline int32_t ins_shape(uint32_t ty, int32_t v, uint32_t sh)
{
    switch (ty) {
    case IT_SOFT:
        return softclip(v);
    case IT_HARD:
        return clamp(v, -24000, 24000);
    case IT_FOLD: {
        int32_t u = (int32_t)((uint32_t)(v + 16384) & 0xFFFFu) - 32768;   /* a triangle of period 65536: +-16384 */
        u = 16384 - (u < 0 ? -u : u);
        return u + (u >> 1);                            /* x 1.5 (no divide) */
    }
    case IT_FUZZ:
        return v > 0 ? (v > 20000 ? 20000 : v) : softclip(v) >> 1;
    case IT_CRUSH:                                      /* sh: 16 - BITS; to the nearest step */
        return sh ? ((v + (1 << (sh - 1u))) >> sh) << sh : v;
    default:
        return v;
    }
}
static inline uint32_t ins_bits_sh(int32_t a) { return 15u - (uint32_t)(clamp(a, 0, 127) >> 3); }   /* BITS 1..16 */

/* SOFT .. FUZZ: low cut -> DRIVE -> shape -> TONE -> LEVEL -> DC blocker; CRUSH: held sample -> bits -> LPF */
static __attribute__((noinline)) void ins_drive(ins_t *s, const track_t *t, int32_t *b, uint32_t n, int32_t tw)
{
    uint32_t i, ty = s->type, crush = ty == IT_CRUSH, sh = ins_bits_sh(t->p[P_IA]);
    uint32_t hold = INS_HOLD[clamp(t->p[P_IB], 0, 127) >> 3];
    int32_t a = t->p[P_IA], g = 4096 + a * a * 3;       /* DRIVE, Q12: 1x .. 12.8x, gentle at first */
    int32_t k = ins_lpk(crush ? t->p[P_IC] : t->p[P_IB]), lvl = crush ? 4096 : LEVEL_Q12[t->p[P_IC] & 127];
    for (i = 0; i < n; i++) {
        int32_t x = b[i], y, v;
        s->w += clamp(tw - s->w, -SL_SLOPE, SL_SLOPE);
        if (crush) {
            if (!s->cnt) {
                s->hold = ins_shape(IT_CRUSH, clamp(x, -262144, 262143), sh);
                s->cnt = (uint8_t)hold;
            }
            s->cnt--;
            y = s->hold;
        } else {
            s->hp += ((x << 8) - s->hp) >> 8;            /* ~27 Hz low cut (Q8: no offset left by the rounding) */
            v = clamp(x - (s->hp >> 8), -230000, 230000);
            y = ins_shape(ty, ((v >> 3) * g) >> 9, 0);    /* (>> 3 first: 32 bits) */
        }
        y <<= 4;                                        /* (x16: the low cutoffs' rounding leaves no offset) */
        v = mulq16(y - s->lp1, (uint32_t)k);             /* TONE / LPF: two one-pole low-passes */
        y = v + s->lp1;
        s->lp1 = y + v;
        v = mulq16(y - s->lp2, (uint32_t)k);
        y = v + s->lp2;
        s->lp2 = y + v;
        y = (y + 8) >> 4;
        if (!crush) {
            y = (y * lvl) >> 12;                        /* LEVEL */
            s->dc += ((y << 8) - s->dc) >> 9;           /* DC blocker, ~14 Hz (FUZZ's lopsided halves) */
            y -= s->dc >> 8;
        }
        b[i] = pf_mix(x, y, s->w);
    }
}

/* the DRIVE at rest at once (mix_part: TYPE turned OFF while the part was silent, its fade never ran) */
static __attribute__((noinline)) void ins_rest(track_t *t)
{
    memset(&ins[(uint32_t)(t - trk) % NTRK], 0, sizeof(ins_t));
    t->ins_run = 0;
}

/* the track's DRIVE on its block b (mix_part, mix_spread): nothing while TYPE is OFF (or retired) or MIX is 0 and
 * faded */
static __attribute__((noinline)) void track_insert(track_t *t, int32_t *b, uint32_t n)
{
    ins_t *s = &ins[(uint32_t)(t - trk) % NTRK];
    int32_t ty = (uint32_t)t->p[P_ITYPE] < IT_N ? t->p[P_ITYPE] : IT_OFF, tw;
    if (s->type != ty && !s->w)                         /* faded out: the new type from rest */
        s->type = (uint8_t)ty;
    tw = s->type == ty && ty ? ins_mixq(t->p[P_IMIX]) : 0;
    if (!s->w) {
        if (!tw) {
            t->ins_run = 0;
            return;                                     /* dry: bit for bit */
        }
        memset(s, 0, sizeof *s);                        /* starting: no stale filter */
        s->type = (uint8_t)ty;
    }
    ins_drive(s, t, b, n, tw);
    t->ins_run = s->w != 0;                             /* (mix_part calls again while it fades) */
}

/* one block of the whole mix (shared with hostsim.c): events -> each part (with its modulation matrix)
 * -> SLICER -> DRIVE -> EQ (M-Vave-OP1) -> level / pan / sends -> buses -> master; out: stereo Q15 */
static void events_block(uint32_t n);                    /* seq.c */
#if FELUCCA_CPU2
static int32_t wet[CTL];                                 /* (the dry mix and the sends: mx[0], above) */
#else
static int32_t send_c[CTL], send_d[CTL], send_r[CTL], send_sh[CTL], wet[CTL], mix_l[CTL], mix_r[CTL];
#endif

/* Silent buses sleep only after 256 zero-output blocks and empty delay/filter states. A low-pass can retain
 * an inaudible +/-1 rounding residue, and its fractional remainder alone cannot produce output. */
static uint8_t shim_live, shim_quiet;
static __attribute__((noinline)) void shimmer_send(const track_t *t, const int32_t *b, uint32_t n SM_DECL)
{
    uint32_t i;
    int32_t g = t->p[P_SHIM] * 258, lvl = LEVEL_Q12[t->p[P_LEVEL] & 127];
    int32_t lim = 0x7FFFFFFF / (g | 1);
    SM(shl) = 1;                                        /* (two cores: the sums' flag, shim_live after the parts) */
    for (i = 0; i < n; i++) {
        int32_t x = ((b[i] >> 2) * lvl) >> 10;
        SM(sh)[i] += mulq15(clamp(x, -lim, lim), g);
    }
}

/* a stage's input at its level g (Q15, below 1: MIX 2; as Q16: several loud tracks' sends would wrap a Q15 multiply) */
static __attribute__((noinline)) void chain_scale(int32_t *x, uint32_t n, int32_t g)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        x[i] = mulq16(x[i], (uint32_t)g << 1);
}
static __attribute__((noinline)) void shimmer_bus(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t g = chain_g(1);
    if (!shim_live)
        return;
    if (g != 32768)
        chain_scale(send_sh, n, g);                 /* (MIX 2's SHIMMER level on its input) */
    hall_block(&shim_hl, shim_comb, shim_ap, shim_shb, send_sh, out, shim_p[0], shim_p[1], 1);
    for (i = 0; i < n; i++) {
        mix_l[i] += shim_hl.sd[i];
        mix_r[i] -= shim_hl.sd[i];
    }
    if (shim_hl.m | shim_hl.s | shim_hl.x | shim_hl.hp | (shim_hl.shlp < -1 || shim_hl.shlp > 1))
        shim_quiet = 0;
    else if (!++shim_quiet) {
        int32_t busy = 0;
        for (i = 0; i < sizeof shim_comb / sizeof shim_comb[0]; i++) busy |= shim_comb[i];
        for (i = 0; i < sizeof shim_ap / sizeof shim_ap[0]; i++) busy |= shim_ap[i];
        for (i = 0; i < SH_LEN; i++) busy |= shim_shb[i];
        for (i = 0; i < HL_NL; i++) busy |= shim_hl.lp[i];
        for (i = 0; i < HL_NC; i++) busy |= shim_hl.sh[i];
        if (!busy) shim_live = 0;
    }
}

/* the chain after the CHORUS: its wet into the SHIMMER (a stage with something to take wakes up) and both into the DELAY */
static __attribute__((noinline)) void chain_shimmer_in(uint32_t n)
{
    uint32_t i;
    shim_live = 1;
    for (i = 0; i < n; i++)
        send_sh[i] += cho_wet[i];
}
static __attribute__((noinline)) void chain_delay_in(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        send_d[i] = clamp(send_d[i] + cho_wet[i] + shim_wet[i], -262143, 262143);
}
/* the CHORUS's stereo: its side to the left and from the right of the dry mix (cho_wet, the mid, is in wet) */
static __attribute__((noinline)) void cho_side_mix(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        mix_l[i] += cho_sd[i];
        mix_r[i] -= cho_sd[i];
    }
}
static __attribute__((noinline)) void shimmer_return(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) wet[i] += shim_wet[i];
}

/* SPREAD (#148): a POLY or UNISON part whose SPRD is above 0. Its voices start left and right of PAN in turn (voice.c
 * voice_start; UNISON's detuned voices alternate), at PAN -/+ SPRD / 2 (SPRD 127: hard left and right), with the pan
 * law PAN has (the far side down, the near one as it is). track_render gives every voice (b, as always) and the left
 * ones (spread_buf); here d = left - right, the side signal, so for a pair of pans L = b (gla + glb) / 2 + d (gla - glb)
 * / 2 (R alike): the mono sum is b's, the level of a centre pan less by SPRD / 256 at most. b goes through the DRIVE, the
 * SLICER and the FX layer's mute key as always; d gets the SLICER's gate and the mute's ramp (slicer_track_sd,
 * perf_mute_sd) and passes the DRIVE by (the spread of a distorted chord is its voices' clean difference). The sends
 * take b, mono, as before. SPRD 0, MONO or LEGATO: mix_part's own loop, bit for bit as before. The cost: a block's
 * clear and sum of spread_buf, the side and a wider mix loop, per part; per voice none (it picks its buffer) */
static __attribute__((noinline)) void mix_spread(track_t *t, int32_t *b, uint32_t n SM_DECL)
{
    int32_t *d = SPREAD_BUF(t);
    uint32_t i;
    int32_t lvl = LEVEL_Q12[t->p[P_LEVEL] & 127], pan = t->p[P_PAN], s = (t->p[P_SPRD] + 1) >> 1;
    int32_t pa = clamp(pan - s, -64, 63), pb = clamp(pan + s, -64, 63);
    int32_t gla = 4096 - (pa > 0 ? pa * 64 : 0), gra = 4096 + (pa < 0 ? pa * 64 : 0);
    int32_t glb = 4096 - (pb > 0 ? pb * 64 : 0), grb = 4096 + (pb < 0 ? pb * 64 : 0);
    int32_t glm = (gla + glb) >> 1, grm = (gra + grb) >> 1, gld = (gla - glb) >> 1, grd = (gra - grb) >> 1;
    int32_t c = t->p[P_CHOR] * 258, dl = t->p[P_DLY] * 258, r = t->p[P_REV] * 258, pk = t->peak;
    int32_t xmax = c > dl ? c : dl;
    xmax = 0x7FFFFFFF / ((xmax > r ? xmax : r) | 1);
    for (i = 0; i < n; i++)
        d[i] = (d[i] << 1) - b[i];                      /* left - right */
    slicer_track_sd(t, b, d, n);                        /* (M-Vave-OP1: the SLICER first, then the DRIVE) */
    if (t->p[P_ITYPE] | t->ins_run)
        track_insert(t, b, n);                          /* (the mono sum) */
    if (eqa.run[t - trk] | eqx.req[t - trk])
        track_eq(t, b, d, n);                           /* (M-Vave-OP1: the EQ, the side too) */
    if ((pf.mute >> (t - trk)) & 1u)
        perf_mute_sd((uint32_t)(t - trk), b, d, n);
    for (i = 0; i < n; i++) {
        int32_t x = ((b[i] >> 2) * lvl) >> 10, a = x < 0 ? -x : x, y = ((d[i] >> 2) * lvl) >> 10;
        int32_t xs = clamp(x, -xmax, xmax);
        if (a > pk)
            pk = a;
        if (c)
            SM(c)[i] += mulq15(xs, c);
        if (dl)
            SM(d)[i] += mulq15(xs, dl);
        if (r)
            SM(v)[i] += mulq15(xs, r);
        SM(l)[i] += ((x * glm) >> 12) + ((y * gld) >> 12);
        SM(r)[i] += ((x * grm) >> 12) + ((y * grd) >> 12);
    }
    t->peak = pk;
}

/* a part's cost for the second core's split (mix_split, after Felucca 1.6.1 / Jangada): its voices rendered x its
 * engine's cost a voice (ENG_VCOST: the host's instructions a sample, tests/cpu_baseline.txt at 8 notes held less the
 * idle mix, over 8 voices; the engines it lacks scaled from the host's time a voice, ~9 instructions a ns; the FPU
 * engines by the unit's measure, a WSHAPE voice ~6.3 % of the audio's time, about three ANALOG voices), and the DRIVE,
 * EQ and sends while it sounds or its tail runs. ENGINES[] order, a retired number 0. An estimate: it only steers the
 * split, the sound does not depend on it (plan step 4 may measure the parts' time on the unit instead) */
#if FELUCCA_CPU2
static uint16_t part_cost[NPART];                       /* the last block's, per part */
static const uint16_t ENG_VCOST[NENGINES] = {
    147,                /* 0 ANALOG */
    0, 0,               /* 1 DIGITAL, 2 PHASE (retired) */
    65,                 /* 3 LOFI */
    78,                 /* 4 SAMPLE */
    0, 0,               /* 5 VOICE, 6 TRIO (retired) */
    142,                /* 7 WHEEL */
    78,                 /* 8 GRAIN */
    146,                /* 9 PHYS */
    116,                /* 10 DRUM */
    139,                /* 11 NOISE */
    149,                /* 12 FM6 */
    0,                  /* 13 SLICE (retired) */
    170,                /* 14 ANALOG2 */
    130,                /* 15 VDRUM */
    0,                  /* 16 DBOX (retired) */
    450,                /* 17 WSHAPE (the FPU, as the four after it) */
    600,                /* 18 HRMNC */
    600,                /* 19 PRTCL */
    600,                /* 20 WTABLE */
    900,                /* 21 SWARM */
    400,                /* 22 ACID (one voice, its ladder on the FPU) */
};
#endif

/* one synth part into the dry mix and the sends of sm (mx[0]; mx[1] on the second core); a part with no voice sounding
 * costs the LFO tick and a cleared buffer only (after the DRIVE tail has run out) */
static void mix_part(track_t *t, uint32_t n SM_DECL)
{
    int32_t *b = SM(b);
    uint32_t i, k = (uint32_t)(t - trk), nr;
    mod_begin(t);                                       /* the matrix's per-block values into t->p (mod.c) */
    T_SPREAD(t) = t->p[P_SPRD] > 0 && (trk_vmode(t) == V_POLY || trk_vmode(t) == V_UNISON);   /* (SPREAD, mix_spread) */
    nr = track_render(t, b, n);
#if FELUCCA_CPU2
    part_cost[k % NPART] = (uint16_t)(nr * ENG_VCOST[eng_idx(t->engine)] + (nr || t->tail ? 60u : 0u));
#endif
    if (nr)
        t->tail = 16;                                   /* blocks of DRIVE / EQ state to run out after the last voice */
    else if ((!t->tail || !(t->p[P_ITYPE] | eqa.run[k]) || !--t->tail) && !slicer_busy(t)) {
        slicer_track(t, 0, n);                          /* (the SLICER's step clock runs on) */
        if (eqa.live[k] | eqx.req[k])
            eq_rest(t);                                 /* (M-Vave-OP1: the EQ from rest at the next note) */
        if (t->ins_run && !t->p[P_ITYPE])
            ins_rest(t);                                /* TYPE OFF while silent: no fade-out left for the next note */
        if (MOD(t).on)
            mod_end(t);
        return;
    }
    if (T_SPREAD(t))
        mix_spread(t, b, n SM_ARG(sm));
    else {
        slicer_track(t, b, n);                          /* slicer.c (M-Vave-OP1: first, before the DRIVE) */
        if (t->p[P_ITYPE] | t->ins_run)
            track_insert(t, b, n);                      /* the DRIVE: off, nothing */
        if (eqa.run[k] | eqx.req[k])
            track_eq(t, b, 0, n);                       /* M-Vave-OP1: the EQ (flat: nothing) */
        if ((pf.mute >> k) & 1u)
            perf_mute(k, b, n);                         /* perform.c: a black key in the FX layer */
    }
    if (!T_SPREAD(t)) {                                 /* (the mix's values read after the calls: none is held over
                                                         * them, no spill in the loop: the ISR's cost as before 1.5) */
        int32_t lvl = LEVEL_Q12[t->p[P_LEVEL] & 127], pan = t->p[P_PAN];
        int32_t gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
        int32_t c = t->p[P_CHOR] * 258, d = t->p[P_DLY] * 258, r = t->p[P_REV] * 258, pk = t->peak;
        int32_t xmax = c > d ? c : d;
        xmax = 0x7FFFFFFF / ((xmax > r ? xmax : r) | 1);   /* sends: loud chords at a high LEVEL */
        for (i = 0; i < n; i++) {
            int32_t x = ((b[i] >> 2) * lvl) >> 10, a = x < 0 ? -x : x;   /* pre-shift: 8 loud voices */
            int32_t xs = clamp(x, -xmax, xmax);         /* sends: mulq15 would overflow */
            if (a > pk)
                pk = a;
            if (c)
                SM(c)[i] += mulq15(xs, c);
            if (d)
                SM(d)[i] += mulq15(xs, d);
            if (r)
                SM(v)[i] += mulq15(xs, r);
            SM(l)[i] += (x * gl) >> 12;
            SM(r)[i] += (x * gr) >> 12;
        }
        t->peak = pk;
    }
    if (t->p[P_SHIM])
        shimmer_send(t, b, n SM_ARG(sm));
    if (pf.thr)
        perf_throw(t, b, SM(d), SM(v), n);              /* perform.c: REV / ECHO THROW (1.5.1), the sends towards full */
    if (MOD(t).on)
        mod_end(t);                                     /* the stored values back */
}

/* the HOME oscilloscope (ui_graph.c graph_scope): every other sample of the left channel. MENU > DISPLAY > SCOPE (1.2,
 * Discussion #165): OUT (0, the default) what goes out, after MASTER (audio.c audio_block, before the click); MIX the
 * mix before MASTER (and before the FX layer, SPEAKER EQ and the limiter), at the level MASTER at its top gives, so its
 * size does not follow the volume. The click is in neither */
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;
static volatile uint8_t scope_mix;
static __attribute__((noinline)) void scope_take_mix(uint32_t n)
{
    uint32_t i;
    for (i = 1; i < n; i += 2u)                         /* (((x >> 2) * MASTER_FULL) >> 10 is x) */
        scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)clamp(mix_l[i] + wet[i], -32767, 32767);
}

#include "dyn.c"                                     /* M-Vave-OP1: HOME > MASTER, the compressor (master_block) */

/* the master with the FX layer's effects before it (perform.c): the buses' returns in the mix (wet 0 after), the layer
 * on the sum at the full level, then master_block. (M-Vave-OP1: the layer took the sum after MASTER until HOME >
 * MASTER put the knob at the end) */
static __attribute__((noinline)) void perf_master(int32_t *out, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        mix_l[i] = (mix_l[i] + wet[i]) & ~3;            /* (dyn.c master_block says why ~3) */
        mix_r[i] = (mix_r[i] + wet[i]) & ~3;
        wet[i] = 0;
    }
    perf_block(mix_l, mix_r, n);
    master_block(out, n, ~0);
}

/* ---- the second core (docs/DUALCORE_PLAN.md; after Felucca 1.6.1 and Jangada, github.com/zednaked/jangada, GPL-3.0)
 * renders some of the parts of each block into its own sums (mx[1]) while this core renders the others; then this core
 * adds them in and goes on (the LFOs' blocks, the FX layer, the chain, the master) as with one core. The sums are
 * integers, so the order they add up in changes nothing: the output is bit for bit one core's. A part's render touches
 * its own state only (its track_t with its SPREAD, ENV SYNC and LFO phase, its voices, its matrix: mod.c MOD, its SPREAD
 * buffer, its engine's memory, its SLICER, DRIVE and EQ; the engines' scratch is on the stack) and what nobody writes
 * meanwhile (song, the settings, the tables, the FX layer's values of this block); what the parts share is done before
 * the split (events_block: the notes, voice stealing; perf_begin) or after it (the LFOs' S&H draw from the shared
 * generator, part by part: voice.c track_lfo_end; shim_live). tests/dualcore_test.c checks it, under ThreadSanitizer
 * too. Without a second core (the host, the browser, until it is started, one that stopped answering) C1_ON() is 0 and
 * this core renders them all */
#if FELUCCA_CPU2
#ifndef C1_RUN
#define C1_ON() 0                                       /* a second core is answering */
#define C1_RUN(fn, arg) 0                               /* hand it fn(arg): 1 = taken */
#define C1_WAIT() 0                                     /* wait for it: 0 = done, else it stopped (its parts not done) */
#endif
#ifndef C1_SPLIT
#define C1_SPLIT() mix_split()
#endif
#ifndef C1_FPU
#define C1_FPU() 1                                      /* the second core's FPU is right (the unit: checked at its start,
                                                         * cpu2.c; the host: plain float either way) */
#endif
static uint32_t c1_blocks;                              /* blocks the second core took parts of (console, INFO) */

static void c1_parts(uint32_t arg)                      /* arg: the parts (bit k = part k) | n << 8 */
{
    uint32_t i, n = arg >> 8;
    mixsum_t *sm = &mx[1];
    for (i = 0; i < n; i++)
        sm->l[i] = sm->r[i] = sm->c[i] = sm->d[i] = sm->v[i] = sm->sh[i] = 0;
    for (i = 0; i < NPART; i++)
        if (arg >> i & 1u)
            mix_part(&trk[i], n, sm);
}

/* the parts that compute on the FPU: the Plaits engines and ACID (ENGINES[] 17..22, core.h FM1_FPU) and a part whose
 * EQ runs (eq.c). Nobody had run the second core's FPU on an FM-1 before: until it has given the right results at its
 * start (C1_FPU), these stay on this core */
static __attribute__((noinline)) uint32_t mix_fpu_parts(void)
{
    uint32_t i, m = 0;
    for (i = 0; i < NPART; i++)
        if (eng_idx(trk[i].engine) >= 17u || (eqa.run[i] | eqa.live[i] | eqx.req[i]))
            m |= 1u << i;
    return m;
}
/* which parts go to the second core: the split of the last block's costs that leaves the busier core the least; none
 * when that saves less than handing them over costs (C1_HAND) */
#define C1_HAND 40u
static uint32_t mix_split(void)
{
    uint32_t m, i, best = 0, all = 0, lo, keep = C1_FPU() ? 0u : mix_fpu_parts();
    for (i = 0; i < NPART; i++)
        all += part_cost[i];
    lo = all;
    for (m = 1; m < (1u << NPART) - 1u; m++) {
        uint32_t a = 0, w;
        if (m & keep)
            continue;
        for (i = 0; i < NPART; i++)
            if (m >> i & 1u)
                a += part_cost[i];
        w = (a > all - a ? a : all - a) + C1_HAND;
        if (w < lo) {
            lo = w;
            best = m;
        }
    }
    return best;
}

/* the second core's sums into this core's (and the parts' SHIMMER flags into shim_live) */
static __attribute__((noinline)) void c1_add(uint32_t n)
{
    const mixsum_t *sm = &mx[1];
    uint32_t i;
    for (i = 0; i < n; i++) {
        mix_l[i] += sm->l[i];
        mix_r[i] += sm->r[i];
        send_c[i] += sm->c[i];
        send_d[i] += sm->d[i];
        send_r[i] += sm->v[i];
        send_sh[i] += sm->sh[i];
    }
}

/* the parts of a block: those of the split (mix_split) on the second core, the others here, then its sums added */
static __attribute__((noinline)) void mix_parts2(uint32_t n)
{
    uint32_t i, m = C1_SPLIT() & ((1u << NPART) - 1u);
    if (m && !C1_RUN(c1_parts, m | n << 8))
        m = 0;
    c1_blocks += m != 0;
    for (i = 0; i < NPART; i++)
        if (!(m >> i & 1u))
            mix_part(&trk[i], n, &mx[0]);
    if (m) {
        if (C1_WAIT())                                  /* it stopped answering (held from now on): its parts here */
            c1_parts(m | n << 8);
        c1_add(n);
    }
}

/* after the parts, on this core: what they share (the LFOs' S&H from the shared generator, part by part as one core
 * draws it: voice.c; shim_live, from the sums' flags) */
static __attribute__((noinline)) void mix_parts_end(void)
{
    uint32_t i;
    for (i = 0; i < NPART; i++)
        track_lfo_end(&trk[i]);
    if (mx[0].shl | mx[1].shl) {                        /* a part fed the SHIMMER (shimmer_send) */
        shim_live = 1;
        mx[0].shl = mx[1].shl = 0;
    }
}
#endif

static void mix_block(int32_t *out, uint32_t n)
{
    uint32_t i;
    int perf;
    for (i = 0; i < n; i++)
        send_c[i] = send_d[i] = send_r[i] = send_sh[i] = shim_wet[i] = mix_l[i] = mix_r[i] = 0;
    events_block(n);
    perf = perf_begin(n);                               /* the FX hold layer at work (perform.c) */
#if FELUCCA_CPU2
    if (C1_ON())
        mix_parts2(n);
    else
#endif
        for (i = 0; i < NPART; i++) {
            mix_part(&trk[i], n SM_ARG(&mx[0]));
#ifdef FX_PART_TAP
            FX_PART_TAP(i, n);                          /* (M-Vave-OP1, web/bounce: each part's share of mix_l / mix_r;
                                                         * one core) */
#endif
        }
#if FELUCCA_CPU2
    mix_parts_end();
#endif
    if (perf)
        perf_pre(mix_l, mix_r, send_d, send_r, n);
    cho_bus(send_c, n);                                 /* the chain: CHORUS -> SHIMMER -> DELAY -> REVERB */
    if (cho_on && chain_g(1))
        chain_shimmer_in(n);
    shimmer_bus(shim_wet, n);
    if (cho_on | shim_live)
        chain_delay_in(n);
    fx_buses(send_d, send_r, wet, n);
    if (cho_on)
        cho_side_mix(n);                                /* CHORUS: stereo */
    if (hl.side)
        rev_side_mix(mix_l, mix_r, n);                  /* HALL: stereo */
    if (shim_live) shimmer_return(n);
    if (scope_mix)
        scope_take_mix(n);
    if (perf) {
        perf_master(out, n);
#ifdef FX_MIX_TAP
        FX_MIX_TAP(out, n, 1);                          /* (web/bounce: the master as USB audio has it; mix_l / mix_r
                                                         * what went into master_out, the buses and the layer in it) */
#endif
        return;
    }
    master_block(out, n, ~3);
#ifdef FX_MIX_TAP
    FX_MIX_TAP(out, n, 0);                              /* (mix_l / mix_r the parts', the HALL's side; wet the buses') */
#endif
}
