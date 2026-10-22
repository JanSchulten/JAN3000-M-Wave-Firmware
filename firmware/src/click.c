/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The metronome's click (1.1, Ideas Discussion #131): MENU > AUDIO CLICK (OFF / REC / ON), CLICK LEVEL (LOW / MID / HIGH)
 * and COUNT-IN (OFF / 1 BAR / 2 BARS). seq.c decides when a beat clicks (click_beat: the song tempo's quarter notes, 4/4,
 * the bar's first beat accented; the count-in's beats whatever CLICK is); this file makes the sound. A short sine blip
 * that decays away (no sample): 1760 Hz on the bar's first beat, 1320 Hz on the others, 6 dB quieter; ~6 ms
 * decay constant, silent after ~35 ms. It is added after the master (audio.c audio_block, after uac_tap and USB LEVEL):
 * never through the FX, the reverb or the limiter, never into a recording (the sequence or USB audio), only into the
 * DAC (headphones / line out, the speaker), scaled by MASTER as the rest of what the DAC gets.
 * The MENU's three settings are bits of a byte no engine uses (ui.c ui_rec_prefs), copied here every UI frame
 * (ui_input.c); 0 in every older setting = OFF / MID / OFF, today's behaviour. */
enum { CLICK_OFF, CLICK_REC, CLICK_ON };
static volatile uint8_t click_mode;         /* CLICK_*: when the beats click while the transport runs */
static volatile uint8_t click_lvl = 1;      /* 0 LOW 1 MID 2 HIGH */
static volatile uint8_t cin_bars;           /* COUNT-IN: 0 off, 1 or 2 bars */
static volatile uint8_t click_req;          /* seq.c: 1 a beat, 2 the bar's first; the next block starts it */
/* the magic circle (s, c: a sine and its quadrature) at k = 2 sin(pi f / FS), Q15; env Q15, decaying 1/256 a sample */
#define CLICK_K_DOWN 8195                    /* 1760 Hz */
#define CLICK_K_BEAT 6154                    /* 1320 Hz */
#define CLICK_AMP 16384                      /* the circle's amplitude */
static const uint16_t CLICK_GAIN[3] = {1024, 2048, 4096};   /* LOW MID HIGH, Q12: the downbeat's peak -18 / -12 / -6 dB
                                                             * of a full-scale mix (Q15) at MASTER's top */
static struct { int32_t s, c, k, env, g; } clk;

/* add the click to one block of DAC samples (stereo Q15, after the master): out of line, a loop only while it sounds */
static __attribute__((noinline)) void click_render(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t s, c, k, env, g;
    if (click_req) {
        uint32_t down = click_req == 2u;
        clk.s = 0;                                  /* (from 0: no step at the start) */
        clk.c = CLICK_AMP;
        clk.k = down ? CLICK_K_DOWN : CLICK_K_BEAT;
        clk.env = 32767;
        clk.g = CLICK_GAIN[click_lvl % 3u] >> (down ? 0 : 1);   /* Q12: the downbeat x1, a beat x0.5 (-6 dB) */
        click_req = 0;
    }
    if (clk.env <= 0)
        return;
    s = clk.s; c = clk.c; k = clk.k; env = clk.env;
    g = (int32_t)(((uint32_t)clk.g * song.master_q12) >> 12);   /* (MASTER, as the rest of the DAC's signal) */
    for (i = 0; i < n; i++) {
        int32_t v, l, r;
        s += (k * c) >> 15;
        c -= (k * s) >> 15;
        v = (((s * env) >> 15) * g) >> 12;
        env -= (env >> 8) + 1;
        if (env < 0)
            env = 0;
        l = out[2u * i] + v;
        r = out[2u * i + 1u] + v;
        out[2u * i] = l > 32767 ? 32767 : l < -32767 ? -32767 : l;
        out[2u * i + 1u] = r > 32767 ? 32767 : r < -32767 ? -32767 : r;
    }
    clk.s = s; clk.c = c; clk.env = env;
}

/* M-Vave-OP1, MENU > AUDIO > SYNC OUT PO: the Pocket Operators' sync (their SY modes with a sync out) on the DAC's
 * output: the left channel a pulse on every eighth note while the transport runs (2 a quarter, as the POs and the
 * volcas take it; 15 ms, + then -, whatever MASTER), the right the mix in mono. A stereo-to-two-mono cable gives a PO /
 * volca its sync from the left and the mixer the sound from the right. Not on USB (as the click). seq.c's click_tick
 * asks for the pulses (po_req), on the same eighths the click counts.
 * The pulse's height is the DAC's own full scale, not the mix's: audio.c's OUT_SHIFT puts Q15's 32767 at -6 dBFS
 * (the headroom the mix keeps), half the voltage the jack can give. A PO's own sync out is ~1 V for ~2.5 ms and its
 * sync in wants more than ~0.6 Vpp (community measurements, no spec): a 3.3 V codec's half scale is about that, and
 * the first build's pulses were heard but the PO did not follow. PO_HIGH is Q15 x 2, so OUT_SHIFT makes it 0x7FFF80,
 * just under the 24-bit word's top (audio.c asserts it). The second build's 5 ms of +full scale were heard on the left
 * alone and the PO (SY2 / SY4, a stereo cable) still did not follow, so a pulse is now both halves of the scale:
 * PO_HALF at +full scale, then PO_HALF at -full scale (15 ms in all: longer than the ~8 ms a PO is reported to need,
 * far shorter than an eighth at 300 BPM). Its swing is twice the voltage again, and whichever way round the output
 * stage puts it, one half rises (the edge a PO takes; TE: "use the right sync polarity", not saying which) */
#define PO_HALF 331u                                   /* samples: 7.5 ms up, then 7.5 ms down */
#define PO_HIGH 65535                                  /* the left channel during a pulse: 0 dBFS after OUT_SHIFT */
static volatile uint8_t po_sync;                       /* SYNC OUT PO (ui.c rp_apply) */
static volatile uint8_t po_req;                        /* seq.c: an eighth started; the next block starts its pulse */
static uint32_t po_left;                               /* samples of the pulse still to come */
static __attribute__((noinline)) void po_render(int32_t *out, uint32_t n)
{
    uint32_t i;
    if (po_req) {
        po_req = 0;
        po_left = 2u * PO_HALF;
    }
    for (i = 0; i < n; i++) {
        int32_t m = (out[2u * i] + out[2u * i + 1u]) >> 1;   /* (a shift: no divide in the ISR) */
        out[2u * i] = !po_left ? 0 : po_left > PO_HALF ? PO_HIGH : -PO_HIGH;
        out[2u * i + 1u] = m;
        if (po_left)
            po_left--;
    }
}
