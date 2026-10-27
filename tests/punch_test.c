/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* E2, the OP-Z's punch-ins on the FX layer (firmware/src/perform.c), on the firmware's own sources (hostsim.c):
 *   CODES    six effects appended before the mutes (PF_NFX 21: a 5-bit code in the key map, + 1 below FXT_MUTE), named,
 *            each cell its own; MAP B's defaults in the OP-Z's order; its codes in their own 10 bytes, MAP A's untouched.
 *   DUCK     on a steady input: down to 0.1 within 2.2 ms of each beat (in time with the clock, also stopped), back to
 *            1 by three quarters of it, no step above what that slope makes; KNOB 4 DEPTH 100: none; let go: the live
 *            signal back bit for bit.
 *   STEREO   a mono sine panned: right on the first 1/16 of each pair, left on the second (6 dB at least), the power
 *            kept (equal-power pan); let go: bit for bit.
 *   PITCH    5TH UP / 4TH DN: a 220 Hz sine comes out at 329.6 / 164.8 Hz (the strongest line within one splice spacing)
 *            with no 220 Hz left (no live part); KNOB 4 DEPTH 100: the live signal (no shimmer); let go: bit for bit.
 *   RAMP UP  the tape from standing: quiet at first, the speed rising past 1 (a sine read faster at 2/3 of the ramp),
 *            the read on the writing at the end (in time: no lag), then the live signal bit for bit while still held;
 *            a silent side silent.
 *   GLITCH   from the next 1/16 (nothing before it changes, the first slice is live: nothing recorded yet); then each
 *            1/16 one of the slices, at least six kinds over 60, about a quarter live; the last 1/16 again / backwards
 *            match the input a 1/16 back, silence is silent, a live slice is the input; no click, bounded, a silent
 *            side silent; let go: bit for bit.
 *   MAP B    FX + ALGORITHM right: MAP B (the message, the header), the keys play its effects, a key held takes the new
 *            map's at once; left: MAP A; PRESETS / EDIT on a key in MAP B change MAP B's bytes only; kept with the
 *            settings, older settings: the defaults; FX TRACKS list the new effects. With a directory: the map's screens.
 *   With a second directory: WAV demos of each on a song.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("punch: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

/* ------------------------------------------------------- the stage alone --- */
#define NT (10u * 44100u)
#define P16 5512u                                  /* a 1/16 at 120 BPM (beat_samples() >> 2) */
#define BEAT 22048u                                /* (the clock's beat: four of its 1/16s) */
static int32_t in_l[NT], in_r[NT], out_l[NT], out_r[NT];
static void sig_sine(double fl, double al, double fr, double ar)
{
    uint32_t t;
    for (t = 0; t < NT; t++) {
        in_l[t] = (int32_t)lrint(al * sin(2 * M_PI * fl * t / FS));
        in_r[t] = (int32_t)lrint(ar * sin(2 * M_PI * fr * t / FS + 1.0));
    }
}
static void p_reset(void)
{
    memset(&pf, 0, sizeof pf);
    pf.src = pf.next = PF_N;
    pf.lc = PF_TOP;
    pf.mg[0] = pf.mg[1] = pf.mg[2] = pf.mg[3] = 32768;
    perf_held = perf_act = perf_latched = 0;
    perf_fxt = perf_fxt_now = 0;
    perf_kill = 0;
    memcpy((void *)perf_map, PF_DEF, sizeof PF_DEF);
    memset((void *)perf_k, 0, sizeof perf_k);
    sl_lent = 0;
    memset(sl, 0, sizeof sl);
}
typedef struct { uint32_t t; int e; } ev_t;        /* at sample t: press +(e + 1), let go -(e + 1) */
static void (*on_block)(uint32_t t);               /* (after each block: what the test looks at) */
static void prun(uint32_t bpm, int playing, const ev_t *ev, uint32_t nev, uint32_t t1, int8_t k4)
{
    uint32_t t, k = 0;
    p_reset();
    perf_k[3] = k4;
    song.g[G_BPM] = (int16_t)bpm;
    song.g[G_CLOCK] = 0;
    song.playing = (uint8_t)playing;
    perf_start();
    for (t = 0; t < t1; t += CTL) {
        while (k < nev && ev[k].t <= t) {
            perf_press((uint32_t)(ev[k].e > 0 ? ev[k].e - 1 : -ev[k].e - 1), ev[k].e > 0);
            k++;
        }
        memcpy(out_l + t, in_l + t, CTL * 4);
        memcpy(out_r + t, in_r + t, CTL * 4);
        if (perf_begin(CTL))
            perf_block(out_l + t, out_r + t, CTL);
        if (on_block)
            on_block(t);
    }
    song.playing = 0;
}
static int same(uint32_t a, uint32_t b)            /* out == in on [a, b), both sides */
{
    for (; a < b; a++)
        if (out_l[a] != in_l[a] || out_r[a] != in_r[a])
            return 0;
    return 1;
}
static int32_t max_step(const int32_t *x, uint32_t a, uint32_t b)
{
    int32_t m = 0;
    for (; a + 1u < b; a++)
        m = abs(x[a + 1] - x[a]) > m ? abs(x[a + 1] - x[a]) : m;
    return m;
}
static int32_t peak(const int32_t *x, uint32_t a, uint32_t b)
{
    int32_t m = 0;
    for (; a < b; a++)
        m = abs(x[a]) > m ? abs(x[a]) : m;
    return m;
}
static double rms(const int32_t *x, uint32_t a, uint32_t b)
{
    double s = 0;
    uint32_t n = b - a;
    for (; a < b; a++)
        s += (double)x[a] * x[a];
    return sqrt(s / n);
}
static double dft_pow(const int32_t *x, uint32_t a, uint32_t b, double f)
{
    double re = 0, im = 0;
    uint32_t t;
    for (t = a; t < b; t++) {
        double w = 0.5 - 0.5 * cos(2 * M_PI * (t - a) / (b - a));   /* (Hann) */
        re += w * x[t] * cos(2 * M_PI * f * t / FS);
        im += w * x[t] * sin(2 * M_PI * f * t / FS);
    }
    return re * re + im * im;
}
static double peak_freq(const int32_t *x, uint32_t a, uint32_t b, double lo, double hi)
{
    double f, best = lo, bp = -1;
    for (f = lo; f <= hi; f += 0.25) {
        double p = dft_pow(x, a, b, f);
        if (p > bp) {
            bp = p;
            best = f;
        }
    }
    return best;
}
static uint32_t zero_x(const int32_t *x, uint32_t a, uint32_t b)   /* upward zero crossings */
{
    uint32_t n = 0;
    for (; a + 1u < b; a++)
        n += x[a] < 0 && x[a + 1] >= 0;
    return n;
}

/* ------------------------------------------------------------ CODES --- */
static void test_codes(void)
{
    static const uint8_t B[PF_KEYS] = {PF_DUCK, PF_LPF, PF_R16, PF_STER, PF_R32, PF_PUP, PF_ETHR, PF_RAMP,
                                       PF_PDN, PF_REV, PF_TAPE, PF_GLT, PF_HPF, PF_RTHR, PF_CFRZ, PF_FRZ};
    uint8_t a[10], b[10], a0[10];
    uint32_t p, e, ok = PF_NFX == 21 && PF_M1 == 21 && PF_N == 25 && PF_NFX < FXT_MUTE && FXT_CHOICES == 2u + PF_N;
    for (e = PF_DUCK; e <= PF_GLT; e++) {
        uint32_t j;
        ok &= PF_NAME[e] && PF_CELL[e] && str_len(PF_CELL[e]) <= 4u && fxt_eff(fxt_code(e)) == e;
        for (j = 0; j < PF_NFX; j++)
            ok &= j == e || !str_eq(PF_CELL[j], PF_CELL[e]);
    }
    ok &= str_eq(PF_NAME[PF_DUCK], "DUCK") && str_eq(PF_NAME[PF_GLT], "GLITCH") && str_eq(PF_NAME[PF_NFX], "NONE");
    ck("CODES: DUCK STEREO PITCH 5TH UP / 4TH DN RAMP UP GLITCH after CLOUD FREEZE (21 effects: 5-bit codes, FX TRACKS)", ok);
    for (ok = 1, p = 0; p < PF_KEYS; p++)
        ok &= PF_DEF[PF_KEYS + p] == B[p] && perf_map[PF_KEYS + p] == B[p];
    ck("  MAP B's defaults: the OP-Z's order (DUCK, FILTER, LOOP 1, STEREO, LOOP 2, PITCH, ECHO, RAMP UP, .., RANDOM)", ok);
    memset(a, 0, sizeof a);
    memset(b, 0, sizeof b);
    for (ok = 1, p = 0; p < 2u * PF_KEYS; p++)
        ok &= perf_map_of(p < PF_KEYS ? a : b, p) == PF_DEF[p];
    perf_map_put(a, 3, PF_GLT);                     /* MAP A: REVERSE -> GLITCH */
    memcpy(a0, a, sizeof a);
    for (p = 0; p < PF_KEYS; p++)                   /* MAP B: each key the effect 5 on from its default, or NONE */
        perf_map_put(b, PF_KEYS + p, p == 7u ? PF_N : (PF_DEF[PF_KEYS + p] + 5u) % PF_NFX);
    for (p = 0; p < PF_KEYS; p++)
        ok &= perf_map_of(b, PF_KEYS + p) == (p == 7u ? PF_N : (PF_DEF[PF_KEYS + p] + 5u) % PF_NFX);
    ok &= !memcmp(a, a0, sizeof a) && perf_map_of(a, 3) == PF_GLT && perf_map_of(a, 4) == PF_LPF;
    perf_map_put(b, PF_KEYS + 2u, PF_R16);          /* its default again: code 0 */
    ok &= perf_map_of(b, PF_KEYS + 2u) == PF_R16 && !((b[1] >> 2) & 31u);
    ck("  the key maps: 0 = the default (MAP A's, MAP B's), any effect or NONE on MAP B's keys, MAP A's bytes untouched", ok);
}

/* ------------------------------------------------------------- DUCK --- */
static void test_duck(void)
{
    static const ev_t ev[] = {{0, PF_DUCK + 1}, {3u * BEAT + 1000u, -(PF_DUCK + 1)}};
    uint32_t t, b, mn, ok = 1;
    for (t = 0; t < NT; t++)
        in_l[t] = in_r[t] = 16000;
    prun(120, 1, ev, 2, 4u * BEAT, 0);
    for (b = 1; b < 3u; b++) {
        uint32_t s = b * BEAT;
        int32_t lo = out_l[s];
        for (mn = s, t = s; t < s + BEAT / 2u; t++)
            if (out_l[t] < lo) {
                lo = out_l[t];
                mn = t;
            }
        ok &= mn >= s + 80u && mn <= s + 140u && abs(lo - 1600) < 200;
        ok &= abs(out_l[s + BEAT * 3u / 4u + 200u] - 16000) < 40 && out_l[s - 1u] > 15900;
        for (t = s + 140u; t < s + BEAT * 3u / 4u; t++)
            ok &= out_l[t + 1u] >= out_l[t] - 1;    /* (back up: never down again) */
        ok &= out_r[s + 2000u] == out_l[s + 2000u];
    }
    ck("DUCK: down to 0.1 within 2.2 ms of each beat, back by 3/4 of it, steadily, both sides alike", ok);
    ck("  no step bigger than the slope makes (16000 * 0.9 / 96 a sample; pressed on the beat: 2x)",
       max_step(out_l, BEAT / 2u, 3u * BEAT) <= 170 && max_step(out_l, 0, BEAT / 2u) <= 330);
    ck("  let go: the live signal back, bit for bit, after the 2.9 ms ramp", same(3u * BEAT + 1000u + 160u, 4u * BEAT));
    prun(120, 1, ev, 1, 2u * BEAT, 100);
    ck("  KNOB 4 DEPTH 100: no ducking (within 1 %)", peak(out_l, BEAT, 2u * BEAT) <= 16000 &&
       out_l[BEAT + 110u] > 15800);
    prun(90, 0, ev, 1, 3u * 29400u, 0);
    for (ok = 1, b = 1; b < 3u; b++)                /* (the fall may start within the block before: 0.7 ms) */
        ok &= out_l[b * 29400u + 110u] < 2000 && out_l[b * 29400u - 40u] > 15900;
    ck("  stopped, 90 BPM: it pumps on the free clock, a beat apart", ok);
}

/* ----------------------------------------------------------- STEREO --- */
static void test_stereo(void)
{
    static const ev_t ev[] = {{0, PF_STER + 1}, {12u * P16 + 7u, -(PF_STER + 1)}};
    uint32_t j, t, ok = 1;
    double rin;
    for (t = 0; t < NT; t++)
        in_l[t] = in_r[t] = (int32_t)lrint(16000.0 * sin(2 * M_PI * 220.0 * t / FS));
    prun(120, 1, ev, 2, 14u * P16, 0);
    rin = rms(in_l, P16, 2u * P16);
    for (j = 2; j < 10u; j++) {
        uint32_t a = j * P16 + P16 / 4u, b = j * P16 + 3u * P16 / 4u;
        double l = rms(out_l, a, b), r = rms(out_r, a, b);
        ok &= j & 1u ? l > 2.0 * r : r > 2.0 * l;
        ok &= fabs((l * l + r * r) / (2.0 * rin * rin) - 1.0) < 0.12;
    }
    ck("STEREO: right on the first 1/16 of a pair, left on the second (6 dB at least), the power kept (12 %)", ok);
    ck("  no click (a 220 Hz sine's own slope at most, with the pan's)", max_step(out_l, 0, 12u * P16) < 1000 &&
       max_step(out_r, 0, 12u * P16) < 1000);
    ck("  let go: the live signal back, bit for bit", same(12u * P16 + 7u + 192u, 14u * P16));
}

/* ------------------------------------------------------------ PITCH --- */
static void test_pitch(void)
{
    static const ev_t up[] = {{0, PF_PUP + 1}, {3u * 44100u, -(PF_PUP + 1)}};
    static const ev_t dn[] = {{0, PF_PDN + 1}, {3u * 44100u, -(PF_PDN + 1)}};
    uint32_t ok;
    double f, pk;
    sig_sine(220.0, 12000.0, 220.0, 6000.0);
    prun(120, 1, up, 2, 3u * 44100u + 4096u, 0);
    f = peak_freq(out_l, 22050, 110250, 150.0, 500.0);
    pk = dft_pow(out_l, 22050, 110250, f);
    ok = fabs(f - 329.63) < 11.0 && dft_pow(out_l, 22050, 110250, 220.0) < pk / 10.0;
    f = peak_freq(out_r, 22050, 110250, 150.0, 500.0);
    ok &= fabs(f - 329.63) < 11.0;
    ck("PITCH 5TH UP: 220 Hz comes out at 329.6 (the strongest line within a splice spacing, 10.7 Hz), no 220 left", ok);
    ck("  let go: the live signal back, bit for bit", same(3u * 44100u + 160u, 3u * 44100u + 4096u));
    prun(120, 1, dn, 2, 3u * 44100u, 0);
    f = peak_freq(out_l, 22050, 110250, 100.0, 400.0);
    pk = dft_pow(out_l, 22050, 110250, f);
    ok = fabs(f - 164.81) < 5.5 && dft_pow(out_l, 22050, 110250, 220.0) < pk / 10.0;
    ck("PITCH 4TH DN: 220 Hz comes out at 164.8 (within 5.4 Hz), no 220 left", ok);
    prun(120, 1, up, 1, 44100u, 100);
    {
        uint32_t t;
        int32_t e = 0;
        for (t = 4096; t < 44100u; t++)
            e = abs(out_l[t] - in_l[t]) > e ? abs(out_l[t] - in_l[t]) : e;
        ck("  KNOB 4 DEPTH 100: the live signal within 0.7 % (DEPTH, not the harmonizer's shimmer)", e < 80);
    }
}

/* ---------------------------------------------------------- RAMP UP --- */
static uint32_t rp_t0, rp_lag = 0xFFFFFFFFu;
static void ramp_watch(uint32_t t)                 /* (the lag at the last block of the ramp) */
{
    if (pf.mode == BM_RAMP && pf.rt < pf.rT)
        rp_lag = pf.wr - (pf.rpos >> 16);
    (void)t;
}
static void test_ramp(void)
{
    static const ev_t ev[] = {{4416u, PF_RAMP + 1}, {4416u + 2u * BEAT, -(PF_RAMP + 1)}};
    uint32_t t0 = 4416u, T = BEAT, ok;
    sig_sine(441.0, 16000.0, 0.0, 0.0);
    for (t0 = 0; t0 < NT; t0++)
        in_r[t0] = 0;
    t0 = 4416u;
    rp_lag = 0xFFFFFFFFu;
    on_block = ramp_watch;
    prun(120, 1, ev, 2, t0 + 3u * BEAT, 0);
    on_block = 0;
    ok = zero_x(out_l, t0, t0 + T / 10u) <= 6u && zero_x(in_l, t0, t0 + T / 10u) >= 21u;
    ck("RAMP UP: the tape from standing: the first tenth of the ramp at most 6 cycles where the input has 22", ok);
    {   /* zero crossings: x 0.2 .. 0.3 at about 0.78x (344 Hz), 0.6 .. 0.73 at about 1.32x (583 Hz) */
        uint32_t a = t0 + T / 5u, b = t0 + 3u * T / 10u, c = t0 + 3u * T / 5u, d = t0 + 73u * T / 100u;
        double f1 = zero_x(out_l, a, b) * (double)FS / (b - a), f2 = zero_x(out_l, c, d) * (double)FS / (d - c);
        ok = f1 > 300.0 && f1 < 390.0 && f2 > 545.0 && f2 < 620.0;
        printf("  ramp: %.0f Hz at a fifth, %.0f Hz at two thirds (441 in)\n", f1, f2);
    }
    ck("  the speed rises past 1: a 441 Hz sine read slower at a quarter, faster (4/3) at two thirds", ok);
    ck("  in time at the end: the read on the writing (at most 3 frames behind)", rp_lag <= 3u);
    ck("  then the live signal bit for bit, still held", same(t0 + T + 192u, t0 + 2u * BEAT));
    ck("  bounded (the input's peak), the silent side silent", peak(out_l, 0, t0 + 2u * BEAT) <= 16100 &&
       peak(out_r, 0, t0 + 3u * BEAT) == 0);
}

/* ----------------------------------------------------------- GLITCH --- */
static uint8_t gl_kind[128];
static uint32_t gl_at[128], gl_n;
static void glt_watch(uint32_t t)
{
    if (pf.mode == BM_GLT && pf.gn <= CTL && gl_n < 128u && (!gl_n || t > gl_at[gl_n - 1u] + CTL)) {
        gl_kind[gl_n] = pf.gk;
        gl_at[gl_n++] = t + CTL - pf.gn;            /* (the slice's first sample) */
    }
}
static void test_glitch(void)
{
    static const ev_t ev[] = {{3000u, PF_GLT + 1}, {3000u + 62u * P16, -(PF_GLT + 1)}};
    uint32_t t, k, kinds = 0, pass = 0, ok_r16 = 1, ok_rev = 1, ok_gate = 1, ok_pass = 1, n16 = 0, nrev = 0;
    for (t = 0; t < NT; t++) {
        in_l[t] = (int32_t)lrint(16000.0 * sin(2 * M_PI * 110.0 * t / FS));
        in_r[t] = 0;
    }
    gl_n = 0;
    on_block = glt_watch;
    prun(120, 1, ev, 2, 3000u + 64u * P16, 0);
    on_block = 0;
    ck("GLITCH: from the next 1/16 (5512), the first slice live (nothing recorded): bit for bit up to 2 1/16s",
       same(0, 2u * P16) && gl_n > 2u && gl_at[0] == P16);
    for (k = 1; k < gl_n && gl_at[k] + P16 < 3000u + 62u * P16; k++) {
        uint32_t b = gl_at[k], j;
        kinds |= 1u << gl_kind[k];
        pass += gl_kind[k] == GK_PASS;
        for (j = 300; j < P16 - 300u; j++) {
            int32_t o = out_l[b + j];
            if (gl_kind[k] == GK_R16)
                ok_r16 &= abs(o - in_l[b - P16 + j]) < 500;
            else if (gl_kind[k] == GK_REV)
                ok_rev &= abs(o - in_l[b - j]) < 600;
            else if (gl_kind[k] == GK_GATE)
                ok_gate &= abs(o) < 40;
            else if (gl_kind[k] == GK_PASS)
                ok_pass &= o == in_l[b + j];
        }
        n16 += gl_kind[k] == GK_R16;
        nrev += gl_kind[k] == GK_REV;
    }
    printf("  glitch: %u slices, kinds 0x%02X, %u live, %u LAST 1/16, %u BACKWARDS\n", k - 1u, kinds, pass, n16, nrev);
    ck("  each 1/16 a slice: at least six kinds over 60, a tenth to 45 % live",
       __builtin_popcount(kinds) >= 6 && pass * 10u >= k - 1u && pass * 100u <= 45u * (k - 1u));
    ck("  the last 1/16 again / backwards: the input a 1/16 back (within 3 %); silence silent; live: the input",
       ok_r16 && ok_rev && ok_gate && ok_pass && n16 && nrev);
    printf("  glitch: largest step %d (the 110 Hz sine's own: 251)\n", max_step(out_l, 0, 3000u + 63u * P16));
    ck("  no click (a step at most 1200), bounded, the silent side silent", max_step(out_l, 0, 3000u + 63u * P16) <= 1200 &&
       peak(out_l, 0, NT > 3000u + 64u * P16 ? 3000u + 64u * P16 : NT) <= 16100 && peak(out_r, 0, 3000u + 64u * P16) == 0);
    ck("  let go: the live signal back, bit for bit", same(3000u + 62u * P16 + 160u, 3000u + 64u * P16));
}

/* ------------------------------------------------------------ MAP B --- */
static void test_map_b(const char *dir)
{
    uint32_t f3 = white(0), g3 = white(1), c5 = white(11);
    uint8_t a0[10];
    int ok;
    ui_power_on();
    memset(fx_keys, 0, 10);
    memset(fx_keys_b, 0, 10);
    frame();
    memcpy(a0, fx_keys, 10);
    btn_down(B_FX);
    frames(512);
    ok = ui.layer == LAYER_FX && !perf_bank && str_eq(layer_head(), "[FX] HOLD");
    if (dir)
        screen_save(dir, "fx_map_a");
    turn(EN_ALGO, 1);
    frame();
    ok &= perf_bank == 1u && msg_is("FX MAP B") && str_eq(layer_head(), "[FX] HOLD B");
    if (dir)
        screen_save(dir, "fx_map_b");
    key_down(f3);
    frame();
    ok &= perf_held == PF_BIT(PF_DUCK);
    key_up(f3);
    frame();
    ck("MAP B: FX + ALGORITHM right, the message and the header; F3 plays DUCK", ok && !perf_held);
    key_down(g3);                                   /* G3 held: MAP B's FILTER (LPF); to MAP A: REPEAT 1/16 at once */
    frame();
    ok = perf_held == PF_BIT(PF_LPF);
    turn(EN_ALGO, -1);
    frame();
    keyboard_block();
    ok &= !perf_bank && msg_is("FX MAP A") && perf_held == PF_BIT(PF_R16) && str_eq(layer_head(), "[FX] HOLD");
    key_up(g3);
    frame();
    ck("  ALGORITHM left: MAP A; a key held takes the effect of the map turned to", ok && !perf_held);
    turn(EN_ALGO, 1);
    frame();
    key_down(c5);                                   /* C5: GLITCH; PRESETS back one: RAMP UP; EDIT: GLITCH again */
    frame();
    turn(EN_PRESET, -1);
    keyboard_block();
    ok = perf_map[PF_KEYS + 11u] == PF_RAMP && perf_held == PF_BIT(PF_RAMP) && msg_is("C5 RAMP UP") &&
         !memcmp(fx_keys, a0, 10) && perf_map_of(fx_keys_b, PF_KEYS + 11u) == PF_RAMP && perf_map[11] == PF_PHS;
    if (dir)
        screen_save(dir, "fx_map_b_assign");
    btn_down(B_EDIT); frame(); btn_up(B_EDIT); frame(); keyboard_block();
    ok &= perf_map[PF_KEYS + 11u] == PF_GLT && msg_is("C5 GLITCH (DEF)");
    turn(EN_PRESET, 1);                             /* GLITCH -> NONE (the last effect, then NONE) */
    keyboard_block();
    ok &= perf_map[PF_KEYS + 11u] == PF_N && !perf_held;
    key_up(c5);
    btn_up(B_FX);
    frames(400);
    ck("  PRESETS / EDIT on a key in MAP B: MAP B's bytes only (MAP A's as they were)", ok && !ui.layer);
    {   /* kept with the settings; settings with 0 there (before E2): the defaults */
        persist_t pp;
        uint32_t p;
        settings_export(&pp);
        memset(fx_keys_b, 0, 10);
        frame();
        ok = perf_map[PF_KEYS + 11u] == PF_GLT;
        ok &= settings_import(&pp, (int)sizeof pp) == 1;
        frame();
        ok &= perf_map[PF_KEYS + 11u] == PF_N;
        memset(&pp.favorites.factory[15][0], 0, 10);
        ok &= settings_import(&pp, (int)sizeof pp) == 1;
        frame();
        for (p = 0; p < 2u * PF_KEYS; p++)
            ok &= perf_map[p] == PF_DEF[p];
        ck("  MAP B kept with the settings (exported, imported); zeros there (older settings): the defaults", ok);
    }
    {   /* the FX TRACKS' EFFECT list has them; recorded GLITCH plays from its step */
        uint32_t c;
        ok = 1;
        for (c = PF_DUCK; c <= PF_GLT; c++)
            ok &= fxt_eff(fxt_of_choice(fxt_choice(fxt_code(c)))) == c;
        ck("  FX TRACKS: the six in the EFFECT list (codes 16 .. 21)", ok && fxt_code(PF_GLT) == 21u);
    }
    perf_bank = 0;
}

/* ------------------------------------------------------------ demos --- */
static void demo_song(void)
{
    static const uint8_t ACID[16] = {45, 45, 57, 45, 0, 48, 45, 55, 45, 0, 57, 52, 45, 48, 0, 50};
    static const uint8_t AM[4] = {57, 60, 64, 67};
    track_t *t1 = &trk[0], *t2 = &trk[1], *td = &trk[3];
    uint32_t i;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    p_reset();
    song.g[G_BPM] = 120;
    host_preset(t1, 0, 4);
    host_preset(t2, 1, 5);
    host_drums(td);
    for (i = 0; i < 16u; i++) {
        uint8_t n = ACID[i], d[4], k = 0;
        put_step(t1, i, n ? 1u : 0u, &n, n ? ST_NOTE : ST_REST, 0);
        put_step(t2, i, i % 8u == 0u ? 4u : 0u, AM, i % 8u == 0u ? ST_NOTE : ST_TIE, 0);
        if (i % 4u == 0u) d[k++] = 36;
        if (i % 2u == 0u) d[k++] = 42;
        if (i == 4u || i == 12u) d[k++] = 38;
        put_step(td, i, k, d, k ? ST_NOTE : ST_REST, 0);
    }
    trk[0].p[P_PAN] = -40;
    trk[1].p[P_PAN] = 40;
}
static void demos(const char *dir)
{
    static const struct { const char *name, *what; uint8_t e; } D[] = {
        {"punch_duck", "DUCK: the master pumps in quarters", PF_DUCK},
        {"punch_stereo", "STEREO: the mix panned left and right a 1/16 each", PF_STER},
        {"punch_pitch_up", "PITCH 5TH UP: the whole mix a fifth higher", PF_PUP},
        {"punch_pitch_down", "PITCH 4TH DN: a fourth lower", PF_PDN},
        {"punch_ramp_up", "RAMP UP: the tape stops dead, spins up over a beat and lands in time (every half bar)", PF_RAMP},
        {"punch_glitch", "GLITCH: each 1/16 a random slice (repeats, backwards, octaves, silence)", PF_GLT},
    };
    static int32_t o[2 * CTL];
    uint32_t k, t, i, bar = 4u * BEAT, frames = 4u * bar + 44100u;
    char path[600];
    for (k = 0; k < NELEM(D); k++) {
        FILE *f;
        demo_song();
        snprintf(path, sizeof path, "%s/%s.wav", dir, D[k].name);
        if (!(f = fopen(path, "wb")))
            continue;
        wav_hdr(f, frames);
        transport_req = 1;
        for (t = 0; t < frames; t += CTL) {         /* a bar plain, two with it, a bar plain (RAMP UP: anew each half bar) */
            uint32_t b = t / CTL, m;
            if (D[k].e != PF_RAMP) {
                if (b == bar / CTL)
                    perf_press(D[k].e, 1);
                if (b == 3u * bar / CTL)
                    perf_press(D[k].e, 0);
            } else {
                for (m = 0; m < 4u; m++) {
                    if (b == (bar + m * 2u * BEAT) / CTL)
                        perf_press(PF_RAMP, 1);
                    if (b == (bar + m * 2u * BEAT + 2u * BEAT - P16) / CTL)
                        perf_press(PF_RAMP, 0);
                }
            }
            mix_block(o, CTL);
            for (i = 0; i < CTL && t + i < frames; i++)
                wav_put(f, o[2 * i], o[2 * i + 1]);
        }
        perf_held = 0;
        transport_req = 2;
        mix_block(o, CTL);
        fclose(f);
        printf("punch: demo %s (%s)\n", path, D[k].what);
    }
}

int main(int argc, char **argv)
{
    test_codes();
    test_duck();
    test_stereo();
    test_pitch();
    test_ramp();
    test_glitch();
    test_map_b(argc > 1 ? argv[1] : 0);
    if (argc > 2)
        demos(argv[2]);
    if (bad) {
        printf("punch: %d FAILED\n", bad);
        return 1;
    }
    printf("punch: all ok\n");
    return 0;
}
