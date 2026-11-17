/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* MIDI OUT (M-Vave-OP1), over USB: everything the FM-1 sends, on each track's channel of MENU > MIDI > MIDI IN's block
 * (seq.c trk_midi_ch: CH1-4 and SEL 1..4, CH5-8 5..8, ..), from the audio ISR into usb.c's OUT ring (midi_out_event,
 * the ISR its only producer). Included by seq.c after midi_control.c (its CC map); seq.c declares note_out and the
 * clock's start / stop before their callers.
 *   Notes (note_out): the keys (the ARP / J-6 off), the ARP's and the J-6's notes, the sequencer's steps and kit hits.
 *     Every note-on gets its note-off where the note ends inside the FM-1. A muted track (P_MUTE) sends no note-on.
 *   MENU > MIDI > T1..T4 OUT (core.h trk_out, kept with the project): BOTH, MIDI (the track's own voices silent,
 *     LOCAL OFF: voice.c trk_note_on) or FM-1 (nothing of the track's sent). A switch to FM-1, and a change of MIDI
 *     IN's block, send All Notes Off (CC123) on the channels left, so nothing they started hangs.
 *   MENU > MIDI > CLOCK OUT (ON by default; ui.c PREF_CLKOUT_OFF): with the FM-1's own clock (CLOCK INT) the MIDI
 *     clock, 24 pulses a quarter at the tempo, running while stopped too; Start with the sequencer's step 1 (the pulse
 *     after it in the same block: the first of bar 1), Stop when it stops. Never with an outside clock (no loop).
 *   MENU > MIDI > CC OUT (OFF by default; ui.c PREF_CC_OUT): a knob turned (midi_learn.c ml_knob, the UI's one place
 *     every knob edit of a track passes) and the automation and locks as they play (motion.c) send the parameter's
 *     CC: the one MIDI LEARN set for it on that track, else the standard map's (midi_control.c MIDI_CC_MAP, CC74 / 71
 *     the engine's CUT / RES), 0..127 over its range, the inverse of midi_cc_set (a CC sent comes back as the same
 *     value). Each track's last value per CC is kept (cc_sent): only a change is sent. A CC that comes in sets the
 *     parameter without sending it (no loop with a host that echoes).
 * Per block O(1) unless something is due (midi_out_block: a few compares); the rare paths noinline. The ISR is the OUT
 * ring's one producer: motion_restore, which the main loop calls too, runs there with the interrupts off (motion_guard). */
static volatile uint8_t clk_out_off;     /* the UI's MENU > MIDI > CLOCK OUT OFF (ui.c rp_apply, every frame) */
static volatile uint8_t cc_out_on;       /* .. CC OUT ON */
static uint32_t clk_out_acc;             /* samples x 24 since the last clock pulse */
static uint8_t clk_out_run, clk_out_due; /* a Start sent and no Stop yet; the pulse that goes with a Start, due */
static uint8_t tout_seen[NTRK];          /* the T OUT midi_out_block last saw (a switch to FM-1: All Notes Off) */
static uint8_t cc_was;                   /* CC OUT as the ISR last saw it (turned on: cc_sent forgotten) */
static uint8_t cc_sent[NTRK][128] __attribute__((section(".pool")));   /* each track's last value + 1 per CC, 0 none */
#define CCQ 16u                          /* the UI's knob moves to send (cc_knob), track << 8 | P_* id */
static volatile uint16_t ccq[CCQ];
static volatile uint8_t ccq_w, ccq_r;

static __attribute__((noinline)) void note_out(const track_t *t, uint32_t note, uint32_t vel)   /* (once a note: out
                                                     * of the ISR's loops, the keyboard's and the arp's) */
{
    uint32_t i = trk_index(t), mc = trk_midi_ch(i);
    if (trk_out[i] == TOUT_FM1 || note > 127u)
        return;
    if (!vel)
        midi_out_event(0x08u | (0x80u | mc) << 8 | note << 16);
    else if (!t->p[P_MUTE])
        midi_out_event(0x09u | (0x90u | mc) << 8 | note << 16 | (vel & 127u) << 24);
}
static void rt_out(uint32_t b) { midi_out_event(0x0Fu | b << 8); }   /* a realtime byte (code index 0xF: one byte) */

/* the sequencer starts (seq_start): Start, and the clock's next pulse at once (the first of step 1) */
static void clk_out_start(void)
{
    if (song.g[G_CLOCK] || clk_out_off)
        return;
    rt_out(0xFAu);
    clk_out_run = 1;
    clk_out_due = 1;
}
static void clk_out_stop(void)            /* (seq_stop; CLOCK OUT turned off while running) */
{
    if (!clk_out_run)
        return;
    rt_out(0xFCu);
    clk_out_run = 0;
}

/* All Notes Off on the channel of track i with MIDI IN on route r (its old block when ROUT changes) */
static void all_off_out(uint32_t i, uint32_t r)
{
    uint32_t mc = (r >= 2u ? (r - 1u) * 4u : 0u) + i % NTRK;
    midi_out_event(0x0Bu | (0xB0u | mc) << 8 | 123u << 16);
}
static __attribute__((noinline)) void out_route_left(uint32_t r)   /* ROUT is leaving r: its four channels' notes end */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if (trk_out[i] != TOUT_FM1)
            all_off_out(i, r);
}

