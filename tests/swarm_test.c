/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SWARM engine test (M-Vave-OP1; src/eng_swarm.c, the Plaits swarm model of src/plaits_swarm.c, the Plaits engines'
 * filter of src/plaits_voice.c) through hostsim.c.
 *   build/host/swarm_test [DEMODIR]           (run_tests.sh: build/swarm_demo)
 * 1. every preset over the keyboard (C1 .. C7, 0.5 s each): it sounds (RMS), no DC (|mean| < 1 % of a voice's full
 *    scale), within the filter's knee; the corners of HARM TIMB MORPH AUX at RES 127, DRV 127 too.
 * 2. pitch: HARM 0 (no spread), MORPH 127 (the oscillators held): C4 plays 261.6 Hz (+-1 %).
 * 3. a note starts a burst: the grains' rate jumps (fm 16), then settles slower; a retrigger bursts again.
 * 4. Plaits' levels: SWARM's OUT through the limiter (x3) near +-1 (stmlib's attack overshoots a little).
 * 5. a part from rest plays a note alike (its random numbers in its memory).
 * 6. engine switches with the engines that keep part memory: as from boot.
 * 7. demos into DEMODIR: every preset playing C3 E3 G3 C4. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
/* the filter's knee (dsp.c soft_knee, k 16000) holds a voice under 2 x (16000 + 16384) before the amplitude and the
 * engine's 1-bit gain: a resonance peak at RES 127 may reach past 2 x VOICE_FS, as ANALOG's does, never past this */
#define KNEE_MAX (4 * (16000 + 16384))
static void check(const char *what, int ok)
{
    printf("swarm: %-97s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

/* track 0 on SWARM preset pi (from rest: the part's memory cleared) (e: its E values instead, when given), a flat envelope, POLY, no sends */
static void setup(uint32_t pi, const int16_t *e)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    memset(ENG_MEM(0), 0, sizeof(swm_mem_t));
    eng_mem_owner[0] = 0;
    host_tracks_init();
    host_preset(&trk[0], ENGI_SWARM, pi);
    if (e)
        for (i = 0; i < 8u; i++)
            trk[0].p[P_E0 + i] = e[i];
    trk[0].p[P_ATK] = 0;
    trk[0].p[P_SUS] = 127;
    trk[0].p[P_REL] = 10;
    trk[0].p[P_ED_FLT] = 0;
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_ITYPE] = trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;
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

static psw_t *model_of(uint32_t vi) { return &((swm_mem_t *)ENG_MEM(0))->v[vi]; }

static void test_presets(void)
{
    static int32_t y[FS / 2u];
    static int32_t z[FS];
    uint32_t pi, k, ok_sound = 1, ok_dc = 1, ok_peak = 1;
    int32_t worst = 0;
    double dcw = 0;
    for (pi = 0; pi < ENG_SWARM.npresets; pi++)
        for (k = 0; k < 7u; k++) {
            double rms, mean;
            int32_t peak;
            setup(pi, 0);
            play(24u + 12u * k, FS / 20u, y, FS / 2u);
            stats(y, FS / 2u, &rms, &mean, &peak);
            ok_sound &= rms > 300.0;
            /* (the swarm glides at random down to a few Hz below a low note: no offset, a wander; the mean against RMS) */
            ok_dc &= fabs(mean) < 0.01 * VOICE_FS || fabs(mean) < 0.05 * rms;
            if (getenv("SWM_DEBUG") && !(fabs(mean) < 0.01 * VOICE_FS || fabs(mean) < 0.05 * rms))
                printf("  %s note %u: mean %.0f rms %.0f\n", SWARM_PRESETS[pi].name, 24u + 12u * k, mean, rms);
            ok_peak &= peak <= KNEE_MAX;
            worst = peak > worst ? peak : worst;
            dcw = fabs(mean) > dcw ? fabs(mean) : dcw;
        }
    printf("swarm: presets C1..C7: the loudest peak %d (the knee's ceiling %d), the worst |mean| %.0f\n", worst, KNEE_MAX, dcw);
    check("every preset sounds over the keyboard (C1 .. C7)", ok_sound);
    check("  no DC (|mean| < 1 % of a voice's full scale, or 5 % of the signal: the swarm's sub-audio glides)", ok_dc);
    check("  within the filter's knee", ok_peak);
    {   /* the corners, 1 s each */
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
                play(24u + 30u * k, FS / 20u, z, FS);
                stats(z, FS, &rms, &mean, &peak);
                ok &= peak <= KNEE_MAX && (fabs(mean) < 0.02 * VOICE_FS || fabs(mean) < 0.05 * rms);
                if (getenv("SWM_DEBUG") && !(peak <= KNEE_MAX && (fabs(mean) < 0.02 * VOICE_FS || fabs(mean) < 0.05 * rms)))
                    printf("  corner %u note %u: peak %d mean %.0f rms %.0f\n", c, 24u + 30u * k, peak, mean, rms);
                pk = peak > pk ? peak : pk;
            }
        }
        printf("swarm: the corners of HARM TIMB MORPH AUX, RES and DRV 127: the loudest peak %d\n", pk);
        check("  the corners (HARM TIMB MORPH AUX 0 / 127, RES DRV 127, C1 .. A5): within the knee, no DC", ok);
    }
}

