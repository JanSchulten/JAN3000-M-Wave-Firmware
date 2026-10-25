/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* FX TRACKS (M-Vave-OP1): two lanes of punch-in effects, after the OP-Z's FX tracks and its punch-ins recorded with REC.
 * The FM-1 has no RAM for two more synth tracks (a part's engine memory, voices and SLICER buffer: ~23 KiB each), but an
 * effect lane costs 130 bytes: so the four instrument tracks stay, and FX1 and FX2 sequence the FX layer's effects
 * (perform.c: REPEAT, REVERSE, the filters, TAPE STOP, FREEZE, OCT UP / DN, FLANGER, PHASER, the THROWs, the mutes of the
 * black keys) on the master, as if a key of the layer held them.
 *
 * A lane: up to 128 steps of a 1/16 (fxt_lane_t, core.h; 64 before), its own LEN (1..128: two lanes of different
 * lengths drift against each other and the pattern, as the OP-Z's tracks do). A step holds nothing, an effect
 * (FXT_MUTE | k for track k's mute), or FXT_TIE: the effect of the step before goes on. An effect plays from the start
 * of its step to the end of its last TIE; a step of another effect, or none, ends it (with the effect's own ramp, as a
 * key let go). The same effect on two steps in a row plays as one (it is not started again: a REPEAT keeps its loop).
 * The clock counts 1/16 steps exactly as click_tick and seq_tick count a 1/16 track's (div_samples(2), the remainder
 * kept), from step 1 with the transport: the lanes never drift from the tracks, on the internal clock or an external one.
 *
 * Recording (REC on the FX TRACKS page arms the selected lane; armed while stopped it starts PLAY): while the transport
 * runs, what the FX layer plays (the keys held with FX, FX LATCH's latched effects) goes onto the armed lanes, a step at
 * a time: the effect pressed last of those held. A press in the first half of a step goes onto that step, a later one
 * onto the next (a press and let go within the second half still makes that one step). Held on, the next steps become
 * TIEs. Nothing held writes nothing: the lane plays on as recorded (overdub), and a punch that ends inside an older take
 * gives the step after it back its old effect (fxt.old), so the rest of that take plays as it did. What the lanes play is
 * never recorded again; neither are the GLO layer's solos or the knob macros.
 * In a song each section plays the lanes saved with its slot (fxt_src; opstore.c keeps them with the project), from step 1
 * with its tracks; nothing is recorded there.
 * Runs in the audio ISR (seq.c events_block after the tracks, perform.c's keys); the UI edits the steps and LEN and sets
 * ARM (bytes: a torn read plays one step of the old value, harmless). */
#define FXT_TIE 0x80u                        /* a step: the effect of the step before goes on */
#define FXT_MUTE 0x40u                       /* .. FXT_MUTE | k: track k muted (a black key of the FX layer) */
#define FXT_START 0xFFFFFFFFu                /* fxt.pos: the transport has just started, step 1 plays first */
#define FXT_LEN(l) ((l)->len ? (uint32_t)(l)->len : 16u)
_Static_assert(PF_NFX < (int)FXT_MUTE && PF_N - PF_M1 == NTRK, "FX track codes: an effect + 1 below FXT_MUTE");

static fxt_lane_t fxt_lane[FXT_N];           /* the lanes (main: edits, ISR: plays and records) */
static struct {
    volatile uint8_t arm;                    /* main: REC armed, a bit a lane */
    uint8_t idx[FXT_N];                      /* ISR: the step playing */
    uint8_t cur[FXT_N];                      /* .. the effect it holds, PF_* + 1, 0 none */
    uint8_t rec[FXT_N];                      /* .. the effect being recorded + 1, 0 none */
    uint8_t pend[FXT_N];                     /* .. pressed late in a step: it goes onto the next, + 1 */
    uint8_t old[FXT_N];                      /* .. what the step played before it was recorded over, + 1 */
    uint16_t rec_ord[FXT_N];                 /* .. the press being recorded (fxt_ord) */
    uint16_t seq;                            /* .. the presses of the FX layer, counted */
    uint32_t pos;                            /* .. samples into the 1/16 (FXT_START: none yet) */
} fxt = {.pos = FXT_START};
static uint16_t fxt_ord[PF_N];               /* ISR: each effect's last press of the FX layer (fxt.seq) */
static const fxt_lane_t fxt_empty;           /* (a section on no slot: nothing) */

/* the lane l plays: the FX TRACKS page's; in a song the section's (song_chain.c: saved with the slot of its clock track) */
static const fxt_lane_t *fxt_src(uint32_t l)
{
    uint32_t s;
    if (!chain.running)
        return &fxt_lane[l % FXT_N];
    s = chain.lane[chain.clk % NTRK];
    return s < 4u ? &chain.source[s].fx[l % FXT_N] : &fxt_empty;
}
/* song_chain.c chain_apply: a section starts, the lanes from their step 1 with its tracks (in this block) */
static void fxt_section(void) { fxt.pos = FXT_START; }

/* a step's code -> its effect (PF_*), PF_N none (a TIE: the step before's, fxt_steps) */
static uint32_t fxt_eff(uint32_t c)
{
    if (!c || (c & FXT_TIE))
        return PF_N;
    if (c & FXT_MUTE)
        return (c & 0x3Fu) < NTRK ? PF_M1 + (c & 0x3Fu) : PF_N;
    return c - 1u < PF_NFX ? c - 1u : PF_N;  /* (a later firmware's effect: none) */
}
static uint8_t fxt_code(uint32_t e)
{
    return (uint8_t)(e < PF_NFX ? e + 1u : e < PF_N ? FXT_MUTE | (e - PF_M1) : 0u);
}

/* the effect the FX layer plays that was pressed last (keys held, FX LATCH), PF_N none */
static uint32_t fxt_live(void)
{
    uint32_t a = perf_kill ? 0u : perf_held | perf_latched, e, best = PF_N;
    for (e = 0; e < PF_N; e++)
        if (((a >> e) & 1u) && (best == PF_N || (int16_t)(fxt_ord[e] - fxt_ord[best]) > 0))
            best = e;
    return best;
}

/* perform.c perf_press: an effect of the FX layer pressed (FX LATCH: latched on). Armed and playing: onto the step it is
 * in, or the next one late in its step */
static void fxt_press(uint32_t e)
{
    uint32_t l;
    fxt_ord[e] = ++fxt.seq;
    if (!fxt.arm || !song.playing || chain.running || fxt.pos == FXT_START)
        return;                               /* (a song plays its slots' lanes: nothing recorded there) */
    for (l = 0; l < FXT_N; l++) {
        if (!((fxt.arm >> l) & 1u))
            continue;
        if (2u * fxt.pos < div_samples(2)) {
            fxt_lane[l].step[fxt.idx[l]] = fxt_code(e);
            fxt.rec[l] = (uint8_t)(e + 1u);
            fxt.rec_ord[l] = fxt_ord[e];
        } else {
            fxt.pend[l] = (uint8_t)(e + 1u);
        }
    }
}

/* armed lane l: what goes onto step i before it plays (live: the FX layer's, fxt_live). fxt.old: what the step before
 * played before it was recorded over, + 1 */
static void fxt_record(uint32_t l, uint32_t i, uint32_t live)
{
    uint8_t *s = &fxt_lane[l].step[i];
    uint32_t c = *s, before = c == FXT_TIE ? fxt.old[l] : fxt_eff(c) < PF_N ? fxt_eff(c) + 1u : 0u, e;
    fxt.old[l] = (uint8_t)before;
    if (fxt.pend[l]) {                        /* pressed late in the step before (maybe let go already) */
        e = fxt.pend[l] - 1u;
        fxt.pend[l] = 0;
        *s = fxt_code(e);
        fxt.rec[l] = (uint8_t)(e + 1u);
        fxt.rec_ord[l] = fxt_ord[e];
    } else if (live < PF_N) {
        if (fxt.rec[l] == live + 1u && fxt.rec_ord[l] == fxt_ord[live]) {
            *s = FXT_TIE;                     /* the same press held on */
        } else {
            *s = fxt_code(live);
            fxt.rec[l] = (uint8_t)(live + 1u);
            fxt.rec_ord[l] = fxt_ord[live];
        }
    } else if (fxt.rec[l]) {                  /* let go: a TIE of the older take gets its effect back */
        fxt.rec[l] = 0;
        if (c == FXT_TIE)
            *s = before ? fxt_code(before - 1u) : 0u;
    }
}

/* a 1/16 starts (first: the transport's first, step 1): each lane's next step, recorded first if armed, then played.
 * noinline: once a 1/16, out of events_block's code (the audio ISR's static cost, tests/target_budget.py) */
static __attribute__((noinline)) void fxt_steps(int first)
{
    uint32_t l, held = 0, arm = chain.running ? 0u : fxt.arm, live = arm ? fxt_live() : PF_N;
    for (l = 0; l < FXT_N; l++) {
        const fxt_lane_t *ln = fxt_src(l);
        uint32_t i = first ? 0u : (fxt.idx[l] + 1u) % FXT_LEN(ln), was = fxt.cur[l], c, e;
        fxt.idx[l] = (uint8_t)i;
        if (first)
            fxt.old[l] = fxt.rec[l] = fxt.pend[l] = 0;
        if ((arm >> l) & 1u)
            fxt_record(l, i, live);
        else
            fxt.rec[l] = fxt.pend[l] = 0;
        c = ln->step[i];
        e = c == FXT_TIE ? (was ? was - 1u : PF_N) : fxt_eff(c);
        fxt.cur[l] = (uint8_t)(e < PF_N ? e + 1u : 0u);
        if (!((arm >> l) & 1u))
            fxt.old[l] = fxt.cur[l];          /* (nothing recorded over: what played is the take) */
        if (e < PF_N) {
            held |= PF_BIT(e);
            if (fxt.cur[l] != was) {          /* a new effect: the latest pressed (a buffer effect: it plays) */
                perf_ord[e] = ++perf_seq;
                perf_fxt_now |= PF_BIT(e);
            }
        }
    }
    perf_fxt = held;
}

/* events_block, after the tracks: n samples on */
static void fxt_tick(uint32_t n)
{
    uint32_t p;
    if (!song.playing)
        return;
    p = div_samples(2);
    if (fxt.pos == FXT_START) {               /* (a song's section: from the time its tracks carry over) */
        fxt.pos = chain.carry;                /* (0 from PLAY: seq_start; a section's, a scene's) */
        fxt_steps(1);
        return;
    }
    fxt.pos += n;
    while (fxt.pos >= p) {                    /* (as seq_tick: the remainder kept) */
        fxt.pos -= p;
        fxt_steps(0);
    }
}

/* seq_start / seq_stop: from step 1 with the transport; stopped, the lanes hold nothing */
static void fxt_stop(void)
{
    uint32_t l;
    fxt.pos = FXT_START;
    for (l = 0; l < FXT_N; l++)
        fxt.cur[l] = fxt.rec[l] = fxt.pend[l] = 0;
    perf_fxt = perf_fxt_now = 0;
}

/* --------------------------------------------------------------- UI --- */
static uint32_t fxt_used(uint32_t l)          /* the steps with an effect or a TIE, in the lane's length */
{
    uint32_t i, n = 0, len = FXT_LEN(&fxt_lane[l % FXT_N]);
    for (i = 0; i < len; i++)
        n += fxt_lane[l % FXT_N].step[i] != 0u;
    return n;
}
static void fxt_clear(uint32_t l)             /* its steps (LEN stays): what it holds ends with its next step */
{
    memset(fxt_lane[l % FXT_N].step, 0, NSTEP);
}
/* the step's effect as the list of the EFFECT knob orders them: 0 nothing, 1 TIE, 2.. the effects, then the mutes */
#define FXT_CHOICES (2u + PF_N)
static uint32_t fxt_choice(uint32_t c)
{
    uint32_t e = fxt_eff(c);
    return c == FXT_TIE ? 1u : e < PF_N ? 2u + e : 0u;
}
static uint8_t fxt_of_choice(uint32_t i) { return (uint8_t)(i == 1u ? FXT_TIE : i >= 2u ? fxt_code(i - 2u) : 0u); }
/* what step i of lane l plays (PF_*, PF_N none): a TIE the effect it continues; in a song the section's lane */
static uint32_t fxt_plays(uint32_t l, uint32_t i)
{
    const fxt_lane_t *ln = fxt_src(l);
    uint32_t n = 0;
    while (ln->step[i] == FXT_TIE && n++ < NSTEP)
        i = i ? i - 1u : FXT_LEN(ln) - 1u;
    return fxt_eff(ln->step[i]);
}