/* the CC parameter id of track t sends: MIDI LEARN's for it on that track, else the standard map's unless that CC is
 * learned (it is the user's then: midi_learned); 0xFF none */
static uint32_t cc_of(const track_t *t, uint32_t id)
{
    uint32_t k = trk_index(t), i, e, c = 0xFFu;
    for (i = 0; ml_tab && i < ML_N; i++) {
        e = ml_tab[2u * i] | (uint32_t)ml_tab[2u * i + 1u] << 8;
        if ((e >> 9) && ((e >> 7) & 3u) == k && ml_id(e >> 9) == id)
            return e & 127u;
    }
    for (i = 0; i < NELEM(MIDI_CC_MAP); i++) {
        uint32_t m = MIDI_CC_MAP[i][1];
        if (m >= MCC_CUT) {
            e = MIDI_CC_ENG[eng_idx(t->eng_req)] >> (m == MCC_CUT ? 4 : 0) & 15u;
            if (!e || id != P_E0 + e - 1u)
                continue;
        } else if (m != id) {
            continue;
        }
        c = MIDI_CC_MAP[i][0];
        break;
    }
    for (i = 0; c < 128u && ml_tab && i < ML_N; i++) {
        e = ml_tab[2u * i] | (uint32_t)ml_tab[2u * i + 1u] << 8;
        if ((e >> 9) && (e & 127u) == c)
            return 0xFFu;
    }
    return c;
}
/* parameter id of track t has its value now: its CC, if CC OUT is on, the track sends and the value is new */
static __attribute__((noinline)) void cc_out_param(const track_t *t, uint32_t id)
{
    uint32_t k = trk_index(t), cc, v, range;
    const param_desc_t *d;
    if (!cc_out_on || trk_out[k] == TOUT_FM1 || id >= P_COUNT || (cc = cc_of(t, id)) > 127u)
        return;
    d = param_desc_of(eng_idx(t->eng_req), id);
    if (d->max <= d->min)
        return;
    range = (uint32_t)(d->max - d->min);
    v = ((uint32_t)(clamp(t->p[id], d->min, d->max) - d->min) * 127u + range / 2u) / range;
    if (cc_sent[k][cc] == v + 1u)
        return;
    cc_sent[k][cc] = (uint8_t)(v + 1u);
    midi_out_event(0x0Bu | (0xB0u | trk_midi_ch(k)) << 8 | cc << 16 | v << 24);
}
/* the UI (main loop): a knob set parameter id of track t (ml_knob); sent from the next block. A full queue drops it
 * (the next detent sends the value then) */
static void cc_knob(const track_t *t, uint32_t id)
{
    uint8_t w = ccq_w;
    if (!cc_out_on || id >= P_COUNT || (uint8_t)(w - ccq_r) >= CCQ)
        return;
    ccq[w % CCQ] = (uint16_t)(trk_index(t) << 8 | id);
    RING_PUBLISH();
    ccq_w = (uint8_t)(w + 1u);
}

static __attribute__((noinline)) void midi_out_rare(void)   /* T OUT, CC OUT switched; the knobs' CCs */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if (tout_seen[i] != trk_out[i]) {
            if (trk_out[i] == TOUT_FM1)
                all_off_out(i, (uint32_t)song.g[G_ROUTE]);
            tout_seen[i] = trk_out[i];
        }
    if (cc_was != cc_out_on) {
        cc_was = cc_out_on;
        memset(cc_sent, 0, sizeof cc_sent);         /* (on again: the values as they are then go out) */
    }
    while (ccq_r != ccq_w) {
        uint32_t e = ccq[ccq_r % CCQ];
        RING_PUBLISH();
        ccq_r = (uint8_t)(ccq_r + 1u);
        cc_out_param(&trk[(e >> 8) % NTRK], e & 0xFFu);
    }
}

/* every block (events_block, after the transport's requests, before the tracks step): the clock's pulses due in it.
 * noinline: inlined, its pulse loop's backward branch came to span much of events_block (+23 % on the ISR's static
 * cost, tests/target_budget.py); a call a block costs next to nothing */
static __attribute__((noinline)) void midi_out_block(uint32_t n)
{
    uint32_t beat;
    trk[0].out = trk_out[0];                        /* (the voices' LOCAL OFF: track_t's copy, kept so if an undo or a
                                                     * load writes the track over) */
    trk[1].out = trk_out[1];
    trk[2].out = trk_out[2];
    trk[3].out = trk_out[3];
    if (ccq_r != ccq_w || cc_was != cc_out_on || (tout_seen[0] ^ trk_out[0]) | (tout_seen[1] ^ trk_out[1]) |
                                                     (tout_seen[2] ^ trk_out[2]) | (tout_seen[3] ^ trk_out[3]))
        midi_out_rare();
    if (song.g[G_CLOCK] || clk_out_off) {
        clk_out_stop();
        clk_out_acc = 0;
        clk_out_due = 0;
        return;
    }
    beat = beat_samples();
    if (clk_out_due) {                              /* (Start: this pulse is step 1's) */
        clk_out_due = 0;
        clk_out_acc = 0;
        rt_out(0xF8u);
    }
    clk_out_acc += n * 24u;
    while (clk_out_acc >= beat) {
        clk_out_acc -= beat;
        rt_out(0xF8u);
    }
}
