/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* TAPE (M-Vave-OP1): a four-track tape in the spirit of the OP-1's, of notes instead of sound.
 * The FM-1 has 512 KiB of RAM and 1 MiB of flash: it cannot hold minutes of audio (one second of 16-bit mono is 86 KiB).
 * So the tape keeps what the keys, MIDI IN and the arp play into the armed tracks, each note at its time on the tape
 * (TAPE_PPQN ticks a quarter note of the song tempo, not quantised), and plays it back into the tracks' engines as the
 * notes come under the head: a sound changed later plays the old take with the new sound.
 * One transport: PLAY runs the tape with the sequencer, from where the head stands (as the OP-1's, STOP does not
 * rewind it); GLO + PLAY (RESTART) from the loop's start, or the tape's (tape_top). The tape tricks: LOOP (a run of
 * bars from the bar the head was in, played over and over and recorded over each pass: overdub), SPEED (varispeed:
 * tempo and pitch together, -12..+12 semitones, as a real tape), DIR REV (backwards: a note ends where it started).
 * A note recorded at a SPEED plays at its pitch at that SPEED (it is stored SPEED semitones lower): at another SPEED
 * it moves with the tape, as a recording would.
 *
 * The events: one gap buffer (tape_ev) for the four tracks, sorted by tick. [0, g0) lie before the head, the last na
 * at or after it (all zero, as the pool starts: an empty tape). The head moving forward moves each event it passes from after the gap to before it, and plays it
 * (tape_fwd); backwards the other way (tape_back). A recorded event goes in at the gap: forward before it, backwards
 * after it, so the head never plays it again on its way: O(1) in the audio ISR. Only a jump (LOOP's wrap, a seek from
 * the UI) moves a run of events, one each (tape_seek).
 * An event, 32 bits: the tick (TE_TICK, 0..TAPE_END), the note, the track, TE_OFF (a note-off) and the velocity / 2. An
 * off keeps its note-on's velocity: played backwards it is the note-on.
 * A note the tape plays is remembered as it sounds (tape.snd: the stored note, the note sounding after SPEED), so its
 * off ends that one whatever SPEED is by then; a note being recorded (tape.rec) as it was stored, so its off (the key
 * let go, STOP, the end, a jump, DIR) matches its on. A note-on is only taken while the gap has room for it and for the
 * offs of every note held: an on is never left without its off.
 * Runs in the audio ISR (seq.c: events_block, the note inputs, the transport). The UI sets ARM, LOOP, SPEED and DIR and
 * asks for seeks and erasing through req_seek / req_clear, which the ISR takes at the start of a block.
 * Kept in flash (opstore.c): one tape, as the OP-1's, written a while after it changed (gen) with the transport stopped
 * (hold: meanwhile the ISR neither moves nor changes it), read back at power-on. */
#define TAPE_PPQN 96u                        /* ticks a quarter note: 5.2 ms at 120 BPM */
#define TAPE_BAR (4u * TAPE_PPQN)
#define TAPE_BARS 128u                       /* the tape's length: 4:16 at 120 BPM */
#define TAPE_END (TAPE_BARS * TAPE_BAR)
#define TAPE_EV 4096u                        /* events (16 KiB, in the pool): ~2000 notes over the four tracks */
#define TAPE_POLY 16u                        /* notes a track holds on the tape at once (sounding; recording) */
#define TAPE_SPEED_MAX 12

#define TE_OFF (1u << 25)
#define TE_TICK(e) ((e) & 0xFFFFu)
#define TE_NOTE(e) ((e) >> 16 & 0x7Fu)
#define TE_TRK(e) ((e) >> 23 & 3u)
#define TE_VEL(e) ((e) >> 26 << 1 | 1u)     /* 1..127 */
_Static_assert(NTRK <= 4 && TAPE_END <= 0xFFFFu, "tape event: two bits of track, 16 of tick");

static uint32_t te_make(uint32_t tick, uint32_t k, uint32_t note, uint32_t vel, uint32_t off)
{
    return tick | note << 16 | k << 23 | (off ? TE_OFF : 0u) | (vel > 127u ? 127u : vel) >> 1 << 26;
}

/* SPEED: the tape's rate, Q12, at -12..+12 semitones (2^(s / 12)) */
static const uint16_t TAPE_RATE[2 * TAPE_SPEED_MAX + 1] = {
    2048, 2170, 2299, 2435, 2580, 2734, 2896, 3069, 3251, 3444, 3649, 3866, 4096,
    4340, 4598, 4871, 5161, 5468, 5793, 6137, 6502, 6889, 7298, 7732, 8192};
/* LOOP: the bars it holds, 0 = OFF */
static const uint8_t TAPE_LOOP_BARS[] = {0, 1, 2, 4, 8, 16, 32};
#define TAPE_NLOOP (sizeof TAPE_LOOP_BARS / sizeof TAPE_LOOP_BARS[0])

