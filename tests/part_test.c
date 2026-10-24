/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* PRTCL engine test (M-Vave-OP1; src/eng_part.c, the Plaits particle model and diffuser of src/plaits_part.c, the
 * Plaits engines' filter of src/plaits_voice.c) through hostsim.c.
 *   build/host/part_test [DEMODIR]            (run_tests.sh: build/part_demo)
 * 1. every preset over the keyboard (C1 .. C7, 0.5 s each): it sounds (RMS), no DC (|mean| < 1 % of a voice's full
 *    scale), within the filter's knee; the corners of HARM TIMB MORPH AUX at RES 127, DRV 127 too (2 s each; the
 *    diffuser's x2 wet path may double the knee, HARM 127's sub-audio band-passes may wander: the mean against RMS).
 * 2. pitch: HARM 0 (no detune), MORPH 127 (ringing band-passes), dense: C4 rings at 261.6 Hz (+-1 %).
 * 3. a note fires the particles 0.7 ms in (Plaits' trigger, past the amplitude's first ramp): TIMB 0 sounds at once.
 * 4. TIMB is the density: more impulses (AUX 127, the raw dust) at 90 than at 30.
 * 5. the diffuser (MORPH below 64): the part rings on after its last voice (engine_t.post), then falls silent; at
 *    MORPH 64 it stops with its voices.
 * 6. a part from rest plays a note alike (its random numbers in its memory); pun_sqrt is IEEE's sqrtf.
 * 7. engine switches with the engines that keep part memory: as from boot.
 * 8. demos into DEMODIR: every preset playing C3 E3 G3 C4. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
/* the filter's knee (dsp.c soft_knee, k 16000) holds a voice under 2 x (16000 + 16384) before the amplitude and the
 * engine's 1-bit gain: a resonance peak at RES 127 may reach past 2 x VOICE_FS, as ANALOG's does, never past this */
#define KNEE_MAX (4 * (16000 + 16384))
static void check(const char *what, int ok)
{
    printf("prtcl: %-97s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

/* track 0 on PRTCL preset pi (from rest: the part's memory cleared) (e: its E values instead, when given), a flat envelope, POLY, no sends */
static void setup(uint32_t pi, const int16_t *e)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    memset(ENG_MEM(0), 0, sizeof(prt_mem_t));
    eng_mem_owner[0] = 0;
    host_tracks_init();
    host_preset(&trk[0], ENGI_PRTCL, pi);
    if (e)
        for (i = 0; i < 8u; i++)
            trk[0].p[P_E0 + i] = e[i];
    trk[0].p[P_ATK] = 0;
    trk[0].p[P_SUS] = 127;
    trk[0].p[P_REL] = 10;
    trk[0].p[P_ED_FLT] = 0;
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_DIST] = trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;
}

/* note held: n samples of the part's sum (track_render), after skip samples */
static void play(uint32_t note, uint32_t skip, int32_t *y, uint32_t n)
{
    static int32_t b[CTL];
    uint32_t i, k;
    trk_note_on(&trk[0], note, 100);
    for (i = 0; i < skip; i += CTL)
        track_render(&trk[0], b, CTL);
    for (i = 0; i < n; i += CTL) {
        memset(b, 0, sizeof b);
        track_render(&trk[0], b, CTL);
        for (k = 0; k < CTL && i + k < n; k++)
            y[i + k] = b[k];
    }
}

static void stats(const int32_t *y, uint32_t n, double *rms, double *mean, int32_t *peak)
{
    double s = 0, q = 0;
    uint32_t i;
    *peak = 0;
    for (i = 0; i < n; i++) {
        int32_t a = y[i] < 0 ? -y[i] : y[i];
        s += y[i];
        q += (double)y[i] * y[i];
        *peak = a > *peak ? a : *peak;
    }
    *mean = s / n;
    *rms = sqrt(q / n);
}

