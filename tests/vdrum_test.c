/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* VDRUM test (src/eng_vdrum.c, M-Vave-OP1) on the host, through hostsim.c as drum_test.c.
 *   build/host/vdrum_test [DEMODIR]          (run_tests.sh: build/vdrum_demo)
 * 1. every factory kit, every part struck alone at velocity 127 (ACC 127): the part's output (track_render) never
 *    reaches VD_PEAK_MAX, DC under 2 % of the peak, it sounds (RMS above -40 dB of full scale in its first 50 ms),
 *    and the track is silent (track_render returns 0, no voice) within the part's ring time + the resonator's
 *    DECAY + 0.6 s; a second hit renders the same samples again (the part from rest: deterministic).
 *    M-Vave-OP1: the kick's tail is clean: the kits whose kick is a sine (INIT TUBES STRINGS DUB WOOD), struck at 127,
 *    from 0.1 to 0.6 s the RMS of the second difference (the high end: a 50 Hz sine's is ~-86 dB under its own
 *    level) under -76 dBFS. The level ramp of a layer fell short of the next block's start, a sawtooth at the block
 *    rate (1378 Hz) on the decaying sine at ~-70 dBFS, heard as a hiss (eng_vdrum.c vd_layer_ctl; DRUM's 808 kick:
 *    -81.5 dBFS).
 * 2. the GM map: each note on its part (vd_part_note), part 6 pitched by the toms (41..50 around 45).
 * 3. choke: the closed hat (part 4) brings a sounding open hat (part 5, the same group) below -60 dB of its level
 *    within 3 ms; with the groups apart (CHOK 0) it rings on.
 * 4. the resonator: MIX 0 renders the parts alone (MIX does nothing to the dry sound); TUNE moves its pitch
 *    (STRING: the dominant frequency of the tail within 3 % of TUNE's note); DECAY up -> a longer tail; TUBE and
 *    STRING differ; it sleeps after the tail (vd_mem_t.on 0, the line cleared).
 * 5. KIT OWN without an own kit plays INIT; an own kit plays its own bytes; KIT F n the factory ones.
 * 6. the engine: two GM notes of one part share its voice, six parts sound together (six voices), a key-off does not
 *    end a hit.
 * 7. cost: host instructions per sample for the six parts with both layers and the resonator (every part struck,
 *    retriggered), and for one kick alone.
 * 8. demos into DEMODIR: each kit, its six parts one after the other, then a beat.
 * (storage and the pages: tests/vdrum_ui_test.c) */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
#define CHECK(cond, ...) do { if (!(cond)) { printf("vdrum: " __VA_ARGS__); printf(" FAIL\n"); fails++; } } while (0)
#define VD_PEAK_MAX 120000                      /* the part's output, before the mix (two voices' worth) */
static const uint8_t PART_NOTE[VD_NP] = {36, 38, 39, 42, 46, 45};
#define NMAX (FS * 10)
static int32_t yb[NMAX];

static void kit_fresh(uint32_t preset)
{
    uint32_t p;
    host_tracks_init();
    host_preset(&trk[0], ENGI_VDRUM, preset);
    for (p = 0; p < NPART; p++)
        memset(trk[p].v, 0, sizeof trk[p].v);
    memset(ENG_MEM(0), 0, sizeof(vd_mem_t));
}

static uint32_t voices_on(const track_t *t)
{
    uint32_t i, n = 0;
    for (i = 0; i < NVOICE; i++)
        n += t->v[i].active;
    return n;
}

/* the part's output of note (velocity vel) for up to nmax samples into yb; returns the samples rendered until the
 * track fell silent (track_render 0 and no voice), nmax if it never did */
static uint32_t hit(uint32_t note, uint32_t vel, uint32_t nmax)
{
    int32_t o[CTL];
    uint32_t i, k, end = nmax;
    trk_note_on(&trk[0], note, vel);
    for (i = 0; i + CTL <= nmax; i += CTL) {
        uint32_t r = track_render(&trk[0], o, CTL);
        for (k = 0; k < CTL; k++)
            yb[i + k] = o[k];
        if (!r && !voices_on(&trk[0]) && i > 0) {
            end = i;
            break;
        }
    }
    for (; i < nmax; i++)
        yb[i] = 0;
    return end;
}

static double rms_db(uint32_t a, uint32_t e)
{
    double s = 0;
    uint32_t i;
    for (i = a; i < e; i++)
        s += (double)yb[i] * yb[i];
    return 10 * log10(s / (e - a) / (32768.0 * 32768.0) + 1e-20);
}

/* the fundamental of yb[a..e) between lo and hi Hz: the first normalised autocorrelation peak within 10 % of the
 * highest (an octave above a period is not taken for it), refined by a parabola */
static double fundamental(uint32_t a, uint32_t e, double lo, double hi)
{
    uint32_t L0 = (uint32_t)(FS / hi), L1 = (uint32_t)(FS / lo), L, best = 0;
    static double r[FS / 20 + 2];
    double mx = 0, e0 = 0;
    uint32_t i;
    for (i = a; i < e; i++)
        e0 += (double)yb[i] * yb[i];
    for (L = L0; L <= L1 + 1u && L < NELEM(r); L++) {
        double s = 0;
        for (i = a; i + L < e; i++)
            s += (double)yb[i] * yb[i + L];
        r[L] = s / (e0 + 1e-9);
        if (r[L] > mx)
            mx = r[L];
    }
    for (L = L0 + 1u; L <= L1 && L + 1u < NELEM(r); L++)
        if (r[L] >= 0.9 * mx && r[L] >= r[L - 1u] && r[L] >= r[L + 1u]) {
            best = L;
            break;
        }
    if (!best)
        return 0;
    {
        double y0 = r[best - 1u], y1 = r[best], y2 = r[best + 1u], d = y0 - 2 * y1 + y2;
        return FS / (best + (d != 0 ? 0.5 * (y0 - y2) / d : 0));
    }
}

static void kits_test(void)
{
    uint32_t pr, k;
    for (pr = 0; pr < NELEM(VDRUM_PRESETS); pr++) {
        for (k = 0; k < VD_NP; k++) {
            int32_t peak = 0;
            double dc = 0, lv;
            uint32_t i, end, ring, limit;
            uint64_t h1 = 1469598103934665603ull, h2 = h1;
            kit_fresh(pr);
            trk[0].p[P_E6] = 127;
            end = hit(PART_NOTE[k], 127, NMAX);
            for (i = 0; i < end; i++) {
                int32_t a = yb[i] < 0 ? -yb[i] : yb[i];
                if (a > peak)
                    peak = a;
                dc += yb[i];
                h1 = (h1 ^ (uint32_t)yb[i]) * 1099511628211ull;
            }
            dc = end ? fabs(dc / end) : 0;
            lv = rms_db(0, 2205);
            /* the part's ring time: its layers' REL (10000^(v / 127) ms), MULT's strikes; the resonator's DECAY */
            {
                const uint8_t *B = VD_FACTORY[VDRUM_PRESETS[pr].e[4]] + VK_PART(k);
                double ms = 0, rs;
                uint32_t l;
                for (l = 0; l < 2u; l++)
                    if (B[7u * l + VK_LLVL]) {
                        double t = pow(10000.0, B[7u * l + VK_LREL] / 127.0) * 2.5 + pow(10000.0, B[7u * l + VK_LATK] / 127.0) * 3;
                        if (t > ms)
                            ms = t;
                    }
                rs = 20.0 * pow(2.0, VDRUM_PRESETS[pr].e[1] * 7.8 / 127) * 1.5;
                ring = (uint32_t)((ms + (B[VK_PSEND] && VDRUM_PRESETS[pr].e[5] ? rs : 0) + 600) * FS / 1000);
            }
            limit = ring < NMAX ? ring : NMAX;
            printf("vdrum: %-8s %-5s peak %6d  level %6.1f dB  DC %5.3f %%  end %5.2f s (limit %5.2f)\n",
                   VDRUM_PRESETS[pr].name, N_VD_PART[k], peak, lv, peak ? 100.0 * dc / peak : 0, end / (double)FS,
                   limit / (double)FS);
            CHECK(peak < VD_PEAK_MAX, "%s %s: peak %d under %d", VDRUM_PRESETS[pr].name, N_VD_PART[k], peak, VD_PEAK_MAX);
            CHECK(!peak || dc < 0.02 * peak, "%s %s: DC %.0f under 2 %% of the peak", VDRUM_PRESETS[pr].name, N_VD_PART[k], dc);
            CHECK(lv > -40, "%s %s: it sounds (%.1f dB)", VDRUM_PRESETS[pr].name, N_VD_PART[k], lv);
            CHECK(end < limit, "%s %s: silent after %.2f s (limit %.2f)", VDRUM_PRESETS[pr].name, N_VD_PART[k],
                  end / (double)FS, limit / (double)FS);
            kit_fresh(pr);
            trk[0].p[P_E6] = 127;
            end = hit(PART_NOTE[k], 127, end);
            for (i = 0; i < end; i++)
                h2 = (h2 ^ (uint32_t)yb[i]) * 1099511628211ull;
            CHECK(h1 == h2, "%s %s: a second hit from rest renders the same", VDRUM_PRESETS[pr].name, N_VD_PART[k]);
        }
    }
}

/* M-Vave-OP1: the sine kicks' tails without the block rate's sawtooth (see the head: 1.) */
static void tail_test(void)
{
    static const uint8_t SINE_KICK[] = {0, 2, 3, 5, 7};   /* INIT TUBES STRINGS DUB WOOD */
    uint32_t j, i, a = FS / 10, e = FS * 6 / 10;
    for (j = 0; j < NELEM(SINE_KICK); j++) {
        double s = 0, db;
        kit_fresh(SINE_KICK[j]);
        hit(36, 127, FS);                             /* (past e: hit zeroes the samples after its last block) */
        for (i = a; i < e; i++) {
            double d = (double)yb[i] - 2.0 * yb[i - 1u] + yb[i - 2u];
            s += d * d;
        }
        db = 10 * log10(s / (e - a) / (32768.0 * 32768.0) + 1e-20);
        printf("vdrum: %-8s KICK tail 0.1 .. 0.6 s: the high end (2nd difference) %6.1f dBFS\n",
               VDRUM_PRESETS[SINE_KICK[j]].name, db);
        CHECK(db < -76, "%s KICK: the tail's high end %.1f dBFS under -76 (no hiss)", VDRUM_PRESETS[SINE_KICK[j]].name, db);
    }
}

static void map_test(void)
{
    static const struct { uint8_t note, part; int8_t st; } M[] = {
        {35, 0, 0}, {36, 0, 0}, {37, 2, 0}, {38, 1, 0}, {39, 2, 0}, {40, 1, 0}, {41, 5, -4}, {42, 3, 0}, {43, 5, -2},
        {44, 3, 0}, {45, 5, 0}, {46, 4, 0}, {47, 5, 2}, {48, 5, 3}, {49, 4, 0}, {50, 5, 5}, {51, 4, 0}, {56, 4, 0},
        {60, 5, 0}, {81, 5, 0}, {24, 0, 0}, {26, 1, 0}, {100, 1, 0},   /* (100: its octave of 36..47: 40) */
    };
    uint32_t i;
    for (i = 0; i < NELEM(M); i++) {
        int32_t st;
        uint32_t k = vd_part_note(M[i].note, &st);
        CHECK(k == M[i].part && st == M[i].st * 16, "map: note %u -> part %u (%d) as %u (%d)", M[i].note, k + 1u,
              st / 16, M[i].part + 1u, M[i].st);
    }
}

/* the open hat's level after a closed hat at 100 ms, CHOK as the kit has it (or both 0) */
static double choke_level(int apart)
{
    int32_t o[CTL];
    uint32_t i, k, n = 0;
    double before = 0, after = 0;
    kit_fresh(0);
    if (apart) {
        trk[0].p[P_E4] = VD_OWN;
        memcpy(vd_own[0], VD_FACTORY[0], VK_SIZE);
        vd_own[0][VK_PART(3) + VK_PCHOK] = 0;
        vd_own[0][VK_PART(4) + VK_PCHOK] = 0;
        vd_own[0][VK_PART(3) + 0 * 7 + VK_LLVL] = 0;     /* the closed hat silent: only the open one is heard */
    } else {
        trk[0].p[P_E4] = VD_OWN;
        memcpy(vd_own[0], VD_FACTORY[0], VK_SIZE);
        vd_own[0][VK_PART(3) + 0 * 7 + VK_LLVL] = 0;
    }
    trk_note_on(&trk[0], 46, 110);
    for (i = 0; i < FS / 10u; i += CTL) {
        track_render(&trk[0], o, CTL);
        if (i >= FS / 10u - 441u)
            for (k = 0; k < CTL; k++)
                before += (double)o[k] * o[k], n++;
    }
    before /= n;
    trk_note_on(&trk[0], 42, 110);
    n = 0;
    for (i = 0; i < 441u; i += CTL) {                  /* 3 ms later: 132 .. 573 samples */
        track_render(&trk[0], o, CTL);
        if (i >= 128u)
            for (k = 0; k < CTL; k++)
                after += (double)o[k] * o[k], n++;
    }
    after /= n;
    return 10 * log10(after / before + 1e-30);
}

static void choke_test(void)
{
    double c = choke_level(0), a = choke_level(1);
    printf("vdrum: choke: the open hat %.1f dB 3 ms after the closed one (groups apart %.1f dB)\n", c, a);
    CHECK(c < -60, "choke: the open hat below -60 dB within 3 ms (%.1f)", c);
    CHECK(a > -6, "choke: CHOK 0 lets it ring (%.1f)", a);
}

/* a STRING / TUBE tail: the snare of a kit with only the resonator heard after 60 ms */
static uint32_t reso_tail(int type, int tune, int decay, int mix, double *f)
{
    uint32_t end;
    kit_fresh(3);
    trk[0].p[P_E0] = (int16_t)type;
    trk[0].p[P_E1] = (int16_t)decay;
    trk[0].p[P_E3] = (int16_t)tune;
    trk[0].p[P_E5] = (int16_t)mix;
    end = hit(38, 110, NMAX);
    if (f)
        *f = end > FS * 3u / 10u ? fundamental(FS / 10u, FS * 3u / 10u, 30, 1000) : 0;   /* (after the snare: 50 ms) */
    return end;
}

static void reso_test(void)
{
    double f, want;
    uint32_t e1, e2, i, same = 1;
    vd_mem_t *M = (vd_mem_t *)ENG_MEM(0);
    int32_t dry[FS / 2];
    /* MIX 0: the dry parts alone, whatever the resonator does */
    kit_fresh(3);
    trk[0].p[P_E5] = 0;
    hit(38, 110, FS / 2u);
    memcpy(dry, yb, sizeof dry);
    kit_fresh(3);
    trk[0].p[P_E5] = 0;
    trk[0].p[P_E1] = 127;
    trk[0].p[P_E0] = 0;
    hit(38, 110, FS / 2u);
    for (i = 0; i < FS / 2u; i++)
        same &= dry[i] == yb[i];
    CHECK(same, "reso: MIX 0 leaves the parts as they are");
    for (i = 0; i < 3u; i++) {
        static const int TUNE[3] = {20, 50, 80};
        reso_tail(1, TUNE[i], 110, 127, &f);
        want = 440.0 * pow(2.0, ((24 + TUNE[i] * 72 / 127) - 69) / 12.0);
        printf("vdrum: reso STRING TUNE %d: %.1f Hz (TUNE's note %.1f Hz)\n", TUNE[i], f, want);
        CHECK(fabs(f / want - 1) < 0.03, "reso: STRING at TUNE %d sounds %.1f Hz, wants %.1f", TUNE[i], f, want);
        reso_tail(0, TUNE[i], 110, 127, &f);
        printf("vdrum: reso TUBE   TUNE %d: %.1f Hz\n", TUNE[i], f);
        CHECK(fabs(f / want - 1) < 0.03, "reso: TUBE at TUNE %d sounds %.1f Hz, wants %.1f", TUNE[i], f, want);
    }
    e1 = reso_tail(1, 40, 60, 127, 0);
    e2 = reso_tail(1, 40, 110, 127, 0);
    printf("vdrum: reso DECAY 60: %.2f s, DECAY 110: %.2f s\n", e1 / (double)FS, e2 / (double)FS);
    CHECK(e2 > e1 + FS / 4u, "reso: DECAY up, a longer tail (%.2f -> %.2f s)", e1 / (double)FS, e2 / (double)FS);
    CHECK(e2 < NMAX, "reso: the tail ends");
    {
        uint32_t nz = 0;
        for (i = 0; i < VD_LINE; i++)
            nz += M->line[i] != 0;
        CHECK(!M->on && !nz, "reso: asleep after the tail (on %u, %u samples left in the line)", M->on, nz);
    }
}

static void own_test(void)
{
    uint64_t h[3];
    uint32_t j, i;
    for (j = 0; j < 3u; j++) {
        kit_fresh(0);
        memset(vd_own[0], 0, VK_SIZE);
        if (j == 1u)
            trk[0].p[P_E4] = VD_OWN;                 /* no own kit yet: INIT */
        if (j == 2u) {
            trk[0].p[P_E4] = VD_OWN;
            memcpy(vd_own[0], VD_FACTORY[0], VK_SIZE);
            vd_own[0][VK_PART(0) + VK_LPIT] = 60;    /* the kick higher */
        }
        hit(36, 110, FS / 4u);
        h[j] = 1469598103934665603ull;
        for (i = 0; i < FS / 4u; i++)
            h[j] = (h[j] ^ (uint32_t)yb[i]) * 1099511628211ull;
    }
    CHECK(h[0] == h[1], "own: KIT OWN without an own kit plays INIT");
    CHECK(h[2] != h[0], "own: an own kit plays its own bytes");
}

static void engine_test(void)
{
    int32_t o[CTL];
    uint32_t i;
    kit_fresh(0);
    trk_note_on(&trk[0], 36, 110);
    track_render(&trk[0], o, CTL);
    trk_note_on(&trk[0], 35, 110);
    track_render(&trk[0], o, CTL);
    CHECK(voices_on(&trk[0]) == 1, "engine: two notes of one part share its voice (%u)", voices_on(&trk[0]));
    for (i = 0; i < VD_NP; i++)
        trk_note_on(&trk[0], PART_NOTE[i], 110);
    track_render(&trk[0], o, CTL);
    CHECK(voices_on(&trk[0]) == VD_NP, "engine: six parts, six voices (%u)", voices_on(&trk[0]));
    for (i = 0; i < VD_NP; i++)
        trk_note_off(&trk[0], PART_NOTE[i]);
    for (i = 0; i < 20u; i++)
        track_render(&trk[0], o, CTL);
    CHECK(voices_on(&trk[0]) >= 4u, "engine: a key-off does not end a hit (%u sound 15 ms later)", voices_on(&trk[0]));
}

static void cost_test(void)
{
    int32_t o[CTL];
    uint32_t i, j;
    uint64_t t0, t1;
    kit_fresh(4);                                   /* METAL: both layers in most parts, sends, folds */
    t0 = now_ns();
    for (j = 0; j < 40u; j++) {
        for (i = 0; i < VD_NP; i++)
            trk_note_on(&trk[0], PART_NOTE[i], 120);
        for (i = 0; i < 1378u / 40u; i++)
            track_render(&trk[0], o, CTL);
    }
    t1 = now_ns();
    printf("vdrum: cost: six parts + the resonator %.1f ns per sample (host)\n", (t1 - t0) / (double)(1378u / 40u * 40u * CTL));
}

static void demo(const char *dir)
{
    char path[512];
    uint32_t pr, k, i, s;
    int32_t o[CTL];
    static const uint8_t BEAT[16] = {36, 42, 42, 42, 38, 42, 36, 46, 36, 42, 39, 42, 38, 42, 45, 41};
    for (pr = 0; pr < NELEM(VDRUM_PRESETS); pr++) {
        FILE *f;
        uint32_t frames = (VD_NP * FS * 6u / 10u) + FS * 4u;
        snprintf(path, sizeof path, "%s/vdrum_%u_%s.wav", dir, pr + 1u, VDRUM_PRESETS[pr].name);
        if (!(f = fopen(path, "wb")))
            return;
        wav_hdr(f, frames);
        kit_fresh(pr);
        for (k = 0, s = 0; k < VD_NP; k++) {
            trk_note_on(&trk[0], PART_NOTE[k], 110);
            for (i = 0; i < FS * 6u / 10u; i += CTL, s += CTL) {
                track_render(&trk[0], o, CTL);
                for (uint32_t q = 0; q < CTL; q++)
                    wav_put(f, o[q] / 2, o[q] / 2);
            }
        }
        for (i = 0; i < FS * 4u; i += CTL) {         /* 120 BPM 16ths: 5512 samples a step */
            if (i % 5504u == 0)
                trk_note_on(&trk[0], BEAT[(i / 5504u) % 16u], (i / 5504u) % 4u ? 90 : 120);
            track_render(&trk[0], o, CTL);
            for (uint32_t q = 0; q < CTL; q++)
                wav_put(f, o[q] / 2, o[q] / 2);
        }
        fclose(f);
    }
}

int main(int argc, char **argv)
{
    map_test();
    kits_test();
    tail_test();
    choke_test();
    reso_test();
    own_test();
    engine_test();
    cost_test();
    if (argc > 1)
        demo(argv[1]);
    printf(fails ? "vdrum: %d FAIL\n" : "vdrum: all ok\n", fails);
    return fails != 0;
}