static uint32_t tape_ev[TAPE_EV] __attribute__((section(".pool")));
static struct {
    volatile uint16_t g0, na;                /* the gap: [0, g0) before the head, the last na at or after it (zero: empty) */
    volatile uint32_t head;                  /* the head, ticks from the tape's start (0..TAPE_END) */
    uint32_t sub;                            /* samples x TAPE_PPQN x rate (Q12 >> 12) into the tick */
    volatile uint8_t arm;                    /* ARM: bit per track, its live notes are recorded */
    volatile uint8_t rev;                    /* DIR: 1 REV (the UI's) */
    uint8_t dir;                             /* .. the direction the ISR runs in (a change ends the held notes first) */
    volatile int8_t speed;                   /* SPEED, semitones */
    volatile uint8_t loop;                   /* LOOP: TAPE_LOOP_BARS[loop] bars from loop_in, 0 OFF */
    volatile uint32_t loop_in;               /* the loop's start, a bar */
    volatile uint32_t req_seek;              /* the UI: the head to req_seek - 1, 0 none */
    volatile uint8_t req_clear;              /* the UI: bit per track, its events erased */
    volatile uint8_t full;                   /* a note was not recorded: no room (the UI says so, and clears it) */
    uint8_t sn[NTRK], rn[NTRK];              /* notes sounding from the tape, notes being recorded, per track */
    uint8_t snd[NTRK][TAPE_POLY][2];         /* sounding: the note stored, the note it sounds as */
    uint8_t rec[NTRK][TAPE_POLY][3];         /* recording: the note played, the note stored, its velocity */
    volatile uint16_t gen;                   /* + 1 each change of the events (opstore.c keeps the tape when it moved) */
    volatile uint8_t hold;                   /* main: the tape is being written to flash: the ISR leaves it as it is */
} tape;

static uint32_t tape_beat(void) { return 4u * div_samples(2); }   /* a quarter as the 1/16 tracks count it */
static uint32_t tape_note(int32_t n) { return (uint32_t)(n < 0 ? 0 : n > 127 ? 127 : n); }
static uint32_t tape_loop_out(void)
{
    uint32_t e = tape.loop_in + TAPE_LOOP_BARS[tape.loop % TAPE_NLOOP] * TAPE_BAR;
    return e < TAPE_END ? e : TAPE_END;
}
/* the head can move: the transport plays and the head is not at the end it runs to */
static int tape_rolling(void) { return song.playing && !tape.hold && (tape.dir ? tape.head > 0u : tape.head < TAPE_END); }

/* an event under the head: a note on or off (backwards, rev 1: the other way round) */
static __attribute__((noinline)) void tape_play(uint32_t e, uint32_t rev)
{
    uint32_t k = TE_TRK(e), note = TE_NOTE(e), i, n = tape.sn[k];
    uint8_t (*s)[2] = tape.snd[k];
    track_t *t = &trk[k];
    for (i = 0; i < n && s[i][0] != note; i++)
        ;
    if (i < n) {                                 /* sounding: it ends (an on again plays it anew below) */
        trk_note_off(t, s[i][1]);
        s[i][0] = s[n - 1u][0];
        s[i][1] = s[n - 1u][1];
        tape.sn[k] = (uint8_t)--n;
    }
    if (((e & TE_OFF) ? 1u : 0u) != rev)         /* an off (backwards: an on) */
        return;
    if (n == TAPE_POLY) {                        /* (full: the oldest ends) */
        trk_note_off(t, s[0][1]);
        for (i = 1; i < n; i++) {
            s[i - 1u][0] = s[i][0];
            s[i - 1u][1] = s[i][1];
        }
        n--;
    }
    s[n][0] = (uint8_t)note;
    s[n][1] = (uint8_t)tape_note((int32_t)note + tape.speed);
    tape.sn[k] = (uint8_t)(n + 1u);
    trk_note_on(t, s[n][1], TE_VEL(e));
}

/* the tracks of mask: every note the tape plays ends */
static void tape_silence(uint32_t mask)
{
    uint32_t k, i;
    for (k = 0; k < NTRK; k++) {
        if (!(mask >> k & 1u))
            continue;
        for (i = 0; i < tape.sn[k]; i++)
            trk_note_off(&trk[k], tape.snd[k][i][1]);
        tape.sn[k] = 0;
    }
}

#define TAPE_G1 (TAPE_EV - tape.na)            /* the first event after the gap */
static uint32_t tape_free(void) { return TAPE_EV - tape.g0 - tape.na; }