/* the period (samples) of y's strongest repetition between lo and hi samples */
static double period(const int32_t *y, uint32_t n, uint32_t lo, uint32_t hi)
{
    uint32_t lag, i, best = lo;
    double bv = -1e300;
    for (lag = lo; lag <= hi; lag++) {
        double c = 0, e0 = 0, e1 = 0;
        for (i = 0; i + lag < n; i++) {
            c += (double)y[i] * y[i + lag];
            e0 += (double)y[i] * y[i];
            e1 += (double)y[i + lag] * y[i + lag];
        }
        c /= sqrt(e0 * e1) + 1e-9;
        if (c > bv) {
            bv = c;
            best = lag;
        }
    }
    {   /* parabolic refinement around the best lag */
        double c[3];
        int32_t d;
        for (d = -1; d <= 1; d++) {
            double s = 0, e0 = 0, e1 = 0;
            uint32_t l = best + (uint32_t)d;
            for (i = 0; i + l < n; i++) {
                s += (double)y[i] * y[i + l];
                e0 += (double)y[i] * y[i];
                e1 += (double)y[i + l] * y[i + l];
            }
            c[d + 1] = s / (sqrt(e0 * e1) + 1e-9);
        }
        return best + 0.5 * (c[0] - c[2]) / (c[0] - 2 * c[1] + c[2] + 1e-12);
    }
}

static prt_mem_t *mem0(void) { return (prt_mem_t *)ENG_MEM(0); }

static void test_presets(void)
{
    static int32_t y[FS / 2u];
    uint32_t pi, k, ok_sound = 1, ok_dc = 1, ok_peak = 1;
    int32_t worst = 0;
    double dcw = 0;
    for (pi = 0; pi < ENG_PRTCL.npresets; pi++)
        for (k = 0; k < 7u; k++) {
            double rms, mean;
            int32_t peak;
            setup(pi, 0);
            play(24u + 12u * k, 0, y, FS / 2u);         /* (from the note: its first sample fires every particle) */
            stats(y, FS / 2u, &rms, &mean, &peak);
            ok_sound &= peak > 300;
            ok_dc &= fabs(mean) < 0.01 * VOICE_FS;
            ok_peak &= peak <= KNEE_MAX;
            worst = peak > worst ? peak : worst;
            dcw = fabs(mean) > dcw ? fabs(mean) : dcw;
        }
    printf("prtcl: presets C1..C7: the loudest peak %d (the knee's ceiling %d), the worst |mean| %.0f\n", worst, KNEE_MAX, dcw);
    check("every preset sounds over the keyboard (C1 .. C7, from the note)", ok_sound);
    check("  no DC (|mean| < 1 % of a voice's full scale)", ok_dc);
    check("  within the filter's knee", ok_peak);
    {   /* the corners, 2 s each (a sparse random signal at C1: its mean over less wanders) */
        static int32_t z[2u * FS];
        uint32_t c, ok = 1;
        int32_t pk = 0;
        for (c = 0; c < 16u; c++) {
            int16_t e[8];
            double rms, mean;
            int32_t peak;
            for (k = 0; k < 4u; k++)
                e[k] = (c >> k) & 1u ? 127 : 0;
            e[4] = 127; e[5] = 127; e[6] = 127; e[7] = 64;
            for (k = 0; k < 3u; k++) {
                setup(0, e);
                play(24u + 30u * k, 0, z, 2u * FS);
                stats(z, 2u * FS, &rms, &mean, &peak);
                {   /* the diffuser (MORPH 0) after the voices: its wet path's x2 may lift the part past one voice's
                     * knee; HARM 127 sends band-passes 4 octaves below the note (C1: 2 Hz), whose sub-audio wander is
                     * no offset: the mean small against the signal */
                    const int32_t lim = e[2] < 64 ? 2 * KNEE_MAX : KNEE_MAX;
                    const int good = peak <= lim && (fabs(mean) < 0.02 * VOICE_FS || fabs(mean) < 0.05 * rms);
                    ok &= good;
                    if (getenv("PRT_DEBUG") && !good)
                        printf("  corner %u note %u: peak %d mean %.0f rms %.0f\n", c, 24u + 30u * k, peak, mean, rms);
                }
                pk = peak > pk ? peak : pk;
            }
        }
        printf("prtcl: the corners of HARM TIMB MORPH AUX, RES and DRV 127: the loudest peak %d\n", pk);
        check("  the corners (HARM TIMB MORPH AUX 0 / 127, RES DRV 127, C1 .. A5): within the knee (x2 diffused), no DC", ok);
    }
}

