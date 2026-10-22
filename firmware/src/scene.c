/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SCENES (M-Vave-OP1): 16 patterns in a project, as the OP-Z's, picked while they play (SEQ held + a white key; SEQ +
 * OCT+ / OCT- the next / the one before; SEQ + OCT+ held a copy into the next free one: ui_layer.c). A scene is what
 * the SONG chain calls a pattern (song_chain.c chain_pattern_t): the four tracks' steps, their LEN DIV SWING GATE and
 * QUANTIZE, the automation and the locks, and the two FX TRACKS. The sounds, the mixer, ARP, SCL, FX1 and the song stay
 * the project's.
 * The scene playing is where the music always was (trk[].step and p[], motion, fxt_lane): every page, tool, undo,
 * recording, save and the autosave work on it as before, and a project saved by Felucca or an older M-Vave-OP1 is its
 * scene 1. The other fifteen wait in the bank (scb, in the pool), packed one after the other in scene order
 * (scn_encode: a track's timing; a step only where it differs from an empty one (SCN_REST), and of it only the bytes
 * that do; the automation's records; the lanes' used steps). An empty scene takes no byte, and takes the timing of the
 * one it is reached from. SCN_CAP bytes in all: a scene of four 16-step parts takes ~250 .. 500 B, the densest
 * possible ~3.9 KB.
 * A pick (scene_queue): the main loop unpacks the scene into the buffer (SCN_BUF: the SONG chain's first source, free
 * while no song plays; songs and scene picks exclude each other) and then, stopped, swaps it in at once (with the
 * interrupts off); playing, the ISR swaps it where the pattern ends (SCN_END): at the end of a loop of the scene's
 * longest track that holds a note or a hit (scene_clock; none: track 1), as a SONG section ends on its clock track
 * (song_chain.c chain_tick), the sample's remainder carried so the grid does not drift. A swap first measures the scene
 * that leaves (scn_encode, no output): if it does not fit what the bank has for it (scn.room) the pick is dropped
 * (scn.full: "SCENE MEMORY FULL") and nothing is lost. Then the sequenced notes are released and the automated values
 * go back (as at a SONG section), the buffer and the tracks trade places (steps, timing, automation, lanes), and every
 * track and lane starts from its step 1. The buffer then holds the scene left: scene_poll packs it into the bank and
 * drops the bank's copy of the one now playing. Kept with the project: opstore.c (USR2's flash). */
#define SCN_N 16u
#define SCN_CAP 7880u                             /* the bank's bytes: a flash copy's two sectors less its commit page and
                                                  * the bank's header (opstore.c scb_hdr_t, 52 B), 4-aligned */
#define SCN_BUF (&chain.source[0])                /* the scene waiting, after a swap the one left (no song plays) */
enum { SCN_IDLE, SCN_END };
/* the bank: main loop only (the ISR never reads it) */
static struct {
    uint16_t len[SCN_N];                         /* each scene's bytes in data, in scene order (0: empty; the one
                                                  * playing: 0, it is in the tracks) */
    uint16_t used;                               /* .. their sum */
    uint16_t gen;                                /* bumped by every change (the SEQ layer's cells redraw) */
    uint8_t data[SCN_CAP];
} scb __attribute__((section(".pool")));
static volatile struct {
    uint8_t req;                                 /* main: SCN_END the buffer waits for the pattern's end; ISR: back 0 */
    uint8_t next;                                /* main: the scene in the buffer */
    uint8_t clk;                                 /* main: the track whose loop ends the pattern (scene_clock) */
    uint8_t cur;                                 /* the scene the tracks hold (main; the ISR at a swap) */
    uint8_t from;                                /* ISR: after a swap, the scene the buffer holds */
    uint8_t done;                                /* ISR: swapped: the buffer holds the scene left (scene_poll) */
    uint8_t full;                                /* ISR: a pick dropped: the scene left would not fit the bank */
    uint16_t room;                               /* main: the bank's bytes for the scene leaving (next's taken out) */
    uint8_t moved;                               /* main: the tracks hold another scene since the UI last looked */
} scn;
static const step_t SCN_REST = {{0, 0, 0, 0}, 0, ST_REST, 0, 0, 0, 0, 0};   /* step_clear's empty step */
typedef struct {                                 /* a scene where it is: the tracks, or a chain_pattern_t */
    const step_t *st[NTRK];
    uint8_t tm[NTRK][5];                         /* LEN DIV SWING GATE QUANTIZE */
    const motion_store_t *mo;
    const fxt_lane_t *fx;
} scn_view_t;
#define SCN_F_MO 0x10u                           /* the scene's packed flags: bit k track k has steps; its automation; */
#define SCN_F_FX 0x20u                           /* .. its FX TRACKS */

