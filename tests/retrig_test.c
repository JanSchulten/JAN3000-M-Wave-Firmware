/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: retriggers and bit-crushers without a click or an offset, on the firmware's own sources (hostsim.c):
 *   ANLG2    MONO, overlapping notes: the sounding voice retriggers and keeps its two filters and its noise
 *            (eng_analog2.c .keep 0x73). (TRIO, retired, had this check first: its .keep 0x33.)
 *   LOFI     PULSE LD, C4 repeated in its release: the held sample and the low-pass kept (eng_lofi.c .keep 0x11).
 *            (PHASE, retired, had this check first: its WAVE / WAVE2 toggles, .keep 0x03.)
 *   CRUSH    the bit-crushers round to the nearest step: SAMPLE BITS, LOFI CRSH above 64, NOISE CRSH: a held note's
 *            mean as the uncrushed note's (floored, half a step of DC that followed the envelope: SAMPLE PIANO BITS
 *            127 -2346 against an RMS of 10355; NOISE WIND's low noise has a mean of its own, so the difference counts);
 *            the FX layer's CRUSH (perform.c) keeps silence silent (mid-rise: +1024 at KNOB 2 100, a thump when let go).
 * A click: the largest second difference |y - 2 y1 + y2| in the 4 samples at a retrigger, against the largest in
 * the 16 blocks before it (the note's own curvature, a period or more of its wave: its edges too). */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("retrig: %-100s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

#define NB 1600u
static int32_t buf[NB * CTL];
typedef struct { uint32_t blk; int on; uint32_t note; } ev_t;

/* part 1 with engine e, preset pi, voice mode vm, parameter pid at pval (0: none) and LOFI's WAVE at wave (-1: the
 * preset's); ev (by block) played into it; buf: its raw part buffer */
static int lofi_wave = -1;
static void run(uint32_t e, uint32_t pi, int vm, const ev_t *ev, uint32_t nev, uint32_t nb, uint32_t pid, int pval)
{
    uint32_t b, k = 0;
    track_t *t = &trk[0];
    host_tracks_init();
    host_preset(t, e, pi);
    t->p[P_VOICE] = (int16_t)vm;
    if (pid)
        t->p[pid] = (int16_t)pval;
    if (lofi_wave >= 0)
        t->p[P_E1] = (int16_t)lofi_wave;
    memset(buf, 0, sizeof buf);
    for (b = 0; b < nb; b++) {
        engine_block(t);
        while (k < nev && ev[k].blk == b) {
            if (ev[k].on)
                trk_note_on(t, ev[k].note, 100);
            else
                trk_note_off(t, ev[k].note);
            k++;
        }
        track_render(t, &buf[b * CTL], CTL);
    }
}
static double d2(uint32_t s) { return fabs((double)buf[s] - 2.0 * buf[s - 1] + buf[s - 2]); }
/* the worst ratio over the note-ons of ev that retrigger a sounding voice (the signal loud just before) */
static double worst_onset(const ev_t *ev, uint32_t nev, uint32_t *n)
{
    uint32_t k, s;
    double w = 0;
    *n = 0;
    for (k = 0; k < nev; k++) {
        uint32_t s0 = ev[k].blk * CTL;
        double before = 0, at = 0;
        int32_t pk = 0;
        if (!ev[k].on || s0 < 17u * CTL)
            continue;
        for (s = s0 - 16u * CTL; s < s0 - 2u; s++) {
            before = d2(s) > before ? d2(s) : before;
            pk = abs(buf[s]) > pk ? abs(buf[s]) : pk;
        }
        if (pk < 2000)
            continue;                                   /* (a voice that had ended: nothing to click against) */
        for (s = s0 - 1u; s < s0 + 3u; s++)
            at = d2(s) > at ? d2(s) : at;
        (*n)++;
        if (getenv("RT_DEBUG"))
            printf("    onset blk %u: before %.0f at %.0f  y %d %d | %d %d %d\n", ev[k].blk, before, at, buf[s0 - 2u],
                   buf[s0 - 1u], buf[s0], buf[s0 + 1u], buf[s0 + 2u]);
        w = before > 0 && at / before > w ? at / before : w;
    }
    return w;
}

static void test_trio(void)
{
    static ev_t ev[40];
    uint32_t n = 0, k, b, cnt;
    double w;
    char what[200];
    for (k = 0, b = 40; k < 10u; k++, b += 120u) {     /* legato over-lap: the next note before the last's off */
        ev[n++] = (ev_t){b, 1, k & 1u ? 55u : 48u};
        ev[n++] = (ev_t){b + 125u, 0, k & 1u ? 55u : 48u};
    }
    for (k = 1; k < n; k++)                             /* (by block) */
        for (b = k; b > 0 && ev[b].blk < ev[b - 1].blk; b--) {
            ev_t x = ev[b];
            ev[b] = ev[b - 1];
            ev[b - 1] = x;
        }
    run(14, 3, V_MONO, ev, n, 1400, 0, 0);              /* ANLG2 DUO BASS */
    w = worst_onset(ev, n, &cnt);
    snprintf(what, sizeof what, "ANLG2 MONO, overlapping notes: %u retriggers, the worst %.1f x the note's own curvature", cnt, w);
    check(what, cnt >= 8u && w < 2.0);
}

static void test_phase(void)
{
    static ev_t ev[40];
    uint32_t n = 0, k, b, cnt;
    double w;
    char what[200];
    for (k = 0, b = 40; k < 12u; k++, b += 97u) {      /* C4 repeated, each into the last one's release */
        ev[n++] = (ev_t){b, 1, 60};
        ev[n++] = (ev_t){b + 95u, 0, 60};
    }
    run(3, 0, V_POLY, ev, n, 1400, 0, 0);               /* LOFI PULSE LD */
    w = worst_onset(ev, n, &cnt);
    snprintf(what, sizeof what, "LOFI PULSE LD, C4 repeated in its release: %u retriggers, the worst %.1f x the note's own curvature",
             cnt, w);
    check(what, cnt >= 8u && w < 2.0);
}

/* a held C4's mean against its RMS, blocks 20 .. 1540 (15 ms .. 1.1 s) */
static void mean_rms(double *mean, double *rms)
{
    uint32_t s, a = 20u * CTL, z = 1540u * CTL;
    double m = 0, e = 0;
    for (s = a; s < z; s++) {
        m += buf[s];
        e += (double)buf[s] * buf[s];
    }
    *mean = m / (z - a);
    *rms = sqrt(e / (z - a));
}
static void test_crush(void)
{
    static const ev_t ev[2] = {{10, 1, 60}, {1550, 0, 60}};
    static const struct { const char *name; uint32_t e, pi, pid; int v; } C[4] = {
        {"SAMPLE PIANO BITS 127", 4, 0, P_E2, 127},
        {"SAMPLE PIANO BITS 100", 4, 0, P_E2, 100},
        {"LOFI 8BIT SAW CRSH 100", 3, 2, P_E3, 100},  /* (ARP 8BIT, WAVE SAW: its pulse is on the steps) */
        {"NOISE WIND CRSH 127", 11, 0, P_E7, 127},
    };
    uint32_t k, i;
    char what[200];
    for (k = 0; k < 4u; k++) {                          /* the mean against the same note uncrushed (NOISE's own
                                                         * low notes have one: the crush's alone counts) */
        double m, r, m0, r0;
        lofi_wave = C[k].e == 3u ? RW_SAW : -1;
        run(C[k].e, C[k].pi, V_POLY, ev, 2, 1590, C[k].pid, 0);
        mean_rms(&m0, &r0);
        run(C[k].e, C[k].pi, V_POLY, ev, 2, 1590, C[k].pid, C[k].v);
        mean_rms(&m, &r);
        snprintf(what, sizeof what, "%s: a held note's mean %.0f (uncrushed %.0f), RMS %.0f (floored: half a step of DC)",
                 C[k].name, m, m0, r);
        check(what, r > 500 && fabs(m - m0) < 0.03 * r);
    }
    lofi_wave = -1;
    {   /* the FX layer's CRUSH at KNOB 2 100 on silence, once its fade-in is through: silence */
        static int32_t l[CTL], r[CTL];
        int32_t pk = 0;
        memset(&pf, 0, sizeof pf);
        perf_k[1] = 100;
        for (k = 0; k < 200u; k++) {
            memset(l, 0, sizeof l);
            memset(r, 0, sizeof r);
            perf_block(l, r, CTL);
            if (k > 100u)
                for (i = 0; i < CTL; i++)
                    pk = abs(l[i]) > pk ? abs(l[i]) : abs(r[i]) > pk ? abs(r[i]) : pk;
        }
        perf_k[1] = 0;
        snprintf(what, sizeof what, "FX CRUSH (KNOB 2 100) on silence: %d out (mid-rise: +1024, a DC and a thump when let go)", pk);
        check(what, pk == 0);
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    test_trio();
    test_phase();
    test_crush();
    printf(bad ? "retrig: %d FAILED\n" : "retrig: all passed\n", bad);
    return bad != 0;
}