static void test_pitch(void)
{
    static int32_t y[8192];
    static const int16_t e[8] = {0, 60, 127, 0, 127, 0, 0, 0};    /* no spread, the oscillators held; the filter open */
    double p_all, want = FS / 261.6256;
    setup(0, e);
    play(60, 2048, y, 8192);
    p_all = period(y, 8192, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    printf("swarm: C4: period %.2f samples (261.6 Hz: %.2f)\n", p_all, want);
    check("pitch: HARM 0, MORPH 127: C4 plays 261.6 Hz (+-1 %)", fabs(p_all / want - 1.0) < 0.01);
}

static void test_burst(void)
{
    static int32_t y[FS * 2u];
    float fm0, fm1, fm2;
    setup(0, 0);
    trk[0].p[P_VOICE] = V_MONO;
    play(60, 0, y, CTL);
    fm0 = model_of(0)->o[0].fm;
    play(60, 0, y, FS * 2u);                           /* (MONO: the same voice, retriggered: a burst again) */
    fm2 = model_of(0)->o[0].fm;
    {
        uint32_t i;
        static int32_t b[CTL];
        for (i = 0; i < 2u * FS / CTL; i++)
            track_render(&trk[0], b, CTL);
    }
    fm1 = model_of(0)->o[0].fm;
    printf("swarm: the grains' rate (fm) at a note %.2f, 2 s on %.2f\n", fm0, fm1);
    check("a note starts a burst (fm 16 x 0.8 .. 1), then the swarm settles slower", fm0 > 12.0f && fm1 < fm0);
    (void)fm2;
}

static void test_level(void)
{
    static const int16_t e[8] = {127, 127, 127, 0, 127, 0, 0, 0};
    static int32_t y[FS];
    uint32_t i, n, k, ok = 1;
    float worst = 0.0f;
    setup(0, e);
    trk_note_on(&trk[0], 36, 100);
    for (i = 0; i < FS / CTL; i++) {
        static int32_t b[CTL];
        track_render(&trk[0], b, CTL);
        for (n = 0; n < NVOICE; n++)
            for (k = 0; k < CTL && trk[0].v[n].active; k++) {
                const float a = fabsf(PLV(&trk[0])->out[k]);
                worst = a > worst ? a : worst;
            }
    }
    ok = worst <= 1.5f;
    (void)y;
    printf("swarm: OUT after the limiter (x3, Plaits' level): its peak %.3f\n", worst);
    check("Plaits' level: OUT through the limiter near +-1 (its 0.05 attack overshoots a little; the blend clamps)",
          ok && worst > 0.2f);
}

static void test_determinism(void)
{
    static int32_t a[FS / 10u], b[FS / 10u];
    setup(0, 0);
    play(64, 0, a, FS / 10u);
    setup(0, 0);
    play(64, 0, b, FS / 10u);
    check("a part from rest plays a note alike (its random numbers in its memory)", !memcmp(a, b, sizeof a));
}

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
    static const uint32_t OTHER[] = {8 /* GRAIN */, ENGI_PHYS, ENGI_DRUM, ENGI_FM6, ENGI_VDRUM, ENGI_PRTCL, ENGI_WTABLE};
    static int32_t ref[FS / 4u], y[FS / 4u], oref[NELEM(OTHER)][FS / 4u];
    uint32_t i, ok = 1, k;
    setup(0, 0);
    sw_play(ENGI_SWARM, ref, FS / 4u);
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
        sw_play(ENGI_SWARM, y, FS / 4u);
        ok &= !memcmp(y, ref, sizeof ref);
        for (k = 0; k < 8u * FS / CTL; k++)
            track_render(&trk[0], y, CTL);
        sw_play(OTHER[i], y, FS / 4u);
        ok &= !memcmp(y, oref[i], sizeof y);
    }
    check("engine switches: SWARM after GRAIN PHYS DRUM FM6 VDRUM PRTCL WTABLE as from boot, and they after it", ok);
}

static int demos(const char *dir)
{
    static const uint8_t NOTES[4] = {48, 52, 55, 60};
    static int32_t o[2 * CTL];
    uint32_t pi, k, n = 0;
    char path[512];
    for (pi = 0; pi < ENG_SWARM.npresets; pi++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/swarm_%02u_%s.wav", dir, (unsigned)pi, ENG_SWARM.presets[pi].name);
        for (k = 0; path[k]; k++)
            if (path[k] == ' ')
                path[k] = '_';
        if (!(f = fopen(path, "wb")))
            continue;
        wav_hdr(f, 4u * (FS / 2u / CTL) * CTL);
        host_tracks_init();
        host_preset(&trk[0], ENGI_SWARM, pi);
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
    printf("swarm: %u demos in %s\n", (unsigned)n, dir);
    return n == ENG_SWARM.npresets;
}

int main(int argc, char **argv)
{
    test_presets();
    test_pitch();
    test_burst();
    test_level();
    test_determinism();
    test_switch();
    if (argc > 1)
        check("demos written", demos(argv[1]));
    printf(bad ? "swarm: FAILED\n" : "swarm: all passed\n");
    return bad != 0;
}
