/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* ACID engine test (M-Vave-OP1; src/eng_acid.c: a TB-303 bass line) through hostsim.c.
 *   build/host/acid_test [DEMODIR]           (run_tests.sh: build/acid_demo)
 * 1. every preset over the bass range (C1 .. C5, 0.5 s each): it sounds, no DC, within its ceiling.
 * 2. pitch: SAW, RES 0, ENV 0, CUT 127: C3 plays 130.8 Hz (+-1 %); TUNE +12 an octave up.
 * 3. the corners (CUT 0 / 127, RES 127, ENV 127, ACC 127, DIST 0 / 127, both waves): bounded, and silent again after
 *    the release (the ladder below self-oscillation, the voice ends).
 * 4. the accent: an accented note (velocity 127) louder and brighter than the same note at 100; ACC 0: alike.
 * 5. the accent sweep: accented notes in a row open the filter further (the third brighter than the first).
 * 6. the slide: a LEGATO note slid to keeps the envelope running (no restart), a fresh note restarts it.
 * 7. ENV MOD and RES: more ENV a brighter attack; more RES a stronger peak (brighter at the same cutoff).
 * 8. demos into DEMODIR: an acid line with accents and slides per preset. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("acid: %-98s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

/* track 0 on ACID preset pi from rest (e: its E values instead, when given), mode, no sends */
static void setup(uint32_t pi, const int16_t *e, uint32_t mode)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    memset(ENG_MEM(0), 0, sizeof(acid_mem_t));
    eng_mem_owner[0] = 0;
    host_tracks_init();
    host_preset(&trk[0], ENGI_ACID, pi);
    if (e)
        for (i = 0; i < 8u; i++)
            trk[0].p[P_E0 + i] = e[i];
    trk[0].p[P_VOICE] = (int16_t)mode;
    trk[0].p[P_ITYPE] = trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;
}

static void render(int32_t *y, uint32_t n)
{
    static int32_t b[CTL];
    uint32_t i, k;
    for (i = 0; i < n; i += CTL) {
        memset(b, 0, sizeof b);
        track_render(&trk[0], b, CTL);
        for (k = 0; k < CTL && i + k < n; k++)
            if (y)
                y[i + k] = b[k];
    }
}

static double rms_of(const int32_t *y, uint32_t n)
{
    double q = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        q += (double)y[i] * y[i];
    return sqrt(q / n);
}
static double mean_of(const int32_t *y, uint32_t n)
{
    double s = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        s += y[i];
    return s / n;
}
static int32_t peak_of(const int32_t *y, uint32_t n)
{
    int32_t p = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        p = abs(y[i]) > p ? abs(y[i]) : p;
    return p;
}
/* brightness: the RMS of the first difference against the RMS (rises with the spectrum's centre) */
static double bright(const int32_t *y, uint32_t n)
{
    double d = 0;
    uint32_t i;
    for (i = 1; i < n; i++)
        d += (double)(y[i] - y[i - 1]) * (y[i] - y[i - 1]);
    return sqrt(d / (n - 1)) / (rms_of(y, n) + 1e-9);
}
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
    return best;
}
static acid_v_t *st0(void) { return &((acid_mem_t *)ENG_MEM(0))->v[0]; }

static void test_presets(void)
{
    static int32_t y[FS / 2u];
    uint32_t pi, k, ok_sound = 1, ok_dc = 1, ok_peak = 1;
    int32_t worst = 0;
    for (pi = 0; pi < ENG_ACID.npresets; pi++)
        for (k = 0; k < 5u; k++) {
            setup(pi, 0, V_LEGATO);
            trk_note_on(&trk[0], 24u + 12u * k, 100);
            render(y, FS / 2u);
            ok_sound &= rms_of(y, FS / 2u) > 300.0;
            ok_dc &= fabs(mean_of(y, FS / 2u)) < 0.05 * rms_of(y, FS / 2u) + 0.002 * VOICE_FS;
            if (getenv("ACID_DEBUG"))
                printf("  %s note %u: mean %.0f rms %.0f\n", ACID_PRESETS[pi].name, 24u + 12u * k, mean_of(y, FS / 2u), rms_of(y, FS / 2u));
            ok_peak &= peak_of(y, FS / 2u) <= 2 * VOICE_FS + 2;
            worst = peak_of(y, FS / 2u) > worst ? peak_of(y, FS / 2u) : worst;
        }
    printf("acid: presets C1..C5: the loudest peak %d (ceiling %d)\n", worst, 2 * VOICE_FS);
    check("every preset sounds over the bass range (C1 .. C5)", ok_sound);
    check("  no DC (|mean| under 5 % of the RMS)", ok_dc);
    check("  within the ceiling (2 x VOICE_FS)", ok_peak);
}

