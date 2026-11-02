/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* WTABLE engine test (M-Vave-OP1; src/eng_wtable.c, the Plaits wavetable model of src/plaits_wt.c with its 8-bit tables
 * and the part's cache of decoded waves, the Plaits engines' filter of src/plaits_voice.c) through hostsim.c.
 *   build/host/wtable_test [DEMODIR]          (run_tests.sh: build/wtable_demo)
 * 1. every preset over the keyboard (C1 .. C7, 0.5 s each): it sounds (RMS), no DC (|mean| < 1 % of a voice's full
 *    scale), bounded (a voice's sum within 2 x VOICE_FS), and the corners of HARM TIMB MORPH AUX at RES 127, DRV 127
 *    within the filter's knee.
 * 2. pitch: C4 with the filter open plays 261.6 Hz (the autocorrelation's period, +-1 %), from its first 10 ms.
 * 3. a retrigger of a sounding voice (MONO) keeps the model running: its phase goes on, not back to the start.
 * 4. UNISON's voices start at different phases (voice.c spreads them), so they do not stack into one.
 * 5. TIMB and the SHP routings: m.shape moves TIMBRE (another sound), clamped at its range's ends (as TIMB 127).
 * 6. a note from silence renders the same samples again; need 1: OUT bit for bit without AUX.
 * 7. the cache: every wave decoded as plaits_wt8.h says, its map consistent under eviction; 8 voices sweeping the
 *    whole table at once (each its own TIMB through ENV -> SHP) decode a bounded number of waves.
 * 8. engine switches with the engines that keep part memory: as from boot.
 * 9. demos into DEMODIR: every preset playing C3 E3 G3 C4. */
static unsigned pwc_decodes;
#define PWC_COUNT pwc_decodes
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
/* the filter's knee (dsp.c soft_knee, k 16000) holds a voice under 2 x (16000 + 16384) before the amplitude and the
 * engine's 1-bit gain: a resonance peak at RES 127 may reach past 2 x VOICE_FS, as ANALOG's does, never past this */
#define KNEE_MAX (4 * (16000 + 16384))
static void check(const char *what, int ok)
{
    printf("wtable: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

/* track 0 on WTABLE preset pi (e: its E values instead, when given), a flat envelope, POLY, no sends */
static void setup(uint32_t pi, const int16_t *e)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    host_preset(&trk[0], ENGI_WTABLE, pi);
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

static pwt_t *model_of(uint32_t vi) { return &((wtb_mem_t *)ENG_MEM(0))->v[vi]; }

static void test_presets(void)
{
    static int32_t y[FS / 2u];
    uint32_t pi, k, ok_sound = 1, ok_dc = 1, ok_peak = 1;
    int32_t worst = 0;
    double dcw = 0;
    for (pi = 0; pi < ENG_WTABLE.npresets; pi++)
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
    printf("wtable: presets C1..C7: the loudest peak %d (2 x VOICE_FS %d), the worst |mean| %.0f\n", worst, 2 * VOICE_FS, dcw);
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
        printf("wtable: the corners of HARM TIMB MORPH AUX, RES and DRV 127: the loudest peak %d (the knee's ceiling %d)\n",
               pk, KNEE_MAX);
        check("  the corners (HARM TIMB MORPH AUX 0 / 127, RES DRV 127, C1 .. A5): within the knee, no DC", ok);
    }
}

static void test_pitch(void)
{
    static int32_t y[8192];
    static const int16_t e[8] = {10, 40, 40, 0, 127, 0, 0, 0};   /* WAVE SCAN's wave, the filter open, no key tracking */
    double p_all, p_first, want = FS / 261.6256;
    setup(0, e);
    play(60, 0, y, 8192);
    p_first = period(y, 441, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    p_all = period(y + 2048, 4096, (uint32_t)(want * 0.8), (uint32_t)(want * 1.25));
    printf("wtable: C4: period %.2f samples (261.6 Hz: %.2f), in the first 10 ms %.2f\n", p_all, want, p_first);
    check("pitch: C4 plays 261.6 Hz (+-1 %)", fabs(p_all / want - 1.0) < 0.01);
    check("  from its first 10 ms (no glide into the note)", fabs(p_first / want - 1.0) < 0.01);
}

static void test_retrigger(void)
{
    static int32_t y[CTL * 8];
    float ph0, inc;
    pwt_t *s;
    setup(1, 0);
    trk[0].p[P_VOICE] = V_MONO;
    play(48, 0, y, CTL * 8);                           /* one voice sounding */
    s = model_of(0);
    ph0 = s->phase;
    inc = s->pf0;
    play(55, 0, y, CTL);                               /* the next note on the same voice: one block */
    {
        float adv = s->phase - ph0;
        while (adv < 0.0f)
            adv += 1.0f;
        check("MONO retrigger: the model runs on (its phase advanced by about a block's worth, not reset)",
              trk[0].v[0].active && !trk[0].v[0].s[4] && adv > 0.0f &&
              fabsf(adv - fmodf((float)CTL * (inc + s->pf0) * 0.5f, 1.0f)) < 0.05f);
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
                same += fabsf(model_of(i)->phase - model_of(j)->phase) < 1e-4f;
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
            wtb_model(&trk[0], &trk[0].v[0], model_of(0), &((wtb_mem_t *)ENG_MEM(0))->c, CTL, &m);
            memcpy(y + i * CTL, PLV(&trk[0])->o, sizeof(int32_t) * CTL);
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
            wtb_model(&trk[0], &trk[0].v[0], model_of(0), &((wtb_mem_t *)ENG_MEM(0))->c, CTL, &m);
            for (k = 0; k < CTL; k++)
                eq &= PLV(&trk[0])->o[k] == c[i * CTL + k];
        }
    }
    check("SHP moves TIMBRE (another sound); past the end clamped (as TIMB at 127)", diff && eq);
}

/* pwt_render's need 1 (AUX 0): OUT bit for bit as with both outputs (AUX is OUT at 5 bits: nothing else to leave out) */
static void test_need(void)
{
    static float o1[CTL], a1[CTL], o3[CTL], a3[CTL];
    static pwc_t c;
    pwt_t s1, s3;
    pwt_par_t q = {0.3f, 0.6f, 0.2f, 0.01f, WTB_CXY, WTB_CZ};
    uint32_t b, same = 1;
    memset(&c, 0, sizeof c);
    pwt_start(&s1, &c, 0.25f, &q);
    s3 = s1;
    for (b = 0; b < 64u; b++) {
        q.timbre = (float)b / 64.0f;
        pwt_render(&s1, &c, &q, o1, a1, CTL, 1u);
        pwt_render(&s3, &c, &q, o3, a3, CTL, 3u);
        same &= !memcmp(o1, o3, sizeof o1);
        c.now++;
    }
    check("pwt_render need 1: OUT bit for bit as with AUX (a TIMB sweep)", same);
}

/* the cache: a wave as plaits_wt8.h decodes it; the map and the slots agree after any order of loads */
static void test_cache(void)
{
    static pwc_t c;
    uint32_t w, k, ok = 1, i, slots = 0;
    memset(&c, 0, sizeof c);
    for (i = 0; i < 3u * PWT_N; i++) {               /* every wave, three times over, in a scattered order */
        const int16_t *t;
        int32_t v;
        w = (i * 37u + i / PWT_N) % PWT_N;
        t = pwc_get(&c, w);
        v = pwt_first[w];
        ok &= t[0] == v;
        for (k = 1; k < PWT_LEN; k++) {
            v += PWT_GRID * pwt_steps[w][k - 1];
            ok &= t[k] == v;
        }
        if (!(i % 5u))
            c.now++;
    }
    for (w = 0; w < PWT_N; w++)
        if (c.map[w]) {
            ok &= c.who[c.map[w] - 1u] == w + 1u;
            slots++;
        }
    check("the cache: every wave decoded as plaits_wt8.h says; map and slots agree (192 waves, 3 rounds)",
          ok && slots == PWC_SLOTS);
    {   /* 8 voices, each its own TIMB (ENV -> SHP, notes a moment apart, a long decay), MORPH swept over the table */
        static int32_t b[CTL];
        uint32_t n, act = 0, d0;
        setup(0, 0);
        trk[0].p[P_ED_SHP] = 127;
        trk[0].p[P_DEC] = 127;
        trk[0].p[P_SUS] = 0;
        for (n = 0; n < 8u; n++) {
            trk_note_on(&trk[0], 48u + 3u * n, 100);
            for (i = 0; i < 20u; i++) {
                track_render(&trk[0], b, CTL);
                wtb_post(&trk[0], b, CTL, 32767);
            }
        }
        d0 = pwc_decodes;
        for (i = 0; i < 2000u; i++) {
            trk[0].p[P_E2] = (int16_t)(i * 127u / 2000u);
            track_render(&trk[0], b, CTL);
            wtb_post(&trk[0], b, CTL, 32767);
        }
        for (n = 0; n < NVOICE; n++)
            act += trk[0].v[n].active;
        printf("wtable: 8 voices on their own rows, MORPH swept over the table in 2000 blocks: %u decodes (%u voices sounding)\n",
               pwc_decodes - d0, act);
        check("  .. decoding only where the sweep crosses a column (fewer than 3 a block; 8 voices sounding)",
              act == NVOICE && pwc_decodes - d0 < 3u * 2000u && pwc_decodes - d0 > 0u);
    }
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

/* engine switches on the part (the shared region, engines.c eng_mem_claim): WTABLE after GRAIN, PHYS, DRUM, FM6 and
 * VDRUM plays as from boot, and each of them after WTABLE as from boot */
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
    static const uint32_t OTHER[] = {8 /* GRAIN */, ENGI_PHYS, ENGI_DRUM, ENGI_FM6, ENGI_VDRUM, ENGI_PRTCL};
    static int32_t ref[FS / 4u], y[FS / 4u], oref[NELEM(OTHER)][FS / 4u];
    uint32_t i, ok = 1, k;
    setup(0, 0);
    sw_play(ENGI_WTABLE, ref, FS / 4u);
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
        sw_play(ENGI_WTABLE, y, FS / 4u);
        ok &= !memcmp(y, ref, sizeof ref);
        for (k = 0; k < 8u * FS / CTL; k++)
            track_render(&trk[0], y, CTL);
        sw_play(OTHER[i], y, FS / 4u);
        ok &= !memcmp(y, oref[i], sizeof y);
    }
    check("engine switches: WTABLE after GRAIN PHYS DRUM FM6 VDRUM as from boot, and they after WTABLE", ok);
}

static int demos(const char *dir)
{
    static const uint8_t NOTES[4] = {48, 52, 55, 60};
    static int32_t o[2 * CTL];
    uint32_t pi, k, n = 0;
    char path[512];
    for (pi = 0; pi < ENG_WTABLE.npresets; pi++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/wtable_%02u_%s.wav", dir, (unsigned)pi, ENG_WTABLE.presets[pi].name);
        for (k = 0; path[k]; k++)
            if (path[k] == ' ')
                path[k] = '_';
        if (!(f = fopen(path, "wb")))
            continue;
        wav_hdr(f, 4u * (FS / 2u / CTL) * CTL);
        host_tracks_init();
        host_preset(&trk[0], ENGI_WTABLE, pi);
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
    printf("wtable: %u demos in %s\n", (unsigned)n, dir);
    return n == ENG_WTABLE.npresets;
}

int main(int argc, char **argv)
{
    test_presets();
    test_pitch();
    test_retrigger();
    test_unison();
    test_shape();
    test_determinism();
    test_need();
    test_cache();
    test_switch();
    if (argc > 1)
        check("demos written", demos(argv[1]));
    printf(bad ? "wtable: FAILED\n" : "wtable: all passed\n");
    return bad != 0;
}