static uint8_t scn_u8(int32_t v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }
/* the main loop's guard against the audio ISR (as motion.c's: the target's interrupts; the host builds run one thread) */
static void scn_irq_off(void)
{
#if defined(FM1_IRQ_TARGET)
    fm1_irq_off();
#endif
}
static void scn_irq_on(void)
{
#if defined(FM1_IRQ_TARGET)
    fm1_irq_on();
#endif
}
static void scn_view_trk(scn_view_t *v)          /* the scene playing */
{
    uint32_t k, j;
    for (k = 0; k < NTRK; k++) {
        v->st[k] = trk[k].step;
        for (j = 0; j < 4u; j++)
            v->tm[k][j] = scn_u8(trk[k].p[P_SLEN + j]);
        v->tm[k][4] = scn_u8(trk[k].p[P_SQNT]);
    }
    v->mo = &motion;
    v->fx = fxt_lane;
}
static void scn_view_pat(scn_view_t *v, const chain_pattern_t *b)
{
    uint32_t k, j;
    for (k = 0; k < NTRK; k++) {
        v->st[k] = b->step[k];
        for (j = 0; j < 4u; j++)
            v->tm[k][j] = scn_u8(b->timing[k][j]);
        v->tm[k][4] = b->qnt[k];
    }
    v->mo = &b->motion;
    v->fx = b->fx;
}

/* scene v packed into out (cap bytes; out 0: only measured): its bytes, 0 for an empty scene (no step that differs from
 * SCN_REST, no automation, empty lanes). noinline: the ISR measures with it once a switch (scene_due) */
