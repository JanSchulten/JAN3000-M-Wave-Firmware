/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The shared voice budget (M-Vave-OP1; src/voice.c voice_space, src/audio.c shed_check) through hostsim.c.
 *   build/host/budget_test
 * 1. a full load (or none measured yet: the host, the first half): NVOICE voices of all parts, the rest taken, as
 *    Felucca's budget always was.
 * 2. room on the CPU (the last half's load under VOICE_ADMIT): one voice past NVOICE a half, up to every part's NVOICE;
 *    20 notes on four parts sound after a dozen halves, none taken.
 * 3. the load at VOICE_ADMIT or over: no voice past those sounding; new notes take voices as before.
 * 4. a chord on four parts in one half: one voice past NVOICE in it, the others taken (no burst past the CPU). */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("budget: %-97s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static void setup(void)
{
    uint32_t p;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    for (p = 0; p < NPART; p++) {
        host_preset(&trk[p], ENGI_WSHAPE, 3);       /* (a pad: long release) */
        trk[p].p[P_VOICE] = V_POLY;
        trk[p].p[P_SUS] = 127;
    }
    voice_extra = 0;
}

/* one half buffer (4 blocks), then as audio.c: the load it measured, the admission free again */
static void half(uint8_t load)
{
    static int32_t o[2 * CTL];
    uint32_t b;
    for (b = 0; b < HALF_FRAMES / CTL; b++)
        mix_block(o, CTL);
    voice_load_q8 = load;
    voice_extra = 0;
}

/* 5 notes on each of the four parts, one note a half */
static void play20(uint8_t load)
{
    uint32_t p, k;
    for (k = 0; k < 5u; k++)
        for (p = 0; p < NPART; p++) {
            trk_note_on(&trk[p], 48u + 4u * k + p, 100);
            half(load);
        }
    for (k = 0; k < 16u; k++)
        half(load);
}

int main(void)
{
    uint32_t n, kills;
    setup();
    voice_load_q8 = 255;
    kills = voice_kills;
    play20(255);
    n = voices_busy();
    printf("budget: full load: %u voices sound, %u taken\n", n, voice_kills - kills);
    check("a full load (or none measured): NVOICE voices of all parts, the rest taken or restarted (Felucca's)",
          n == NVOICE);

    setup();
    voice_load_q8 = 0;
    kills = voice_kills;
    play20(0);
    n = voices_busy();
    printf("budget: room on the CPU: %u voices sound, %u taken\n", n, voice_kills - kills);
    check("room on the CPU: 20 notes on four parts all sound (one past NVOICE a half), none taken",
          n == 20u && voice_kills == kills);

    setup();
    voice_load_q8 = 0;
    play20(VOICE_ADMIT);                                 /* (the first note's half admits; then the load is at the line) */
    n = voices_busy();
    printf("budget: the load at VOICE_ADMIT: %u voices sound\n", n);
    check("the load at VOICE_ADMIT: no voice past NVOICE (the first half's one aside)", n <= NVOICE + 1u);

    {   /* a chord of 5 notes on each part in one half */
        uint32_t p, k;
        setup();
        voice_load_q8 = 0;
        kills = voice_kills;
        for (p = 0; p < NPART; p++)
            for (k = 0; k < 5u; k++)
                trk_note_on(&trk[p], 48u + 4u * k + p, 100);
        n = voices_busy();
        printf("budget: four chords in one half: %u voices sound, %u taken\n", n, voice_kills - kills);
        check("four chords in one half: NVOICE + 1 sound (one admitted), the rest taken", n == NVOICE + 1u);
    }
    printf(bad ? "budget: FAILED\n" : "budget: all passed\n");
    return bad != 0;
}
