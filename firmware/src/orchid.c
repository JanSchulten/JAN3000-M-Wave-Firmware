/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* PLAY MODE CHORD (M-Vave-OP1): the keyboard as the Telepathic Instruments Orchid's, after ChoralRoot FM-1 by
 * Quixotic7 (GPL-3.0, https://github.com/Quixotic7/ChoralRootFM1: its chord block, chord tables, voicing rotation
 * and LOCK latch, ported with its default settings). Included by seq.c after the ARP's modes. A track whose PLAY MODE
 * (P_AMODE, the ARP's MODE up to here) is CHORD and that is no kit (chord.c chord_kit), on the selected track:
 *   F#3 G#3 A#3 C#4 (black)  DIM MIN MAJ SUS   the chord's type while held (two held: the one pressed last)
 *   F3 G3 A3 C4 (white)      6 m7 M7 9         extensions while held; they stack (they need a type)
 *   B3                       LOCK              a tap: the chord block latches, or no more
 *   D4 .. G5 (18 keys)       roots             a single note alone, the chord with a type held or latched
 * The chord is fixed when its root goes down (ChoralRoot's Simple play style): the root, the type's tones (DIM 3 6,
 * MIN 3 7, MAJ 4 7, SUS 5 7 semitones), the extensions' (6: 9, m7: 10, M7: 11, 9: 14), then VOICE (PLAY MODE's KNOB 2,
 * -12..+12, a track's: orc_voice, kept in the project's extras, opstore.c): each step up the lowest note an octave up,
 * each step down the highest an octave down, a step that would leave 0..127 stops there. Up to ORC_MAX notes. The root
 * key is mapped as any key (kb_map: the octave, TRN, QNT); a one-voice part (MONO / LEGATO / UNISON) plays the root
 * alone, as chord.c does.
 * LOCK (ChoralRoot's "Type key: Clears", "Ext keys: Toggle"): with LOCK on a chord key let go stays latched (lit);
 * with no chord key held, a type key makes the latch that type alone (when no type is latched, it joins the latched
 * extensions), an extension key switches its extension on or off, and the only latched type pressed again alone goes
 * off when let go (the roots play single notes again; the extensions stay for the next type). LOCK off clears the
 * latch. The ISR owns orc (seq.c keyboard_block); the UI reads it a byte at a time and asks for LOCK through
 * orc_lock_req (taken at the next block) */
#define ORC_MAX 7u                               /* a chord's notes: three and four extensions */
#define ORC_KEYS 9u                              /* F3 .. C4: the chord block; D4 (key 9) the first root */
#define ORC_BLOCK 0x1FFu                         /* .. as key bits */
enum { ORC_DIM, ORC_MIN, ORC_MAJ, ORC_SUS, ORC_X6, ORC_XM7, ORC_XMA7, ORC_X9, ORC_LOCK };
#define ORC_TOP 0x0Fu                            /* the types' bits */
#define ORC_NONE 0xFFu
/* key k (F3 + k) of the block: what it is */
static const uint8_t ORC_KEY[ORC_KEYS] = {ORC_X6, ORC_DIM, ORC_XM7, ORC_MIN, ORC_XMA7, ORC_MAJ, ORC_LOCK, ORC_SUS, ORC_X9};
static const uint8_t ORC_IV[4][2] = {{3, 6}, {3, 7}, {4, 7}, {5, 7}};   /* DIM MIN MAJ SUS above the root */
static const uint8_t ORC_XIV[4] = {9, 10, 11, 14};                      /* 6 m7 M7 9 */

static volatile struct {
    uint8_t down;                                /* bit ORC_*: the chord's keys: held, and with LOCK latched */
    uint8_t phys;                                /* .. the chord keys physically held */
    uint8_t latch, kill, tpend;                  /* LOCK: latched; toggled off by the press held; the only type, again */
    uint8_t lock;                                /* LOCK on */
    uint8_t order[4];                            /* the types in the order pressed, the last at [3] (ORC_NONE: none) */
    uint8_t gen;                                 /* bumped by every change (the UI's signature) */
    uint8_t ltrk, lroot, ltype, lext, ln, lnote[ORC_MAX], lgen;   /* the last chord played (the PLAY MODE page) */
} orc = {0, 0, 0, 0, 0, 0, {ORC_NONE, ORC_NONE, ORC_NONE, ORC_NONE}, 0, 0, 0, ORC_NONE, 0, 0, {0}, 0};
static int8_t orc_voice[NTRK];                   /* main: each track's VOICE (-12..+12) */
static uint32_t kb_orc;                          /* ISR: the keys the chord block took (their release is its too) */
_Static_assert(AM_CHORD == PM_CHORD, "PLAY MODE CHORD: params.c's number for it");
_Static_assert(sizeof kb_chord[0] >= ORC_MAX, "a key's chord holds the Orchid's");
static volatile uint8_t orc_lock_req;            /* main: 1 LOCK off, 2 LOCK on; the ISR takes it (orc_poll) */

/* track t plays the Orchid's keyboard */
static int orc_on(const track_t *t) { return t->p[P_AMODE] == AM_CHORD && !chord_kit(t); }

/* the type the chord takes: the last pressed of those down (ORC_NONE: none) */
static uint32_t orc_type(void)
{
    int32_t i;
    for (i = 3; i >= 0; i--)
        if (orc.order[i] != ORC_NONE && ((orc.down >> orc.order[i]) & 1u))
            return orc.order[i];
    return ORC_NONE;
}

static void orc_lock_set(uint32_t on)            /* LOCK on / off (off: the latched keys not held go up) */
{
    orc.lock = (uint8_t)(on != 0u);
    if (!on) {
        orc.down &= (uint8_t)~(orc.latch & ~orc.phys);
        orc.latch = orc.kill = orc.tpend = 0;
    }
    orc.gen++;
}

/* the chord block's key k (< ORC_KEYS) down: ChoralRoot's cu_mod_press (Clears, Toggle). noinline: once a press */
static __attribute__((noinline)) void orc_press(uint32_t k)
{
    uint32_t m = ORC_KEY[k], bit = 1u << m, others, i, j;
    if (m == ORC_LOCK) {
        orc_lock_set(!orc.lock);
        return;
    }
    others = orc.phys & ~bit;
    orc.phys |= (uint8_t)bit;
    if (orc.tpend != bit)                        /* (any other chord key in a type key's push: no lone repeat) */
        orc.tpend = 0;
    if (orc.lock && !others && orc.latch) {      /* nothing else held, a latch: reset or toggle */
        uint32_t keep = 0xFFu;
        if ((bit & ORC_TOP) && (orc.latch & ORC_TOP) == bit)
            orc.tpend = (uint8_t)bit;            /* the only type, again: off when let go alone (orc_release) */
        else if (bit & ORC_TOP)                  /* a type: the latch that type alone (none latched: with the */
            keep = bit | ((orc.latch & ORC_TOP) ? 0u : ~ORC_TOP & 0xFFu);   /* extensions waiting there) */
        else if (orc.latch & bit) {              /* a latched extension: off */
            orc.latch &= (uint8_t)~bit;
            orc.kill |= (uint8_t)bit;
            orc.down &= (uint8_t)~bit;
            orc.gen++;
            return;
        }
        orc.down &= (uint8_t)~(orc.latch & ~keep);
        orc.latch &= (uint8_t)keep;
    }
    orc.down |= (uint8_t)bit;
    if (bit & ORC_TOP) {                         /* the type pressed last at the end, the others before it in */
        uint8_t o[3];                            /* their order (at most 3: four types, m not among them) */
        for (i = 0, j = 0; i < 4u; i++)
            if (orc.order[i] != m && orc.order[i] != ORC_NONE)
                o[j++] = orc.order[i];
        for (i = 0; i < 3u; i++)
            orc.order[i] = i + j < 3u ? ORC_NONE : o[i + j - 3u];
        orc.order[3] = (uint8_t)m;
    }
    orc.gen++;
}
static __attribute__((noinline)) void orc_release(uint32_t k)
{
    uint32_t m = ORC_KEY[k], bit = 1u << m;
    if (m == ORC_LOCK)
        return;
    orc.phys &= (uint8_t)~bit;
    orc.gen++;
    if (orc.kill & bit) {                        /* toggled off by this press */
        orc.kill &= (uint8_t)~bit;
        return;
    }
    if (orc.tpend & bit) {                       /* the only type, pushed again alone: off */
        orc.tpend &= (uint8_t)~bit;
        if (orc.lock) {
            orc.latch &= (uint8_t)~bit;
            orc.down &= (uint8_t)~bit;
            return;
        }
    }
    if (orc.lock) {                              /* LOCK: it stays down */
        orc.latch |= (uint8_t)bit;
        return;
    }
    orc.down &= (uint8_t)~bit;
}

/* the chord of type `type` (ORC_NONE: the root alone), extensions ext (bit 0 6 .. bit 3 9), voiced by v, on note
 * root -> out (ascending), its number of notes (1..ORC_MAX). one: the part plays one voice (the root alone) */
static uint32_t orc_chord(uint32_t root, uint32_t type, uint32_t ext, int32_t v, int one, uint8_t *out)
{
    uint8_t b[ORC_MAX];
    uint32_t n = 0, iv[ORC_MAX], ni = 1, i, j, x;
    iv[0] = 0;
    if (type < 4u && !one) {
        iv[ni++] = ORC_IV[type][0];
        iv[ni++] = ORC_IV[type][1];
        for (i = 0; i < 4u; i++)                 /* (an extension on a tone already there: once) */
            if ((ext >> i) & 1u) {
                for (j = 0; j < ni && iv[j] != ORC_XIV[i]; j++)
                    ;
                if (j == ni)
                    iv[ni++] = ORC_XIV[i];
            }
    }
    for (i = 0; i < ni; i++)
        if (root + iv[i] <= 127u)
            b[n++] = (uint8_t)(root + iv[i]);
    if (!n) {
        out[0] = (uint8_t)(root > 127u ? 127u : root);
        return 1;
    }
    for (i = 1; i < n; i++)                      /* ascending */
        for (j = i; j > 0 && b[j - 1u] > b[j]; j--) {
            x = b[j];
            b[j] = b[j - 1u];
            b[j - 1u] = (uint8_t)x;
        }
    for (; n > 1u && v > 0; v--) {               /* VOICE up: the lowest an octave up */
        if (b[0] + 12u > 127u)
            break;
        x = b[0] + 12u;
        for (i = 1; i < n && b[i] < x; i++)
            b[i - 1u] = b[i];
        b[i - 1u] = (uint8_t)x;
    }
    for (; n > 1u && v < 0; v++) {               /* VOICE down: the highest an octave down */
        if (b[n - 1u] < 12u)
            break;
        x = b[n - 1u] - 12u;
        for (i = n - 1u; i > 0 && b[i - 1u] > x; i--)
            b[i] = b[i - 1u];
        b[i] = (uint8_t)x;
    }
    for (i = 0; i < n; i++)
        out[i] = b[i];
    return n;
}

/* the chord a root key plays now on track t (the block's state, the track's VOICE) -> out; kept as the last chord */
static __attribute__((noinline)) uint32_t orc_build(const track_t *t, uint32_t root, uint8_t *out)
{
    uint32_t k = trk_index(t), type = orc_type(), ext = (orc.down >> ORC_X6) & 15u, one = trk_vmode(t) != V_POLY, n, i;
    n = orc_chord(root, type, ext, orc_voice[k], (int)one, out);
    if (one)                                     /* (a one-voice part: the root alone, named so) */
        type = ORC_NONE;
    orc.ltrk = (uint8_t)k;
    orc.lroot = (uint8_t)(root > 127u ? 127u : root);
    orc.ltype = (uint8_t)type;
    orc.lext = (uint8_t)(type < 4u ? ext : 0u);
    orc.ln = (uint8_t)n;
    for (i = 0; i < ORC_MAX; i++)
        orc.lnote[i] = i < n ? out[i] : 0u;
    orc.lgen++;
    return n;
}

/* each block, before the keys: LOCK asked for by the UI */
static void orc_poll(void)
{
    uint32_t r = orc_lock_req;
    if (r) {
        orc_lock_req = 0;
        orc_lock_set(r == 2u);
    }
}

/* the chord block back to nothing held or latched, LOCK off (a project loaded, power-on, panic) */
static void orc_reset(void)
{
    orc.down = orc.phys = orc.latch = orc.kill = orc.tpend = orc.lock = 0;
    orc.order[0] = orc.order[1] = orc.order[2] = orc.order[3] = ORC_NONE;
    orc.ltype = ORC_NONE;
    orc.ln = 0;
    orc.gen++;
}

/* a chord's name the Orchid's way (ChoralRoot's default naming): the root, the type (MAJ alone "M", with an extension
 * nothing; MIN "m", DIM "dim", SUS "sus") -> b; the extensions -> x ("6", "7" (on SUS "m7"), "M7", "9" in that order,
 * three "JAZZ", four "WTF"); a single note: its name, x empty */
static void orc_name(char *b, char *x, uint32_t root, uint32_t type, uint32_t ext)
{
    static const char *const XN[4] = {"6", "7", "M7", "9"};
    uint32_t i, c = 0;
    str_cpy(b, N_NOTE[root % 12u], 8);
    x[0] = 0;
    if (type >= 4u)
        return;
    for (i = 0; i < 4u; i++)
        c += (ext >> i) & 1u;
    str_cpy(b + str_len(b), type == ORC_DIM ? "dim" : type == ORC_MIN ? "m" : type == ORC_SUS ? "sus" : c ? "" : "M",
            8 - str_len(b));
    if (c >= 3u) {
        str_cpy(x, c == 3u ? "JAZZ" : "WTF", 12);
        return;
    }
    for (i = 0; i < 4u; i++)
        if ((ext >> i) & 1u) {
            if (x[0])
                str_cpy(x + str_len(x), " ", 12 - str_len(x));
            str_cpy(x + str_len(x), i == 1u && type == ORC_SUS ? "m7" : XN[i], 12 - str_len(x));
        }
}