static __attribute__((noinline)) uint32_t scn_encode(const scn_view_t *v, uint8_t *out, uint32_t cap)
{
    uint32_t n = 1, k, i, j, l, fl = 0;
#define SCN_PUT(b) do { if (out && n < cap) out[n] = (uint8_t)(b); n++; } while (0)
    for (k = 0; k < NTRK; k++)                   /* (which tracks have steps, first: the flags) */
        for (i = 0; i < NSTEP; i++)
            if (memcmp(&v->st[k][i], &SCN_REST, sizeof SCN_REST)) {
                fl |= 1u << k;
                break;
            }
    if (v->mo->count || v->mo->on)
        fl |= SCN_F_MO;
    for (l = 0; l < FXT_N; l++)
        for (i = 0; i < NSTEP + 1u; i++)
            if (i < NSTEP ? v->fx[l].step[i] != 0u : v->fx[l].len != 0u)
                fl |= SCN_F_FX;
    if (!fl)
        return 0;
    if (out && cap)
        out[0] = (uint8_t)fl;
    for (k = 0; k < NTRK; k++) {
        for (j = 0; j < 5u; j++)
            SCN_PUT(v->tm[k][j]);
        if (!((fl >> k) & 1u))
            continue;
        {
            uint32_t m0 = 0, m1 = 0;              /* the steps that differ, a bit each (64) */
            for (i = 0; i < NSTEP; i++)
                if (memcmp(&v->st[k][i], &SCN_REST, sizeof SCN_REST)) {
                    if (i < 32u)
                        m0 |= 1u << i;
                    else
                        m1 |= 1u << (i - 32u);
                }
            for (j = 0; j < 4u; j++)
                SCN_PUT(m0 >> (8u * j));
            for (j = 0; j < 4u; j++)
                SCN_PUT(m1 >> (8u * j));
            for (i = 0; i < NSTEP; i++) {
                const uint8_t *s = (const uint8_t *)&v->st[k][i], *r = (const uint8_t *)&SCN_REST;
                uint32_t bm = 0;
                if (!((i < 32u ? m0 >> i : m1 >> (i - 32u)) & 1u))
                    continue;
                for (j = 0; j < sizeof(step_t); j++)
                    bm |= (uint32_t)(s[j] != r[j]) << j;
                SCN_PUT(bm);
                SCN_PUT(bm >> 8);
                for (j = 0; j < sizeof(step_t); j++)
                    if ((bm >> j) & 1u)
                        SCN_PUT(s[j]);
            }
        }
    }
    if (fl & SCN_F_MO) {
        SCN_PUT(v->mo->count);
        SCN_PUT(v->mo->on);
        for (i = 0; i < v->mo->count && i < MOTION_MAX; i++) {
            SCN_PUT(v->mo->event[i].place);
            SCN_PUT(v->mo->event[i].param);
            SCN_PUT((uint8_t)v->mo->event[i].value);
        }
    }
    if (fl & SCN_F_FX)
        for (l = 0; l < FXT_N; l++) {
            uint32_t m0 = 0, m1 = 0;
            SCN_PUT(v->fx[l].len);
            for (i = 0; i < NSTEP; i++)
                if (v->fx[l].step[i]) {
                    if (i < 32u)
                        m0 |= 1u << i;
                    else
                        m1 |= 1u << (i - 32u);
                }
            for (j = 0; j < 4u; j++)
                SCN_PUT(m0 >> (8u * j));
            for (j = 0; j < 4u; j++)
                SCN_PUT(m1 >> (8u * j));
            for (i = 0; i < NSTEP; i++)
                if (v->fx[l].step[i])
                    SCN_PUT(v->fx[l].step[i]);
        }
#undef SCN_PUT
    return n;
}

/* a timing value as a track parameter takes it (LEN DIV SWING GATE: TP's ranges; QUANTIZE 0 / 1) */
static int16_t scn_tm(uint32_t j, uint32_t v)
{
    const param_desc_t *d = &TP[j < 4u ? P_SLEN + j : P_SQNT];
    return (int16_t)clamp((int32_t)v, d->min, d->max);
}
/* n packed bytes (0: an empty scene, timing tm0's) -> the RAM form b. 0: not a scene this firmware packed (the bank
 * from flash: never trusted) */