/* an event in at the head: forward before the gap, backwards after it (the head passes neither again) */
static void tape_put(uint32_t e)
{
    if (!tape_free()) {
        tape.full = 1;
        return;
    }
    if (tape.dir)
        tape_ev[TAPE_EV - ++tape.na] = e;
    else
        tape_ev[tape.g0++] = e;
    tape.gen++;
}

static uint32_t tape_held(void)
{
    uint32_t k, n = 0;
    for (k = 0; k < NTRK; k++)
        n += tape.rn[k];
    return n;
}

/* the notes being recorded end at the head (forward an off, backwards an on: a note runs from where the key went
 * down to where it came up, as the tape ran); keep: they stay held, for tape_rec_reopen after a jump */
static void tape_rec_close(int keep)
{
    uint32_t k, i;
    for (k = 0; k < NTRK; k++) {
        for (i = 0; i < tape.rn[k]; i++)
            tape_put(te_make(tape.head, k, tape.rec[k][i][1], tape.rec[k][i][2], !tape.dir));
        if (!keep)
            tape.rn[k] = 0;
    }
}

/* the keys still held across LOOP's wrap go on recording from the loop's start (the room for their offs too) */
static void tape_rec_reopen(void)
{
    uint32_t k, i, n = tape_held();
    if (tape_free() < 2u * n) {
        for (k = 0; k < NTRK; k++)
            tape.rn[k] = 0;
        return;
    }
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < tape.rn[k]; i++)
            tape_put(te_make(tape.head, k, tape.rec[k][i][1], tape.rec[k][i][2], tape.dir));
}

/* a live note into track t (seq.c: the keys and MIDI IN, the arp's notes), vel 0 the note let go: recorded while
 * the track is armed and the head moves */
static void tape_in(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t k = trk_index(t) % NTRK, i, n = tape.rn[k];
    uint8_t (*r)[3] = tape.rec[k];
    for (i = 0; i < n && r[i][0] != note; i++)
        ;
    if (!vel) {
        if (i == n)
            return;                              /* (not recorded) */
        tape_put(te_make(tape.head, k, r[i][1], r[i][2], !tape.dir));
        r[i][0] = r[n - 1u][0];
        r[i][1] = r[n - 1u][1];
        r[i][2] = r[n - 1u][2];
        tape.rn[k] = (uint8_t)(n - 1u);
        return;
    }
    if (!(tape.arm >> k & 1u) || !tape_rolling() || i < n || n == TAPE_POLY)
        return;
    if (tape_free() < tape_held() + 2u) {      /* its on and off, and every held note's off */
        tape.full = 1;
        return;
    }
    r[n][0] = (uint8_t)note;
    r[n][1] = (uint8_t)tape_note((int32_t)note - tape.speed);
    r[n][2] = (uint8_t)vel;
    tape.rn[k] = (uint8_t)(n + 1u);
    tape_put(te_make(tape.head, k, r[n][1], vel, tape.dir));
}

/* the head to `to` without playing what it passes (the gap moves with it) */
static void tape_seek(uint32_t to)
{
    while (tape.g0 && TE_TICK(tape_ev[tape.g0 - 1u]) >= to)
        tape_ev[TAPE_EV - ++tape.na] = tape_ev[--tape.g0];
    while (tape.na && TE_TICK(tape_ev[TAPE_G1]) < to)
        tape_ev[tape.g0++] = tape_ev[TAPE_EV - tape.na--];
    tape.head = to;
}

/* the head forward to `to`, playing the events in [head, to); backwards to `to` < head, those in [to, head) */
static void tape_fwd(uint32_t to)
{
    while (tape.na && TE_TICK(tape_ev[TAPE_G1]) < to) {
        uint32_t e = tape_ev[TAPE_EV - tape.na--];
        tape_ev[tape.g0++] = e;
        tape_play(e, 0);
    }
    tape.head = to;
}
static void tape_back(uint32_t to)
{
    while (tape.g0 && TE_TICK(tape_ev[tape.g0 - 1u]) >= to) {
        uint32_t e = tape_ev[--tape.g0];
        tape_ev[TAPE_EV - ++tape.na] = e;
        tape_play(e, 1);
    }
    tape.head = to;
}

/* a jump of the head: the recorded notes end where it leaves (held over LOOP's wrap: on again where it lands), the
 * tape's notes end */
static void tape_jump(uint32_t to, int reopen)
{
    tape_rec_close(reopen);
    tape_silence((1u << NTRK) - 1u);
    tape_seek(to);
    if (reopen)
        tape_rec_reopen();
}