static void test_pitch(void)
{
    static int32_t y[FS / 2u];
    static const int16_t E[8] = {0, 0, 127, 0, 0, 45, 0, 0}, U[8] = {0, 12, 127, 0, 0, 45, 0, 0};
    double p1, p2;
    setup(0, E, V_LEGATO);
    trk_note_on(&trk[0], 48, 100);
    render(0, FS / 10u);
    render(y, FS / 4u);
    p1 = period(y, FS / 4u, 200, 500);
    setup(0, U, V_LEGATO);
    trk_note_on(&trk[0], 48, 100);
    render(0, FS / 10u);
    render(y, FS / 4u);
    p2 = period(y, FS / 4u, 100, 250);
    printf("acid: C3 %.2f Hz, TUNE +12 %.2f Hz\n", FS / p1, FS / p2);
    check("pitch: C3 plays 130.8 Hz (+-1 %)", fabs(FS / p1 - 130.81) < 1.31);
    check("  TUNE +12: an octave up (261.6 Hz +-1 %)", fabs(FS / p2 - 261.63) < 2.62);
}

static void test_corners(void)
{
    static int32_t y[FS / 2u], z[FS / 4u];
    uint32_t c, ok_b = 1, ok_q = 1, ok_end = 1;
    for (c = 0; c < 16u; c++) {
        int16_t e[8] = {(int16_t)(c & 1u), 0, (int16_t)(c & 2u ? 127 : 0), 127, 127, (int16_t)(c & 4u ? 127 : 0), 127,
                        (int16_t)(c & 8u ? 127 : 0)};
        uint32_t i;
        setup(0, e, V_LEGATO);
        trk_note_on(&trk[0], 36, 127);
        render(y, FS / 2u);
        ok_b &= peak_of(y, FS / 2u) <= 2 * VOICE_FS + 2;
        trk_note_off(&trk[0], 36);
        render(0, FS / 10u);
        render(z, FS / 4u);
        ok_q &= peak_of(z, FS / 4u) < 64;
        for (i = 0; i < NVOICE; i++)
            ok_end &= !trk[0].v[i].active;
    }
    check("corners (CUT 0/127, RES ENV ACC 127, DEC 0/127, DIST 0/127, SAW/SQR): bounded", ok_b);
    check("  silent after the release (below self-oscillation)", ok_q);
    check("  the voice ends (engine_t.done)", ok_end);
}

static void test_accent(void)
{
    static int32_t a[FS / 4u], b[FS / 4u];
    static const int16_t E[8] = {0, 0, 40, 100, 80, 45, 100, 0}, Z[8] = {0, 0, 40, 100, 80, 45, 0, 0};
    double ra, rb, ba, bb;
    setup(0, E, V_LEGATO);
    trk_note_on(&trk[0], 36, 100);
    render(b, FS / 4u);
    setup(0, E, V_LEGATO);
    trk_note_on(&trk[0], 36, 127);
    render(a, FS / 4u);
    ra = rms_of(a, FS / 4u); rb = rms_of(b, FS / 4u); ba = bright(a, FS / 4u); bb = bright(b, FS / 4u);
    printf("acid: accent: RMS %.0f vs %.0f, brightness %.3f vs %.3f\n", ra, rb, ba, bb);
    check("accent (velocity 127): louder than velocity 100", ra > rb * 1.3);
    check("  brighter", ba > bb * 1.1);
    setup(0, Z, V_LEGATO);
    trk_note_on(&trk[0], 36, 100);
    render(b, FS / 4u);
    setup(0, Z, V_LEGATO);
    trk_note_on(&trk[0], 36, 127);
    render(a, FS / 4u);
    check("  ACC 0: an accented note as a plain one", fabs(rms_of(a, FS / 4u) - rms_of(b, FS / 4u)) < 0.02 * rms_of(b, FS / 4u));
}

static void test_sweep(void)
{
    static int32_t y[FS / 8u];
    static const int16_t E[8] = {0, 0, 35, 110, 60, 45, 127, 0};
    double b1 = 0, b3 = 0;
    uint32_t k;
    setup(0, E, V_LEGATO);
    for (k = 0; k < 3u; k++) {
        trk_note_on(&trk[0], 36, 127);
        render(y, FS / 8u);
        if (!k)
            b1 = bright(y, FS / 8u);
        b3 = bright(y, FS / 8u);
        trk_note_off(&trk[0], 36);
        render(0, FS / 40u);
    }
    printf("acid: sweep: brightness 1st %.3f, 3rd %.3f; the sweep %.2f\n", b1, b3, st0()->sweep);
    check("accent sweep: accented notes in a row open the filter further (the third brighter)", b3 > b1 * 1.05);
}

