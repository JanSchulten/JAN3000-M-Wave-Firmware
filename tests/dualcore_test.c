/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 gospodindilly-ops
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* The second core (fx.c mix_block, docs/DUALCORE_PLAN.md) on the host, from Felucca 1.6.1's tests/dualcore_test.c (after
 * Jangada's, github.com/zednaked/jangada, GPL-3.0), on M-Vave-OP1's engines and track chain. Four parts of the same
 * engine at once (every preset of every engine: the Plaits engines, FM6, VDRUM, ACID, ANALOG2 too), and mixes of
 * different engines; every part with something that keeps state of its own while it renders: the DRIVE (most parts),
 * the EQ (parts 0 and 2: the mono sum, a SPREAD part's side), the SLICER and the SHIMMER send (part 1), SPREAD (part 2),
 * the matrix with LFO S&H, which draws from the shared generator (part 0) or the part's own (part 3), ENV SYNC (part
 * 0); REV / ECHO THROW and the FX layer's mutes (a solo) for a while; and PHYS's model changed under a sounding note.
 * Each scene is rendered on one core, and again with a thread as core 1 (tests/c1_host.h) taking every split of the
 * parts in turn, the firmware's own choice (mix_split) and fixed splits. The outputs must be bit for bit the same;
 * built with -fsanitize=thread (run_tests.sh TSAN=1), nothing two parts share may be touched by both at once. Built
 * with -DDUALCORE_BREAK, the parts' own copies are put back into one (core.h C1_PI 0): the mixes must then differ, or
 * the test proves nothing (run_tests.sh runs both).
 *   build/host/dualcore_test          SCENE=text: only the scenes whose name holds it */
#include "c1_host.h"
#ifdef DUALCORE_BREAK
#define C1_PI(t) 0u                                 /* the parts' copies put back into one (the matrix, SPREAD, the
                                                     * Plaits engines' blocks): the test must see it */
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main

#define SECS 3u

typedef struct { const char *name; uint8_t eng[NPART], pre[NPART]; uint8_t morph; } scen_t;

static int32_t peak[NPART];                         /* each part's loudest (track_t.peak), for the check */

static void part_setup(track_t *t, uint32_t k, uint32_t e)
{
    t->p[P_ITYPE] = (int16_t)((k * 4u + e) % IT_N);  /* the DRIVE on most parts (its own state, ins[]) */
    t->p[P_IA] = 70;
    t->p[P_IMIX] = 90;
    t->p[P_REV] = (int16_t)(20 + 10 * k);
    t->p[P_DLY] = (int16_t)(10 * k);
    t->p[P_CHOR] = (int16_t)(30 - 5 * k);
    if (k == 0u) {
        t->p[P_ESYNC] = 1;                          /* ENV SYNC: t->esync_k */
        t->p[P_LWAVE] = 4;                          /* S&H: lfo_rand, the shared generator (parts 0..2) */
        t->p[P_LRATE] = 110;
        t->p[P_LD_FLT] = 20;
        eq_set[k][1] = (eq_band_t){56, 12, EQT_BELL, 2};   /* the EQ: a bell +6 dB (eqa, its own) */
    }
    if (k == 1u) {
        t->p[P_SLCR] = 1;                           /* the SLICER: sl[1] */
        t->p[P_SLDEPTH] = 100;
        t->p[P_SHIM] = 90;                          /* the SHIMMER send (send_sh, the sums' flag) */
    }
    if (k == 2u) {
        t->p[P_SPRD] = 100;                         /* SPREAD: t->spread_on, its buffer */
        eq_set[k][3] = (eq_band_t){80, 0, EQT_LP, 4};   /* the EQ's LP on the mono sum and the side */
    }
    if (k == 3u) {
        t->p[P_LWAVE] = 4;                          /* S&H (part 3: its own generator) into the matrix */
        t->p[P_LRATE] = 100;
        t->p[P_M1SRC] = MS_SH;
        t->p[P_M1DST] = MD_CUT;
        t->p[P_M1AMT] = 40;
        t->p[P_M2SRC] = MS_LFO;
        t->p[P_M2DST] = MD_RATE;                    /* (a per-block destination: t->p while it renders) */
        t->p[P_M2AMT] = 20;
        t->p[P_SHIM] = 40;
    }
}

