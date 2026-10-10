/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* A song uses the sequences of four saved projects, with the current sounds.
 * Sources are copied in the main loop before PLAY. The ISR changes only their
 * index and five timing parameters; the editable steps stay untouched.
 *
 * Lanes (1.2): a section (a row) names the slot each track plays (core.h chain_row_t). Each track reads its steps (with
 * CHANCE, RATCH, NUDGE), its automation and locks, LEN / DIV / SWING / GATE and QUANTIZE from its own slot's copy for
 * that track only; all four slots in use are copied before PLAY (chain_prepare: the four sources were there already
 * for whole rows, so lanes cost no more RAM). Sections keep one boundary for all four tracks: a section lasts `repeat`
 * loops of its clock track (chain_clock): the first track that is not silent there (track 1 when it plays: every song
 * before lanes counts as it did), or, every track silent, a 4/4 bar (CHAIN_SIL_*). At each section all four tracks
 * start together from their step 1, as rows always did.
 * A silent track ("-", CHAIN_SILENT) plays nothing in that section: at its start the track's sequenced notes are
 * released (seq_release, as at every section) and its automation goes back to the sound's own values (motion_restore);
 * no step, automation or lock of any slot plays there. Live keys and the arp still play it (the sound stays). */
/* M-Vave-OP1, the OP-Z's pattern chains: a song may LOOP (CHAIN_LOOP: from its first section again at its end, not
 * STOP) and changes while it plays. The UI edits chain_config as always and hands a copy over (chain_live: chain_next,
 * chain.req); the ISR takes it where a section ends: an edit (CHN_EDIT) goes on from the section after the one that
 * played, as edited; a new chain (CHN_NEW, typed with REC on SONG) starts from its first section at the end of the loop
 * playing now (its repeats not waited for), as the OP-Z's next pattern. All four slots are copied before PLAY
 * (chain_prepare): a slot a live edit brings in plays at once (one not saved: silent). The FX TRACKS of a section are
 * those saved with the slot its clock track plays (fxtrack.c fxt_src; a section of all four on one slot, as the chains
 * typed: that pattern's), from their step 1 with it */
enum { CHN_EDIT = 1, CHN_NEW };
typedef struct {
    step_t step[NTRK][NSTEP];
    int16_t timing[NTRK][4];
    uint8_t qnt[NTRK];                       /* each track's QUANTIZE (P_SQNT) as saved */
    motion_store_t motion;
    fxt_lane_t fx[FXT_N];                    /* M-Vave-OP1: its FX TRACKS as saved (opstore.c; none: empty) */
} chain_pattern_t;
static chain_config_t chain_config;
static chain_config_t chain_next;            /* main (M-Vave-OP1): the chain the playing song takes next (chain_live) */
static struct {
    chain_pattern_t source[4];
    chain_config_t config;
    int16_t timing[NTRK][4];                 /* the editable LEN DIV SWING GATE and QUANTIZE, back on STOP */
    uint8_t qnt[NTRK];
    volatile uint8_t armed, running, row, remaining;
    volatile uint8_t req;                    /* main: chain_next waits (CHN_EDIT, CHN_NEW), 0 none */
    uint8_t lane[NTRK];                      /* the slot each track plays in this section, CHAIN_SILENT none */
    uint8_t clk;                             /* the track whose loop counts the section (chain_clock) */
    uint8_t rec;
    uint32_t carry;
} chain;
/* a silent lane: its steps (empty: nothing plays) and its timing (a 4/4 bar of 1/16: the section's length when every
 * track is silent) */
static const step_t chain_silent[NSTEP] = {{{0}}};
#define CHAIN_SIL_LEN 16
#define CHAIN_SIL_DIV 2                      /* 1/16 */

static void seq_release(track_t *t);
static void seq_stop(void);
static uint32_t div_samples(uint32_t div);
static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx);

static chain_row_t chain_row_of(uint32_t slot, uint32_t repeat)   /* a section with all four tracks on one slot */
{
    chain_row_t r;
    memset(r.slot, (int)slot, sizeof r.slot);
    r.repeat = (uint8_t)repeat;
    return r;
}
static void chain_defaults(chain_config_t *c)
{
    uint32_t i;
    memset(c, 0, sizeof *c);
    for (i = 0; i < CHAIN_ROWS; i++)
        c->row[i].repeat = 1;
}
static int chain_valid(const chain_config_t *c)
{
    uint32_t i, k;
    if (c->count > CHAIN_ROWS)
        return 0;
    for (i = 0; i < c->count; i++) {
        for (k = 0; k < NTRK; k++)
            if (c->row[i].slot[k] > CHAIN_SILENT)
                return 0;
        if (!c->row[i].repeat || c->row[i].repeat > 16u)
            return 0;
    }
    return 1;
}
/* the chain of FUN6..FUN9 (rows {slot, repeat}) -> today's: each row a section with the four tracks on its slot. 0: not
 * a valid one (as chain_valid then) */
