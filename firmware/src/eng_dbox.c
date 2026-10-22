/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DBOX: twelve drum sounds of two oscillators each, one a key, after the concept of the OP-1's DBOX (M-Vave-OP1;
 * docs/VDRUM_DBOX_SPEC.md). Our own design: no code or sounds of Teenage Engineering's.
 *
 * Keys: the first C is the kick as on DRUM (drum_keys); a note plays sound n mod 12 (C .. B: the kit's twelve), the next
 * octave the same twelve an octave up, the one below an octave down (as far as +-2 octaves). The factory kits put
 * their sounds where General MIDI has them (C kick, D snare, F# closed hat, A# open hat ..), so the BEAT pattern and
 * MIDI drum parts play on them. On the SEQ grid (kitlane.c) eight lanes strike C D D# F# A# A C# B, KICK .. BELL as on
 * DRUM; a note shows, lights (drum_lit) and is muted (drum_mute, the EDIT layer) with the lane of its key.
 *
 * A sound (the kit's 10 bytes, DK_*): two oscillators, each PIT (12 + PIT x 108 / 127 semitones: ~16 Hz .. ~8 kHz),
 * WAV (a morph sine -> triangle -> saw -> square -> noise, 0 .. 127, blended between neighbours) and ENV (one value, as
 * on the OP-1: 0..63 a decay (99 % of the way) from ~12 ms to ~0.3 s with no attack, 64..127 longer decays up to ~6 s
 * and an attack of up to ~80 ms). Both start with a short pitch drop (the thump: up to +12 semitones, deeper at short ENV, ~8 ms). XMOD is
 * the amount of MODE's cross modulation:
 *   1 MIX   both heard (XMOD: oscillator 2's share)        2 FM    osc 2 into osc 1's phase (index up to ~4)
 *   3 RING  osc 1 x osc 2 (XMOD: its share)                4 SYNC  osc 2 restarts with osc 1 (XMOD: its share)
 *   5 AM    osc 1 scaled by osc 2 (osc 2 not heard)        6 NOISE white noise into osc 1's phase, osc 2 mixed
 *   7 FILT  osc 2 sweeps the filter's cutoff (osc 2 not heard)
 * (The OP-1 manual shows a MODE dial 1..7 without saying what it does: this list is ours.) Then CUT (a low-pass, 127
 * open) and LVL (square law). The oscillator 2's envelope is its level as a modulator too: FM's index rings down with it.
 *
 * Engine values: KIT (F1..F4, OWN), TUNE (64: as stored, +-24 semitones), DECY (64: as stored, every ENV longer or
 * shorter), TONE (64: as stored, every CUT up or down), ACC (the accent at full velocity), DRV (a soft clip, the level
 * kept). HOME: TUNE DECY TONE DRV. Kits as VDRUM's (eng_vdrum.c): F n factory, OWN the track's own (dbx_own, 128 B a
 * track), an edit copies a factory kit into OWN; projects and user presets keep it where an FM6 track keeps its patch.
 *
 * Voices: one a note (a hit of the same note takes its voice again: voice.c), the state in the voice: ph[0] ph[1] the
 * oscillators, s[0] s[1] their envelopes (Q24), s[2] s[3] the filter, s[4] the noise, s[5] the pitch drop (Q24), s[6] the
 * attacks' state (bit 0 osc 1, bit 1 osc 2 still rising), s[7] the noise held for osc's NOISE wave. */
#define DBX_NK 12u                                   /* sounds a kit */
#define DBX_NFAC 4u
#define DBX_OWN DBX_NFAC
#define DK_TAG 0x44u                                 /* 'D' */
#define DK_VER 1u
#define DK_KEY(k) (2u + 10u * (k))
enum { DK_PIT1, DK_WAV1, DK_ENV1, DK_PIT2, DK_WAV2, DK_ENV2, DK_XMOD, DK_CUT, DK_MODE, DK_LVL };
enum { DM_MIX, DM_FM, DM_RING, DM_SYNC, DM_AM, DM_NOISE, DM_FILT };
static const char *const N_DBX_MODE[] = {"1 MIX", "2 FM", "3 RING", "4 SYNC", "5 AM", "6 NOIS", "7 FILT"};
static const char *const N_DBX_KIT[] = {"INIT", "808-ISH", "CHIP", "METAL", "OWN"};
static const char *const N_DBX_KEY[DBX_NK] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/* the factory kits: per key S(PIT1, WAV1, ENV1, PIT2, WAV2, ENV2, XMOD, CUT, MODE 1..7, LVL). WAV: 0 sine, 32 tri,
 * 64 saw, 96 square, 127 noise; PIT: kick ~20, snare body ~48, hats ~110 */
#define S(p1, w1, e1, p2, w2, e2, x, c, md, l) p1, w1, e1, p2, w2, e2, x, c, (md) - 1, l
static const uint8_t DBX_FACTORY[DBX_NFAC][VK_SIZE] = {
    {DK_TAG, DK_VER,                                                    /* F1 INIT */
     S(20, 0, 64, 30, 0, 10, 0, 127, 1, 110),           /* C   kick */
     S(70, 32, 14, 100, 127, 12, 60, 110, 1, 90),        /* C#  rim */
     S(48, 32, 24, 90, 127, 30, 90, 110, 1, 100),        /* D   snare */
     S(80, 127, 30, 70, 127, 22, 40, 90, 1, 100),        /* D#  clap */
     S(52, 32, 26, 100, 127, 30, 80, 120, 1, 95),        /* E   snare 2 */
     S(32, 0, 56, 40, 0, 10, 0, 127, 1, 100),           /* F   low tom */
     S(110, 127, 12, 120, 127, 10, 60, 127, 1, 80),      /* F#  closed hat */
     S(38, 0, 56, 46, 0, 10, 0, 127, 1, 100),           /* G   mid tom */
     S(105, 127, 18, 115, 127, 14, 60, 127, 1, 70),      /* G#  pedal hat */
     S(44, 0, 56, 52, 0, 10, 0, 127, 1, 100),           /* A   high tom */
     S(108, 127, 58, 118, 127, 44, 60, 127, 1, 75),      /* A#  open hat */
     S(76, 96, 36, 82, 96, 30, 64, 110, 1, 80),          /* B   cowbell */
     0, 0, 0, 0, 0, 0},
    {DK_TAG, DK_VER,                                                    /* F2 808-ISH */
     S(18, 0, 76, 26, 0, 8, 0, 127, 1, 120),
     S(74, 96, 10, 96, 96, 8, 70, 100, 3, 85),
     S(50, 0, 26, 92, 127, 34, 100, 112, 1, 100),
     S(84, 127, 34, 74, 127, 26, 50, 88, 7, 100),
     S(54, 32, 28, 96, 127, 32, 90, 116, 1, 95),
     S(30, 0, 62, 36, 0, 8, 0, 127, 1, 105),
     S(112, 96, 10, 121, 96, 8, 110, 127, 3, 75),
     S(36, 0, 62, 42, 0, 8, 0, 127, 1, 105),
     S(108, 96, 16, 117, 96, 12, 110, 127, 3, 65),
     S(42, 0, 62, 48, 0, 8, 0, 127, 1, 105),
     S(110, 96, 60, 119, 96, 46, 110, 127, 3, 70),
     S(77, 96, 40, 84, 96, 34, 70, 100, 1, 85),
     0, 0, 0, 0, 0, 0},
    {DK_TAG, DK_VER,                                                    /* F3 CHIP */
     S(22, 96, 60, 34, 96, 10, 20, 100, 4, 100),
     S(80, 96, 8, 92, 96, 8, 40, 127, 1, 80),
     S(52, 96, 22, 0, 127, 26, 90, 120, 6, 95),
     S(86, 127, 28, 0, 127, 20, 60, 100, 6, 90),
     S(56, 96, 24, 0, 127, 24, 100, 127, 6, 90),
     S(34, 96, 48, 40, 32, 10, 30, 110, 4, 95),
     S(118, 127, 8, 0, 127, 8, 0, 127, 1, 70),
     S(40, 96, 48, 46, 32, 10, 30, 110, 4, 95),
     S(114, 127, 12, 0, 127, 10, 0, 127, 1, 60),
     S(46, 96, 48, 52, 32, 10, 30, 110, 4, 95),
     S(116, 127, 50, 0, 127, 36, 0, 127, 1, 65),
     S(84, 96, 32, 96, 96, 26, 90, 127, 4, 80),
     0, 0, 0, 0, 0, 0},
    {DK_TAG, DK_VER,                                                    /* F4 METAL */
     S(20, 0, 70, 47, 0, 24, 50, 120, 2, 110),
     S(78, 0, 14, 101, 0, 12, 100, 120, 2, 85),
     S(50, 64, 28, 83, 0, 26, 80, 110, 3, 95),
     S(88, 127, 32, 95, 64, 22, 70, 100, 3, 95),
     S(58, 64, 30, 91, 0, 28, 110, 120, 2, 90),
     S(34, 0, 60, 61, 0, 30, 60, 110, 2, 100),
     S(112, 96, 12, 125, 96, 10, 120, 127, 3, 75),
     S(40, 0, 60, 67, 0, 30, 60, 110, 2, 100),
     S(108, 96, 20, 123, 96, 16, 120, 127, 3, 65),
     S(46, 0, 60, 73, 0, 30, 60, 110, 2, 100),
     S(110, 96, 70, 124, 96, 56, 120, 127, 3, 70),
     S(82, 0, 76, 101, 0, 66, 90, 127, 2, 80),
     0, 0, 0, 0, 0, 0},
};
#undef S

static uint8_t dbx_own[NTRK][VK_SIZE] __attribute__((section(".pool")));   /* the tracks' own kits (OWN) */
static volatile uint8_t dbx_pick[NTRK];        /* the key struck last by hand (+ 1, 0 none): seq.c input_on, the pages */

static int dbx_kit_ok(const uint8_t *b)
{
    uint32_t i;
    if (b[0] != DK_TAG || b[1] != DK_VER)
        return 0;
    for (i = 2; i < VK_SIZE; i++)
        if (b[i] > 127u)
            return 0;
    return 1;
}
/* the kit track t plays: a factory one (KIT F1..F4) or its own (INIT while it has none); 0 for a track without one */
static const uint8_t *dbx_kit_of(const track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk), k = (uint32_t)t->p[P_E0];
    if (tr >= NTRK)
        return 0;
    if (k < DBX_NFAC)
        return DBX_FACTORY[k];
    return dbx_own[tr][0] == DK_TAG && dbx_own[tr][1] == DK_VER ? dbx_own[tr] : DBX_FACTORY[0];
}
static void dbx_own_reset(uint32_t tr) { memcpy(dbx_own[tr % NTRK], DBX_FACTORY[0], VK_SIZE); }
static void dbx_pack(uint32_t tr, uint8_t *b)
{
    const uint8_t *k = dbx_own[tr % NTRK];
    memcpy(b, k[0] == DK_TAG && k[1] == DK_VER ? k : DBX_FACTORY[0], VK_SIZE);
}
static void dbx_unpack(uint32_t tr, const uint8_t *b)
{
    if (dbx_kit_ok(b))
        memcpy(dbx_own[tr % NTRK], b, VK_SIZE);
    else
        dbx_own_reset(tr);
}

/* a note's sound (0..11) and octave shift (1/16 semitone, +-2 octaves around the first C's) */
static uint32_t dbx_key(uint32_t note, int32_t *oct16)
{
    int32_t o = (int32_t)(note / 12u) - 3;
    *oct16 = clamp(o, -2, 2) * 12 * 16;
    return note % 12u;
}

/* ENV (0..127) -> the decay's ENV_EXP index and the attack's ENV_LIN index (0: none) */
static uint32_t dbx_dec(uint32_t env, int32_t decy) { return (uint32_t)clamp(34 + (int32_t)env * 86 / 127 + (decy - 64) / 2, 0, 127); }
static uint32_t dbx_atk(uint32_t env) { return env < 64u ? 0u : (env - 64u) * 60u / 63u; }

static void dbx_note_on(track_t *t, voice_t *v)
{
    const uint8_t *K = dbx_kit_of(t), *b;
    int32_t o;
    if (!K)
        return;
    b = K + DK_KEY(dbx_key(v->note, &o));
    v->ph[0] = v->ph[1] = 0;
    v->s[0] = dbx_atk(b[DK_ENV1]) ? 0 : 1 << 24;
    v->s[1] = dbx_atk(b[DK_ENV2]) ? 0 : 1 << 24;
    v->s[6] = (dbx_atk(b[DK_ENV1]) ? 1 : 0) | (dbx_atk(b[DK_ENV2]) ? 2 : 0);
    v->s[2] = v->s[3] = 0;
    if (!v->s[4])
        v->s[4] = (int32_t)(0x2545F491u ^ ((uint32_t)v->note * 0x9E3779B9u));
    v->s[5] = 1 << 24;
    v->env_out = v->vel * 258;                       /* the hit starts at its level (drum_note_on says why) */
    v->env = 1 << 24;
    drum_lit[(uint32_t)(t - trk) % NTRK] |= (uint8_t)(1u << DBX_KEY_LANE[v->note % 12u]);   /* (its grid lane's LED) */
}

/* the voice's amplitude: its envelopes', not the ADSR's; a voice whose two have rung out ends */
static int32_t dbx_amp(track_t *t, voice_t *v, int32_t adsr)
{
    (void)t;
    (void)adsr;
    if (!v->active)
        return 0;
    if (v->s[0] < (1 << 10) && v->s[1] < (1 << 10) && !(v->s[6] & 3)) {
        v->active = v->gate = 0;
        v->stage = 0;
        v->env = 0;
        return 0;
    }
    v->env = 1 << 24;
    return 32767;
}

/* one block of an oscillator's envelope (bit: its attack bit in s[6]) from e: the value at the block's end */
static int32_t dbx_env(voice_t *v, int32_t e, uint32_t env, int32_t decy, uint32_t bit)
{
    if (v->s[6] & (int32_t)bit) {
        e += (int32_t)ENV_LIN[dbx_atk(env)];
        if (e >= 1 << 24) {
            e = 1 << 24;
            v->s[6] &= ~(int32_t)bit;
        }
        return e;
    }
    e -= mulq16(e, ENV_EXP[dbx_dec(env, decy)]);
    return e < (1 << 10) ? 0 : e;
}

/* WAV's morph at phase ph (increment inc: the saw's and square's polyBLEP), n the voice's noise */
static inline int32_t dbx_wave(uint32_t w, uint32_t ph, uint32_t inc, int32_t *n)
{
    uint32_t seg = w >> 5, f = (w & 31u) << 10;      /* the two neighbours, the blend (Q15) */
    int32_t a, b;
    switch (seg) {
    case 0: a = sine_i(ph); b = osc_tri(ph); break;
    case 1: a = osc_tri(ph); b = osc_saw(ph, inc); break;
    case 2: a = osc_saw(ph, inc); b = osc_pulse(ph, inc, 0x80000000u) << 1; break;
    default: a = osc_pulse(ph, inc, 0x80000000u) << 1; b = (int32_t)noise32(n) >> 16; f = w >= 127u ? 32768u : f; break;
    }
    return a + (((b - a) * (int32_t)f) >> 15);
}

/* a block's control values (dbx_render: the divides out of the samples) */
typedef struct {
    uint32_t inc1, inc2, w1, w2, mode, sync;
    int32_t g1, d1, g2, d2, x, lv;               /* (g, d: the level ramps, Q23) */
    tsvf_t c;
    int32_t cut;
} dbx_blk_t;
static __attribute__((noinline)) void dbx_block(const track_t *t, voice_t *v, const vmod_t *m, dbx_blk_t *B)
{
    const int16_t *p = t->p;
    const uint8_t *K = dbx_kit_of(t), *b;
    int32_t o, base, drop, e0, e1, tune, l;
    uint32_t k = dbx_key(v->note, &o);
    b = K + DK_KEY(k);
    tune = (p[P_E1] - 64) * 6 + (m->pitch16 - (int32_t)v->note * 16) + o;   /* TUNE (+-24 st), bend, PIT, the octave */
    drop = (int32_t)(((int64_t)((12 - (int32_t)b[DK_ENV1] * 10 / 127) * 16) * v->s[5]) >> 24);   /* the thump */
    v->s[5] -= mulq16(v->s[5], ENV_EXP[30]);
    base = 12 * 16 + tune + drop;
    B->inc1 = pitch_inc((uint32_t)clamp(base + (int32_t)(b[DK_PIT1] & 127u) * 108 * 16 / 127, 0, 2047));
    B->inc2 = pitch_inc((uint32_t)clamp(base + (int32_t)(b[DK_PIT2] & 127u) * 108 * 16 / 127, 0, 2047));
    B->w1 = b[DK_WAV1] & 127u;
    B->w2 = b[DK_WAV2] & 127u;
    B->mode = (b[DK_MODE] & 7u) % 7u;
    B->x = (int32_t)(b[DK_XMOD] & 127u) * 258;
    l = (int32_t)(b[DK_LVL] & 127u);
    B->lv = l * l * 2;
    e0 = v->s[0];                                /* the level ramps, Q23 (the envelope Q24 >> 1), from the level at */
    e1 = v->s[0] = dbx_env(v, e0, b[DK_ENV1], p[P_E2], 1u);   /* the block's start to the one at its end. M-Vave-OP1: */
    B->g1 = e0 >> 1;                             /* Q15 steps a sample fell short of the next block's start by up to */
    B->d1 = ((e1 >> 1) - B->g1) >> CTL_LOG2;     /* ~32 units, the level jumped back up at every block: a sawtooth at */
    e0 = v->s[1];                                /* the block rate on the decaying kick, heard as a hiss (as VDRUM's, */
    e1 = v->s[1] = dbx_env(v, e0, b[DK_ENV2], p[P_E2], 2u);   /* eng_vdrum.c vd_layer_ctl) */
    B->g2 = e0 >> 1;
    B->d2 = ((e1 >> 1) - B->g2) >> CTL_LOG2;
    B->cut = clamp((int32_t)(b[DK_CUT] & 127u) + (p[P_E3] - 64) + (m->cutoff >> 8), 0, 127);
    tsvf_coef_k(&B->c, B->cut << 8, 3000);
}

static void dbx_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    dbx_blk_t B;
    int32_t acc, dg = 0, dk = 0, g1, g2, ic1, ic2, nz, mute[CTL];
    uint32_t i, ph1, ph2;
    if (!dbx_kit_of(t))
        return;
    if (n > CTL)
        n = CTL;
    if ((drum_mute[(uint32_t)(t - trk) % NPART] >> DBX_KEY_LANE[v->note % 12u]) & 1u) {
        memset(mute, 0, sizeof mute);                /* (its grid lane muted on the EDIT layer: it rings on unheard) */
        out = mute;
    }
    dbx_block(t, v, m, &B);
    acc = 32768 + ((p[P_E4] * v->vel * v->vel) >> 6);
    if (p[P_E5] > 0) {                               /* DRV: x1 .. x4 into the soft clip, the level kept (Q12) */
        dg = 4096 + p[P_E5] * 3 * 4096 / 127;
        dk = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * dg) >> 12));
    }
    g1 = B.g1;
    g2 = B.g2;
    ph1 = v->ph[0];
    ph2 = v->ph[1];
    ic1 = v->s[2];
    ic2 = v->s[3];
    nz = v->s[4];
    for (i = 0; i < n; i++) {
        int32_t o2 = mulq15(dbx_wave(B.w2, ph2, B.inc2, &nz), g2 >> 8), o1, s, pm = 0;
        if (B.mode == DM_FM)
            pm = mulq15(o2, B.x) << 3;               /* (an index up to ~4: x8 of a cycle's 1/8) */
        else if (B.mode == DM_NOISE)
            pm = mulq15((int32_t)noise32(&nz) >> 16, B.x) << 3;
        o1 = mulq15(dbx_wave(B.w1, ph1 + ((uint32_t)pm << 13), B.inc1, &nz), g1 >> 8);
        switch (B.mode) {
        case DM_RING: s = o1 + mulq15(mulq15(o1, o2) * 2 - o1, B.x); break;
        case DM_AM: s = mulq15(o1, 32768 - (B.x >> 1) + mulq15(o2, B.x >> 1)); break;
        case DM_FILT: s = o1; break;
        case DM_FM: s = o1; break;
        default: s = o1 + mulq15(o2, B.x); break;    /* MIX, SYNC, NOISE: osc 2 at XMOD */
        }
        if (B.cut < 127 || B.mode == DM_FILT) {
            if (B.mode == DM_FILT && (i & 7u) == 0u) /* osc 2 sweeps the cutoff (every 8 samples) */
                tsvf_coef_k(&B.c, clamp((B.cut << 8) + mulq15(o2, B.x) * 3, 0, 127 << 8), 3000);
            s = tsvf_lp(&B.c, s, &ic1, &ic2);
        }
        s = (mulq15(s, B.lv) * (acc >> 3)) >> 12;    /* LVL, then ACC (Q12: the product fits) */
        if (dg)
            s = (softclip((s * dg) >> 12) * dk) >> 15;
        out[i] += voice_amp(soft_knee(s, 24000), m, i) << 1;
        g1 += B.d1;
        g2 += B.d2;
        {
            uint32_t was = ph1;
            ph1 += B.inc1;
            ph2 += B.inc2;
            if (B.mode == DM_SYNC && ph1 < was)      /* osc 1 wrapped: osc 2 restarts (in phase with it) */
                ph2 = ph1;
        }
    }
    v->ph[0] = ph1;
    v->ph[1] = ph2;
    v->s[2] = ic1;
    v->s[3] = ic2;
    v->s[4] = nz;
}