static uint64_t render(const scen_t *sc, int dual, const char *mask)
{
    static const uint8_t CH[3] = {0, 7, 12};
    uint64_t h = 0xCBF29CE484222325ull;
    uint32_t f, k, i;
    int32_t o[2 * CTL];
    host_tracks_init();
    eq_power_on();
    c1h_off = !dual;
    if (mask)
        setenv("C1_MASK", mask, 1);
    for (k = 0; k < NPART; k++) {
        host_preset(&trk[k], sc->eng[k], sc->pre[k]);
        part_setup(&trk[k], k, sc->eng[k]);
    }
    eq_poll();                                      /* (the main loop's: the EQ's sets to the ISR) */
    for (f = 0; f < SECS * FS; f += CTL) {
        uint32_t ms = f / (FS / 1000u);
        for (k = 0; k < NPART; k++) {
            uint32_t on = 100u + 330u * k, off = on + 1400u, base = 48u + 5u * k;
            if (ms == on || ms == on + 700u)
                for (i = 0; i < 2u; i++)
                    trk_note_on(&trk[k], base + CH[i + (ms != on)], 90 + 10 * k);
            if (ms == off)
                for (i = 0; i < 3u; i++)
                    trk_note_off(&trk[k], base + CH[i]);
        }
        if (sc->morph && ms == 600u)                /* PHYS: MODEL changed under a sounding note (phys_render) */
            trk[0].p[P_E0] = (int16_t)((trk[0].p[P_E0] + 1) % PM_COUNT);
        perf_held = ms >= 1200u && ms < 1700u ? PF_BIT(PF_RTHR) | PF_BIT(PF_ETHR) : 0u;   /* THROW: perf_throw */
        perf_solo = ms >= 1900u && ms < 2200u ? 5u : 0u;   /* parts 1 and 3 muted a while: perf_mute(_sd) */
        mix_block(o, CTL);
        for (k = 0; k < NPART; k++)
            if (trk[k].peak > peak[k])
                peak[k] = trk[k].peak;
        for (i = 0; i < 2u * CTL; i++) {
            uint32_t u = (uint32_t)o[i];
            for (k = 0; k < 4u; k++) {
                h ^= (u >> (8u * k)) & 0xFFu;
                h *= 0x100000001B3ull;
            }
        }
    }
    return h;
}

/* in a child of its own: every render starts from the state at boot */
static uint64_t render_fork(const scen_t *sc, int dual, const char *mask, uint32_t *taken)
{
    int fd[2];
    uint64_t h = 0;
    pid_t pid;
    if (pipe(fd))
        return 0;
    pid = fork();
    if (!pid) {
        h = render(sc, dual, mask);
        if (write(fd[1], &h, sizeof h) != sizeof h || write(fd[1], peak, sizeof peak) != sizeof peak ||
            write(fd[1], &c1h_taken, sizeof c1h_taken) != sizeof c1h_taken)
            _exit(2);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &h, sizeof h) != sizeof h || read(fd[0], peak, sizeof peak) != sizeof peak ||
        read(fd[0], taken, sizeof *taken) != sizeof *taken)
        h = 0;
    close(fd[0]);
    waitpid(pid, 0, 0);
    return h;
}

/* the split while the second core's FPU is not known to be right (fx.c mix_split, C1_FPU 0): the parts that compute
 * on it (a Plaits engine or ACID, a part whose EQ runs) stay on core 0, the others are split as ever */
static uint32_t split_of(const uint8_t eng[NPART], const uint16_t cost[NPART], uint32_t eq, uint32_t fpu)
{
    uint32_t k, m;
    host_tracks_init();
    eq_power_on();
    for (k = 0; k < NPART; k++) {
        trk[k].engine = eng[k];
        part_cost[k] = cost[k];
        eqa.run[k] = (uint8_t)(eq >> k & 1u);
    }
    c1h_fpu = (uint8_t)fpu;
    m = mix_split();
    c1h_fpu = 1;
    for (k = 0; k < NPART; k++)
        part_cost[k] = 0, eqa.run[k] = 0;
    return m;
}
static uint32_t split_test(void)
{
    static const uint8_t MIX[NPART] = {ENGI_WSHAPE, 0, 0, 0}, INT[NPART] = {0, 14, 0, 14},
                         FPU[NPART] = {ENGI_WSHAPE, ENGI_HARMON, ENGI_ACID, ENGI_SWARM};
    static const uint16_t C1[NPART] = {900, 300, 300, 300}, C2[NPART] = {300, 300, 300, 300};
    uint32_t bad = 0, m;
    m = split_of(MIX, C1, 0, 1);
    if (m != 1u && m != 14u) bad++, printf("dualcore: split, FPU right: WSHAPE against three ANALOG is %u, not 1 or 14   FAIL\n", m);
    m = split_of(MIX, C1, 0, 0);
    if (m != 14u) bad++, printf("dualcore: split, FPU unknown: the WSHAPE part must stay (14), is %u   FAIL\n", m);
    m = split_of(INT, C2, 1u << 2, 0);
    if (!m || (m & 4u)) bad++, printf("dualcore: split, FPU unknown: the part with an EQ must stay, is %u   FAIL\n", m);
    m = split_of(INT, C2, 0, 0);
    if (!m) bad++, printf("dualcore: split, FPU unknown: integer parts are split as ever, is 0   FAIL\n");
    m = split_of(FPU, C1, 0, 0);
    if (m) bad++, printf("dualcore: split, FPU unknown: four FPU parts all stay (0), is %u   FAIL\n", m);
    m = split_of(FPU, C1, 0, 1);
    if (!m) bad++, printf("dualcore: split, FPU right: four FPU parts are split, is 0   FAIL\n");
    if (!bad)
        printf("dualcore: until core 1's FPU is checked, the parts that compute on it stay on core 0   ok\n");
    return bad;
}