static int scn_decode(const uint8_t *in, uint32_t n, chain_pattern_t *b, const uint8_t tm0[NTRK][5])
{
    uint32_t pos = 1, fl, k, i, j, l;
#define SCN_GET(x) do { if (pos >= n) return 0; (x) = in[pos++]; } while (0)
    memset(b, 0, sizeof *b);
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < NSTEP; i++)
            b->step[k][i] = SCN_REST;
    if (!n) {
        for (k = 0; k < NTRK; k++) {
            for (j = 0; j < 4u; j++)
                b->timing[k][j] = scn_tm(j, tm0[k][j]);
            b->qnt[k] = (uint8_t)scn_tm(4, tm0[k][4]);
        }
        return 1;
    }
    fl = in[0];
    if (fl & ~0x3Fu)
        return 0;
    for (k = 0; k < NTRK; k++) {
        uint32_t m[2] = {0, 0}, x;
        for (j = 0; j < 5u; j++) {
            SCN_GET(x);
            if (j < 4u)
                b->timing[k][j] = scn_tm(j, x);
            else
                b->qnt[k] = (uint8_t)scn_tm(4, x);
        }
        if (!((fl >> k) & 1u))
            continue;
        for (j = 0; j < 8u; j++) {
            SCN_GET(x);
            m[j >> 2] |= x << (8u * (j & 3u));
        }
        for (i = 0; i < NSTEP; i++) {
            uint8_t *s = (uint8_t *)&b->step[k][i];
            const step_t *st = &b->step[k][i];
            uint32_t bm, lo, hi;
            if (!((m[i >> 5] >> (i & 31u)) & 1u))
                continue;
            SCN_GET(lo);
            SCN_GET(hi);
            bm = lo | hi << 8;
            if (!bm || (bm >> sizeof(step_t)))
                return 0;                            /* (a step kept differs in some byte, of 11) */
            for (j = 0; j < sizeof(step_t); j++)
                if ((bm >> j) & 1u)
                    SCN_GET(s[j]);
            if (st->n > 4u || st->time > ST_REST || (st->flags & ~(3u | SF_RATCH | SF_NUDGE)) || st->probability > 101u)
                return 0;                            /* (what proj_pack takes: a scene played is always saved) */
        }
    }
    if (fl & SCN_F_MO) {
        uint32_t c, on;
        SCN_GET(c);
        SCN_GET(on);
        if (c > MOTION_MAX)
            return 0;
        b->motion.count = (uint8_t)c;
        b->motion.on = (uint8_t)on;
        for (i = 0; i < c; i++) {
            SCN_GET(b->motion.event[i].place);
            SCN_GET(b->motion.event[i].param);
            SCN_GET(j);
            b->motion.event[i].value = (int8_t)j;
        }
        if (!motion_valid(&b->motion))
            return 0;
    }
    if (fl & SCN_F_FX)
        for (l = 0; l < FXT_N; l++) {
            uint32_t m[2] = {0, 0}, x;
            SCN_GET(x);
            if (x > NSTEP)
                return 0;
            b->fx[l].len = (uint8_t)x;
            for (j = 0; j < 8u; j++) {
                SCN_GET(x);
                m[j >> 2] |= x << (8u * (j & 3u));
            }
            for (i = 0; i < NSTEP; i++)
                if ((m[i >> 5] >> (i & 31u)) & 1u) {
                    SCN_GET(b->fx[l].step[i]);
                    if (!b->fx[l].step[i])
                        return 0;
                }
        }
#undef SCN_GET
    return pos == n;
}

/* ------------------------------------------------------------ the bank --- */
static void scn_move(uint8_t *d, const uint8_t *s, uint32_t n)   /* (overlapping: libc.c has no memmove) */
{
    if (d < s)
        while (n--)
            *d++ = *s++;
    else
        while (n--)
            d[n] = s[n];
}
static uint32_t scb_off(uint32_t k)
{
    uint32_t i, o = 0;
    for (i = 0; i < k && i < SCN_N; i++)
        o += scb.len[i];
    return o;
}
/* scene k's place in the bank resized to n bytes (the rest moved along; the caller checked it fits): its offset */
static uint32_t scb_resize(uint32_t k, uint32_t n)
{
    uint32_t off = scb_off(k), old = scb.len[k], tail = scb.used - off - old;
    scn_move(scb.data + off + n, scb.data + off + old, tail);
    scb.len[k] = (uint16_t)n;
    scb.used = (uint16_t)(scb.used - old + n);
    scb.gen++;
    return off;
}
/* the timing of the scene playing (an empty scene reached from it takes it) */
static void scn_timing_now(uint8_t tm[NTRK][5])
{
    scn_view_t v;
    scn_view_trk(&v);
    memcpy(tm, v.tm, sizeof v.tm);
}
/* the bank emptied: one scene, the one playing (a project loaded, power-on: every scene but 1 empty) */
static void scene_reset(void)
{
    scn.req = SCN_IDLE;
    scn.done = scn.full = 0;
    memset(scb.len, 0, sizeof scb.len);
    scb.used = 0;
    scb.gen++;
    scn.cur = 0;
}
/* a bank read back (opstore.c): every scene must unpack, the one playing has no copy; 0 refused (the bank emptied) */
static int scene_bank_check(uint32_t cur)
{
    uint32_t k, off = 0;
    uint8_t tm[NTRK][5];
    memset(tm, 0, sizeof tm);
    if (cur >= SCN_N || scb.len[cur] || scb.used > SCN_CAP)
        return 0;
    for (k = 0; k < SCN_N; k++) {
        if (off + scb.len[k] > scb.used || !scn_decode(scb.data + off, scb.len[k], SCN_BUF, tm))
            return 0;
        off += scb.len[k];
    }
    return off == scb.used;
}