/* {KIT, TUNE, DECY, TONE, ACC, DRV, -, -}; each kit suggests the BEAT pattern (GM notes) */
static const preset_t DBOX_PRESETS[] = {
    {"INIT", {0, 64, 64, 64, 64, 0, 0, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 10), PAT(12)},
    {"808-ISH", {1, 64, 64, 64, 70, 10, 0, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 15), PAT(12)},
    {"CHIP", {2, 64, 64, 64, 64, 0, 0, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 5), PAT(12)},
    {"METAL", {3, 64, 64, 64, 64, 20, 0, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 20), PAT(12)},
};

static const engine_t ENG_DBOX = {
    .name = "DBOX",
    .page_title = {"KIT", "HIT"},
    .edit = {
        {"KIT", F_ENUM, 0, DBX_OWN, 0, N_DBX_KIT, 0},
        {"TUNE", F_PCT, 0, 127, 64, 0, 0},
        {"DECY", F_PCT, 0, 127, 64, 0, 0},
        {"TONE", F_PCT, 0, 127, 64, 0, 0},
        {"ACC", F_PCT, 0, 127, 64, 0, 0},
        {"DRV", F_PCT, 0, 127, 0, 0, 0},
        {"-", F_INT, 0, 0, 0, 0, 0},
        {"-", F_INT, 0, 0, 0, 0, 0},
    },
    .presets = DBOX_PRESETS,
    .npresets = NELEM(DBOX_PRESETS),
    .note_on = dbx_note_on,
    .render = dbx_render,
    .amp = dbx_amp,
    .knob = {P_E1, P_E2, P_E3, P_E5},
    .poly = 6,
    .oneshot = 1,
    .keys = drum_keys,
};