/* erase the events of the tracks in mask (their notes end, held or sounding) */
static void tape_erase(uint32_t mask)
{
    uint32_t i, j, k;
    tape_silence(mask);
    for (k = 0; k < NTRK; k++)
        if (mask >> k & 1u)
            tape.rn[k] = 0;
    for (i = j = 0; i < tape.g0; i++)
        if (!(mask >> TE_TRK(tape_ev[i]) & 1u))
            tape_ev[j++] = tape_ev[i];
    tape.g0 = (uint16_t)j;
    for (i = j = TAPE_EV; i > TAPE_G1; i--)
        if (!(mask >> TE_TRK(tape_ev[i - 1u]) & 1u))
            tape_ev[--j] = tape_ev[i - 1u];
    tape.na = (uint16_t)(TAPE_EV - j);
    tape.full = 0;
    tape.gen++;
}

/* the transport (seq.c seq_start / seq_stop): from the head; on STOP the held and sounding notes end where it stands */
static void tape_start(void) { tape.sub = 0; }
static void tape_stop(void)
{
    tape_rec_close(0);
    tape_silence((1u << NTRK) - 1u);
}
/* where RESTART (GLO + PLAY) starts the tape: the loop's start, or the tape's (backwards: their ends) */
static uint32_t tape_top(void)
{
    if (tape.dir)
        return tape.loop ? tape_loop_out() : TAPE_END;
    return tape.loop ? tape.loop_in : 0u;
}

/* the UI's requests, at a block's start */
static __attribute__((noinline)) void tape_requests(void)
{
    uint32_t c = tape.req_clear, s = tape.req_seek;
    if (c) {
        tape.req_clear = 0;
        tape_erase(c);
    }
    if (s) {
        tape.req_seek = 0;
        tape_jump(s - 1u <= TAPE_END ? s - 1u : TAPE_END, 0);
    }
    if (tape.rev != tape.dir) {                  /* DIR: the held notes end the way they were recorded */
        tape_jump(tape.head, 0);
        tape.dir = tape.rev;
    }
}

/* n samples of the transport (seq.c events_block: the internal clock's, or the external clock's) */
static __attribute__((noinline)) void tape_run(uint32_t n)
{
    uint32_t adv, beat, to, l0, l1;
    tape.sub += (n < 4096u ? n : 4096u) * TAPE_PPQN * TAPE_RATE[(tape.speed + TAPE_SPEED_MAX) % 25u] >> 12;
    beat = tape_beat();
    adv = tape.sub / beat;
    if (!adv)
        return;
    tape.sub -= adv * beat;
    l0 = tape.loop_in;
    l1 = tape_loop_out();
    if (!tape.dir) {
        to = tape.head + adv;
        if (tape.loop && tape.head < l1 && to >= l1 && l0 < l1) {   /* LOOP: back to its start */
            tape_fwd(l1);
            tape_jump(l0, 1);
            to = l0 + (to - l1);
        }
        if (to >= TAPE_END) {                    /* the end: the head stops there */
            tape_fwd(TAPE_END);
            tape_jump(TAPE_END, 0);
            return;
        }
        tape_fwd(to);
        return;
    }
    to = tape.head > adv ? tape.head - adv : 0u;
    if (tape.loop && tape.head > l0 && to <= l0 && l0 < l1) {       /* LOOP backwards: on from its end */
        uint32_t rest = l0 - to;
        tape_back(l0);
        tape_jump(l1, 1);
        to = l1 - rest;
    }
    tape_back(to);
    if (!to)
        tape_jump(0, 0);                         /* the start: the head stops there */
}

static void tape_tick(uint32_t n)
{
    if (tape.hold)
        return;                                  /* (being written to flash: its requests wait) */
    if (tape.req_clear | tape.req_seek | (uint32_t)(tape.rev != tape.dir))
        tape_requests();
    if (tape_rolling())
        tape_run(n);
}

/* --------------------------------------------------------------- UI --- */
/* the main loop's side: what the TAPE page shows and sets */
static uint32_t tape_pos(void) { return tape.req_seek ? tape.req_seek - 1u : tape.head; }
static void tape_seek_ui(int32_t ticks)          /* move the head by ticks (from where it was asked to go) */
{
    int32_t to = (int32_t)tape_pos() + ticks;
    tape.req_seek = (uint32_t)(to < 0 ? 0 : to > (int32_t)TAPE_END ? (int32_t)TAPE_END : to) + 1u;
}
static void tape_loop_ui(uint32_t i)             /* LOOP: from OFF, it starts at the bar the head is in */
{
    i %= TAPE_NLOOP;
    if (i && !tape.loop)
        tape.loop_in = tape_pos() / TAPE_BAR * TAPE_BAR;
    tape.loop = (uint8_t)i;
}
static uint32_t tape_count(uint32_t k)           /* the events of track k (a snapshot: the ISR may move some) */
{
    uint32_t i, n = 0, g0 = tape.g0, g1 = TAPE_G1;
    for (i = 0; i < TAPE_EV; i++)
        if ((i < g0 || i >= g1) && TE_TRK(tape_ev[i]) == k)
            n++;
    return n;
}