/* ----------------------------------------------------------- the switch --- */
/* track t holds a note or a hit (the pattern's length counts the tracks that play something) */
static int scn_plays(const track_t *t)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        if (t->step[i].n || t->step[i].hit)
            return 1;
    return 0;
}
/* the track whose loop ends the pattern: the longest (LEN x DIV) of those that play something; none: track 1 */
static uint32_t scene_clock(void)
{
    uint32_t k, best = 0, bl = 0;
    for (k = 0; k < NTRK; k++) {
        uint32_t l = (uint32_t)trk[k].p[P_SLEN] * div_samples((uint32_t)trk[k].p[P_SDIV]);
        if (scn_plays(&trk[k]) && l > bl) {
            bl = l;
            best = k;
        }
    }
    return best;
}

static void scn_swap(void *a, void *b, uint32_t n)
{
    uint8_t *x = (uint8_t *)a, *y = (uint8_t *)b, c;
    while (n--) {
        c = *x;
        *x++ = *y;
        *y++ = c;
    }
}
/* the swap (ISR at the pattern's end, or the main loop with the interrupts off, stopped): 0 done; 1 the scene leaving
 * does not fit the bank (nothing changed). carry: the samples past the end, the tracks start that far into step 1 */
static __attribute__((noinline)) int scene_swap(uint32_t carry)
{
    chain_pattern_t *b = SCN_BUF;
    scn_view_t v;
    uint32_t k;
    scn_view_trk(&v);
    if (scn_encode(&v, 0, 0) > scn.room) {
        scn.req = SCN_IDLE;
        scn.full = 1;
        return 1;
    }
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        int16_t tm[4];
        seq_release(t);
        motion_restore(t);
        scn_swap(t->step, b->step[k], sizeof t->step);
        memcpy(tm, &t->p[P_SLEN], sizeof tm);
        memcpy(&t->p[P_SLEN], b->timing[k], sizeof tm);
        memcpy(b->timing[k], tm, sizeof tm);
        tm[0] = t->p[P_SQNT];
        t->p[P_SQNT] = b->qnt[k];
        b->qnt[k] = (uint8_t)tm[0];
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFFu;                    /* (step 1 on this block, carry samples in) */
        t->rh_n = t->rskip_n = 0;
        t->rat_left = 0;
        t->nd_late = t->nd_early = ND_NONE;
        t->nd_fired = 0;
    }
    scn_swap(&motion, &b->motion, sizeof motion);
    scn_swap(fxt_lane, b->fx, sizeof fxt_lane);
    chain.carry = carry;
    fxt_section();
    scn.from = scn.cur;
    scn.cur = scn.next;
    scn.req = SCN_IDLE;
    scn.done = 1;
    return 0;
}
/* events_block, before the tracks: the pattern's end, where a scene waits (noinline: once a block only while one
 * waits; the check in events_block is one load) */
static __attribute__((noinline)) void scene_due(uint32_t n)
{
    const track_t *t = &trk[scn.clk % NTRK];
    uint32_t length;
    if (!song.playing || chain.running || t->seq_pos >= 0x7FFFFFFFu || t->seq_idx + 1u != (uint32_t)t->p[P_SLEN])
        return;
    length = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx);
    if (t->seq_pos + n < length)
        return;
    scene_swap(t->seq_pos + n - length);
}

