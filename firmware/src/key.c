/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* KEY (M-Vave-OP1, GLO > KEY, in place of the TAPE): a chord progression over the patterns, as the OP-Z's key track. A
 * run of LEN bars (1..16), each a root (a pitch class, or none: as written); while it is ON and the transport runs, the
 * melodic tracks' sequenced notes and their arps are moved to the bar's root inside each track's own scale (SCL ROOT,
 * SCALE): a pattern in C major under KEY Am F C G plays on degrees 6 4 1 5, A minor, F major, C major, G major, the
 * scale's chords. The keys played live are not moved (they play along as they are); kits (DRUM, the sample maps, one-
 * shots: chord_kit) never; a bar with no root, KEY OFF or the transport stopped: as written.
 * The move (key_shift): the bar's root as the scale's degree k (ROOT -> it, the nearer way round: at most half the
 * scale's degrees up or down, so the parts stay where they are written, about); each note to the degree k further, its
 * octave carried, a note between the scale's degrees (a passing note) moved with the degree below it and kept as far
 * above it. A root outside the scale: the notes moved chromatically by as many semitones (the nearer way). A track in
 * CHR (no scale of its own): its ROOT's major scale (as chord.c).
 * The bars: counted from PLAY on the sequencer's 1/16 grid (key_tick: before the tracks' steps, so the step on a bar's
 * first sixteenth plays in the new bar); seq_start sets bar 1. A note keeps the pitch it started with: the sequencer
 * keeps its sounding notes (seq_notes) as moved, the arp its note, so every off matches its on.
 * Played as the OP-Z's key track: on GLO > KEY the keys sound nothing, each is a root (its name, F3 .. G5 whatever
 * the octave). Stopped, a key sets the cursor bar's root and moves the cursor on. Running, a key moves the tracks
 * at once (key_live): it holds until a bar of the run with a root of its own starts (KEY ON), or the transport stops.
 * REC on the page (key_arm) records it too: into the bar playing, or the next one in its last quarter (a key played a
 * little early is meant for the bar coming), and KEY goes ON.
 * Kept with the project (its extras, opstore.c). The ISR reads key, the UI writes it (bytes: a root or LEN changes
 * whole).
 * Each scene has its own (M-Vave-OP1, scene.c): keyt is the scene playing's, as the tracks hold its steps; GLO > KEY
 * edits it. A scene packed into the bank keeps its KEY when it holds anything, or when its KEY was set for it (key_own:
 * edited on GLO > KEY, or the scene came with one). A scene with none of its own (empty, or packed before KEY was a
 * scene's) takes the KEY playing when it is picked, so a project of before plays as it did. A switch starts the new
 * scene's run at bar 1, as its tracks start at step 1 (key_scene). */
#define KEY_BARS 16u
#define KEY_NONE 0xFFu                          /* a bar with no root of its own */
static volatile struct {
    uint8_t on;                                 /* KEY ON */
    uint8_t len;                                /* LEN: bars 1..KEY_BARS */
    uint8_t root[KEY_BARS];                     /* each bar's root, 0..11 (C..B), or KEY_NONE */
} keyt = {0, 4, {KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
                 KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE}};
static struct {
    uint32_t pos;                               /* samples into the 1/16 */
    uint16_t step;                              /* sixteenths into the run (0 .. LEN x 16) */
    uint8_t started;                            /* the first block after PLAY: bar 1 */
} keyr;
static volatile uint8_t key_bar;                /* the bar playing (0..LEN-1): the UI's */
static volatile uint8_t key_live = KEY_NONE;    /* a root played live (GLO > KEY, running), KEY_NONE none */
static volatile uint8_t key_arm;                /* REC on GLO > KEY: the keys record into the run */
static volatile uint8_t key_own;                /* M-Vave-OP1: the scene playing has a KEY of its own (see the top) */
_Static_assert(sizeof keyt == 2u + KEY_BARS, "a scene's KEY: ON, LEN, the roots (scene.c packs these bytes)");

static void key_defaults(void)
{
    uint32_t i;
    keyt.on = 0;
    keyt.len = 4;
    for (i = 0; i < KEY_BARS; i++)
        keyt.root[i] = KEY_NONE;
}

/* the run as key_defaults leaves it (no KEY: OFF, 4 bars, no roots) */
static int key_is_default(void)
{
    uint32_t i;
    for (i = 0; i < KEY_BARS; i++)
        if (keyt.root[i] != KEY_NONE)
            return 0;
    return !keyt.on && keyt.len == 4u;
}

/* seq_start / seq_stop: from bar 1 with the transport */
static void key_start(void)
{
    keyr.pos = 0;
    keyr.step = 0;
    keyr.started = 0;
    key_bar = 0;
    key_live = KEY_NONE;
}

/* scene.c scene_swap (events_block, before key_tick, at the pattern's end; or stopped): the new scene's run from bar 1,
 * carry samples into its first 1/16 at the block's end as its tracks are into step 1 (started 0: this block's key_tick
 * adds nothing more); its bar 1 with a root of its own ends a live one */
static void key_scene(uint32_t carry)
{
    keyr.pos = carry;
    keyr.step = 0;
    keyr.started = 0;
    key_bar = 0;
    if (keyt.on && keyt.root[0] < 12u)
        key_live = KEY_NONE;
}

/* each block, before the tracks' steps (seq.c events_block): n samples further on the 1/16 grid */
static void key_tick(uint32_t n)
{
    uint32_t p, len = keyt.len >= 1u && keyt.len <= KEY_BARS ? keyt.len : 1u;
    if (!song.playing)
        return;
    if (!keyr.started) {                        /* (the first block from PLAY: its step 1 is bar 1's) */
        keyr.started = 1;
        return;
    }
    p = div_samples(2);
    keyr.pos += n;
    while (keyr.pos >= p) {
        keyr.pos -= p;
        keyr.step = (uint16_t)((keyr.step + 1u) % (len * 16u));
    }
    if (keyr.step >= len * 16u)                 /* (LEN made shorter while it ran) */
        keyr.step = 0;
    if (keyr.step / 16u != key_bar) {           /* a bar starts: a root of the run's own ends a live one */
        key_bar = (uint8_t)(keyr.step / 16u);
        if (keyt.on && keyt.root[key_bar] < 12u)
            key_live = KEY_NONE;
    }
}

/* the scale k's notes are moved in: the track's, its ROOT's major for CHR */
static uint32_t key_mask(const track_t *t)
{
    uint32_t m = scale_mask(t);
    return m == 0xFFFu ? SCALE_MASK[1] : m;
}

/* note n of track t as it plays in the bar now (see above) */
static __attribute__((noinline)) uint32_t key_shift_to(const track_t *t, uint32_t n, uint32_t r)
{
    uint32_t mask = key_mask(t), deg[12], c = 0, i, rel, rn, j;
    int32_t R = t->p[P_ROOT], k = -1, idx, oct, out, d;
    for (i = 0; i < 12u; i++)
        if ((mask >> i) & 1u)
            deg[c++] = i;
    rel = (r + 120u - (uint32_t)R) % 12u;
    for (i = 0; i < c; i++)
        if (deg[i] == rel)
            k = (int32_t)i;
    if (k < 0) {                                /* a root outside the scale: chromatically, the nearer way */
        d = rel > 6u ? (int32_t)rel - 12 : (int32_t)rel;
        return (uint32_t)clamp((int32_t)n + d, 0, 127);
    }
    if (k > (int32_t)c / 2)                     /* the nearer way round */
        k -= (int32_t)c;
    rn = ((uint32_t)((int32_t)n - R + 120)) % 12u;   /* the note above ROOT, its degree below or on it */
    for (j = c - 1u; j > 0u && deg[j] > rn; j--)
        ;
    idx = (int32_t)j + k;
    oct = idx >= 0 ? idx / (int32_t)c : -((-idx + (int32_t)c - 1) / (int32_t)c);
    idx -= oct * (int32_t)c;
    out = (int32_t)n - (int32_t)rn + 12 * oct + (int32_t)deg[idx] + (int32_t)(rn - deg[j]);
    return (uint32_t)clamp(out, 0, 127);
}
/* the root the tracks are moved to now (live, else the run's bar), KEY_NONE none */
static uint32_t key_root_now(void)
{
    if (!song.playing)
        return KEY_NONE;
    if (key_live < 12u)
        return key_live;
    return keyt.on ? keyt.root[key_bar % KEY_BARS] : KEY_NONE;
}
static uint32_t key_shift(const track_t *t, uint32_t n)
{
    uint32_t r = key_root_now();
    if (r >= 12u || chord_kit(t))
        return n;
    return key_shift_to(t, n, r);
}

/* ------------------------------------------------------------------ UI --- */
static uint8_t key_cur;                         /* GLO > KEY's cursor bar */
/* the bar's root's chord in track t's scale, "Am" "F" "Bdim" "C#" (a root outside it: its name alone), "-" none */
static void key_chord_name(const track_t *t, uint32_t r, char *b)
{
    static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint32_t mask = key_mask(t), rel, third, fifth;
    if (r >= 12u) {
        str_cpy(b, "-", 2);
        return;
    }
    str_cpy(b, NN[r], 4);
    rel = (r + 120u - (uint32_t)t->p[P_ROOT]) % 12u;
    if (!((mask >> rel) & 1u))
        return;
    third = (mask >> ((rel + 3u) % 12u) & 1u) ? 3u : (mask >> ((rel + 4u) % 12u) & 1u) ? 4u : 0u;
    fifth = (mask >> ((rel + 7u) % 12u) & 1u) ? 7u : (mask >> ((rel + 6u) % 12u) & 1u) ? 6u : 0u;
    if (third == 3u && fifth == 6u)
        str_cpy(b + str_len(b), "dim", 4);
    else if (third == 3u)
        str_cpy(b + str_len(b), "m", 2);
}
