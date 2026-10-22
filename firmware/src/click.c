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

/* M-Vave-OP1, MENU > AUDIO > SYNC OUT PO / PO INV: the Pocket Operators' sync (their SY modes with a sync in) on the
 * DAC's output: the left channel a pulse on every eighth note while the transport runs (2 a quarter, as the POs and
 * the volcas take it), the right the mix in mono. A stereo cable into the PO's line in (or a stereo-to-two-mono one:
 * the left into a PO's / volca's sync, the right as audio). Not on USB (as the click). seq.c's click_tick asks for the
 * pulses (po_req), on the same eighths the click counts.
 * The shape is PocketSync's (github.com/chr15m/PocketSync, a phone app the POs follow): the left channel at +full scale
 * for 20 ms, and between the pulses at -1 % of it, never at 0. Three builds before were heard on the left alone and
 * the PO (SY2 / SY4, PLAY first, a stereo cable; it follows other sources) did not move: 5 ms at +full scale from 0,
 * then 15 ms of + and - full scale, back to 0. What they shared was a left channel at digital 0 between the pulses,
 * and an I2S DAC mutes a channel that stays at 0 (auto-mute), so each pulse came out of a muted channel, ramped up:
 * a tick in the headphones, no edge for the PO. -1 % keeps the channel live (as PocketSync does, for the phones'
 * DACs). PO INV is the same upside down (-full scale, idle at +1 %), for an output stage that inverts (TE: "use the
 * right sync polarity", not saying which).
 * The pulse's height is the DAC's own full scale, not the mix's: audio.c's OUT_SHIFT puts Q15's 32767 at -6 dBFS
 * (the headroom the mix keeps); PO_HIGH is Q15 x 2, so OUT_SHIFT makes it 0x7FFF80, just under the 24-bit word's top
 * (audio.c asserts it), whatever MASTER */
#define PO_PULSE 882u                                  /* samples: 20 ms (PocketSync's: 882 at 44.1 kHz) */
#define PO_HIGH 65535                                  /* the left channel during a pulse: 0 dBFS after OUT_SHIFT */
#define PO_IDLE 655                                    /* .. between the pulses, below 0: -1 % of it (the DAC stays live) */
static volatile uint8_t po_sync;                       /* SYNC OUT: 0 OFF, 1 PO, 2 PO INV (ui.c rp_apply) */
static volatile uint8_t po_req;                        /* seq.c: an eighth started; the next block starts its pulse */
static uint32_t po_left;                               /* samples of the pulse still to come */
static __attribute__((noinline)) void po_render(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t sg = po_sync == 2u ? -1 : 1;               /* (PO INV: upside down) */
    if (po_req) {
        po_req = 0;
        po_left = PO_PULSE;
    }
    for (i = 0; i < n; i++) {
        int32_t m = (out[2u * i] + out[2u * i + 1u]) >> 1;   /* (a shift: no divide in the ISR) */
        out[2u * i] = sg * (po_left ? PO_HIGH : -PO_IDLE);
        out[2u * i + 1u] = m;
        if (po_left)
            po_left--;
    }
}
