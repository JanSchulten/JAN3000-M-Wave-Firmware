/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: the FPU probe (MENU > SYSTEM > INFO, OCT+; docs/PLAITS_PLAN.md). Felucca never used the chip's FPU; before
 * the Plaits engines rely on it, this measures on the unit what the host cannot: that its results are right and what it
 * costs. Run on request only, in the main loop, never at boot: if the FPU were off and an instruction trapped, the crash
 * report shows and the unit restarts (main.c fm1_fault). It refuses while the boot guard still counts this boot
 * (bootguard.pending, the first 30 s), so such a crash can never count as a failed boot.
 *   CHECK  a few results bit for bit (multiply-add, divide, float <-> int) and a waveshaper render hashed (pws_render,
 *          16 blocks): equal to the host's, as on an IEEE single FPU (tests/fpu_probe_test.c keeps FPB_WS_HASH)
 *   TIME   TIMER4 (24 MHz) round each run, the shortest of several (the audio ISR interrupts some): a float
 *          multiply-add, an integer one, a float divide (ns, a dependent chain each); one voice of the waveshaper, a
 *          32-sample block, as per mille of the audio's time (32 samples at 44.1 kHz: 17 415 ticks)
 * Its functions are noinline: the chains are measured as compiled, not folded into the caller. */
/* (plaits_ws.c: included by engines.c with the WSHAPE engine) */

#define FPB_WS_HASH 0x5D2308EDu              /* FNV-1a over the bits of pws_render's 16 blocks (tests/fpu_probe_test.c) */
#define FPB_N 256u                            /* links of a timed chain */
#define FPB_RUNS 16u
#define FPB_BLOCK_TICKS 17415u                /* 32 samples at 44.1 kHz in TIMER4 ticks */
enum { FPB_NONE, FPB_OK, FPB_WS_DIFF, FPB_WRONG };
static struct {
    uint8_t state;                            /* FPB_*: NONE not run */
    uint16_t f_ns10, i_ns10, d_ns10;          /* ns x 10: float multiply-add, integer multiply-add, float divide */
    uint16_t ws_pm;                           /* one waveshaper voice: per mille of the audio's time */
    uint32_t ws_hash, runs;
} fpb;

__attribute__((noinline)) FM1_FPU static float fpb_fmac(float x, float a, float b, uint32_t n)
{
    while (n--)
        x = x * a + b;
    return x;
}
__attribute__((noinline)) static int32_t fpb_imac(int32_t x, int32_t a, int32_t b, uint32_t n)
{
    while (n--)
        x = x * a + b;
    return x;
}
__attribute__((noinline)) FM1_FPU static float fpb_fdiv(float x, float a, uint32_t n)
{
    while (n--)
        x = a / x;
    return x;
}
/* the exact results: 1.5 x 2.25 + 0.125 = 3.5, 1 / 3 rounded to nearest, (int)-2.75 = -2, 7 -> 7.0 */
__attribute__((noinline)) FM1_FPU static int fpb_check(volatile const float *in, volatile const int32_t *iv)
{
    union { float f; uint32_t u; } a, b, c;
    int32_t k;
    a.f = in[0] * in[1] + in[2];
    b.f = in[3] / in[4];
    k = (int32_t)in[5];
    c.f = (float)iv[0];
    return a.u == 0x40600000u && b.u == 0x3EAAAAABu && k == -2 && c.u == 0x40E00000u;
}
/* 16 blocks of the waveshaper from rest, its parameters moving; the hash of every output's bits */
static pws_t fpb_ws;
static float fpb_out[32], fpb_aux[32];
FM1_FPU static void fpb_ws_block(uint32_t b)
{
    float t = (float)b * (1.0f / 16.0f);
    pws_render(&fpb_ws, 0.0025f + 0.01f * t, 0.2f + 0.6f * t, 0.9f - 0.7f * t, 0.1f + 0.8f * t, fpb_out, fpb_aux, 32u, 3u);
}
static uint32_t fpb_ws_hash(void)
{
    uint32_t b, i, h = 2166136261u;
    union { float f; uint32_t u; } v;
    pws_init(&fpb_ws);
    for (b = 0; b < 16u; b++) {
        fpb_ws_block(b);
        for (i = 0; i < 32u; i++) {
            v.f = fpb_out[i];
            h = (h ^ v.u) * 16777619u;
            v.f = fpb_aux[i];
            h = (h ^ v.u) * 16777619u;
        }
    }
    return h;
}

static uint32_t fpb_ns10(uint32_t ticks, uint32_t n) { return (ticks * 10000u / 24u + n / 2u) / n; }   /* ticks of n -> ns x 10 a link */
static void fpu_probe(void)
{
    static volatile const float IN[6] = {1.5f, 2.25f, 0.125f, 1.0f, 3.0f, -2.75f};
    static volatile const int32_t IV[1] = {7};
    static volatile float sink;
    static volatile int32_t isink;
    uint32_t r, t0, d, best_f = ~0u, best_i = ~0u, best_d = ~0u, best_w = ~0u;
    int ok = fpb_check(IN, IV);
    fpb.ws_hash = fpb_ws_hash();
    for (r = 0; r < FPB_RUNS; r++) {
        t0 = fm1_ticks();
        sink = fpb_fmac(IN[0], 0.999f, 0.001f, FPB_N);
        d = fm1_ticks() - t0;
        best_f = d < best_f ? d : best_f;
        t0 = fm1_ticks();
        isink = fpb_imac(IV[0], 1103515245, 12345, FPB_N);
        d = fm1_ticks() - t0;
        best_i = d < best_i ? d : best_i;
        t0 = fm1_ticks();
        sink = fpb_fdiv(IN[4], 1.0f, FPB_N);
        d = fm1_ticks() - t0;
        best_d = d < best_d ? d : best_d;
    }
    pws_init(&fpb_ws);
    for (r = 0; r < 4u * FPB_RUNS; r++) {          /* (the first runs warm the cache) */
        t0 = fm1_ticks();
        fpb_ws_block(r & 15u);
        d = fm1_ticks() - t0;
        best_w = d < best_w ? d : best_w;
    }
    fpb.f_ns10 = (uint16_t)fpb_ns10(best_f, FPB_N);
    fpb.i_ns10 = (uint16_t)fpb_ns10(best_i, FPB_N);
    fpb.d_ns10 = (uint16_t)fpb_ns10(best_d, FPB_N);
    fpb.ws_pm = (uint16_t)((best_w * 1000u + FPB_BLOCK_TICKS / 2u) / FPB_BLOCK_TICKS);
    fpb.state = (uint8_t)(!ok ? FPB_WRONG : fpb.ws_hash != FPB_WS_HASH ? FPB_WS_DIFF : FPB_OK);
    fpb.runs++;
}

/* v / 10 with one decimal and its leading zero, "0.4" (libc.c fmt_fix writes ".4") */
static void fpb_fix1(char *b, uint32_t v)
{
    fmt_int(b, (int32_t)(v / 10u));
    b += str_len(b);
    *b++ = '.';
    *b++ = (char)('0' + v % 10u);
    *b = 0;
}

/* INFO's OCT+: the probe, or why not yet */
static void fpu_probe_go(void)
{
    if (bootguard.pending) {
        ui_message("FPU TEST AFTER 30 S");
        return;
    }
    fpu_probe();
    ui_message(fpb.state == FPB_OK ? "FPU OK" : fpb.state == FPB_WS_DIFF ? "FPU: WAVESHAPER DIFFERS" : "FPU WRONG");
}