/* main loop: a swap done -> the scene left packed into the bank (it fits: scene_swap measured it), the one now playing
 * out of it. Returns 1 when the tracks changed (the UI drops what it kept of the old scene: scene_switched) */
static int scene_done(void)
{
    scn_view_t v;
    uint32_t n, off;
    if (!scn.done)
        return 0;
    scn_view_pat(&v, SCN_BUF);
    scb_resize(scn.cur, 0);                      /* (the one playing: in the tracks now) */
    n = scn_encode(&v, 0, 0);
    if (scb.used + n > SCN_CAP)                  /* (cannot be: measured before the swap) */
        n = 0;
    off = scb_resize(scn.from, n);
    if (n)
        scn_encode(&v, scb.data + off, n);
    scn.done = 0;
    scn.moved = 1;                               /* (the UI drops what it kept of the scene left: ui_layer.c) */
    return 1;
}
/* a pick (main loop): scene k, at once stopped, at the pattern's end playing. The results: */
enum { SQ_NOW, SQ_END, SQ_SAME, SQ_CANCEL, SQ_FULL, SQ_SONG, SQ_BAD };
static uint32_t scene_queue(uint32_t k)
{
    uint8_t tm[NTRK][5];
    uint32_t off, need, was = scn.req;
    k %= SCN_N;
    if (chain.running || chain.armed)
        return SQ_SONG;
    scn.req = SCN_IDLE;                          /* (the ISR leaves the buffer alone from here) */
    scene_done();
    if (k == scn.cur)                            /* (the one playing: a pick waiting taken back) */
        return was == SCN_END ? SQ_CANCEL : SQ_SAME;
    scn_timing_now(tm);
    off = scb_off(k);
    if (!scn_decode(scb.data + off, scb.len[k], SCN_BUF, tm))
        return SQ_BAD;
    {
        scn_view_t v;
        scn_view_trk(&v);
        need = scn_encode(&v, 0, 0);
    }
    if (scb.used - scb.len[k] + need > SCN_CAP)
        return SQ_FULL;                          /* (the scene leaving would not fit: as it is now) */
    scn.room = (uint16_t)(SCN_CAP - (scb.used - scb.len[k]));
    scn.next = (uint8_t)k;
    scn.full = 0;
    scn.clk = (uint8_t)scene_clock();
    scn_irq_off();                               /* (one coherent look at the transport; main loop: interrupts on) */
    if (!song.playing && transport_req != 1u) {  /* stopped: now */
        scene_swap(0);
        scn_irq_on();
        if (scn.full) {
            scn.full = 0;
            return SQ_FULL;
        }
        scene_done();
        return SQ_NOW;
    }
    scn.req = SCN_END;
    scn_irq_on();
    return SQ_END;
}
/* the scene a pick counts from: the one waiting, else the one playing (OCT+ / OCT- step from it) */
static uint32_t scene_target(void) { return scn.req == SCN_END ? scn.next : scn.cur; }
/* scene k holds something (its cell on the layer); the one playing: the tracks */
static int scene_used(uint32_t k)
{
    scn_view_t v;
    if (k % SCN_N != scn.cur)
        return scb.len[k % SCN_N] != 0u;
    scn_view_trk(&v);
    return scn_encode(&v, 0, 0) != 0u;
}
/* SEQ + OCT+ held (main loop): the scene playing copied into the next free scene (the first empty one after it, round
 * past 16 to 1), which then plays. Nothing is swapped: the copy is the same music, so the tracks play on as they are (no
 * step starts again, nothing is heard, playing or stopped); the bank takes the scene's bytes in its own place instead,
 * and from here on the tracks are the copy (every page, tool and recording works on it). A pick waiting is taken back
 * (the copy is what plays). The results: */