static void test_slide(void)
{
    static const int16_t E[8] = {0, 0, 40, 100, 80, 80, 80, 0};
    float m1, m2;
    setup(0, E, V_LEGATO);
    trk[0].p[P_GLIDE] = 20;
    trk_note_on(&trk[0], 36, 100);
    render(0, FS / 5u);
    m1 = st0()->meg;
    trk_note_on(&trk[0], 43, 100);                      /* slid to (the first still held) */
    trk_note_off(&trk[0], 36);
    render(0, CTL * 2u);
    m2 = st0()->meg;
    check("slide: the envelope runs on (no restart) into the note slid to", m2 <= m1 && m2 > 0.0f && st0()->note == 43);
    trk_note_off(&trk[0], 43);
    render(0, FS / 5u);
    trk_note_on(&trk[0], 48, 100);
    render(0, CTL);
    check("  a fresh note restarts it", st0()->meg > 0.9f);
}

static void test_env_res(void)
{
    static int32_t a[FS / 10u];
    static const int16_t LO[8] = {0, 0, 30, 60, 10, 45, 0, 0}, HI[8] = {0, 0, 30, 60, 127, 45, 0, 0},
                         R0[8] = {0, 0, 50, 0, 0, 45, 0, 0}, R1[8] = {0, 0, 50, 120, 0, 45, 0, 0};
    double bl, bh, b0, b1;
    setup(0, LO, V_LEGATO); trk_note_on(&trk[0], 36, 100); render(a, FS / 10u); bl = bright(a, FS / 10u);
    setup(0, HI, V_LEGATO); trk_note_on(&trk[0], 36, 100); render(a, FS / 10u); bh = bright(a, FS / 10u);
    setup(0, R0, V_LEGATO); trk_note_on(&trk[0], 36, 100); render(a, FS / 10u); b0 = bright(a, FS / 10u);
    setup(0, R1, V_LEGATO); trk_note_on(&trk[0], 36, 100); render(a, FS / 10u); b1 = bright(a, FS / 10u);
    printf("acid: ENV 10 / 127 brightness %.3f / %.3f; RES 0 / 120 %.3f / %.3f\n", bl, bh, b0, b1);
    check("ENV MOD: more ENV, a brighter attack", bh > bl * 1.3);
    check("RES: more resonance, a stronger peak (brighter at the same cutoff)", b1 > b0 * 1.2);
}

/* an acid line: 16 steps of (note, accent, slide) at 125 BPM */
static void demos(const char *dir)
{
    static const uint8_t N[16] = {36, 36, 48, 36, 39, 36, 46, 36, 36, 43, 36, 48, 41, 36, 39, 34};
    static const uint8_t A[16] = {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    static const uint8_t S[16] = {0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0};
    const uint32_t step = FS * 60u / 125u / 4u;
    static int32_t y[CTL];
    uint32_t pi, k, r, i;
    char path[512];
    for (pi = 0; pi < ENG_ACID.npresets; pi++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/acid_%u.wav", dir, pi);
        f = fopen(path, "wb");
        if (!f)
            return;
        setup(pi, 0, V_LEGATO);
        trk[0].p[P_GLIDE] = 20;
        wav_hdr(f, step * 32u);
        for (r = 0; r < 2u; r++)
            for (k = 0; k < 16u; k++) {
                uint32_t prev = N[(k + 15u) % 16u], slide_in = S[(k + 15u) % 16u] && (k || r);
                trk_note_on(&trk[0], N[k], A[k] ? 127u : 100u);
                if (slide_in && prev != N[k])
                    trk_note_off(&trk[0], prev);
                for (i = 0; i < step; i += CTL) {
                    uint32_t j;
                    if (i >= step / 2u && !S[k])
                        trk_note_off(&trk[0], N[k]);
                    memset(y, 0, sizeof y);
                    track_render(&trk[0], y, CTL);
                    for (j = 0; j < CTL; j++)
                        wav_put(f, y[j], y[j]);
                }
            }
        fclose(f);
    }
    printf("acid: demos in %s (acid_0.wav .. acid_%u.wav: %s ..)\n", dir, ENG_ACID.npresets - 1u, ACID_PRESETS[0].name);
}

int main(int argc, char **argv)
{
    test_presets();
    test_pitch();
    test_corners();
    test_accent();
    test_sweep();
    test_slide();
    test_env_res();
    if (argc > 1)
        demos(argv[1]);
    printf("acid: %s\n", bad ? "FAILED" : "all ok");
    return bad != 0;
}
