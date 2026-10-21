/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* VDRUM: six drum parts of two synthesis layers each into a waveguide resonator, after the concept of Korg's
 * volca drum (M-Vave-OP1; docs/VDRUM_DBOX_SPEC.md). Our own design: no code, tables or sounds of Korg's.
 *
 * Keys: the GM drum map with the kick on the first C (as DRUM: drum_keys). A note strikes one part (VD_PART_OF):
 *   1 KICK 35 36   2 SNARE 38 40   3 CLAP 37 39   4 HAT CL 42 44   5 HAT OP 46 49 51..59   6 TOM 41 43 45 47 48 50
 * and every other note; part 6 is pitched by the note's distance from 45 (the toms of one part), the others are not.
 * One voice a part (vd_part_t.owner, as DRUM's lanes): a new hit of the part restarts it, the others ring on.
 *
 * A layer (the kit's 7 bytes, VK_L*): SRC sine, saw (polyBLEP), or white noise through a high-, low- or band-pass
 * at PITCH; a pitch modulator MOD: EXP a sweep down from MAMT (up to 48 semitones) that decays in MRAT's time, TRI a
 * triangle, RND a sample-and-hold, both at MRAT's rate (0.2 .. 160 Hz) and MAMT's depth (up to 24 semitones); an
 * amplitude envelope EG: AD (a linear attack in ATK's time, an exponential decay in REL's), EXP (a curved attack:
 * slow at first), MULT (struck 3 times ATK apart, short decays between, then REL's: claps). LVL its level (square
 * law). The times are voice.c's ENV_LIN / ENV_EXP for 1 ms .. 10 s (gen_tables.py), worked once a block (CTL) and
 * ramped linearly inside it; the pitch is set once a block and the phase increment ramped likewise.
 *
 * A part (7 bytes, VK_P*): the two layers summed, then BIT (bits dropped: none .. 12), FOLD (a triangle wavefolder,
 * x1 .. x8 into it), DRV (a soft clip x1 .. x4, the level kept), the voice's amplitude (velocity, the track's LFO AMP,
 * the fades), then GAIN into the output and SEND into the resonator (square law, 100 = unity, 127 +4 dB). CHOK: a hit
 * of a part chokes every other part of its group (1..3; 0 none) with a ~1.2 ms release. PAN is reserved (the engine
 * renders the track's one mono signal: open point 1 of the spec).
 *
 * The resonator (vd_post, after the voices: the engine_t.post hook): a delay line fed by the parts' sends.
 *   STRING  y = in + g LP(line[D]): every harmonic of TUNE, plucked, metallic
 *   TUBE    y = in - g LP(line[D]) with half the delay: the inverted loop rings at the same pitch with odd harmonics
 *           only, round and woody (a closed-open tube, a drum shell)
 * TUNE C1 .. C7 (fixed: the keys do not move it, as on the volca), DECAY the ring time ~20 ms .. ~4 s (to -60 dB;
 * the loop gain per pass follows the pitch), BODY the loop's one-pole low-pass: dark .. bright. The read is linearly
 * interpolated, the low-pass's delay at the fundamental taken off the line, a soft knee keeps the loop bounded. MIX
 * is the resonance in the output (square law, 127 x2). It rings on after the last voice: vd_post reports sound
 * until its peak has been below ~-80 dBFS for 50 ms with no input, then clears the line and sleeps (it costs
 * nothing until a part sends again). DRV (EDIT 2) is a drive on the whole output, after the resonator.
 *
 * Kits: the track's KIT (P_E4) is F1 .. F8 (VD_FACTORY) or OWN (8: vd_own, the track's own kit, 128 bytes a track in
 * the pool, kept while the track plays another engine). The ISR reads the bytes of the kit KIT names (vd_kit_of): a
 * byte written by the main loop is taken at the next block, a torn read of one byte is harmless. An edit of a factory
 * kit copies it into OWN first (vd_kit_edit). Projects and user presets keep the own kit in the track's 128-byte FM6
 * patch field (vk_kit.c). Format: VK_TAG, VK_VER, then 6 x 21 bytes, every byte 0..127 (the user presets' 7-bit
 * packing, up_fm6.c).
 *
 * State: per part (engine memory, engines.c eng_mem, zeroed on a switch: all-zero is six silent parts and a quiet
 * resonator) the layers, the noise, the owner and the choke; the resonator's line (2048 samples, int16) and its
 * input for the block. Voices: s[0] the part. */
#define VD_NP 6u                                     /* parts */
#define VD_LINE 2048u                                /* the resonator's line: STRING to ~21.5 Hz */
#define VD_NFAC 8u                                   /* factory kits F1..F8 */
#define VD_OWN VD_NFAC                               /* KIT value: the track's own kit */
#define VK_SIZE 128u
#define VK_TAG 0x56u                                 /* 'V' */
#define VK_VER 1u
#define VK_PART(k) (2u + 21u * (k))                  /* part k's first byte */
enum { VK_LTYPE, VK_LPIT, VK_LMAMT, VK_LMRAT, VK_LATK, VK_LREL, VK_LLVL };      /* a layer's bytes (+ 7 l) */
enum { VK_PBIT = 14, VK_PFOLD, VK_PDRV, VK_PGAIN, VK_PSEND, VK_PCHOK, VK_PPAN };  /* the part's (after the layers) */
enum { VS_SIN, VS_SAW, VS_HPN, VS_LPN, VS_BPN };       /* SRC */
enum { VM_EXP, VM_TRI, VM_RND };                        /* MOD */
enum { VE_AD, VE_EXP, VE_MULT };                        /* EG */
static const char *const N_VD_SRC[] = {"SIN", "SAW", "HPN", "LPN", "BPN"};
static const char *const N_VD_MOD[] = {"EXP", "TRI", "RND"};
static const char *const N_VD_EG[] = {"AD", "EXP", "MULT"};
static const char *const N_VD_TYPE[] = {"TUBE", "STRNG"};
static const char *const N_VD_KIT[] = {"INIT", "ELECTRO", "TUBES", "STRINGS", "METAL", "DUB", "CRUSH", "WOOD", "OWN"};
static const char *const N_VD_PART[VD_NP] = {"KICK", "SNARE", "CLAP", "HATCL", "HATOP", "TOM"};

typedef struct {
    uint32_t ph, inc;                                /* the oscillator's phase, last block's increment */
    uint32_t mph;                                    /* TRI / RND: the modulator's phase */
    int32_t env;                                     /* amplitude, Q24 */
    int32_t pm;                                      /* EXP: the sweep, Q24; RND: the held value, Q15 */
    int32_t f1, f2;                                  /* the noise filter */
    uint8_t stage, strike, cnt, rsv;                 /* stage 0 off, 1 attack, 2 decay; MULT: strikes, blocks left */
} vd_layer_t;
typedef struct {
    vd_layer_t l[2];
    int32_t rng;                                     /* the noise (both layers) */
    int16_t st16;                                    /* part 6: the note's pitch, 1/16 semitone */
    uint8_t owner, choke;                            /* the voice playing it (index + 1, 0 none); 1 = choked */
} vd_part_t;
typedef struct {
    vd_part_t part[VD_NP];
    int32_t in[CTL];                                 /* the sends of this block */
    int16_t line[VD_LINE];
    uint32_t wp, dly, key;                           /* write position, delay (Q16 samples), the values it is for */
    int32_t g, a, lp;                                /* loop gain (Q15, < 0 for TUBE), low-pass coefficient (Q15) */
    uint16_t quiet;                                  /* blocks below the floor */
    uint8_t on, fed;                                 /* ringing; a part sent this block */
} vd_mem_t;
ENG_MEM_FITS("VDRUM", sizeof(vd_mem_t));
_Static_assert(VK_SIZE == FM6_PACKED, "a kit takes the FM6 patch's place in projects and user presets");

/* the factory kits: per part two layers L(SRC, MOD, EG, PITCH, MAMT, MRAT, ATK, REL, LVL) and P(BIT, FOLD, DRV,
 * GAIN, SEND, CHOK). PITCH: 12 + PITCH x 108 / 127 semitones (MIDI; kick ~22, snare body ~49, noise: its filter,
 * hats ~124); MRAT, ATK, REL: 10000^(v / 127) ms (ENV_LIN / ENV_EXP: 50 ~ 40 ms, 64 ~ 100 ms, 82 ~ 400 ms) */
#define VL(s, m, e, pit, mamt, mrat, atk, rel, lvl) (s) | (m) << 3 | (e) << 5, pit, mamt, mrat, atk, rel, lvl
#define VP(bit, fold, drv, gain, send, chok) bit, fold, drv, gain, send, chok, 64
#define VOFF VL(VS_SIN, VM_EXP, VE_AD, 0, 0, 0, 0, 0, 0)
static const uint8_t VD_FACTORY[VD_NFAC][VK_SIZE] = {
    {VK_TAG, VK_VER,                                                                                   /* F1 INIT */
     VL(VS_SIN, VM_EXP, VE_AD, 22, 64, 50, 0, 82, 110), VOFF, VP(0, 0, 0, 100, 0, 0),
     VL(VS_SIN, VM_EXP, VE_AD, 49, 20, 40, 0, 60, 80), VL(VS_BPN, VM_EXP, VE_AD, 98, 0, 0, 0, 70, 100), VP(0, 0, 0, 100, 0, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 87, 0, 0, 32, 74, 110), VOFF, VP(0, 0, 0, 100, 0, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 124, 0, 0, 0, 48, 90), VOFF, VP(0, 0, 0, 100, 0, 1),
     VL(VS_HPN, VM_EXP, VE_AD, 122, 0, 0, 0, 80, 85), VOFF, VP(0, 0, 0, 100, 0, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 40, 12, 56, 0, 74, 105), VL(VS_LPN, VM_EXP, VE_AD, 70, 0, 0, 0, 40, 30), VP(0, 0, 0, 100, 0, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F2 ELECTRO */
     VL(VS_SIN, VM_EXP, VE_AD, 20, 80, 46, 0, 86, 115), VL(VS_SAW, VM_EXP, VE_AD, 30, 90, 30, 0, 30, 40), VP(0, 20, 30, 100, 10, 0),
     VL(VS_SAW, VM_EXP, VE_AD, 52, 30, 36, 0, 56, 70), VL(VS_HPN, VM_EXP, VE_AD, 105, 0, 0, 0, 66, 100), VP(0, 40, 20, 100, 40, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 92, 0, 0, 28, 70, 115), VOFF, VP(0, 0, 10, 100, 50, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 126, 0, 0, 0, 42, 90), VL(VS_SAW, VM_TRI, VE_AD, 118, 6, 120, 0, 36, 30), VP(0, 60, 0, 100, 0, 1),
     VL(VS_HPN, VM_EXP, VE_AD, 124, 0, 0, 0, 76, 85), VL(VS_SAW, VM_TRI, VE_AD, 116, 6, 120, 0, 70, 30), VP(0, 60, 0, 100, 0, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 44, 30, 50, 0, 70, 105), VL(VS_SAW, VM_RND, VE_AD, 60, 4, 90, 0, 50, 25), VP(0, 30, 20, 100, 30, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F3 TUBES */
     VL(VS_SIN, VM_EXP, VE_AD, 26, 36, 46, 0, 70, 110), VL(VS_LPN, VM_EXP, VE_AD, 40, 0, 0, 0, 30, 50), VP(0, 0, 0, 90, 70, 0),
     VL(VS_SIN, VM_EXP, VE_AD, 56, 12, 36, 0, 50, 70), VL(VS_BPN, VM_EXP, VE_AD, 80, 0, 0, 0, 50, 100), VP(0, 0, 0, 80, 90, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 70, 0, 0, 36, 60, 110), VOFF, VP(0, 0, 0, 80, 100, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 110, 0, 0, 0, 40, 80), VOFF, VP(0, 0, 0, 90, 40, 1),
     VL(VS_BPN, VM_EXP, VE_AD, 100, 0, 0, 0, 66, 85), VOFF, VP(0, 0, 0, 90, 50, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 46, 10, 50, 0, 60, 100), VL(VS_LPN, VM_EXP, VE_AD, 60, 0, 0, 0, 34, 40), VP(0, 0, 0, 80, 110, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F4 STRINGS */
     VL(VS_SIN, VM_EXP, VE_AD, 22, 50, 48, 0, 76, 110), VOFF, VP(0, 0, 0, 100, 30, 0),
     VL(VS_BPN, VM_EXP, VE_AD, 90, 0, 0, 0, 44, 100), VOFF, VP(0, 0, 0, 70, 110, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 100, 0, 0, 0, 30, 110), VOFF, VP(0, 0, 0, 50, 120, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 124, 0, 0, 0, 44, 85), VOFF, VP(0, 0, 0, 100, 20, 1),
     VL(VS_HPN, VM_EXP, VE_AD, 120, 0, 0, 0, 74, 80), VOFF, VP(0, 0, 0, 100, 30, 1),
     VL(VS_SAW, VM_EXP, VE_AD, 52, 4, 30, 0, 36, 90), VL(VS_BPN, VM_EXP, VE_AD, 95, 0, 0, 0, 24, 60), VP(0, 0, 0, 50, 120, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F5 METAL */
     VL(VS_SIN, VM_EXP, VE_AD, 24, 70, 44, 0, 74, 110), VL(VS_SAW, VM_EXP, VE_AD, 70, 40, 30, 0, 30, 30), VP(0, 50, 20, 100, 20, 0),
     VL(VS_SAW, VM_TRI, VE_AD, 70, 10, 110, 0, 60, 80), VL(VS_BPN, VM_EXP, VE_AD, 108, 0, 0, 0, 64, 90), VP(0, 70, 0, 90, 60, 0),
     VL(VS_SAW, VM_RND, VE_MULT, 90, 8, 115, 30, 66, 90), VOFF, VP(0, 80, 0, 90, 70, 0),
     VL(VS_SAW, VM_TRI, VE_AD, 120, 12, 125, 0, 46, 70), VL(VS_HPN, VM_EXP, VE_AD, 126, 0, 0, 0, 40, 60), VP(0, 90, 0, 90, 30, 1),
     VL(VS_SAW, VM_TRI, VE_AD, 116, 12, 125, 0, 84, 65), VL(VS_HPN, VM_EXP, VE_AD, 124, 0, 0, 0, 80, 55), VP(0, 90, 0, 90, 40, 1),
     VL(VS_SAW, VM_EXP, VE_AD, 58, 20, 50, 0, 80, 80), VL(VS_SIN, VM_TRI, VE_AD, 77, 16, 118, 0, 80, 60), VP(0, 60, 0, 90, 80, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F6 DUB */
     VL(VS_SIN, VM_EXP, VE_AD, 18, 40, 58, 0, 96, 120), VOFF, VP(0, 0, 10, 100, 10, 0),
     VL(VS_SIN, VM_EXP, VE_AD, 50, 16, 40, 0, 56, 60), VL(VS_BPN, VM_EXP, VE_AD, 84, 0, 0, 0, 62, 90), VP(0, 0, 0, 90, 90, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 82, 0, 0, 40, 70, 105), VOFF, VP(0, 0, 0, 80, 110, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 118, 0, 0, 0, 46, 75), VOFF, VP(0, 0, 0, 90, 30, 1),
     VL(VS_HPN, VM_EXP, VE_AD, 116, 0, 0, 0, 88, 70), VOFF, VP(0, 0, 0, 90, 50, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 36, 24, 62, 0, 84, 105), VOFF, VP(0, 0, 0, 90, 100, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F7 CRUSH */
     VL(VS_SIN, VM_EXP, VE_AD, 22, 70, 46, 0, 80, 115), VL(VS_SAW, VM_EXP, VE_AD, 28, 60, 30, 0, 24, 40), VP(70, 20, 60, 100, 10, 0),
     VL(VS_SAW, VM_EXP, VE_AD, 50, 24, 36, 0, 54, 80), VL(VS_HPN, VM_EXP, VE_AD, 100, 0, 0, 0, 64, 100), VP(90, 0, 50, 100, 20, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 88, 0, 0, 30, 70, 110), VOFF, VP(80, 0, 40, 100, 20, 0),
     VL(VS_HPN, VM_EXP, VE_AD, 124, 0, 0, 0, 44, 90), VOFF, VP(100, 0, 0, 100, 0, 1),
     VL(VS_HPN, VM_EXP, VE_AD, 122, 0, 0, 0, 76, 85), VOFF, VP(100, 0, 0, 100, 0, 1),
     VL(VS_SAW, VM_EXP, VE_AD, 42, 24, 50, 0, 66, 100), VOFF, VP(90, 30, 40, 100, 20, 0)},
    {VK_TAG, VK_VER,                                                                                   /* F8 WOOD */
     VL(VS_SIN, VM_EXP, VE_AD, 30, 30, 40, 0, 62, 105), VOFF, VP(0, 0, 0, 90, 60, 0),
     VL(VS_SIN, VM_EXP, VE_AD, 64, 8, 30, 0, 40, 70), VL(VS_BPN, VM_EXP, VE_AD, 92, 0, 0, 0, 40, 80), VP(0, 0, 0, 80, 90, 0),
     VL(VS_BPN, VM_EXP, VE_MULT, 96, 0, 0, 24, 46, 100), VOFF, VP(0, 0, 0, 70, 100, 0),
     VL(VS_SIN, VM_EXP, VE_AD, 100, 4, 20, 0, 26, 80), VL(VS_HPN, VM_EXP, VE_AD, 120, 0, 0, 0, 30, 40), VP(0, 0, 0, 80, 80, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 94, 4, 20, 0, 44, 80), VL(VS_HPN, VM_EXP, VE_AD, 118, 0, 0, 0, 56, 40), VP(0, 0, 0, 80, 80, 1),
     VL(VS_SIN, VM_EXP, VE_AD, 54, 6, 36, 0, 44, 100), VOFF, VP(0, 0, 0, 70, 110, 0)},
};
#undef VL
#undef VP
#undef VOFF

static uint8_t vd_own[NTRK][VK_SIZE] __attribute__((section(".pool")));   /* the tracks' own kits (OWN) */
static volatile uint8_t vd_kgen[NTRK];                                   /* +1 after each change of an own kit */
static volatile uint8_t vd_pick[NTRK];         /* the part struck last by hand (+ 1, 0 none): seq.c input_on, the pages */

/* the kit track t plays: a factory one (KIT F1..F8) or its own; 0 for a track without a kit */
static const uint8_t *vd_kit_of(const track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk), k = (uint32_t)t->p[P_E4];
    if (tr >= NTRK)
        return 0;
    if (k < VD_NFAC)
        return VD_FACTORY[k];
    return vd_own[tr][0] == VK_TAG && vd_own[tr][1] == VK_VER ? vd_own[tr] : VD_FACTORY[0];   /* (none yet: INIT) */
}

/* main loop: track tr's own kit valid (a power-on, a project that had none): the INIT kit */
static void vd_own_reset(uint32_t tr)
{
    memcpy(vd_own[tr % NTRK], VD_FACTORY[0], VK_SIZE);
    vd_kgen[tr % NTRK]++;
}

/* a project's or a user preset's 128 bytes (the FM6 patch's place) <-> track tr's own kit: what it holds goes in;
 * a record that is not a kit of this format (a wrong tag or version, a byte above 127) leaves the INIT kit */
static void vd_pack(uint32_t tr, uint8_t *b)
{
    const uint8_t *k = vd_own[tr % NTRK];
    memcpy(b, k[0] == VK_TAG && k[1] == VK_VER ? k : VD_FACTORY[0], VK_SIZE);
}
static int vd_kit_ok(const uint8_t *b)
{
    uint32_t i;
    if (b[0] != VK_TAG || b[1] != VK_VER)
        return 0;
    for (i = 2; i < VK_SIZE; i++)
        if (b[i] > 127u)
            return 0;
    return 1;
}
static void vd_unpack(uint32_t tr, const uint8_t *b)
{
    if (vd_kit_ok(b)) {
        memcpy(vd_own[tr % NTRK], b, VK_SIZE);
        vd_kgen[tr % NTRK]++;
    } else {
        vd_own_reset(tr);
    }
}

/* the GM note -> its part (0..5) and, for part 6, the note's pitch from the low tom (45) */
static const uint8_t VD_PART_OF[47] = {
    0, 0, 2, 1, 2, 1,                                /* 35 kick, 36 kick, 37 rim, 38 snare, 39 clap, 40 snare */
    5, 3, 5, 3, 5, 4,                                /* 41 tom, 42 hat cl, 43 tom, 44 pedal hat, 45 tom, 46 hat op */
    5, 5, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4,           /* 47 48 toms, 49 crash, 50 tom, 51..59 cymbals, bells */
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,   /* 60..81 percussion: part 6 */
};
static uint32_t vd_part_note(uint32_t note, int32_t *st16)
{
    uint32_t n = note >= 35u && note <= 81u ? note : 36u + (note + 120u - 36u) % 12u, k = VD_PART_OF[n - 35u];
    *st16 = 0;
    if (k == 5u && (n == 41u || n == 43u || n == 45u || n == 47u || n == 48u || n == 50u))
        *st16 = ((int32_t)n - 45) * 16;
    return k;
}

static vd_mem_t *vd_mem_of(const track_t *t)
{
    return t >= &trk[0] && t < &trk[NPART] ? (vd_mem_t *)ENG_MEM(t - trk) : 0;
}

/* the part voice v plays, 0 when it plays none (any more) */
static vd_part_t *vd_part_of(track_t *t, const voice_t *v)
{
    vd_mem_t *M = vd_mem_of(t);
    vd_part_t *P;
    uint32_t i = (uint32_t)(v - t->v);
    if (!M || i >= NVOICE)
        return 0;
    P = &M->part[(uint32_t)v->s[0] % VD_NP];
    return P->owner == i + 1u ? P : 0;
}

/* voice.c voice_alloc: a hit of a part that sounds takes its voice again (two GM notes of one part share it) */
static voice_t *vd_reuse(track_t *t, uint32_t note)
{
    vd_mem_t *M = vd_mem_of(t);
    int32_t st;
    uint32_t k = vd_part_note(note, &st), owner = M ? M->part[k].owner : 0;
    voice_t *v;
    if (!owner || owner > NVOICE)
        return 0;
    v = &t->v[owner - 1u];
    return v->active && (uint32_t)v->s[0] == k ? v : 0;
}

static void vd_note_on(track_t *t, voice_t *v)
{
    vd_mem_t *M = vd_mem_of(t);
    const uint8_t *K = vd_kit_of(t), *B;
    vd_part_t *P;
    uint32_t i = (uint32_t)(v - t->v), k, l, j, chok;
    int32_t st;
    if (!M || !K || i >= NVOICE)
        return;
    k = vd_part_note(v->note, &st);
    P = &M->part[(uint32_t)v->s[0] % VD_NP];
    if (P->owner == i + 1u)                          /* this voice played another part: it stops there */
        P->owner = 0;
    P = &M->part[k];
    P->owner = (uint8_t)(i + 1u);
    P->choke = 0;
    P->st16 = (int16_t)st;
    if (!P->rng)
        P->rng = (int32_t)(0x9E3779B9u ^ (k * 0x85EBCA6Bu));
    B = K + VK_PART(k);
    for (l = 0; l < 2u; l++) {
        vd_layer_t *L = &P->l[l];
        const uint8_t *b = B + 7u * l;
        if (L->env < (1 << 21))                      /* from (near) silence: the wave from its start */
            L->ph = 0;
        L->stage = 1;
        L->strike = 0;
        L->cnt = 0;
        L->pm = 1 << 24;                             /* EXP: the sweep from its top */
        L->mph = 0;
        if (!b[VK_LATK] || (b[VK_LTYPE] >> 5) == VE_MULT)
            L->env = 1 << 24;                        /* no attack (MULT: struck) */
    }
    v->s[0] = (int32_t)k;
    v->env_out = v->vel * 258;                       /* the hit starts at its level (no ramp from 0) */
    v->env = 1 << 24;                                /* the ADSR held at full (drum_note_on says why) */
    chok = K[VK_PART(k) + VK_PCHOK] & 3u;
    if (chok)
        for (j = 0; j < VD_NP; j++)
            if (j != k && (K[VK_PART(j) + VK_PCHOK] & 3u) == chok && (M->part[j].l[0].stage | M->part[j].l[1].stage))
                M->part[j].choke = 1;
}

/* the voice's amplitude: the part's, not the ADSR's; a voice whose part rang out, or went to another voice, ends */
static int32_t vd_amp(track_t *t, voice_t *v, int32_t adsr)
{
    const vd_part_t *P = vd_part_of(t, v);
    (void)adsr;
    if (!v->active)
        return 0;
    if (!P || !(P->l[0].stage | P->l[1].stage)) {
        v->active = v->gate = 0;
        v->stage = 0;
        v->env = 0;
        return 0;
    }
    v->env = 1 << 24;
    return 32767;
}

/* one block of a layer's envelope: from L->env to the value at the block's end (Q24) */
static void vd_env_step(vd_layer_t *L, const uint8_t *b, int choke)
{
    uint32_t eg = ((uint32_t)b[VK_LTYPE] >> 5) % 3u;
    int32_t e = L->env;
    if (choke) {
        e -= mulq16(e, ENV_EXP[3]);                  /* ~1.2 ms */
        L->stage = e < (1 << 12) ? 0 : 2;
    } else if (L->stage == 1) {
        if (eg == VE_EXP)                            /* curved: slow at first, then towards 1.25 */
            e += mulq16((5 << 22) - e, ENV_EXP[b[VK_LATK] & 127]) + 64;
        else if (eg == VE_AD)
            e += (int32_t)ENV_LIN[b[VK_LATK] & 127];
        if (e >= (1 << 24) || eg == VE_MULT) {
            e = 1 << 24;
            L->stage = 2;
            L->cnt = (uint8_t)(4u + (b[VK_LATK] & 127u) * 40u / 127u);
        }
    } else if (L->stage == 2) {
        if (eg == VE_MULT && L->strike < 2u) {       /* between the strikes: short decays */
            e -= mulq16(e, ENV_EXP[24]);
            if (!--L->cnt) {
                L->strike++;
                L->cnt = (uint8_t)(4u + (b[VK_LATK] & 127u) * 40u / 127u);
                e = 1 << 24;
            }
        } else {
            e -= mulq16(e, ENV_EXP[b[VK_LREL] & 127]);
        }
        if (e < (1 << 10)) {
            e = 0;
            L->stage = 0;
        }
    }
    L->env = e;
}

/* the noise filter's state-variable taps (analog_tap: 0 LP, 1 BP, 2 HP) */
static inline int32_t vd_noise(const tsvf_t *c, int32_t k, int32_t in, int32_t *f1, int32_t *f2, uint32_t src)
{
    return analog_tap(c, k, in, f1, f2, src == VS_LPN ? 0u : src == VS_BPN ? 1u : 2u);
}

/* a layer's block (vd_render, once a block and layer: its divides stay out of the samples): the envelope and the
 * pitch modulator one block on; the pitch (1/16 semitone) and the level ramp (Q15 per sample from g0); 0 = silent */
static __attribute__((noinline)) int vd_layer_ctl(vd_part_t *P, vd_layer_t *L, const uint8_t *b, int32_t poff,
                                                  int32_t *p16, int32_t *g0, int32_t *gd)
{
    uint32_t mod = ((uint32_t)b[VK_LTYPE] >> 3 & 3u) % 3u, lv = b[VK_LLVL] & 127u;
    int32_t e0 = L->env, md = 0, amt = b[VK_LMAMT] & 127, g;
    vd_env_step(L, b, P->choke);
    if (!lv)                                     /* silent: its envelope runs on (it ends the part) */
        return 0;
    if (mod == VM_EXP) {                         /* the sweep: MAMT's semitones (up to 48) down in MRAT's time */
        md = (int32_t)(((int64_t)(amt * 48 * 16 / 127) * L->pm) >> 24);
        L->pm -= mulq16(L->pm, ENV_EXP[b[VK_LMRAT] & 127]);
    } else {                                     /* TRI / RND at MRAT's rate x4 (0.2 .. 160 Hz), MAMT's depth (24) */
        uint32_t was = L->mph;
        L->mph += LFO_INC[b[VK_LMRAT] & 127] << 2;
        if (mod == VM_TRI) {
            md = (osc_tri(L->mph) * (amt * 24 * 16 / 127)) >> 15;
        } else {
            if (L->mph < was || L->pm > 32767 || L->pm < -32768)   /* a new value each cycle (and the first) */
                L->pm = (int32_t)noise32(&P->rng) >> 16;
            md = (L->pm * (amt * 24 * 16 / 127)) >> 15;
        }
    }
    *p16 = clamp(12 * 16 + (int32_t)(b[VK_LPIT] & 127u) * 108 * 16 / 127 + poff + md, 0, 2047);
    g = (int32_t)(lv * lv * 2u);                 /* LVL, square law (Q15) */
    *gd = mulq15(g, (L->env - e0) >> 9) >> CTL_LOG2;   /* the ramp: Q15 per sample */
    *g0 = mulq15(g, e0 >> 9);
    return 1;
}

/* a layer's samples into y (added): the oscillator at p16 (its increment ramped from the last block's) or the noise
 * through the filter at p16, at the level ramp g0 + i gd */
static __attribute__((noinline)) void vd_layer_run(vd_part_t *P, vd_layer_t *L, uint32_t src, int32_t p16, int32_t g,
                                                   int32_t gd, int32_t *y, uint32_t n)
{
    uint32_t i;
    if (src <= VS_SAW) {
        uint32_t inc1 = pitch_inc((uint32_t)p16), inc = L->inc ? L->inc : inc1, ph = L->ph;
        int32_t di = ((int32_t)(inc1 - inc)) >> CTL_LOG2;
        for (i = 0; i < n; i++) {
            int32_t s = src == VS_SIN ? sine_i(ph) : osc_saw(ph, inc);
            y[i] += mulq15(s, g);
            g += gd;
            ph += inc;
            inc += (uint32_t)di;
        }
        L->ph = ph;
        L->inc = inc1;
    } else {                                     /* noise through the filter at PITCH */
        tsvf_t c;
        int32_t kd = src == VS_BPN ? 2048 : 4096;   /* damping (Q12): Q 2, Q 1 */
        tsvf_coef_k(&c, (p16 - 360) * 299 >> 4, kd);
        for (i = 0; i < n; i++) {
            int32_t s = vd_noise(&c, kd, (int32_t)noise32(&P->rng) >> 17, &L->f1, &L->f2, src);
            y[i] += mulq15(s, g);
            g += gd;
        }
    }
}

static void vd_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    vd_mem_t *M = vd_mem_of(t);
    vd_part_t *P = vd_part_of(t, v);
    const uint8_t *K = vd_kit_of(t), *B;
    const int16_t *p = t->p;
    int32_t y[CTL], poff, acc, gain, send, sh, fg, dg = 0, dk = 0;
    uint32_t i, l, live = 0, k;
    if (!P || !K || !M)
        return;
    if (n > CTL)
        n = CTL;
    k = (uint32_t)v->s[0] % VD_NP;
    B = K + VK_PART(k);
    poff = m->pitch16 - (int32_t)v->note * 16 + P->st16;          /* TUNE, bend, the track's PIT routings, part 6 */
    for (i = 0; i < n; i++)
        y[i] = 0;
    for (l = 0; l < 2u; l++) {
        vd_layer_t *L = &P->l[l];
        const uint8_t *b = B + 7u * l;
        int32_t p16, g0, gd;
        if (!L->stage)
            continue;
        if (vd_layer_ctl(P, L, b, poff, &p16, &g0, &gd))
            vd_layer_run(P, L, (b[VK_LTYPE] & 7u) % 5u, p16, g0, gd, y, n);
        live |= L->stage | (L->env != 0);
    }
    if (!live)
        return;
    sh = (B[VK_PBIT] & 127) * 12 / 127;              /* BIT: bits dropped */
    fg = 4096 + (B[VK_PFOLD] & 127) * 7 * 4096 / 127;   /* FOLD: x1 .. x8 into the folder (Q12) */
    if (B[VK_PDRV] & 127) {                          /* DRV: x1 .. x4 into the soft clip, the level kept (Q12) */
        dg = 4096 + (B[VK_PDRV] & 127) * 3 * 4096 / 127;
        dk = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * dg) >> 12));
    }
    acc = 32768 + ((p[P_E6] * v->vel * v->vel) >> 6);  /* ACC: up to +3.5 dB at full velocity (Q15) */
    gain = mulq15((B[VK_PGAIN] & 127) * (B[VK_PGAIN] & 127) * 105 >> 5, acc) >> 3;   /* (Q12: o x gain fits) */
    send = mulq15((B[VK_PSEND] & 127) * (B[VK_PSEND] & 127) * 105 >> 5, acc) >> 3;
    if (send)
        M->fed = 1;
    for (i = 0; i < n; i++) {
        int32_t s = y[i], o;
        if (sh)                                      /* (rounded: a floor would add half a step of DC) */
            s = ((s + (1 << (sh - 1))) >> sh) << sh;
        if (fg > 4096) {                             /* a triangle fold: identity inside +-1, reflected outside */
            uint32_t u = (uint32_t)(((s * fg) >> 12) + 32768) & 0x1FFFFu;
            s = u < 65536u ? (int32_t)u - 32768 : 98303 - (int32_t)u;
        }
        if (dg)
            s = (softclip((s * dg) >> 12) * dk) >> 15;
        o = voice_amp(soft_knee(s, 24000), m, i) << 1;
        out[i] += (o * gain) >> 12;
        M->in[i] += (o * send) >> 13;
    }
}

/* the resonator's loop for TYPE DECAY BODY TUNE (once when they change: the divides stay out of the samples) */
static __attribute__((noinline)) void vd_reso_coef(vd_mem_t *M, const int16_t *p)
{
    uint32_t type = (uint32_t)p[P_E0] & 1u, inc, per, t60, comp;
    int32_t note16 = (24 + clamp(p[P_E3], 0, 127) * 72 / 127) * 16, a, e;
    a = 3277 + clamp(p[P_E2], 0, 127) * 29164 / 127;          /* BODY: the low-pass, 0.1 .. 0.99 (Q15) */
    inc = pitch_inc((uint32_t)note16);
    per = (uint32_t)(((uint64_t)1 << 48) / inc);            /* the period, Q16 samples */
    if (type == 0)                                           /* TUBE: the inverted loop sounds an octave down */
        per >>= 1;
    comp = (uint32_t)(((uint64_t)(32768 - a) << 16) / (uint32_t)a);   /* the low-pass's delay, Q16 */
    per = per > comp + (4u << 16) ? per - comp : 4u << 16;
    if (per > (VD_LINE - 4u) << 16)
        per = (VD_LINE - 4u) << 16;
    /* DECAY: -60 dB in 882 x 2^(7.8 v / 127) samples (20 ms .. ~4.4 s); per pass g = 2^(-9.97 D / T60) */
    t60 = (882u * (pow2_q16(clamp(p[P_E1], 0, 127) * 1498 / 127) >> 8)) >> 8;
    e = (int32_t)(((uint64_t)(per >> 8) * 1914u) / (t60 ? t60 : 1u) >> 8);   /* 9.97 x 192 = 1914 per 1/192 oct */
    M->g = clamp((int32_t)(pow2_q16(-clamp(e, 0, 192 * 15)) >> 1), 0, 32700);
    if (type == 0)
        M->g = -M->g;
    M->a = a;
    M->dly = per;
}

/* a x b / 32768 rounded towards zero: every product in the loop (the read, the low-pass, the gain) is, so its values
 * only ever shrink once nothing feeds it (a floor, or the low-pass as lp += a (r - lp), holds a small offset against
 * the gain for ever: a limit cycle of a few units) */
static inline int32_t vd_mulz(int32_t a, int32_t b)
{
    int32_t x = a * b;
    return (x + ((x >> 31) & 32767)) >> 15;
}

static __attribute__((noinline)) void vd_reso_clear(vd_mem_t *M)
{
    uint32_t i;
    for (i = 0; i < VD_LINE; i++)
        M->line[i] = 0;
    M->lp = 0;
    M->on = 0;
    M->quiet = 0;
}

/* engine_t.post (voice.c track_render, after the voices): the resonator and DRV on the part's block; fade the
 * engine switch's (Q15: the tail goes with the voices); 1 while the resonator rings (the part is mixed although no
 * voice sounds) */
static __attribute__((noinline)) int vd_post(track_t *t, int32_t *out, uint32_t n, int32_t fade)
{
    vd_mem_t *M = vd_mem_of(t);
    const int16_t *p = t->p;
    uint32_t i, key, rp, wp, fr;
    int32_t mix, peak = 0, g, a, lp;
    if (!M)
        return 0;
    if (n > CTL)
        n = CTL;
    if (M->fed || M->on) {
        key = (uint32_t)p[P_E0] | (uint32_t)p[P_E1] << 7 | (uint32_t)p[P_E2] << 14 | (uint32_t)p[P_E3] << 21 | 1u << 31;
        if (key != M->key) {
            vd_reso_coef(M, p);
            M->key = key;
        }
        mix = mulq15(p[P_E5] * p[P_E5] * 4, fade);   /* MIX: square law, 127 x2 */
        g = M->g;
        a = M->a;
        lp = M->lp;
        wp = M->wp;
        for (i = 0; i < n; i++) {
            int32_t r, x, fb;
            rp = wp - (M->dly >> 16) - 1u;
            fr = (M->dly & 0xFFFFu) >> 1;            /* (Q15) */
            x = M->line[rp & (VD_LINE - 1u)] * (int32_t)fr + M->line[(rp + 1u) & (VD_LINE - 1u)] * (int32_t)(32768u - fr);
            r = (x + ((x >> 31) & 32767)) >> 15;
            lp = vd_mulz(lp, 32768 - a) + vd_mulz(r, a);   /* each term towards zero: the loop always dies out */
            fb = vd_mulz(lp, g);
            x = soft_knee(M->in[i] + fb, 24000);
            M->line[wp & (VD_LINE - 1u)] = (int16_t)clamp(x, -32767, 32767);
            wp++;
            out[i] += mulq15(fb, mix);
            peak |= fb < 0 ? -fb : fb;
            M->in[i] = 0;
        }
        M->lp = lp;
        M->wp = wp;
        if (peak > 3 || M->fed) {
            M->on = 1;
            M->quiet = 0;
        } else if (++M->quiet > 70u) {               /* ~50 ms below ~-80 dBFS: silent, asleep */
            vd_reso_clear(M);
        }
        M->fed = 0;
    }
    if (p[P_E7] > 0) {                               /* DRV: x1 .. x4 into the soft clip on the whole output */
        int32_t dg = 4096 + p[P_E7] * 3 * 4096 / 127, dk = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * dg) >> 12));
        for (i = 0; i < n; i++)
            out[i] = (softclip(((out[i] >> 1) * dg) >> 12) * dk) >> 14;
    }
    return M->on;
}