enum { SC_OK, SC_EMPTY, SC_NONE, SC_FULL, SC_SONG };
/* where a copy goes (SCN_N: no scene free); *n the bytes the scene playing packs to (0: empty, nothing to copy) */
static uint32_t scene_copy_to(uint32_t *n)
{
    scn_view_t v;
    uint32_t k, f;
    scn_view_trk(&v);
    *n = scn_encode(&v, 0, 0);
    for (k = 1; k < SCN_N; k++)                  /* (the one playing has no bytes in the bank: never itself) */
        if (!scb.len[f = (scn.cur + k) % SCN_N])
            return f;
    return SCN_N;
}
static uint32_t scene_copy(void)
{
    chain_pattern_t *b = SCN_BUF;
    scn_view_t v;
    uint32_t n, f, k, off, was;
    if (chain.running || chain.armed)
        return SC_SONG;
    scn_irq_off();                               /* (a pick waiting taken back: the ISR leaves the buffer and scn.cur */
    was = scn.req;                               /* alone from here) */
    scn.req = SCN_IDLE;
    scn_irq_on();
    scene_done();                                /* (a switch the ISR made: packed away first) */
    f = scene_copy_to(&n);
    if (!n || f >= SCN_N || scb.used + n > SCN_CAP) {
        scn.req = (uint8_t)was;                  /* (refused: nothing changed, a pick waiting still waits) */
        return !n ? SC_EMPTY : f >= SCN_N ? SC_NONE : SC_FULL;
    }
    scn_irq_off();                               /* the scene into the buffer whole (live recording writes the tracks */
    for (k = 0; k < NTRK; k++) {                 /* in the ISR): a memcpy with the interrupts off, not the packing */
        memcpy(b->step[k], trk[k].step, sizeof b->step[k]);
        memcpy(b->timing[k], &trk[k].p[P_SLEN], sizeof b->timing[k]);
        b->qnt[k] = scn_u8(trk[k].p[P_SQNT]);
    }
    memcpy(&b->motion, &motion, sizeof motion);
    memcpy(b->fx, fxt_lane, sizeof b->fx);
    scn_irq_on();
    scn_view_pat(&v, b);
    n = scn_encode(&v, 0, 0);
    if (!n || scb.used + n > SCN_CAP)            /* (it changed since, by recording: the pick waiting is gone then) */
        return n ? SC_FULL : SC_EMPTY;
    off = scb_resize(scn.cur, n);
    scn_encode(&v, scb.data + off, n);
    scn.cur = (uint8_t)f;
    scn.moved = 1;                               /* (the UI drops what it kept of the scene: scene_switched_ui) */
    return SC_OK;
}
/* main loop, every pass: a swap the ISR made packed away; the pattern's clock kept to the tracks' LEN and DIV while a
 * scene waits; one waiting when the transport stopped: at once. Returns SP_SWITCHED when the tracks changed,
 * SP_FULL when a pick was dropped (the scene leaving grew past what the bank had for it) */
enum { SP_SWITCHED = 1, SP_FULL = 2 };
static uint32_t scene_poll(void)
{
    uint32_t r = scene_done() ? SP_SWITCHED : 0u;
    if (scn.req == SCN_END) {
        if (!song.playing && transport_req != 1u && !chain.running && !chain.armed) {
            scn_irq_off();
            if (scn.req == SCN_END && !song.playing)
                scene_swap(0);
            scn_irq_on();
            r |= scene_done() ? SP_SWITCHED : 0u;
        } else {
            scn.clk = (uint8_t)scene_clock();
        }
    }
    if (scn.full) {
        scn.full = 0;
        r |= SP_FULL;
    }
    return r;
}
/* before a song starts or the project is written: nothing waiting, the bank whole (a swap packed away) */
static void scene_settle(void)
{
    scn.req = SCN_IDLE;
    scene_done();
}
