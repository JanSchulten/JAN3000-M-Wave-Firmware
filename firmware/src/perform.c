/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* PERFORM: the effects of the FX hold layer, on the master. A key pressed while FX is held belongs to the
 * layer (seq.c keyboard_block): it plays no note, sends no MIDI, records nothing, and holds its effect until
 * it is let go (ui_input.c opens the layer and shows the map). The 16 white keys (F3 .. G5) hold the effect the map
 * gives them (perf_map: the settings, ui_layer.c; any effect on any key, or none). The default (PF_DEF, 1.1.5's
 * keys and the two of 1.2 after them):
 *   REPEAT 1/8 1/16 1/32, REVERSE, LPF | HPF, TAPE STOP, FREEZE, OCT UP, OCT DN, FLANGER, PHASER, REV THROW, ECHO
 *   THROW (1.5.1, D5 E5);  the other 2 white keys nothing; black keys 1..4 (F#3 G#3 A#3 C#4): tracks 1..4 muted while
 *   held (not P_MUTE, never saved).
 * REV THROW / ECHO THROW (1.5.1): while held every track's reverb / delay send is full (as REV / DLY 127; perf_throw),
 * ECHO THROW also lifts the delay's feedback to 0.7 if it is lower (fx.c fx_buses); both glide in and out over 11.6 ms
 * (THR_SLOPE). Let go, the sends are the tracks' own again and what the buses hold rings out. The buses are the song's
 * (no RAM of their own); the tracks' REV / DLY / FDBK values are never touched (nothing saved, nothing recorded).
 * REPEAT and REVERSE start on the next 1/16 (at once while stopped); the others at once; all end when let go,
 * with a 2.9 ms ramp. Effects of different kinds stack; of the buffer ones (REPEAT, REVERSE, TAPE STOP,
 * FREEZE, OCT UP, OCT DN) the last pressed plays, and letting it go returns to the one held before.
 * OCT UP / OCT DN: a harmonizer, the mix an octave up / down beside itself (after the author's earlier
 * fxHarmonizer, the classic rotating-delay shifter): two read taps of a delay sweep 128 .. 2176 samples, half a sweep apart, at the
 * speed that makes the pitch 2x / 0.5x; a raised-cosine cross-fade (the sine table) hides each tap's jump
 * back; reads between samples, linear. Left and right apart, at 44.1 kHz in the loop's frames (4096 of
 * them). Out: 0.56 the live mix + 0.7 the shifted one (its 0.55, 0.7). While it plays, DEPTH (KNOB 4) is its
 * SHIMMER instead of the level: the shifted sound fed back into its own delay (0 .. 0.82, through the soft
 * clip): with OCT UP each pass climbs another octave.
 * FLANGER (1.2): left and right each through a short delay swept 0.5 .. 8 ms (a squared triangle: longer near the
 * short end, where the comb sweeps fastest), a bar a sweep in time with the transport (the 1/16 clock), the right a
 * quarter of it behind the left; feedback 0.5 (through the soft clip); out (x + delayed) / 2 * 0.85: the comb's peaks
 * and DC +2 dB, its notches -17 dB, white noise -4 dB. Its own line (fl_buf, 356 frames: 1.4 KB; the loop's buffer is the buffer
 * effects', which it stacks with); heard once the line holds 8 ms (8 ms after the press).
 * PHASER (1.2): four first-order all-pass stages a side (the classic four-stage pedal), their break swept 150 Hz ..
 * 4.8 kHz (log; a triangle of half a bar, the right a quarter of it behind; the coefficients a block at a time), the
 * last stage's output fed back (0.4) into the first; out (x + all-passed) / 2 * 0.9: two notches moving, the peaks
 * +1.6 dB, white noise -1 .. -3.5 dB. KNOB 4 DEPTH is the FLANGER's and the PHASER's level too (OCT UP /
 * DN playing: full, KNOB 4 its shimmer).
 * The buffer: the SLICER's recordings (sl_buf, 32 KB) borrowed as one stereo loop of 8192 frames at 22.05 kHz
 * (371 ms). While borrowed, STUT tracks play live; their recordings are dropped afterwards. A REPEAT 1/8 or
 * REVERSE longer than the loop at the tempo (below 81 BPM) does nothing (the map shows it dimmed).
 * KNOB 1..4 with FX: the macros FILTER (the LPF / HPF), CRUSH, THROW (the dry mix into the delay and reverb
 * sends), DEPTH (the buffer effects' level, FLANGER's and PHASER's; OCT UP / DN: the shimmer).
 * MENU > FX LATCH ON (#40, a hand that cannot hold FX, a key and a knob together): a key pressed with FX turns its
 * effect on, the next press turns it off (perf_latched; letting the key or FX go does nothing), the macros keep
 * their values when FX is let go, FX + OCT- turns everything off (ui_layer.c), so does the menu or a dialog.
 * Chain: [REPEAT / REVERSE / TAPE / FREEZE] -> LPF -> HPF -> CRUSH -> FLANGER -> PHASER (-> DUCK -> STEREO, E2), after the master level and
 * before master_out (the limiter); THROW and the mutes act before the buses (fx.c mix_block / mix_part).
 * Idle (no key, no knob, no ramp left) every stage is skipped: the output is bit-identical.
 * M-Vave-OP1: CLOUD FREEZE (F5 by default) holds FX1's CLOUD as it is (fx1.c: nothing new goes in while held); it does
 * nothing, dimmed on the map, while FX1 is the DELAY.
 * M-Vave-OP1: the FX TRACKS (fxtrack.c) hold effects as the keys do (perf_fxt), a REPEAT / REVERSE of theirs at once
 * (their steps are 1/16s); a press of the layer is also recorded onto an armed FX track (fxt_press).
 * M-Vave-OP1 E2, the OP-Z's punch-ins (docs/EFFEKTE_PLAN.md):
 *   DUCK: the master pumps in quarters, as under a kick's sidechain: down to 0.1 (-20 dB) over 2.2 ms on each beat,
 *   back up over the next three quarters of it (an ease-out). In time with the transport's clock (pf.ph).
 *   STEREO: the mix (its mid) panned left and right with equal power, a sine of two 1/16s: left on one, right on the next.
 *   PITCH 5TH UP / 4TH DN: the harmonizer's rotating delay at 2^(7/12) / 2^(-5/12), the shifted sound alone (no live
 *   part, no shimmer: KNOB 4 is its DEPTH as for the buffer effects).
 *   RAMP UP: TAPE STOP backwards: the tape stands, then spins up over a beat (at most PB_TAPE) and ends in time: its
 *   speed v = 4x - 3x^2 (x the ramp's share done) rises past 1 (4/3 at x = 2/3) so that what it reads catches up with the
 *   live mix (the integral of v is 1), and lands on 1 with no jump; then the live mix again (the read is on it).
 *   GLITCH (the OP-Z's RANDOM): from the next 1/16 on, each 1/16 at random one of: the live mix (4 of 16), the last 1/32
 *   or 1/64 repeated, the last 1/16 again, backwards, at half speed (an octave down) or twice (up), or silence. Out of a
 *   ring of what came in (the loop's buffer, recorded all along), each slice faded at its ends (2.9 ms) and at its loop's.
 *   The buffer ones (PITCH, RAMP UP, GLITCH) are buffer effects as REPEAT: the last pressed plays.
 * MAP B (M-Vave-OP1 E2): a second map of the 16 white keys, in the OP-Z's order of its punch-ins (DUCK, FILTER SWEEP,
 * LOOP 1, STEREO, LOOP 2, PITCH, FOLLOW / ECHO, RAMP UP, .., RAMP DOWN, RANDOM), FX + ALGORITHM switches (ui_layer.c);
 * any effect on any key there too, kept with the settings as MAP A's. */
/* the effects (their numbers are stored: the key map in the settings, ui_layer.c perf_map_put; append-only, before
 * PF_M1), then the mutes of the black keys */
enum { PF_R8, PF_R16, PF_R32, PF_REV, PF_LPF, PF_HPF, PF_TAPE, PF_FRZ, PF_OUP, PF_ODN, PF_FLG, PF_PHS,
       PF_RTHR, PF_ETHR,                      /* 1.5.1: REV THROW, ECHO THROW */
       PF_CFRZ,                               /* M-Vave-OP1: CLOUD FREEZE (fx1.c: the cloud stops taking sound in) */
       PF_DUCK, PF_STER, PF_PUP, PF_PDN,      /* M-Vave-OP1 E2: DUCK, STEREO, PITCH 5TH UP, PITCH 4TH DN, */
       PF_RAMP, PF_GLT,                       /* .. RAMP UP, GLITCH */
       PF_M1, PF_N = PF_M1 + NTRK };
#define PF_NFX PF_M1                          /* the effects a white key can hold */
#define PF_KEYS 16u                           /* the white keys F3 .. G5 (a map: MAP A, MAP B after it) */
_Static_assert(PF_N <= 32 && PF_NFX <= 29, "the effects: a bit each in a word, a 5-bit code in the key map");
#define PF_BIT(e) (1u << (e))
#define PF_REPEAT 0x7u                                         /* the three REPEAT rates */
#define PF_Q (PF_REPEAT | PF_BIT(PF_REV) | PF_BIT(PF_GLT))     /* start on the 1/16 */
#define PF_HARM (PF_BIT(PF_OUP) | PF_BIT(PF_ODN))
#define PF_PITCH (PF_BIT(PF_PUP) | PF_BIT(PF_PDN))            /* (E2: the harmonizer's delay, the shifted sound alone) */
#define PF_BUF (PF_REPEAT | PF_BIT(PF_REV) | PF_BIT(PF_TAPE) | PF_BIT(PF_FRZ) | PF_HARM | PF_PITCH | PF_BIT(PF_RAMP) | \
                PF_BIT(PF_GLT))
#define PF_MUTE (((1u << NTRK) - 1u) << PF_M1)
#define PF_MOD (PF_BIT(PF_FLG) | PF_BIT(PF_PHS))   /* FLANGER, PHASER: their own stages after CRUSH */
#define PB_FRAMES (NTRK * SL_LEN / 2u)        /* stereo frames in sl_buf: 8192, 371 ms at 22.05 kHz */
#define PB_MAX (2u * PB_FRAMES)               /* the longest loop, 44.1 kHz samples */
#define PB_TAPE 32000u                        /* TAPE STOP takes 1 beat, at most this (it lags a quarter of it) */
#define PB_FREC 4096u                         /* FREEZE: frames it records (186 ms) */
#define PB_GRAIN 2048u                        /* .. its two overlapping grains, samples (46 ms) */
#define HB_MASK 4095u                         /* OCT UP / DN: the delay's frames (of the loop's 8192) */
#define HB_BASE 128u                          /* .. the taps' shortest delay, samples; the sweep: 2048 */
#define HB_FB 269                             /* .. KNOB 4 (0 .. 100) -> the shimmer, Q15: 0 .. 0.82 */
#define HB_UP7 1045026u                       /* PITCH 5TH UP: the delay's step, -(2^(7/12) - 1) * 2^21 (see hb_tap) */
#define HB_DN5 526063u                        /* PITCH 4TH DN: (1 - 2^(-5/12)) * 2^21 */
#define DK_ATT 96u                            /* DUCK: the gain's fall on the beat, samples (2.2 ms) */
#define DK_FLOOR 3277u                        /* .. its floor, Q15 (0.1: -20 dB) */
#define GL_EDGE 64u                           /* GLITCH: a loop's ends faded over this many frames */
#define PF_TOP (63 << 8)                      /* filter cutoff index, Q8 (PF_SVF): the LPF's open end */
#define PF_LOW (14 << 8)                      /* .. the LPF sweep's end, the HPF sweep's (and K1's) top */
enum { BM_NONE, BM_LOOP, BM_TAPE, BM_FRZ, BM_HARM, BM_RAMP, BM_GLT };
/* GLITCH's slices: the live mix, the last 1/32 / 1/64 repeated, the last 1/16 again / backwards / at half speed / at
 * twice the speed (twice over), silence; GL_PICK: 16 equally likely */
enum { GK_PASS, GK_R32, GK_R64, GK_R16, GK_REV, GK_HALF, GK_DBL, GK_GATE };
static const uint8_t GL_PICK[16] = {GK_PASS, GK_PASS, GK_PASS, GK_PASS, GK_R32, GK_R32, GK_R64, GK_R64,
                                    GK_R16, GK_R16, GK_REV, GK_REV, GK_HALF, GK_DBL, GK_GATE, GK_GATE};
#define FL_LEN 356u                           /* FLANGER: its line's frames (8.1 ms: FL_MAX + 2) */
#define FL_MIN 22u                            /* .. the delay's sweep, samples: 0.5 .. 8 ms */
#define FL_MAX 353u
#define FL_FB 16384                           /* .. the feedback, Q15: 0.5 */
#define PH_ST 4u                              /* PHASER: all-pass stages a side (two notches, the classic pedal's) */
#define PH_FB 13107                           /* .. the feedback, Q15: 0.4 */
#define THR_SLOPE 64                          /* REV / ECHO THROW: the sends' glide a sample, Q15 (11.6 ms end to end) */
#define THR_FB 22938                          /* ECHO THROW: the delay's feedback while held at least this, Q15 (0.7) */

/* the white keys' effects by default (1.1.5's ten, then FLANGER and PHASER, 1.5.1's REV THROW and ECHO THROW on D5
 * and E5; PF_N: none). M-Vave-OP1: F5 CLOUD FREEZE; MAP B (E2) the OP-Z's punch-ins in its order: DUCK, FILTER SWEEP
 * (LPF), LOOP 1 (1/16), STEREO, LOOP 2 (1/32), PITCH, FOLLOW / ECHO, RAMP UP; then where the OP-Z has SHORT and LONG
 * PITCH 4TH DN and REVERSE; RAMP DOWN (TAPE STOP), RANDOM (GLITCH); then HPF, REV THROW, CLOUD FREEZE, FREEZE */
static const uint8_t PF_DEF[2u * PF_KEYS] = {PF_R8, PF_R16, PF_R32, PF_REV, PF_LPF, PF_HPF, PF_TAPE, PF_FRZ, PF_OUP,
                                             PF_ODN, PF_FLG, PF_PHS, PF_RTHR, PF_ETHR, PF_CFRZ, PF_N,
                                             PF_DUCK, PF_LPF, PF_R16, PF_STER, PF_R32, PF_PUP, PF_ETHR, PF_RAMP,
                                             PF_PDN, PF_REV, PF_TAPE, PF_GLT, PF_HPF, PF_RTHR, PF_CFRZ, PF_FRZ};
static volatile uint8_t perf_map[2u * PF_KEYS] = {PF_R8, PF_R16, PF_R32, PF_REV, PF_LPF, PF_HPF, PF_TAPE, PF_FRZ,
                                                  PF_OUP, PF_ODN, PF_FLG, PF_PHS, PF_RTHR, PF_ETHR, PF_CFRZ, PF_N,
                                                  PF_DUCK, PF_LPF, PF_R16, PF_STER, PF_R32, PF_PUP, PF_ETHR, PF_RAMP,
                                                  PF_PDN, PF_REV, PF_TAPE, PF_GLT, PF_HPF, PF_RTHR, PF_CFRZ, PF_FRZ};
                                      /* main: white key p's effect (PF_N none), MAP B's at PF_KEYS + p, from the settings
                                       * (ui_layer.c) */
static volatile uint8_t perf_bank;    /* main (M-Vave-OP1 E2): the map the white keys play, 0 MAP A, 1 MAP B */
static volatile uint8_t perf_remap;   /* main: perf_map changed (seq.c: a key held takes its new effect) */

static volatile uint32_t perf_mask;   /* main: the FX button's bit while its layer may own keys, 0 = none */
static volatile uint32_t kb_mask;     /* main: the button bits whose hold makes keys a layer's (ui_layer.c), 0 = none */
static volatile uint8_t kb_lock;      /* main: a layer locked open (#83, no button held): 1 keys are the layer's, 2 FX's */
static volatile uint8_t perf_solo;    /* main: tracks soloed in the GLO layer (the others muted as by a black key) */
static volatile uint8_t perf_kill;    /* main: every effect off (the menu, a dialog, UBOOT) */
static volatile int8_t perf_k[4];     /* main: the knob macros, 0 = untouched: FILTER -100..100 (- LPF, + HPF),
                                       * CRUSH 0..100, THROW 0..100, DEPTH cut 0..100 (the buffer effects' level) */
static volatile uint32_t kb_layer;    /* ISR: the keys held that are the layer's, a bit per key */
static volatile uint32_t perf_held;   /* ISR: the effects held (PF_*) */
static volatile uint8_t perf_latch_on;   /* main: MENU > FX LATCH ON (#40): a key with FX toggles its effect */
static volatile uint32_t perf_latched;   /* ISR: the effects latched (FX LATCH; main clears it: OCT-, the menu) */
static volatile uint32_t perf_act;    /* ISR: .. running (a 1/16 one from its start on) */
static uint32_t perf_ord[PF_N], perf_seq;   /* press order: the last pressed buffer effect plays */
static uint32_t perf_fxt;             /* ISR (M-Vave-OP1): the effects the FX TRACKS hold (fxtrack.c), as keys held them */
static uint32_t perf_fxt_now;         /* ISR: .. those started this block: at once (on their step), not on the next 1/16 */
static void fxt_press(uint32_t e);    /* (fxtrack.c: a press of the layer, recorded onto an armed FX track) */

/* the ZDF state-variable filter at cutoff index i (30 Hz * 2^(i / 7), 0..63: up to 15.4 kHz), k = 1:
 * a1 a2 a3 in Q14 (tools: g = tan(pi fc / fs), a1 = 1 / (1 + g (g + k)), a2 = g a1, a3 = g a2) */
static const int16_t PF_SVF[64][3] = {
    {16349, 35, 0}, {16345, 39, 0}, {16341, 43, 0}, {16337, 47, 0}, {16332, 52, 0}, {16327, 57, 0}, {16321, 63, 0},
    {16314, 70, 0}, {16307, 77, 0}, {16299, 85, 0}, {16290, 94, 1}, {16280, 103, 1}, {16269, 114, 1},
    {16257, 126, 1}, {16244, 139, 1}, {16229, 153, 1}, {16213, 169, 2}, {16196, 186, 2}, {16176, 205, 3},
    {16154, 227, 3}, {16130, 250, 4}, {16104, 275, 5}, {16075, 303, 6}, {16043, 334, 7}, {16007, 368, 8},
    {15968, 406, 10}, {15925, 447, 13}, {15877, 492, 15}, {15824, 541, 19}, {15766, 596, 22}, {15702, 655, 27},
    {15631, 720, 33}, {15553, 791, 40}, {15467, 869, 49}, {15372, 953, 59}, {15267, 1046, 72}, {15151, 1146, 87},
    {15024, 1255, 105}, {14883, 1374, 127}, {14729, 1502, 153}, {14559, 1640, 185}, {14372, 1790, 223},
    {14166, 1950, 268}, {13940, 2121, 323}, {13692, 2304, 388}, {13420, 2499, 465}, {13122, 2704, 557},
    {12797, 2921, 667}, {12441, 3147, 796}, {12053, 3382, 949}, {11631, 3624, 1129}, {11173, 3870, 1341},
    {10677, 4119, 1589}, {10140, 4365, 1879}, {9562, 4605, 2218}, {8940, 4832, 2612}, {8274, 5040, 3070},
    {7563, 5219, 3602}, {6807, 5359, 4219}, {6006, 5444, 4934}, {5163, 5456, 5766}, {4282, 5369, 6733},
    {3372, 5149, 7863}, {2449, 4745, 9190},
};

static struct {
    uint32_t ph, P, split;             /* samples since play; the 1/16; this block: where a 1/16 starts */
    uint32_t act, act0;                /* running after / before split */
    uint8_t busy, sync, mute;          /* sync: a 1/16 starts with the next block; mute: tracks to ramp */
    /* the buffer */
    uint8_t mode, src, next, rev;      /* BM_*, the effect it plays (PF_N none), one waiting for the fade, REVERSE */
    uint8_t state, odd, xrev;          /* LOOP: 0 recording, 1 playing; the sample of a pair; the old loop's */
    uint32_t len, wr, rp;              /* loop length (samples), frames written, read (samples into the loop) */
    uint32_t xrp, xlen, xn;            /* the loop it switched from, faded over xn samples */
    uint32_t tv, dv, tpos;             /* TAPE: speed Q16, its step, read position Q16 frames */
    uint32_t gp[2], gs[2];             /* FREEZE: grain positions (samples), starts (frames) */
    uint32_t hph, hinc;                /* OCT UP / DN: tap A's place in the sweep (Q32), its step a sample */
    uint8_t hwet;                      /* .. PITCH (E2): the shifted sound alone, no shimmer */
    int32_t hfb;                       /* .. the shimmer now, Q15 (glides to KNOB 4's a block at a time) */
    int32_t w, tw, hl, hr;             /* the buffer's share and its target, Q15; the first of a pair */
    /* the filters, CRUSH, THROW, mutes */
    int32_t lc, hc, la, ha;            /* cutoffs (Q8 index) and shares (Q15) of LPF and HPF */
    int32_t lz[4], hz[4];              /* their states: L ic1 ic2, R ic1 ic2 */
    int16_t lk[3], hk[3];              /* their coefficients this block */
    uint32_t lt, ht;                   /* samples the LPF / HPF key has been held */
    int32_t cw, cc, chl, chr;          /* CRUSH share, amount, held sample */
    uint32_t cn;
    int32_t td;                        /* THROW: the share of the dry mix sent, Q15 */
    int32_t rw, ew, rw1, ew1;          /* REV / ECHO THROW: their shares (Q15) at this block's start and end */
    uint8_t thr;                       /* .. either at work this block (mix_part: perf_throw) */
    int32_t mg[NTRK];                  /* mute gains, Q15 (32768 = open) */
    /* FLANGER, PHASER */
    int32_t fw, pw;                    /* their shares, Q15 */
    uint32_t fwp, fn;                  /* FLANGER: the frame written next; frames written since it started */
    int32_t px[2][PH_ST], py[2][PH_ST], pfb[2];   /* PHASER: each stage's last input and output, L and R; the fed back */
    /* M-Vave-OP1 E2 */
    int32_t dw, sw;                    /* DUCK, STEREO: their shares, Q15 */
    uint32_t rt, rT, rk, rpos;         /* RAMP UP: samples into the ramp, its length, 2^31 / rT; the read, Q16 frames */
    uint8_t gk;                        /* GLITCH: the slice's kind (GK_*) */
    uint32_t gn, gl;                   /* .. samples into it, its length (a 1/16) */
    uint32_t gst, glen, gpos, grng;    /* .. its source's first frame, its length and the read in it (Q16 frames); RNG */
    int32_t ginc;                      /* .. the read's step a sample, Q16 frames (32768: as recorded) */
} pf = {.src = PF_N, .next = PF_N, .lc = PF_TOP, .mg = {32768, 32768, 32768, 32768}};

static int16_t fl_buf[FL_LEN][2];      /* FLANGER's line: L R, at half the level (headroom) */

/* the key map in the settings (ui_layer.c: favorites.factory[15][14..23]; MAP B's [0..9]): white key p's code in 5 bits
 * from bit 5p of b (p: MAP B's from PF_KEYS, b its bytes): 0 its default (PF_DEF), 1 none, 2 + e effect e (an unknown
 * code, a later firmware's effect: the default) */
static uint32_t perf_map_of(const uint8_t *b, uint32_t p)
{
    uint32_t i = 5u * (p % PF_KEYS), c = (uint32_t)b[i >> 3] >> (i & 7u);
    if ((i & 7u) > 3u)
        c |= (uint32_t)b[(i >> 3) + 1u] << (8u - (i & 7u));
    c &= 31u;
    return c == 1u ? PF_N : c >= 2u && c - 2u < PF_NFX ? c - 2u : PF_DEF[p];
}
static void perf_map_put(uint8_t *b, uint32_t p, uint32_t e)
{
    uint32_t i = 5u * (p % PF_KEYS), sh = i & 7u, c = e == PF_DEF[p] ? 0u : e >= PF_NFX ? 1u : e + 2u, two = sh > 3u, v;
    v = b[i >> 3] | (two ? (uint32_t)b[(i >> 3) + 1u] << 8 : 0u);
    v = (v & ~(31u << sh)) | c << sh;
    b[i >> 3] = (uint8_t)v;
    if (two)
        b[(i >> 3) + 1u] = (uint8_t)(v >> 8);
}

/* the loop length of a REPEAT / REVERSE (1/8) at the tempo, 44.1 kHz samples; 0 = not one */
static const uint8_t PF_DEN[PF_REV + 1] = {2, 4, 8, 2};
static uint32_t perf_len(uint32_t e) { return e <= PF_REV ? beat_samples() / PF_DEN[e] : 0u; }
/* the effects that can run at the tempo: a REPEAT / REVERSE longer than the loop cannot; CLOUD FREEZE only with FX1's
 * CLOUD (fx1.c) */
static uint32_t perf_avail(void)
{
    uint32_t e, m = ~0u, b = beat_samples();
    for (e = PF_R8; e <= PF_REV; e++)
        if (b > PB_MAX * PF_DEN[e])
            m &= ~PF_BIT(e);
    if (fx1s.type != FX1_CLOUD)
        m &= ~PF_BIT(PF_CFRZ);
    return m;
}

/* seq.c keyboard_block: a layer key down / up (e: PF_*, PF_N = no effect) */
static void perf_press(uint32_t e, int down)
{
    if (e >= PF_N)
        return;
    if (perf_latch_on) {                  /* FX LATCH: a press turns it on or off, letting go does nothing */
        if (down) {
            perf_latched ^= PF_BIT(e);
            perf_ord[e] = ++perf_seq;
            if ((perf_latched >> e) & 1u)
                fxt_press(e);
        }
        return;
    }
    if (down) {
        perf_held |= PF_BIT(e);
        perf_ord[e] = ++perf_seq;
        fxt_press(e);
    } else {
        perf_held &= ~PF_BIT(e);
    }
}

static void perf_start(void) { pf.ph = 0; pf.sync = 1; }      /* seq_start: a 1/16 starts with the transport */

/* the SLICER's recordings dropped (when the buffer is taken and given back) */
static void perf_drop_slicer(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        sl[k].rec = sl[k].loop = sl[k].rec_on = 0;
}

/* the buffer effect to play: the last pressed of those running */
static uint32_t perf_pick(uint32_t a)
{
    uint32_t e, best = PF_N;
    a &= PF_BUF;
    for (e = 0; e < PF_M1; e++)
        if (((a >> e) & 1u) && (best == PF_N || perf_ord[e] - perf_ord[best] < 0x80000000u))
            best = e;
    return best;
}

static void perf_buf_start(uint32_t e)
{
    if (!sl_lent) {
        sl_lent = 1;
        perf_drop_slicer();
    }
    pf.src = (uint8_t)e;
    pf.next = PF_N;
    pf.wr = pf.rp = 0;
    pf.odd = 0;
    pf.xn = 0;
    if (e == PF_TAPE) {
        uint32_t t = beat_samples();
        pf.mode = BM_TAPE;
        if (t > PB_TAPE)
            t = PB_TAPE;
        pf.tv = 65536;
        pf.dv = 65536u / t + 1u;
        pf.tpos = 0;
    } else if (((PF_HARM | PF_PITCH) >> e) & 1u) {   /* (wr: the frame written next, len: frames written) */
        pf.mode = BM_HARM;
        pf.len = 0;
        pf.hph = 0;
        pf.hinc = e == PF_OUP ? 0u - (1u << 21) : e == PF_ODN ? 1u << 20      /* the delay: -1 / +0.5 sample a sample */
                : e == PF_PUP ? 0u - HB_UP7 : HB_DN5;                            /* (E2: -0.498 / +0.251) */
        pf.hwet = (uint8_t)((PF_PITCH >> e) & 1u);
        pf.hfb = 0;
    } else if (e == PF_RAMP) {                      /* (E2) the tape from standing, a beat at most PB_TAPE */
        uint32_t t = beat_samples();
        pf.mode = BM_RAMP;
        pf.rT = t > PB_TAPE ? PB_TAPE : t;
        pf.rk = 0x80000000u / pf.rT;
        pf.rt = 0;
        pf.rpos = 0;
    } else if (e == PF_GLT) {                       /* (E2) the first slice: what came in so far is nothing, the live mix */
        pf.mode = BM_GLT;
        pf.gk = GK_PASS;
        pf.gn = 0;
        pf.gl = beat_samples() >> 2;
        pf.grng = 0x9E3779B9u ^ (perf_seq * 0x85EBCA6Bu);
    } else if (e == PF_FRZ) {
        pf.mode = BM_FRZ;
        pf.gp[0] = 0;
        pf.gp[1] = PB_GRAIN / 2u;
        pf.gs[0] = pf.gs[1] = 0;
    } else {
        pf.mode = BM_LOOP;
        pf.state = 0;
        pf.len = perf_len(e);
        pf.rev = e == PF_REV;
    }
}

/* the buffer effect to play becomes e (at a 1/16 for REPEAT / REVERSE): a shorter REPEAT of what is
 * recorded switches with a cross-fade, anything else fades the old one out first */
static void perf_buf_select(uint32_t e)
{
    if (e == pf.src && pf.next == PF_N)
        return;
    if (e != PF_N && pf.mode == BM_LOOP && pf.state && e <= PF_REV && pf.src != PF_N &&
        perf_len(e) <= 2u * pf.wr) {
        pf.xrp = pf.rp;
        pf.xlen = pf.len;
        pf.xrev = pf.rev;
        pf.xn = SL_RAMP;
        pf.src = (uint8_t)e;
        pf.next = PF_N;
        pf.len = perf_len(e);
        pf.rev = e == PF_REV;
        pf.rp = 0;
        return;
    }
    if (pf.mode == BM_NONE || !pf.w) {
        if (e == PF_N) {
            pf.src = PF_N;
            pf.next = PF_N;
        } else {
            perf_buf_start(e);
        }
        return;
    }
    pf.src = PF_N;                                  /* fade out, then the next one (perf_seg) */
    pf.next = (uint8_t)e;
}

/* each block, after the keys (seq.c) and before the parts: what runs, the 1/16, the filters, the mutes.
 * Returns 1 when a stage has something to do */
static __attribute__((noinline)) int perf_begin(uint32_t n)
{
    uint32_t held = perf_kill ? 0u : (perf_held | perf_latched | perf_fxt |
                                      (perf_solo ? (~(uint32_t)perf_solo & 15u) << PF_M1 : 0u)) & perf_avail();
    uint32_t q, k, ph0, bnd;
    int32_t m;
    if (!held && !pf.busy && !(perf_k[0] | perf_k[1] | perf_k[2])) {   /* idle: the clock only */
        pf.ph += n;
        pf.sync = 0;
        perf_act = 0;
        perf_fxt_now = 0;
        return 0;
    }
    pf.P = beat_samples() >> 2;
    if (pf.sync) {
        pf.ph = 0;
        pf.sync = 0;
    }
    ph0 = pf.ph % pf.P;                             /* the 1/16 in this block (n: none) */
    bnd = ph0 ? (pf.P - ph0 < n ? pf.P - ph0 : n) : 0u;
    pf.ph += n;
    pf.act &= held;                                 /* let go: at once */
    pf.act |= held & ~PF_Q;
    pf.act |= held & perf_fxt_now;                  /* an FX track's REPEAT / REVERSE: its step is the 1/16 */
    perf_fxt_now = 0;
    pf.act0 = pf.act;
    q = held & PF_Q & ~pf.act;
    pf.split = n;
    if (q && (!song.playing || bnd < n)) {
        pf.split = song.playing ? bnd : 0u;
        pf.act |= q;
    }
    perf_act = pf.act;
    /* the filters: LPF from open down over a bar while its key is held, KNOB 1 left; HPF up, KNOB 1 right */
    m = perf_k[0];
    {
        uint32_t bar = 16u * pf.P, t;
        int32_t lt = PF_TOP, ht = 0;
        pf.lt = (pf.act & PF_BIT(PF_LPF)) ? pf.lt + n : 0u;
        pf.ht = (pf.act & PF_BIT(PF_HPF)) ? pf.ht + n : 0u;
        if (pf.lt) {
            t = pf.lt < bar ? pf.lt : bar;
            lt = PF_TOP - (int32_t)((uint32_t)(PF_TOP - PF_LOW) / 64u * (t * 64u / bar));
        }
        if (m < 0 && PF_TOP + m * (PF_TOP - PF_LOW) / 100 < lt)
            lt = PF_TOP + m * (PF_TOP - PF_LOW) / 100;
        if (pf.ht) {
            t = pf.ht < bar ? pf.ht : bar;
            ht = (int32_t)((uint32_t)(PF_TOP - PF_LOW) / 64u * (t * 64u / bar));
        }
        if (m > 0 && m * (PF_TOP - PF_LOW) / 100 > ht)
            ht = m * (PF_TOP - PF_LOW) / 100;
        pf.lc += clamp(lt - pf.lc, -192, 192);      /* ~45 ms over the range */
        pf.hc += clamp(ht - pf.hc, -192, 192);
        for (k = 0; k < 3u; k++) {                  /* the coefficients, between two table rows */
            int32_t i = pf.lc >> 8, f = pf.lc & 255, j = pf.hc >> 8, g = pf.hc & 255;
            pf.lk[k] = (int16_t)(PF_SVF[i][k] + (((PF_SVF[i < 63 ? i + 1 : 63][k] - PF_SVF[i][k]) * f) >> 8));
            pf.hk[k] = (int16_t)(PF_SVF[j][k] + (((PF_SVF[j < 63 ? j + 1 : 63][k] - PF_SVF[j][k]) * g) >> 8));
        }
    }
    {   /* REV / ECHO THROW: their shares glide (THR_SLOPE a sample) to full while held, back to 0 when let go */
        int32_t s = THR_SLOPE * (int32_t)n;
        pf.rw = pf.rw1;
        pf.ew = pf.ew1;
        pf.rw1 += clamp((pf.act & PF_BIT(PF_RTHR) ? 32768 : 0) - pf.rw1, -s, s);
        pf.ew1 += clamp((pf.act & PF_BIT(PF_ETHR) ? 32768 : 0) - pf.ew1, -s, s);
        pf.thr = (pf.rw | pf.rw1 | pf.ew | pf.ew1) != 0;
    }
    pf.mute = 0;
    for (k = 0; k < NTRK; k++)
        if (((held >> (PF_M1 + k)) & 1u) || pf.mg[k] != 32768)
            pf.mute |= (uint8_t)(1u << k);
    pf.busy = 1;
    return 1;
}

/* a track muted by its black key: its signal ramps to 0 and back (fx.c mix_part, after the SLICER) */
static __attribute__((noinline)) void perf_mute(uint32_t k, int32_t *b, uint32_t n)
{
    uint32_t i;
    int32_t g = pf.mg[k], t = (pf.act >> (PF_M1 + k)) & 1u ? 0 : 32768;
    for (i = 0; i < n; i++) {
        g += clamp(t - g, -SL_SLOPE, SL_SLOPE);
        b[i] = mulq16(b[i], (uint32_t)g << 1);
    }
    pf.mg[k] = g;
}
/* perf_mute with SPREAD's side signal sd (fx.c mix_spread): the same ramp on both */
static __attribute__((noinline)) void perf_mute_sd(uint32_t k, int32_t *b, int32_t *sd, uint32_t n)
{
    uint32_t i;
    int32_t g = pf.mg[k], t = (pf.act >> (PF_M1 + k)) & 1u ? 0 : 32768;
    for (i = 0; i < n; i++) {
        g += clamp(t - g, -SL_SLOPE, SL_SLOPE);
        b[i] = mulq16(b[i], (uint32_t)g << 1);
        sd[i] = mulq16(sd[i], (uint32_t)g << 1);
    }
    pf.mg[k] = g;
}

/* before the buses: THROW (KNOB 3) adds the dry mix to the delay and reverb sends */
static __attribute__((noinline)) void perf_pre(const int32_t *ml, const int32_t *mr, int32_t *sd, int32_t *sr,
                                               uint32_t n)
{
    uint32_t i;
    int32_t m = perf_k[2] * 327;
    for (i = 0; i < n; i++) {
        pf.td += clamp(m - pf.td, -SL_SLOPE, SL_SLOPE);
        if (pf.td) {
            int32_t x = mulq16((ml[i] + mr[i]) >> 1, (uint32_t)pf.td << 1);
            sd[i] += x;
            sr[i] += x;
        }
    }
}

/* REV THROW / ECHO THROW (1.5.1): track t's reverb / delay send towards full (127) while held, b its signal as its sends
 * take it (fx.c mix_part, after the mute key; t's values as the matrix has them this block). What the full send adds to
 * the track's own (REV / DLY) goes in at the share rw / ew, which glides from the block's start to its end a sample at
 * a time (THR_SLOPE: no click; every track the same glide). The track's REV / DLY are never changed: nothing is saved
 * or recorded, and when the key is let go the share glides back to 0 and the buses ring out as they are */
static __attribute__((noinline)) void perf_throw(const track_t *t, const int32_t *b, int32_t *sd, int32_t *sr, uint32_t n)
{
    uint32_t i;
    int32_t lvl = LEVEL_Q12[t->p[P_LEVEL] & 127];
    int32_t rg = 32766 - t->p[P_REV] * 258, dg = 32766 - t->p[P_DLY] * 258;   /* full (127 * 258) less its own */
    int32_t rw = pf.rw, ew = pf.ew, r1 = pf.rw1, e1 = pf.ew1;
    for (i = 0; i < n; i++) {
        int32_t x = clamp(((b[i] >> 2) * lvl) >> 10, -65536, 65536);   /* (mix_part's xs at a full send) */
        rw += clamp(r1 - rw, -THR_SLOPE, THR_SLOPE);
        ew += clamp(e1 - ew, -THR_SLOPE, THR_SLOPE);
        sr[i] += mulq15(x, mulq16(rg, (uint32_t)rw << 1));
        sd[i] += mulq15(x, mulq16(dg, (uint32_t)ew << 1));
    }
}

/* x towards y by w (Q15, 32768 = y) */
static inline int32_t pf_mix(int32_t x, int32_t y, int32_t w)
{
    return w == 32768 ? y : x + mulq16(y - x, (uint32_t)w << 1);
}

/* the loop: frame f of it, both channels (Q15) */
static inline void pb_put(uint32_t f, int32_t l, int32_t r)
{
    int16_t *p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
    p[0] = (int16_t)clamp(l, -32768, 32767);
    p[1] = (int16_t)clamp(r, -32768, 32767);
}
/* sample s of the frames recorded from 0 (n of them): between two frames on an odd one */
static inline void pb_at(uint32_t s, uint32_t n, int32_t *l, int32_t *r)
{
    uint32_t f = s >> 1 < n ? s >> 1 : n - 1u;    /* (an odd loop's last sample: the frame before) */
    const int16_t *p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
    const int16_t *q;                               /* a frame is the mean of samples 2f, 2f + 1: sample s */
    if (s & 1u)                                     /* lies a quarter frame from it, towards f + 1 or f - 1 */
        q = f + 1u < n ? p + 2 : p;
    else
        q = f ? p - 2 : p;
    *l = (3 * p[0] + q[0]) >> 1;
    *r = (3 * p[1] + q[1]) >> 1;
}
/* REPEAT / REVERSE at sample rp of a loop of len, windowed at both ends */
static inline void pb_loop(uint32_t rp, uint32_t len, int rev, uint32_t n, int32_t *l, int32_t *r)
{
    uint32_t e = len - 1u - rp;
    pb_at(rev ? e : rp, n, l, r);
    if (rp < e)
        e = rp;
    if (e < (uint32_t)SL_RAMP) {
        *l = (*l * (int32_t)e) >> SL_RAMP_LOG2;
        *r = (*r * (int32_t)e) >> SL_RAMP_LOG2;
    }
}

/* record the pair into frame f (the mean, >> 1: headroom) */
static inline uint32_t pb_rec(uint32_t f, int32_t l, int32_t r)
{
    if (!pf.odd) {
        pf.hl = l;
        pf.hr = r;
        pf.odd = 1;
        return f;
    }
    pf.odd = 0;
    pb_put(f, (pf.hl + l) >> 2, (pf.hr + r) >> 2);
    return f + 1u;
}

/* FREEZE: grain g's sample, by its triangle window, added to yl / yr; a new start at each pass */
static inline void perf_grain(uint32_t g, int32_t *yl, int32_t *yr)
{
    uint32_t p = pf.gp[g], tri = p < PB_GRAIN / 2u ? p : PB_GRAIN - p;
    int32_t a, b;
    if (!p)
        pf.gs[g] = ((rng() >> 16) * (pf.wr - PB_GRAIN / 2u + 1u)) >> 16;
    pb_at(2u * pf.gs[g] + p, pf.wr, &a, &b);
    *yl += a * (int32_t)tri;
    *yr += b * (int32_t)tri;
    pf.gp[g] = p + 1u < PB_GRAIN ? p + 1u : 0u;
}

/* OCT UP / DN: the tap at sweep place ph, both channels (as stored: half the level); silence before the
 * delay has that much in it */
static inline void hb_tap(uint32_t ph, int32_t *l, int32_t *r)
{
    uint32_t d = (HB_BASE << 16) + (ph >> 5), di = d >> 16;   /* the delay, Q16: 128 .. 2176 samples */
    int32_t f = (int32_t)((d >> 1) & 0x7FFFu);
    const int16_t *p, *q;
    if (di + 1u > pf.len) {
        *l = *r = 0;
        return;
    }
    p = &sl_buf[0][0] + 2u * ((pf.wr - di) & HB_MASK);
    q = &sl_buf[0][0] + 2u * ((pf.wr - di - 1u) & HB_MASK);
    *l = p[0] + (((q[0] - p[0]) * f) >> 15);
    *r = p[1] + (((q[1] - p[1]) * f) >> 15);
}
static int perf_buf_e2(int32_t l, int32_t r, int32_t *yl, int32_t *yr);
/* OCT UP / DN on one sample: yl / yr the harmonized mix (see the top); 2: KNOB 4 is the shimmer (PITCH: 1). noinline:
 * perf_block's loop keeps its code layout (target_budget.py: a block placed out of line looks like a loop round the
 * per-sample code); for that, E2's RAMP UP and GLITCH come through here too (a branch of their own in perf_buf moved
 * its blocks) */
static __attribute__((noinline)) int perf_harm(int32_t l, int32_t r, int32_t *yl, int32_t *yr)
{
    int32_t wa, al, ar, bl, br, sl_, sr_;
    if (pf.mode != BM_HARM)
        return perf_buf_e2(l, r, yl, yr);
    wa = (32768 - osc_sine(pf.hph + 0x40000000u)) >> 1;   /* A's share, Q15 */
    hb_tap(pf.hph, &al, &ar);
    hb_tap(pf.hph + 0x80000000u, &bl, &br);
    sl_ = (al * wa + bl * (32768 - wa)) >> 15;
    sr_ = (ar * wa + br * (32768 - wa)) >> 15;
    l >>= 1;                                        /* (stored at half: headroom, as the loop's frames) */
    r >>= 1;
    {
        int32_t xl = l, xr = r;
        if (pf.hfb && !pf.hwet) {                   /* the shimmer: back into the delay, soft-clipped (not PITCH's) */
            xl += softclip(mulq15(sl_, pf.hfb));
            xr += softclip(mulq15(sr_, pf.hfb));
        }
        if (pf.len < (uint32_t)SL_RAMP) {           /* the recording fades in (2.9 ms): no step where it */
            xl = (xl * (int32_t)pf.len) >> SL_RAMP_LOG2;   /* begins, for a tap to read across */
            xr = (xr * (int32_t)pf.len) >> SL_RAMP_LOG2;
        }
        pb_put(pf.wr & HB_MASK, xl, xr);
    }
    pf.wr++;
    if (pf.len <= HB_MASK)
        pf.len++;
    pf.hph += pf.hinc;
    if (pf.hwet) {                                  /* PITCH (E2): the shifted sound alone (x2: stored at half) */
        *yl = sl_ << 1;
        *yr = sr_ << 1;
        return 1;
    }
    *yl = l + (l >> 3) + ((sl_ * 45875) >> 15);     /* 0.56 live + 0.7 shifted (x2: stored at half) */
    *yr = r + (r >> 3) + ((sr_ * 45875) >> 15);
    return 2;
}

/* RAMP UP (E2) on one sample, the pair recorded already: what it reads at the speed of the ramp so far, faded in over
 * its first eighth of speed as TAPE STOP fades out over its last; 0 once up to speed (the live mix: the read caught up).
 * noinline: perf_buf's code stays as it was */
static __attribute__((noinline)) int perf_ramp(int32_t *yl, int32_t *yr)
{
    uint32_t x, v, f, env;
    int32_t fr;
    const int16_t *p, *q;
    if (pf.rt >= pf.rT)
        return 0;
    x = (pf.rt++ * pf.rk) >> 16;                    /* the share done, Q15 */
    v = 4u * x - 3u * ((x * x) >> 15);              /* the speed, Q15: 0 .. 4/3 .. 1 */
    f = pf.rpos >> 16;
    if (!pf.wr) {
        *yl = *yr = 0;
    } else {
        if (f + 1u >= pf.wr)                        /* (on the writing: its last frame) */
            f = pf.wr - 1u, fr = 0;
        else
            fr = (int32_t)((pf.rpos >> 1) & 0x7FFFu);
        p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
        q = &sl_buf[0][0] + 2u * ((f + 1u) & (PB_FRAMES - 1u));
        env = v >= 4096u ? 32768u : v << 3;
        *yl = mulq16((p[0] + (((q[0] - p[0]) * fr) >> 15)) << 1, env << 1);
        *yr = mulq16((p[1] + (((q[1] - p[1]) * fr) >> 15)) << 1, env << 1);
    }
    pf.rpos += v;                                   /* (a sample: v / 2 frames, Q16) */
    return 1;
}

/* GLITCH (E2): the next slice, at a 1/16 (the frames recorded: pf.wr, all along). A slice whose source is not there yet,
 * or would be written over while it plays (the ring: PB_FRAMES), is the live mix */
static __attribute__((noinline)) void glt_slice(void)
{
    uint32_t h, len, k;
    pf.gl = beat_samples() >> 2;                    /* (a 1/16; h: in frames) */
    h = pf.gl >> 1;
    pf.grng ^= pf.grng << 13;
    pf.grng ^= pf.grng >> 17;
    pf.grng ^= pf.grng << 5;
    k = GL_PICK[pf.grng >> 28];
    len = k == GK_R32 || k == GK_HALF ? h >> 1 : k == GK_R64 ? h >> 2 : h;
    if (k != GK_GATE && k != GK_PASS && (len > pf.wr || len + h + 2u > PB_FRAMES || len < 2u * GL_EDGE))
        k = GK_PASS;
    pf.gk = (uint8_t)k;
    pf.gn = 0;
    pf.gst = pf.wr - len;
    pf.glen = len << 16;
    pf.ginc = k == GK_HALF ? 16384 : k == GK_DBL ? 65536 : k == GK_REV ? -32768 : 32768;
    pf.gpos = k == GK_REV ? pf.glen - 1u : 0u;
}
/* GLITCH on one sample, the pair recorded already: the slice playing; 0 while it is the live mix */
static __attribute__((noinline)) int perf_glt(int32_t *yl, int32_t *yr)
{
    uint32_t f, d, e, fr;
    const int16_t *p, *q;
    if (pf.gn >= pf.gl)
        glt_slice();
    e = pf.gn < pf.gl - pf.gn ? pf.gn : pf.gl - pf.gn;   /* samples to the slice's nearer end */
    pf.gn++;
    if (pf.gk == GK_PASS)
        return 0;
    *yl = *yr = 0;
    if (pf.gk == GK_GATE)
        return 1;
    f = pf.gpos >> 16;
    d = f < (pf.glen >> 16) - 1u - f ? f : (pf.glen >> 16) - 1u - f;   /* frames to the loop's nearer end */
    e = e < (uint32_t)SL_RAMP ? e << 8 >> SL_RAMP_LOG2 : 256u;
    d = d < GL_EDGE ? (d << 8) / GL_EDGE : 256u;
    e = e < d ? e : d;                              /* (the window, Q8) */
    fr = (pf.gpos >> 1) & 0x7FFFu;
    p = &sl_buf[0][0] + 2u * ((pf.gst + f) & (PB_FRAMES - 1u));
    q = &sl_buf[0][0] + 2u * ((pf.gst + f + 1u) & (PB_FRAMES - 1u));
    *yl = (int32_t)(((p[0] + (((q[0] - p[0]) * (int32_t)fr) >> 15)) * (int32_t)e) >> 7);   /* (x2: stored at half) */
    *yr = (int32_t)(((p[1] + (((q[1] - p[1]) * (int32_t)fr) >> 15)) * (int32_t)e) >> 7);
    pf.gpos += (uint32_t)pf.ginc;
    if (pf.gpos >= pf.glen)                         /* round (backwards: below 0 wraps high) */
        pf.gpos = pf.ginc < 0 ? pf.gpos + pf.glen : pf.gpos - pf.glen;
    return 1;
}

/* RAMP UP / GLITCH (E2) on one sample: the pair recorded, what they play. noinline: perf_block's loop keeps its layout */
static __attribute__((noinline)) int perf_buf_e2(int32_t l, int32_t r, int32_t *yl, int32_t *yr)
{
    pf.wr = pb_rec(pf.wr, l, r);
    return pf.mode == BM_RAMP ? perf_ramp(yl, yr) : perf_glt(yl, yr);
}

/* the buffer effect faded out: the next one, or the buffer back. Once per effect, out of the per-sample loop
 * (noinline: its calls stay out of perf_block's loop code) */
static __attribute__((noinline)) void perf_buf_done(void)
{
    if (pf.next != PF_N) {
        perf_buf_start(pf.next);
    } else {
        pf.mode = BM_NONE;
        if (sl_lent) {
            sl_lent = 0;
            perf_drop_slicer();
        }
    }
}

/* the buffer effect on one sample: the share pf.w of what it plays, the rest live */
static inline void perf_buf(int32_t *pl, int32_t *pr)
{
    int32_t l = *pl, r = *pr, yl = 0, yr = 0, on = 0;
    if (pf.mode == BM_LOOP) {
        if (!pf.state) {
            pf.wr = pb_rec(pf.wr, l, r);
            if (++pf.rp >= pf.len) {                /* (rp: samples recorded) */
                pf.state = 1;
                pf.rp = 0;
            }
        } else {
            pb_loop(pf.rp, pf.len, pf.rev, pf.wr, &yl, &yr);
            if (pf.xn) {                            /* the loop it switched from, fading out */
                int32_t ol, orr, g = (int32_t)pf.xn;
                pb_loop(pf.xrp, pf.xlen, pf.xrev, pf.wr, &ol, &orr);
                yl += (ol * g) >> SL_RAMP_LOG2;
                yr += (orr * g) >> SL_RAMP_LOG2;
                pf.xn--;
                if (++pf.xrp >= pf.xlen)
                    pf.xrp = 0;
            }
            if (++pf.rp >= pf.len)
                pf.rp = 0;
            on = 1;
        }
    } else if (pf.mode == BM_TAPE) {
        pf.wr = pb_rec(pf.wr, l, r);
        if (pf.wr >= 2u) {                          /* read behind the writing, slower and slower */
            uint32_t f = pf.tpos >> 16;
            int32_t fr = (int32_t)((pf.tpos >> 1) & 0x7FFFu), a, b, env;
            const int16_t *p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
            const int16_t *s = &sl_buf[0][0] + 2u * ((f + 1u) & (PB_FRAMES - 1u));
            a = p[0];
            b = s[0];
            yl = (a + (((b - a) * fr) >> 15)) << 1;
            a = p[1];
            b = s[1];
            yr = (a + (((b - a) * fr) >> 15)) << 1;
            env = pf.tv >= 8192u ? 32768 : (int32_t)(pf.tv << 2);   /* the last eighth fades to silence */
            yl = mulq16(yl, (uint32_t)env << 1);
            yr = mulq16(yr, (uint32_t)env << 1);
            pf.tpos += pf.tv >> 1;
            pf.tv = pf.tv > pf.dv ? pf.tv - pf.dv : 0u;
            on = 1;
        }
    } else if (pf.mode == BM_FRZ) {
        if (pf.wr < PB_FREC)
            pf.wr = pb_rec(pf.wr, l, r);
        if (pf.wr >= PB_GRAIN / 2u) {               /* two grains of what it caught, triangle windows */
            perf_grain(0, &yl, &yr);
            perf_grain(1, &yl, &yr);
            yl >>= 10;                              /* (the windows sum to PB_GRAIN / 2) */
            yr >>= 10;
            on = 1;
        }
    } else if (pf.mode >= BM_HARM) {                /* (E2: RAMP UP and GLITCH through it too) */
        on = perf_harm(l, r, &yl, &yr);             /* (2: KNOB 4 is the shimmer, the level stays full; PITCH 1) */
    }
    pf.tw = on && pf.src != PF_N ? on == 2 ? 32768 : 32768 - perf_k[3] * 327 : 0;
    pf.w += clamp(pf.tw - pf.w, -SL_SLOPE, SL_SLOPE);
    if (pf.w) {
        *pl = pf_mix(l, yl, pf.w);
        *pr = pf_mix(r, yr, pf.w);
    } else if (pf.src == PF_N) {                    /* faded out: the next one, or the buffer back */
        perf_buf_done();
    }
}

/* one channel of a state-variable filter (k = 1): x in Q15 >> 2 */
static inline int32_t pf_svf(int32_t x, int32_t *z, const int16_t *c, int hp)
{
    int32_t v3 = x - z[1], v1 = (c[0] * z[0] + c[1] * v3) >> 14, v2 = z[1] + ((c[1] * z[0] + c[2] * v3) >> 14);
    z[0] = 2 * v1 - z[0];
    z[1] = 2 * v2 - z[1];
    return hp ? x - v1 - v2 : v2;
}

static __attribute__((noinline)) void perf_switch(uint32_t a) { perf_buf_select(perf_pick(a)); }

/* the UI: OCT UP / DN is the buffer effect playing (KNOB 4 is its shimmer, the card says so) */
static int perf_harm_on(void) { return (PF_HARM >> perf_pick(perf_act)) & 1u; }

/* FLANGER / PHASER: the sweep's place at this block's first sample, a cycle every `s` 1/16s of the transport's clock
 * (pf.ph: in time with the bars while playing), and its step a sample (*inc) */
static uint32_t perf_lfo(uint32_t s, uint32_t n, uint32_t *inc)
{
    uint32_t per = s * pf.P;
    *inc = 0xFFFFFFFFu / per;
    return ((pf.ph - n) % per) * *inc;
}
/* a triangle of the sweep's place q, 0 .. 65534 (Q16) */
static inline uint32_t pf_tri(uint32_t q)
{
    uint32_t t = q >> 16;
    return t < 32768u ? t << 1 : (65535u - t) << 1;
}
/* the share a FLANGER / PHASER fades to: KNOB 4 DEPTH (its shimmer while OCT UP / DN plays: full) */
static int32_t perf_mod_depth(void) { return (PF_HARM >> perf_pick(perf_act)) & 1u ? 32768 : 32768 - perf_k[3] * 327; }

/* FLANGER on one side c: x in, the line read at the sweep's place q (see the top) */
static inline int32_t fl_side(uint32_t c, int32_t x, uint32_t q)
{
    uint32_t t = pf_tri(q), d = (FL_MIN << 16) + (FL_MAX - FL_MIN) * ((t * t) >> 16);   /* the delay, Q16 samples */
    int32_t r0 = (int32_t)pf.fwp - (int32_t)(d >> 16), r1, a, v, fr = (int32_t)((d >> 1) & 0x7FFFu);
    if (r0 < 0)
        r0 += (int32_t)FL_LEN;
    r1 = r0 ? r0 - 1 : (int32_t)FL_LEN - 1;
    a = fl_buf[r0][c];
    v = a + (((fl_buf[r1][c] - a) * fr) >> 15);
    fl_buf[pf.fwp][c] = (int16_t)clamp((x >> 1) + softclip(mulq15(v, FL_FB)), -32767, 32767);
    return pf_mix(x, mulq16((x >> 1) + v, 55706u), pf.fw);   /* (x + delayed) / 2 * 0.85 (v: stored at half) */
}
/* after CRUSH, while held or fading: FLANGER (noinline, its own loop: the audio ISR's code stays as it was) */
static __attribute__((noinline)) void perf_flanger(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, c, inc, q = perf_lfo(16u, n, &inc);
    int32_t tw = pf.act & PF_BIT(PF_FLG) ? perf_mod_depth() : 0, *const b[2] = {bl, br};
    if (!pf.fw && !(pf.act & PF_BIT(PF_FLG))) {         /* let go and faded: off, the line empty next time */
        pf.fn = 0;
        return;
    }
    if (!pf.fn)                                         /* (starting: the line empty) */
        memset(fl_buf, 0, sizeof fl_buf);
    for (i = 0; i < n; i++, q += inc) {
        pf.fw += clamp((pf.fn > FL_MAX + 1u ? tw : 0) - pf.fw, -SL_SLOPE, SL_SLOPE);   /* (heard once it holds 8 ms) */
        for (c = 0; c < 2u; c++)                        /* (one call: inlined; the right a quarter of the sweep behind) */
            b[c][i] = fl_side(c, b[c][i], q - (c << 30));
        if (++pf.fwp >= FL_LEN)
            pf.fwp = 0;
        if (pf.fn <= FL_MAX + 1u)
            pf.fn++;
    }
}

/* PHASER: the stages' coefficient a (Q14) for a break of 150 Hz * 2^(5 i / 32): a = (g - 1) / (g + 1), g = tan(pi f / fs) */
static const int16_t PH_A[33] = {
    -16038, -15998, -15955, -15906, -15853, -15793, -15727, -15653, -15572, -15481, -15381, -15270, -15147, -15012,
    -14862, -14696, -14514, -14313, -14092, -13849, -13582, -13290, -12970, -12621, -12240, -11825, -11375, -10886,
    -10356, -9784, -9166, -8500, -7783,
};
/* PHASER: the stages' coefficient at the sweep's place q (Q14) */
static int32_t ph_coef(uint32_t q)
{
    uint32_t t = pf_tri(q), i = t >> 11;
    return PH_A[i] + (((PH_A[i + 1u] - PH_A[i]) * (int32_t)(t & 2047u)) >> 11);
}
/* PHASER on one side c: x in, the stages' coefficient a (rounded: no DC creeping in through the low breaks) */
static inline int32_t ph_side(uint32_t c, int32_t x, int32_t a)
{
    int32_t xs = clamp((x + 2) >> 2, -16383, 16383), u = xs + ((pf.pfb[c] * PH_FB + 16384) >> 15), y = u;
    uint32_t s;
    for (s = 0; s < PH_ST; s++) {                       /* y = a (u - y1) + u1 */
        y = clamp(pf.px[c][s] + ((a * (u - pf.py[c][s]) + 8192) >> 14), -65535, 65535);
        pf.px[c][s] = u;
        pf.py[c][s] = y;
        u = y;
    }
    pf.pfb[c] = clamp(y, -32767, 32767);
    u = xs + y;                                         /* (x + y) / 2 * 0.9, x 4 (>> 2 above): (x + y) * 1.8 */
    return pf_mix(x, u + mulq16(u, 52429u), pf.pw);
}
/* after FLANGER, while held or fading: PHASER (noinline, as FLANGER). The coefficients a block at a time (at its
 * middle: 2.9 ms at most, the sweep moves 1 % in frequency meanwhile) */
static __attribute__((noinline)) void perf_phaser(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, c, inc, q = perf_lfo(8u, n, &inc) + (n >> 1) * inc;
    int32_t tw = pf.act & PF_BIT(PF_PHS) ? perf_mod_depth() : 0, *const b[2] = {bl, br};
    int32_t a[2] = {ph_coef(q), ph_coef(q - 0x40000000u)};   /* (the right a quarter of the sweep behind) */
    if (!pf.pw) {                                       /* starting: the stages from rest */
        memset(pf.px, 0, sizeof pf.px);
        memset(pf.py, 0, sizeof pf.py);
        pf.pfb[0] = pf.pfb[1] = 0;
    }
    for (i = 0; i < n; i++) {
        pf.pw += clamp(tw - pf.pw, -SL_SLOPE, SL_SLOPE);
        for (c = 0; c < 2u; c++)
            b[c][i] = ph_side(c, b[c][i], a[c]);
    }
}

/* DUCK (E2): the gain t samples into a beat of q (Q15): down to DK_FLOOR over DK_ATT, back over three quarters of it */
static int32_t duck_gain(uint32_t t, uint32_t q)
{
    uint32_t r = q - (q >> 2) - DK_ATT, y;
    if (t < DK_ATT)
        return 32768 - (int32_t)((32768u - DK_FLOOR) * t / DK_ATT);
    t -= DK_ATT;
    if (t >= r)
        return 32768;
    while (r > 65535u) {                            /* (t << 15 in 32 bits: a slow external clock) */
        r >>= 1;
        t >>= 1;
    }
    y = 32768u - (t << 15) / r;                     /* what is left of the way back, Q15: an ease-out 1 - y^2 */
    return (int32_t)(DK_FLOOR + (((32768u - DK_FLOOR) * (32768u - ((y * y) >> 15))) >> 15));
}
/* DUCK after PHASER, while held or fading (noinline: its own loop, as FLANGER's). The curve at the block's two ends,
 * straight between them (32 samples) */
static __attribute__((noinline)) void perf_duck(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, q = pf.P << 2, t0 = (pf.ph - n) % q;
    int32_t g = duck_gain(t0, q), dg = (duck_gain((t0 + n) % q, q) - g) / (int32_t)n;
    int32_t tw = pf.act & PF_BIT(PF_DUCK) ? 32768 - perf_k[3] * 327 : 0;   /* (KNOB 4 DEPTH: how deep) */
    for (i = 0; i < n; i++, g += dg) {
        uint32_t k;
        pf.dw += clamp(tw - pf.dw, -SL_SLOPE, SL_SLOPE);
        k = (uint32_t)(32768 - mulq15(pf.dw, 32768 - g)) << 1;
        bl[i] = mulq16(bl[i], k);
        br[i] = mulq16(br[i], k);
    }
}
/* STEREO (E2): left and right gains (Q15, sqrt 2 at the side, 1 in the middle) at the sweep's place q: a sine of the
 * pan angle, equal power */
static void ster_gains(uint32_t q, int32_t *gl, int32_t *gr)
{
    uint32_t th = 0x20000000u + ((uint32_t)osc_sine(q) << 14);   /* pi/4 +- pi/4 */
    *gl = (osc_sine(th + 0x40000000u) * 46341) >> 15;
    *gr = (osc_sine(th) * 46341) >> 15;
}
/* STEREO after DUCK, while held or fading: the mix's middle panned, a sweep every two 1/16s in time with the clock */
static __attribute__((noinline)) void perf_stereo(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, inc, q = perf_lfo(2u, n, &inc);
    int32_t gl, gr, gl1, gr1, dl, dr;
    int32_t tw = pf.act & PF_BIT(PF_STER) ? 32768 - perf_k[3] * 327 : 0;
    ster_gains(q, &gl, &gr);
    ster_gains(q + n * inc, &gl1, &gr1);
    dl = (gl1 - gl) / (int32_t)n;
    dr = (gr1 - gr) / (int32_t)n;
    for (i = 0; i < n; i++, gl += dl, gr += dr) {
        int32_t m = (bl[i] >> 1) + (br[i] >> 1);
        pf.sw += clamp(tw - pf.sw, -SL_SLOPE, SL_SLOPE);
        bl[i] = pf_mix(bl[i], mulq16(m, (uint32_t)gl) << 1, pf.sw);   /* (k up to sqrt 2: mulq16 takes Q16 up to 1) */
        br[i] = pf_mix(br[i], mulq16(m, (uint32_t)gr) << 1, pf.sw);
    }
}

/* after the master level: the buffer effect, LPF, HPF, CRUSH on l / r (stereo, Q15) */
static __attribute__((noinline)) void perf_block(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, a = pf.act0, sh, hold;
    int32_t c = perf_k[1], lt, ht, ct;
    if (c)
        pf.cc = c;
    ct = c ? 32768 : 0;
    sh = 4u + (uint32_t)pf.cc * 7u / 100u;
    hold = sh - 3u;
    if (pf.split)
        perf_switch(a);
    pf.hfb += clamp(perf_k[3] * HB_FB - pf.hfb, -512, 512);   /* OCT UP / DN's shimmer glides (~40 ms) */
    for (i = 0; i < n; i++) {
        int32_t l = bl[i], r = br[i];
        if (i == pf.split) {
            a = pf.act;
            perf_switch(a);
        }
        if (pf.mode != BM_NONE)
            perf_buf(&l, &r);
        lt = (a & PF_BIT(PF_LPF)) || perf_k[0] < 0 || pf.lc < PF_TOP ? 32768 : 0;
        pf.la += clamp(lt - pf.la, -SL_SLOPE, SL_SLOPE);
        if (pf.la) {
            int32_t fl = pf_svf(clamp(l >> 2, -32767, 32767), &pf.lz[0], pf.lk, 0) << 2;
            int32_t fr = pf_svf(clamp(r >> 2, -32767, 32767), &pf.lz[2], pf.lk, 0) << 2;
            l = pf_mix(l, fl, pf.la);
            r = pf_mix(r, fr, pf.la);
        } else {
            pf.lz[0] = pf.lz[1] = pf.lz[2] = pf.lz[3] = 0;
        }
        ht = (a & PF_BIT(PF_HPF)) || perf_k[0] > 0 || pf.hc > 0 ? 32768 : 0;
        pf.ha += clamp(ht - pf.ha, -SL_SLOPE, SL_SLOPE);
        if (pf.ha) {
            int32_t fl = pf_svf(clamp(l >> 2, -32767, 32767), &pf.hz[0], pf.hk, 1) << 2;
            int32_t fr = pf_svf(clamp(r >> 2, -32767, 32767), &pf.hz[2], pf.hk, 1) << 2;
            l = pf_mix(l, fl, pf.ha);
            r = pf_mix(r, fr, pf.ha);
        } else {
            pf.hz[0] = pf.hz[1] = pf.hz[2] = pf.hz[3] = 0;
        }
        {   /* CRUSH: fewer bits, a held sample (KNOB 2) */
            pf.cw += clamp(ct - pf.cw, -SL_SLOPE, SL_SLOPE);
            if (pf.cw) {
                if (++pf.cn >= hold) {
                    pf.cn = 0;
                    pf.chl = ((l + (1 << (sh - 1u))) >> sh) << sh;   /* to the nearest step, as the INSERT's */
                    pf.chr = ((r + (1 << (sh - 1u))) >> sh) << sh;   /* (mid-rise put silence at +half a step:
                                                                       * a DC, a thump when let go) */
                }
                l = pf_mix(l, pf.chl, pf.cw);
                r = pf_mix(r, pf.chr, pf.cw);
            }
        }
        bl[i] = l;
        br[i] = r;
    }
    if ((pf.act & PF_BIT(PF_FLG)) || pf.fw || pf.fn)
        perf_flanger(bl, br, n);
    if ((pf.act & PF_BIT(PF_PHS)) || pf.pw)
        perf_phaser(bl, br, n);
    if ((pf.act & PF_BIT(PF_DUCK)) || pf.dw)        /* (E2) */
        perf_duck(bl, br, n);
    if ((pf.act & PF_BIT(PF_STER)) || pf.sw)
        perf_stereo(bl, br, n);
    pf.busy = pf.act || pf.mode != BM_NONE || pf.w || pf.la || pf.ha || pf.cw || pf.td || pf.mute || pf.fw || pf.pw || pf.thr ||
              pf.fn || pf.dw || pf.sw || perf_k[0] || perf_k[1] || perf_k[2];
}