/* {TYPE, DECAY, BODY, TUNE, KIT, MIX, ACC, DRV}; each kit suggests the BEAT pattern (GM notes) */
static const preset_t VDRUM_PRESETS[] = {
    {"INIT", {0, 60, 64, 36, 0, 0, 64, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 10), PAT(12)},
    {"ELECTRO", {1, 50, 90, 48, 1, 24, 64, 20}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 10), PAT(12)},
    {"TUBES", {0, 80, 50, 30, 2, 90, 64, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 15), PAT(12)},
    {"STRINGS", {1, 95, 100, 24, 3, 80, 64, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 20), PAT(12)},
    {"METAL", {1, 85, 120, 70, 4, 70, 70, 10}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 15), PAT(12)},
    {"DUB", {0, 105, 30, 20, 5, 70, 64, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 20, 30), PAT(12)},
    {"CRUSH", {0, 50, 64, 40, 6, 30, 80, 60}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 5), PAT(12)},
    {"WOOD", {0, 45, 70, 55, 7, 80, 64, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 15), PAT(12)},
};

static const engine_t ENG_VDRUM = {
    .name = "VDRUM",
    .page_title = {"RESO", "KIT"},
    .edit = {
        {"TYPE", F_ENUM, 0, 1, 0, N_VD_TYPE, 0},
        {"DECAY", F_PCT, 0, 127, 60, 0, 0},
        {"BODY", F_PCT, 0, 127, 64, 0, 0},
        {"TUNE", F_INT, 0, 127, 36, 0, 0},
        {"KIT", F_ENUM, 0, VD_OWN, 0, N_VD_KIT, 0},
        {"MIX", F_PCT, 0, 127, 0, 0, 0},
        {"ACC", F_PCT, 0, 127, 64, 0, 0},
        {"DRV", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = VDRUM_PRESETS,
    .npresets = NELEM(VDRUM_PRESETS),
    .note_on = vd_note_on,
    .render = vd_render,
    .amp = vd_amp,
    .knob = {P_E0, P_E1, P_E2, P_E3},
    .poly = VD_NP,
    .oneshot = 1,
    .keys = drum_keys,
    .mem = sizeof(vd_mem_t),
    .post = vd_post,
};