static void test_pitch(void)
{
    static int32_t y[8192];
    static const int16_t e[8] = {0, 100, 127, 0, 127, 0, 0, 0};   /* no detune, ringing, dense; the filter open */
    double p_all, want = FS / 261.6256;
    setup(0, e);
    play(60, 2048, y, 8192);
    p_all = period(y, 8192, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    printf("prtcl: C4: the band-passes ring with a period of %.2f samples (261.6 Hz: %.2f)\n", p_all, want);
    check("pitch: HARM 0, MORPH 127: C4 rings at 261.6 Hz (+-1 %)", fabs(p_all / want - 1.0) < 0.01);
}

static void test_sync(void)
{
    static int32_t y[2u * CTL];
    static const int16_t e[8] = {0, 0, 64, 127, 127, 0, 0, 0};    /* TIMB 0: a few impulses a second; AUX: the dust */
    int32_t peak = 0;
    uint32_t i;
    setup(0, e);
    play(60, 0, y, 2u * CTL);
    for (i = 0; i < 2u * CTL; i++)
        peak = abs(y[i]) > peak ? abs(y[i]) : peak;
    check("a note fires the particles in its second block, 0.7 ms in (TIMB 0 still sounds at once)", peak > 1000);
}

static uint32_t impulses(uint32_t timb)
{
    static int32_t y[FS];
    int16_t e[8] = {0, (int16_t)timb, 64, 127, 127, 0, 0, 0};
    uint32_t i, n = 0;
    setup(0, e);
    play(60, CTL, y, FS);
    for (i = 1; i < FS; i++)
        n += abs(y[i] - y[i - 1]) > 2000;              /* (an impulse: a step in the dust) */
    return n;
}
static void test_density(void)
{
    const uint32_t a = impulses(30), b = impulses(90);
    printf("prtcl: impulses a second (AUX 127): TIMB 30 %u, TIMB 90 %u\n", a, b);
    check("TIMB is the density: many more impulses at 90 than at 30", b > 4u * a + 10u);
}

/* blocks the part sounds after its last voice (track_render's count), up to 4 s */
static uint32_t ring_blocks(uint32_t morph)
{
    static int32_t b[CTL];
    int16_t e[8] = {30, 100, (int16_t)morph, 0, 127, 0, 0, 64};
    uint32_t i, n = 0, after = 0;
    setup(0, e);
    trk[0].p[P_REL] = 0;
    trk_note_on(&trk[0], 60, 100);
    for (i = 0; i < FS / 4u / CTL; i++)
        track_render(&trk[0], b, CTL);
    trk_note_off(&trk[0], 60);
    for (i = 0; i < 4u * FS / CTL; i++) {
        uint32_t k, any = 0;
        int sounding = track_render(&trk[0], b, CTL) != 0;
        for (k = 0; k < CTL; k++)
            any |= (uint32_t)b[k];
        if (sounding && i > 10u)
            n++;
        after |= any && i > 4u * FS / CTL - 4u;
    }
    return after ? 0xFFFFFFFFu : n;
}
static void test_diffuser(void)
{
    const uint32_t d = ring_blocks(0), none = ring_blocks(64);
    printf("prtcl: blocks sounding after the voice is out: MORPH 0 %u (%.0f ms), MORPH 64 %u\n", d,
           d == 0xFFFFFFFFu ? -1.0 : d * 1000.0 * CTL / FS, none);
    check("the diffuser: the part rings on after its last voice, then falls silent (MORPH 0)", d > 4u && d < 4u * FS / CTL);
    check("  MORPH 64: no diffusion, the part stops with its voice", none == 0);
}

static void test_determinism(void)
{
    static int32_t a[FS / 10u], b[FS / 10u];
    uint32_t i, bad_sq = 0;
    setup(0, 0);
    play(64, 0, a, FS / 10u);
    setup(0, 0);
    play(64, 0, b, FS / 10u);
    check("a part from rest plays a note alike (its random numbers in its memory)", !memcmp(a, b, sizeof a));
    for (i = 0; i < 1000000u; i++) {
        union { float f; uint32_t u; } x, r, q;
        x.u = 0x00800000u + i * 2129u;
        r.f = pun_sqrt(x.f);
        q.f = sqrtf(x.f);
        bad_sq += r.u != q.u;
    }
    check("pun_sqrt: IEEE's sqrtf, bit for bit (a million normal floats)", !bad_sq);
}

/* engine switches on the part: PRTCL after GRAIN, PHYS, DRUM, FM6, VDRUM, HRMNC as from boot, they after it as from
 * boot (setup clears the region; sw_play then switches on what the part holds) */
static void sw_play(uint32_t e, int32_t *y, uint32_t n)
{
    memset(gr_rng, 0, sizeof gr_rng);
    host_preset(&trk[0], e, 0);
    trk[0].p[P_VOICE] = V_POLY;
    play(60, 0, y, n);
    trk_note_off(&trk[0], 60);
}
static void test_switch(void)
{
    static const uint32_t OTHER[] = {8 /* GRAIN */, ENGI_PHYS, ENGI_DRUM, ENGI_FM6, ENGI_VDRUM, ENGI_HARMON};
    static int32_t ref[FS / 4u], y[FS / 4u], oref[NELEM(OTHER)][FS / 4u];
    uint32_t i, ok = 1, k;
    setup(0, 0);
    sw_play(ENGI_PRTCL, ref, FS / 4u);
    for (i = 0; i < NELEM(OTHER); i++) {
        setup(0, 0);
        sw_play(OTHER[i], oref[i], FS / 4u);
    }
    for (i = 0; i < NELEM(OTHER); i++) {
        setup(0, 0);
        sw_play(OTHER[i], y, FS / 4u);
        for (k = 0; k < 8u * FS / CTL; k++)
            track_render(&trk[0], y, CTL);
        trk[0].p[P_ATK] = 0;
        sw_play(ENGI_PRTCL, y, FS / 4u);
        ok &= !memcmp(y, ref, sizeof ref);
        for (k = 0; k < 8u * FS / CTL; k++)
            track_render(&trk[0], y, CTL);
        sw_play(OTHER[i], y, FS / 4u);
        ok &= !memcmp(y, oref[i], sizeof y);
    }
    check("engine switches: PRTCL after GRAIN PHYS DRUM FM6 VDRUM HRMNC as from boot, and they after it", ok);
}

static int demos(const char *dir)
{
    static const uint8_t NOTES[4] = {48, 52, 55, 60};
    static int32_t o[2 * CTL];
    uint32_t pi, k, n = 0;
    char path[512];
    for (pi = 0; pi < ENG_PRTCL.npresets; pi++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/prtcl_%02u_%s.wav", dir, (unsigned)pi, ENG_PRTCL.presets[pi].name);
        for (k = 0; path[k]; k++)
            if (path[k] == ' ')
                path[k] = '_';
        if (!(f = fopen(path, "wb")))
            continue;
        wav_hdr(f, 4u * (FS / 2u / CTL) * CTL);
        host_tracks_init();
        host_preset(&trk[0], ENGI_PRTCL, pi);
        for (k = 0; k < 4u; k++) {
            uint32_t b, i;
            trk_note_on(&trk[0], NOTES[k], 100);
            for (b = 0; b < FS / 2u / CTL; b++) {
                if (b == FS / 2u / CTL * 3u / 4u)
                    trk_note_off(&trk[0], NOTES[k]);
                mix_block(o, CTL);
                for (i = 0; i < CTL; i++)
                    wav_put(f, o[2u * i], o[2u * i + 1u]);
            }
        }
        fclose(f);
        n++;
    }
    printf("prtcl: %u demos in %s\n", (unsigned)n, dir);
    return n == ENG_PRTCL.npresets;
}

int main(int argc, char **argv)
{
    test_presets();
    test_pitch();
    test_sync();
    test_density();
    test_diffuser();
    test_determinism();
    test_switch();
    if (argc > 1)
        check("demos written", demos(argv[1]));
    printf(bad ? "prtcl: FAILED\n" : "prtcl: all passed\n");
    return bad != 0;
}
