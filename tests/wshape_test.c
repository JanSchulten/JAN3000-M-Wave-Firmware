/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* WSHAPE engine test (M-Vave-OP1; src/eng_wshape.c, the Plaits waveshaper of src/plaits_ws.c) through hostsim.c.
 *   build/host/wshape_test [DEMODIR]          (run_tests.sh: build/wshape_demo)
 * 1. every preset over the keyboard (C1 .. C7, 0.5 s each): it sounds (RMS), no DC (|mean| < 1 % of a voice's full
 *    scale), bounded (a voice's sum within 2 x VOICE_FS), and so do the corners of HARM TIMB MORPH AUX at RES 127, DRV 127.
 * 2. pitch: C4 with the filter open plays 261.6 Hz (the autocorrelation's period, +-1 %), and its first 10 ms already
 *    at that pitch (pws_start: no glide from Plaits' last block, which a voice from rest does not have).
 * 3. a retrigger of a sounding voice (MONO) keeps the model running: its phase goes on, not back to the start.
 * 4. UNISON's voices start at different phases (voice.c spreads them), so they do not stack into one.
 * 5. TIMB and the SHP routings: m.shape moves TIMBRE (another sound), clamped at its range's ends (as TIMB 127).
 * 6. a note from silence renders the same samples again.
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
    printf("wshape: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

/* track 0 on WSHAPE preset pi (e: its E values instead, when given), a flat envelope, POLY, no sends */
static void setup(uint32_t pi, const int16_t *e)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    host_preset(&trk[0], ENGI_WSHAPE, pi);
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

static pws_t *model_of(uint32_t vi) { return &((wsh_mem_t *)ENG_MEM(0))->v[vi]; }

static void test_presets(void)
{
    static int32_t y[FS / 2u];
    uint32_t pi, k, ok_sound = 1, ok_dc = 1, ok_peak = 1;
    int32_t worst = 0;
    double dcw = 0;
    for (pi = 0; pi < ENG_WSHAPE.npresets; pi++)
        for (k = 0; k < 7u; k++) {
            double rms, mean;
            int32_t peak;
            setup(pi, 0);
            double per = FS / (440.0 * pow(2.0, (24.0 + 12.0 * k - 69.0) / 12.0));
            play(24u + 12u * k, FS / 20u, y, FS / 2u);
            stats(y, (uint32_t)(floor(FS / 2.0 / per) * per), &rms, &mean, &peak);   /* whole periods */
            ok_sound &= rms > 300.0;
            ok_dc &= fabs(mean) < 0.01 * VOICE_FS;
            ok_peak &= peak <= 2 * VOICE_FS;
            worst = peak > worst ? peak : worst;
            dcw = fabs(mean) > dcw ? fabs(mean) : dcw;
        }
    printf("wshape: presets C1..C7: the loudest peak %d (2 x VOICE_FS %d), the worst |mean| %.0f\n", worst, 2 * VOICE_FS, dcw);
    check("every preset sounds over the keyboard (C1 .. C7)", ok_sound);
    check("  no DC (|mean| < 1 % of a voice's full scale)", ok_dc);
    check("  bounded: a voice within 2 x VOICE_FS", ok_peak);
    {   /* the corners */
        static const int16_t lo[4] = {0, 0, 0, 0}, hi[4] = {127, 127, 127, 127};
        uint32_t c, ok = 1;
        int32_t pk = 0;
        for (c = 0; c < 16u; c++) {
            int16_t e[8];
            double rms, mean;
            int32_t peak;
            for (k = 0; k < 4u; k++)
                e[k] = (c >> k) & 1u ? hi[k] : lo[k];
            e[4] = 127; e[5] = 127; e[6] = 127; e[7] = 64;    /* filter open, RES and DRV at full */
            for (k = 0; k < 3u; k++) {
                setup(0, e);
                double per = FS / (440.0 * pow(2.0, (24.0 + 30.0 * k - 69.0) / 12.0));
                uint32_t whole = (uint32_t)(floor(FS / 4.0 / per) * per);   /* whole periods: C1's 8 in 1/4 s */
                play(24u + 30u * k, FS / 20u, y, FS / 4u);
                stats(y, whole, &rms, &mean, &peak);
                ok &= peak <= KNEE_MAX && fabs(mean) < 0.02 * VOICE_FS;
                pk = peak > pk ? peak : pk;
            }
        }
        printf("wshape: the corners of HARM TIMB MORPH AUX, RES and DRV 127: the loudest peak %d (the knee's ceiling %d)\n",
               pk, KNEE_MAX);
        check("  the corners (HARM TIMB MORPH AUX 0 / 127, RES DRV 127, C1 .. A5): within the knee, no DC", ok);
    }
}

static void test_pitch(void)
{
    static int32_t y[8192];
    static const int16_t e[8] = {64, 30, 64, 0, 127, 0, 0, 0};   /* SOFT FOLD with the filter open, no key tracking */
    double p_all, p_first, want = FS / 261.6256;
    setup(0, e);
    play(60, 0, y, 8192);
    p_first = period(y, 441, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    p_all = period(y + 2048, 4096, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    printf("wshape: C4: period %.2f samples (261.6 Hz: %.2f), in the first 10 ms %.2f\n", p_all, want, p_first);
    check("pitch: C4 plays 261.6 Hz (+-1 %)", fabs(p_all / want - 1.0) < 0.01);
    check("  from its first 10 ms (no glide into the note)", fabs(p_first / want - 1.0) < 0.01);
}

static void test_retrigger(void)
{
    static int32_t y[CTL * 8];
    float ph0, inc;
    pws_t *s;
    setup(1, 0);
    trk[0].p[P_VOICE] = V_MONO;
    play(48, 0, y, CTL * 8);                           /* one voice sounding */
    s = model_of(0);
    ph0 = s->slope.phase;
    inc = s->slope.freq;
    play(55, 0, y, CTL);                               /* the next note on the same voice: one block */
    {
        float adv = s->slope.phase - ph0;
        while (adv < 0.0f)
            adv += 1.0f;
        check("MONO retrigger: the model runs on (its phase advanced by about a block's worth, not reset)",
              trk[0].v[0].active && !trk[0].v[0].s[4] && adv > 0.0f &&
              fabsf(adv - fmodf((float)CTL * (inc + s->slope.freq) * 0.5f, 1.0f)) < 0.05f);
    }
}

static void test_unison(void)
{
    static int32_t y[CTL * 4];
    uint32_t i, j, same = 0, n = 0;
    setup(0, 0);
    trk[0].p[P_VOICE] = V_UNISON;
    trk[0].p[P_DETUNE] = 0;
    play(60, 0, y, CTL * 4);
    for (i = 0; i < NVOICE; i++)
        for (j = i + 1; j < NVOICE; j++)
            if (trk[0].v[i].active && trk[0].v[j].active) {
                n++;
                same += fabsf(model_of(i)->slope.phase - model_of(j)->slope.phase) < 1e-4f;
            }
    check("UNISON: the voices' models at different phases (no stacking)", n >= 6u && !same);
}

static void test_shape(void)
{
    static int32_t a[CTL * 16], b[CTL * 16], c[CTL * 16];
    vmod_t m;
    uint32_t i, k;
    int diff = 0, eq = 1;
    int16_t e[8] = {64, 40, 64, 0, 127, 0, 0, 0};
    for (k = 0; k < 3u; k++) {                         /* TIMB 40 + SHP 0 / + 60 steps / TIMB 127 */
        int32_t *y = k == 0 ? a : k == 1 ? b : c;
        setup(0, e);
        memset(&m, 0, sizeof m);
        m.inc = pitch_inc(60 * 16);
        trk[0].v[0].s[4] = 1;
        for (i = 0; i < 16u; i++) {
            if (k == 2) {
                trk[0].p[P_E1] = 127;
                m.shape = (64 << 8) + (127 << 8);      /* (past the end: clamped) */
            } else {
                m.shape = (64 + (k ? 60 : 0)) << 8;
            }
            wsh_model(&trk[0], &trk[0].v[0], CTL, &m);
            memcpy(y + i * CTL, wsh_o, sizeof(int32_t) * CTL);
        }
    }
    for (i = 0; i < CTL * 16u; i++)
        diff |= a[i] != b[i];
    {   /* TIMB 127 with SHP past the end == TIMB 40 with SHP +87 (both at 127) */
        setup(0, e);
        memset(&m, 0, sizeof m);
        m.inc = pitch_inc(60 * 16);
        trk[0].v[0].s[4] = 1;
        for (i = 0; i < 16u; i++) {
            m.shape = (64 + 87) << 8;
            wsh_model(&trk[0], &trk[0].v[0], CTL, &m);
            for (k = 0; k < CTL; k++)
                eq &= wsh_o[k] == c[i * CTL + k];
        }
    }
    check("SHP moves TIMBRE (another sound); past the end clamped (as TIMB at 127)", diff && eq);
}

static void test_determinism(void)
{
    static int32_t a[FS / 10u], b[FS / 10u];
    setup(2, 0);
    play(64, 0, a, FS / 10u);
    setup(2, 0);
    play(64, 0, b, FS / 10u);
    check("a note from silence renders the same samples again", !memcmp(a, b, sizeof a));
}

/* engine switches on the part (the shared region, engines.c eng_mem_claim): WSHAPE after GRAIN, PHYS, DRUM, FM6 and
 * VDRUM plays as from boot, and each of them after WSHAPE as from boot */
static void sw_play(uint32_t e, int32_t *y, uint32_t n)
{
    memset(gr_rng, 0, sizeof gr_rng);                 /* (GRAIN's random numbers run outside the region: from boot) */
    host_preset(&trk[0], e, 0);
    trk[0].p[P_VOICE] = V_POLY;
    play(60, 0, y, n);
    trk_note_off(&trk[0], 60);
}
static void test_switch(void)
{
    static const uint32_t OTHER[] = {8 /* GRAIN */, ENGI_PHYS, ENGI_DRUM, ENGI_FM6, ENGI_VDRUM};
    static int32_t ref[FS / 4u], y[FS / 4u], oref[NELEM(OTHER)][FS / 4u];
    uint32_t i, ok = 1, k;
    setup(0, 0);
    sw_play(ENGI_WSHAPE, ref, FS / 4u);
    for (i = 0; i < NELEM(OTHER); i++) {
        setup(0, 0);
        sw_play(OTHER[i], oref[i], FS / 4u);
    }
    for (i = 0; i < NELEM(OTHER); i++) {
        setup(0, 0);
        sw_play(OTHER[i], y, FS / 4u);
        for (k = 0; k < 8u * FS / CTL; k++)          /* (the other engine's voices out: 8 s of their release) */
            track_render(&trk[0], y, CTL);
        trk[0].p[P_ATK] = 0;
        sw_play(ENGI_WSHAPE, y, FS / 4u);
        ok &= !memcmp(y, ref, sizeof ref);
        for (k = 0; k < 8u * FS / CTL; k++)
            track_render(&trk[0], y, CTL);
        sw_play(OTHER[i], y, FS / 4u);
        ok &= !memcmp(y, oref[i], sizeof y);
    }
    check("engine switches: WSHAPE after GRAIN PHYS DRUM FM6 VDRUM as from boot, and they after WSHAPE", ok);
}

static int demos(const char *dir)
{
    static const uint8_t NOTES[4] = {48, 52, 55, 60};
    static int32_t o[2 * CTL];
    uint32_t pi, k, n = 0;
    char path[512];
    for (pi = 0; pi < ENG_WSHAPE.npresets; pi++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/wshape_%02u_%s.wav", dir, (unsigned)pi, ENG_WSHAPE.presets[pi].name);
        for (k = 0; path[k]; k++)
            if (path[k] == ' ')
                path[k] = '_';
        if (!(f = fopen(path, "wb")))
            continue;
        wav_hdr(f, 4u * (FS / 2u / CTL) * CTL);
        host_tracks_init();
        host_preset(&trk[0], ENGI_WSHAPE, pi);
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
    printf("wshape: %u demos in %s\n", (unsigned)n, dir);
    return n == ENG_WSHAPE.npresets;
}

int main(int argc, char **argv)
{
    test_presets();
    test_pitch();
    test_retrigger();
    test_unison();
    test_shape();
    test_determinism();
    test_switch();
    if (argc > 1)
        check("demos written", demos(argv[1]));
    printf(bad ? "wshape: FAILED\n" : "wshape: all passed\n");
    return bad != 0;
}