int main(void)
{
    static scen_t sc[256];
    static char names[256][40];
    static const char *const MASKS[] = {"rot", "auto", "5", "10", "3", "12", "1"};
    uint32_t n = 0, e, i, k, g, fail = 0;
    for (e = 0; e < NENGINES; e++) {                    /* every preset: four parts at a time */
        if (!eng_ok(e))                                 /* (a retired number) */
            continue;
        for (g = 0; g * NPART < ENGINES[e]->npresets; g++) {
            snprintf(names[n], sizeof names[n], "4 x %s %u..%u", ENGINES[e]->name, g * NPART, g * NPART + NPART - 1u);
            sc[n].name = names[n];
            for (k = 0; k < NPART; k++) {
                sc[n].eng[k] = (uint8_t)e;
                sc[n].pre[k] = (uint8_t)((g * NPART + k) % ENGINES[e]->npresets);
            }
            sc[n++].morph = 0;
        }
    }
    sc[n++] = (scen_t){"PHYS FM6 ANALOG DRUM", {ENGI_PHYS, ENGI_FM6, 0, ENGI_DRUM}, {1, 4, 7, 0}, 0};
    sc[n++] = (scen_t){"GRAIN NOISE WSHAPE LOFI", {8, 11, ENGI_WSHAPE, 3}, {1, 2, 3, 4}, 0};
    sc[n++] = (scen_t){"WHEEL HRMNC WTABLE FM6", {7, ENGI_HARMON, ENGI_WTABLE, ENGI_FM6}, {1, 4, 3, 6}, 0};
    sc[n++] = (scen_t){"PRTCL SWARM ACID VDRUM", {ENGI_PRTCL, ENGI_SWARM, ENGI_ACID, ENGI_VDRUM}, {0, 1, 2, 0}, 0};
    sc[n++] = (scen_t){"PHYS morph + DRUM GRAIN ANALOG2", {ENGI_PHYS, ENGI_DRUM, 8, 14}, {2, 1, 3, 5}, 1};
    sc[n++] = (scen_t){"SAMPLE ANALOG2 PRTCL WSHAPE", {ENGI_SAMPLE, 14, ENGI_PRTCL, ENGI_WSHAPE}, {0, 2, 3, 4}, 0};
    for (i = 0; i < n; i++) {
        uint64_t ref;
        uint32_t m, bad = 0, quiet = 0, taken = 0, took = 0;
        if (getenv("SCENE") && !strstr(sc[i].name, getenv("SCENE")))
            continue;
#ifdef DUALCORE_BREAK
        if (!strncmp(sc[i].name, "4 x", 3))             /* (the mixes are enough to see it) */
            continue;
#endif
        ref = render_fork(&sc[i], 0, 0, &taken);
        for (k = 0; k < NPART; k++)
            quiet += peak[k] < 200;
        if (quiet) {                                    /* a silent part proves nothing */
            printf("dualcore: %-34s %u part(s) silent: peaks %d %d %d %d   FAIL\n", sc[i].name, quiet, peak[0],
                   peak[1], peak[2], peak[3]);
            fail++;
            continue;
        }
        for (m = 0; m < sizeof MASKS / sizeof MASKS[0]; m++) {
            uint64_t h = render_fork(&sc[i], 1, MASKS[m], &taken);
            took += taken != 0;
            if (h != ref || !h) {
                printf("dualcore: %-34s split %-4s FAIL (%016llx, one core %016llx)\n", sc[i].name, MASKS[m],
                       (unsigned long long)h, (unsigned long long)ref);
                bad++;
            }
        }
        if (took < 2u) {                                /* the thread must have rendered parts (auto may hand none) */
            printf("dualcore: %-34s the second core never took parts   FAIL\n", sc[i].name);
            bad++;
        }
        if (!bad)
            printf("dualcore: %-34s the same on two cores, every split   ok\n", sc[i].name);
        fail += bad != 0;
    }
#ifndef DUALCORE_BREAK
    if (!getenv("SCENE"))
        fail += split_test();
#endif
#ifdef DUALCORE_BREAK
    printf("dualcore: the parts' copies put back into one: %u scenes differ (must be some)   %s\n", fail,
           fail ? "ok" : "FAIL");
    return fail == 0;
#else
    printf("dualcore: %u of %u scenes differ\n", fail, n);
    return fail != 0;
#endif
}