static int chain_from_v9(chain_config_t *c, const chain_v9_t *o)
{
    uint32_t i;
    chain_defaults(c);
    if (o->count > CHAIN_ROWS)
        return 0;
    for (i = 0; i < o->count; i++) {
        if (o->row[i].slot >= 4u || !o->row[i].repeat || o->row[i].repeat > 16u)
            return 0;
        c->row[i] = chain_row_of(o->row[i].slot, o->row[i].repeat);
    }
    c->count = o->count;
    return 1;
}
/* the track whose loops count section r's length: the first one not silent; every track silent: track 1 (a bar) */
static uint32_t chain_clock(const chain_row_t *r)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if (r->slot[k] < CHAIN_SILENT)
            return k;
    return 0;
}
/* a track parameter a playing song sets from its slots (the editor and the pages leave it alone meanwhile) */
static int chain_owns(uint32_t id) { return (id >= P_SLEN && id <= P_SGATE) || id == P_SQNT; }
static const step_t *seq_steps(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk), s;
    if (!chain.running)
        return t->step;
    s = chain.lane[k];
    return s < 4u ? chain.source[s].step[k] : chain_silent;
}
static void motion_restore(track_t *t);
static void fxt_section(void);               /* (fxtrack.c: the FX TRACKS from their step 1 with the section) */
/* a section starts (chain_start, chain_tick: once a section, not per block; not inlined: off the audio ISR's loop) */
static __attribute__((noinline)) void chain_apply(void)
{
    uint32_t i;
    const chain_row_t *r = &chain.config.row[chain.row];
    chain.clk = (uint8_t)chain_clock(r);
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        uint32_t s = r->slot[i];
        chain.lane[i] = (uint8_t)(s < 4u ? s : CHAIN_SILENT);
        seq_release(t);
        motion_restore(t);
        if (s < 4u) {
            memcpy(&t->p[P_SLEN], chain.source[s].timing[i], sizeof chain.timing[i]);
            t->p[P_SQNT] = chain.source[s].qnt[i];
        } else {
            t->p[P_SLEN] = CHAIN_SIL_LEN;
            t->p[P_SDIV] = CHAIN_SIL_DIV;
            t->p[P_SSWING] = 0;
            t->p[P_SQNT] = 1;
        }
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFFu;
        t->rh_n = t->rskip_n = 0;
    }
    fxt_section();
}
static void chain_start(void)
{
    uint32_t i;
    if (!chain.armed)
        return;
    chain.rec = song.rec;
    song.rec = 0;
    for (i = 0; i < NTRK; i++) {
        memcpy(chain.timing[i], &trk[i].p[P_SLEN], sizeof chain.timing[i]);
        chain.qnt[i] = (uint8_t)trk[i].p[P_SQNT];
    }
    chain.row = 0;
    chain.remaining = chain.config.row[0].repeat;
    chain.carry = 0;
    chain.running = 1;
    chain.armed = 0;
    chain_apply();
}
static void chain_stop(void)
{
    uint32_t i;
    chain.armed = 0;
    chain.req = 0;
    if (!chain.running)
        return;
    for (i = 0; i < NTRK; i++) {
        memcpy(&trk[i].p[P_SLEN], chain.timing[i], sizeof chain.timing[i]);
        trk[i].p[P_SQNT] = chain.qnt[i];
    }
    song.rec = chain.rec;
    chain.running = 0;
}
/* a section's loop ends (chain_tick): the next section, or the chain handed over (noinline: once a loop, out of the
 * audio ISR's events_block). 0: the song is over */
static __attribute__((noinline)) int chain_next_row(void)
{
    uint32_t req = chain.req;
    if (req == CHN_NEW || (req && chain.remaining <= 1u)) {
        chain.config = chain_next;
        chain.req = 0;
    }
    if (req == CHN_NEW) {
        chain.row = 0;
    } else if (chain.remaining > 1u) {
        chain.remaining--;
        return 2;
    } else if (chain.row + 1u < chain.config.count) {
        chain.row++;
    } else if ((chain.config.rsv[0] & CHAIN_LOOP) && chain.config.count) {
        chain.row = 0;
    } else {
        return 0;
    }
    if (chain.row >= chain.config.count)
        return 0;
    chain.remaining = chain.config.row[chain.row].repeat;
    return 1;
}
static void chain_tick(uint32_t n)
{
    const track_t *t = &trk[chain.clk % NTRK];
    uint32_t length;
    int r;
    if (!chain.running || t->seq_pos >= 0x7FFFFFFFu || t->seq_idx + 1u != (uint32_t)t->p[P_SLEN])
        return;
    length = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx);
    if (t->seq_pos + n < length)
        return;
    r = chain_next_row();
    if (r == 2)
        return;                                       /* (the same section again) */
    if (!r) {
        seq_stop();
        return;
    }
    chain.carry = t->seq_pos + n - length;
    chain_apply();
}
