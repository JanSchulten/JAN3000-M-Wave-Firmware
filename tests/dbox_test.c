/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DBOX (M-Vave-OP1; firmware/src/eng_dbox.c, ui_dbox.c), through the UI harness (tests/ui_test.c):
 *   SOUND    every factory kit, every key struck alone at velocity 127 (ACC 127, DRV 127 too): the track's output never
 *            reaches DB_PEAK_MAX, DC under 2 % of the peak, it sounds, the voice frees itself within its ENV's time +
 *            0.3 s; a second hit from rest renders the same samples; each MODE (1..7) renders a sound of its own and
 *            stays bounded at XMOD 127; the next octave sounds an octave up (the kick's sine: its fundamental x2).
 *   KEYS     note n plays sound n mod 12; KIT OWN without an own kit plays INIT.
 *   PAGES    EDIT > OSC 1, OSC 2, SOUND on a DBOX track only; HOME's KNOB 1..4 are TUNE DECY TONE DRV; a key struck by
 *            hand picks the sound the pages edit; a detent makes the kit the track's own (KIT OWN) and writes one byte;
 *            MODE steps 1..7 and stays there; the panel redraws; SAVE held undoes the last gesture.
 *   STORE    a project and a user preset keep a DBOX track's own kit; a VDRUM track's next to it.
 *   SCREENS  with a directory argument: the three pages (grey palette) as PPM; with a second: WAV demos of the kits.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("dbox: %-95s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
#define DB_PEAK_MAX 120000
#define NMAX (FS * 6)
static int32_t yb[NMAX];

static void db_track(uint32_t preset)
{
    ui_power_on();
    set_engine_of(TSEL, ENGI_DBOX);
    apply_preset_to(TSEL, preset);
    TSEL->engine = TSEL->eng_req;
    eng_mem_claim(TSEL);
    memset(TSEL->v, 0, sizeof TSEL->v);
    frames(400);
}
static uint32_t voices_on(const track_t *t)
{
    uint32_t i, n = 0;
    for (i = 0; i < NVOICE; i++)
        n += t->v[i].active;
    return n;
}
/* the track's output of one note for up to nmax samples into yb; the samples until it fell silent */
static uint32_t hit(uint32_t note, uint32_t vel, uint32_t nmax)
{
    int32_t o[CTL];
    uint32_t i, k, end = nmax;
    trk_note_on(TSEL, note, vel);
    for (i = 0; i + CTL <= nmax; i += CTL) {
        uint32_t r = track_render(TSEL, o, CTL);
        for (k = 0; k < CTL; k++)
            yb[i + k] = o[k];
        if (!r && !voices_on(TSEL) && i > 0) {
            end = i;
            break;
        }
    }
    for (; i < nmax; i++)
        yb[i] = 0;
    return end;
}
static uint64_t hash_of(uint32_t n)
{
    uint64_t h = 1469598103934665603ull;
    uint32_t i;
    for (i = 0; i < n; i++)
        h = (h ^ (uint32_t)yb[i]) * 1099511628211ull;
    return h;
}
static double rms_db(uint32_t a, uint32_t e)
{
    double s = 0;
    uint32_t i;
    for (i = a; i < e; i++)
        s += (double)yb[i] * yb[i];
    return 10 * log10(s / (e - a) / (32768.0 * 32768.0) + 1e-20);
}
/* zero crossings upwards in yb[a..e): a sine's frequency */
static double crossings_hz(uint32_t a, uint32_t e)
{
    uint32_t i, n = 0, first = 0, last = 0;
    for (i = a + 1u; i < e; i++)
        if (yb[i - 1u] < 0 && yb[i] >= 0) {
            if (!n)
                first = i;
            last = i;
            n++;
        }
    return n > 1u ? (n - 1u) * (double)FS / (last - first) : 0;
}

static void sound_test(void)
{
    uint32_t pr, k, worst_end = 1, worst_peak = 1, worst_dc = 1, worst_lv = 1, same = 1;
    for (pr = 0; pr < NELEM(DBOX_PRESETS); pr++)
        for (k = 0; k < DBX_NK; k++) {
            const uint8_t *b = DBX_FACTORY[pr] + DK_KEY(k);
            uint32_t i, end, limit, e;
            int32_t peak = 0;
            double dc = 0, ms = 0;
            uint64_t h;
            db_track(pr);
            TSEL->p[P_E4] = 127;
            TSEL->p[P_E5] = 127;
            end = hit(36u + k, 127, NMAX);
            for (i = 0; i < end; i++) {
                int32_t a = yb[i] < 0 ? -yb[i] : yb[i];
                peak = a > peak ? a : peak;
                dc += yb[i];
            }
            dc = end ? fabs(dc / end) : 0;
            for (e = 0; e < 2u; e++) {                /* the longer ENV: its decay and attack (10000^(v / 127) ms) */
                uint32_t env = b[e ? DK_ENV2 : DK_ENV1];
                double t = 2.2 * pow(10000.0, dbx_dec(env, 64) / 127.0) + pow(10000.0, dbx_atk(env) / 127.0);   /* (99 % in the time: -84 dB, the end, in ~2.1x) */
                ms = t > ms ? t : ms;
            }
            limit = (uint32_t)((ms + 300) * FS / 1000);
            if (limit > NMAX)
                limit = NMAX;
            h = hash_of(end);
            db_track(pr);
            TSEL->p[P_E4] = 127;
            TSEL->p[P_E5] = 127;
            hit(36u + k, 127, end);
            same &= h == hash_of(end);
            if (peak >= DB_PEAK_MAX || (peak && dc >= 0.02 * peak) || rms_db(0, 2205) < -45 || end >= limit)
                printf("dbox:   %s %s: peak %d, DC %.0f, level %.1f dB, end %.2f s (limit %.2f)\n", DBOX_PRESETS[pr].name,
                       N_DBX_KEY[k], peak, dc, rms_db(0, 2205), end / (double)FS, limit / (double)FS);
            worst_peak &= peak < DB_PEAK_MAX;
            worst_dc &= !peak || dc < 0.02 * peak;
            worst_lv &= rms_db(0, 2205) > -45;
            worst_end &= end < limit;
        }
    ck("every kit's 12 sounds under full scale (ACC, DRV 127)", worst_peak);
    ck(".. no DC (under 2 % of the peak)", worst_dc);
    ck(".. each sounds (RMS of its first 50 ms above -45 dB)", worst_lv);
    ck(".. each voice frees itself within 2.2 x its ENV's time + 0.3 s", worst_end);
    ck(".. a second hit from rest renders the same", same);
    {   /* MODE 1..7 on one sound: each its own, bounded at XMOD 127 */
        uint64_t hs[7];
        uint32_t m, j, distinct = 1, bounded = 1;
        for (m = 0; m < 7u; m++) {
            int32_t peak = 0;
            db_track(0);
            TSEL->p[P_E0] = DBX_OWN;
            memcpy(dbx_own[0], DBX_FACTORY[0], VK_SIZE);
            dbx_own[0][DK_KEY(4) + DK_MODE] = (uint8_t)m;
            dbx_own[0][DK_KEY(4) + DK_XMOD] = 127;
            dbx_own[0][DK_KEY(4) + DK_WAV1] = 40;
            dbx_own[0][DK_KEY(4) + DK_WAV2] = 0;
            dbx_own[0][DK_KEY(4) + DK_CUT] = 90;
            hit(40, 127, FS / 2u);
            for (j = 0; j < FS / 2u; j++)
                peak = (yb[j] < 0 ? -yb[j] : yb[j]) > peak ? (yb[j] < 0 ? -yb[j] : yb[j]) : peak;
            bounded &= peak < DB_PEAK_MAX && peak > 100;
            hs[m] = hash_of(FS / 2u);
            for (j = 0; j < m; j++)
                distinct &= hs[j] != hs[m];
        }
        ck("MODE 1..7: each renders a sound of its own", distinct);
        ck("MODE 1..7 at XMOD 127: bounded, and they sound", bounded);
    }
    {   /* the octave: the kick's sine (INIT key C) an octave up on the next C */
        double f0, f1;
        db_track(0);
        hit(36, 110, FS / 4u);
        f0 = crossings_hz(FS / 40u, FS / 8u);
        db_track(0);
        hit(48, 110, FS / 4u);
        f1 = crossings_hz(FS / 40u, FS / 8u);
        printf("dbox: the kick %.1f Hz, the next C %.1f Hz\n", f0, f1);
        ck("the next octave plays the same sound an octave up", f0 > 0 && fabs(f1 / f0 - 2) < 0.06);
    }
    {
        int32_t o;
        uint32_t ok = 1, n;
        for (n = 0; n < 128u; n++)
            ok &= dbx_key(n, &o) == n % 12u;
        ck("note n plays sound n mod 12", ok);
    }
    {
        uint64_t h0, h1;
        db_track(0);
        hit(38, 110, FS / 4u);
        h0 = hash_of(FS / 4u);
        db_track(0);
        TSEL->p[P_E0] = DBX_OWN;                     /* (no own kit yet) */
        hit(38, 110, FS / 4u);
        h1 = hash_of(FS / 4u);
        ck("KIT OWN without an own kit plays INIT", h0 == h1);
    }
}

static uint32_t key_of(uint32_t note) { return note - 29u - 12u * (uint32_t)song.octave; }
static void strike(uint32_t note)
{
    uint32_t k = key_of(note);
    fm1_in.notes |= 1u << k;
    host_notes |= 1u << k;
    frame();
    events_block(CTL);
    fm1_in.notes &= ~(1u << k);
    frame();
    events_block(CTL);
}
static int page_shown(uint32_t graph)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == graph)
            return page_visible(i);
    return 0;
}

static void pages_test(void)
{
    uint8_t before[VK_SIZE];
    uint32_t g, vis = 1, hid = 1, sig0, b, i, ok = 1;
    db_track(0);
    for (g = GR_DBO1; g <= GR_DBHIT; g++)
        vis &= page_shown(g) != 0;
    ui_power_on();
    for (g = GR_DBO1; g <= GR_DBHIT; g++)
        hid &= !page_shown(g);
    ck("EDIT > OSC 1, OSC 2, SOUND on a DBOX track only", vis && hid);
    ck("HOME: KNOB 1..4 are TUNE DECY TONE DRV", ENG_DBOX.knob[0] == P_E1 && ENG_DBOX.knob[1] == P_E2 &&
       ENG_DBOX.knob[2] == P_E3 && ENG_DBOX.knob[3] == P_E5);
    db_track(0);
    go_page(GR_DBO1);
    frame();
    ck("no key struck yet: C", dbu_key() == 0u);
    strike(42);
    ck("F# struck by hand: its sound", dbu_key() == 6u);
    trk_note_on(TSEL, 36, 100);
    events_block(CTL);
    ck("a note of the sequencer does not move it", dbu_key() == 6u);
    sig0 = graph_signature();
    memcpy(before, DBX_FACTORY[0], VK_SIZE);
    turn(EN_K1, 3);                                   /* PITCH 1 up */
    b = DK_KEY(6) + DK_PIT1;
    for (i = 0; i < VK_SIZE; i++)
        ok &= i == b ? dbx_own[0][i] > before[i] : dbx_own[0][i] == before[i];
    ck("a detent: KIT OWN, the factory kit with PITCH 1 up, nothing else", TSEL->p[P_E0] == DBX_OWN && ok);
    ck("the panel redraws", graph_signature() != sig0);
    go_page(GR_DBHIT);
    frame();
    for (i = 0; i < 9u; i++)
        turn(EN_K1, 1);
    ck("MODE steps to 7 and stays there", dbx_own[0][DK_KEY(6) + DK_MODE] == DM_FILT);
    frames(400);
    memcpy(before, dbx_own[0], VK_SIZE);
    go_page(GR_DBO2);
    frame();
    turn(EN_K4, -5);                                  /* CUT down */
    ck("CUT moved", memcmp(before, dbx_own[0], VK_SIZE) != 0);
    frames(400);
    hold(B_SAVE);
    ck("SAVE held: the last gesture undone", !memcmp(before, dbx_own[0], VK_SIZE));
}

static void store_test(void)
{
    static project_t p;
    uint8_t want[VK_SIZE], wv[VK_SIZE], pk[FM6_PACKED];
    int rc;
    db_track(3);
    memcpy(dbx_own[0], DBX_FACTORY[2], VK_SIZE);
    dbx_own[0][DK_KEY(9) + DK_PIT2] = 77;
    TSEL->p[P_E0] = DBX_OWN;
    memcpy(want, dbx_own[0], VK_SIZE);
    set_engine_of(&trk[1], ENGI_VDRUM);
    apply_preset_to(&trk[1], 4);
    trk[1].engine = trk[1].eng_req;
    memcpy(vd_own[1], VD_FACTORY[5], VK_SIZE);
    vd_own[1][VK_PART(1) + VK_LREL] = 33;
    trk[1].p[P_E4] = VD_OWN;
    memcpy(wv, vd_own[1], VK_SIZE);
    project_capture(&p);
    ck("a project holds the DBOX track's own kit and the VDRUM track's", !memcmp(p.fm6[0], want, VK_SIZE) &&
       !memcmp(p.fm6[1], wv, VK_SIZE));
    memset(dbx_own[0], 0, VK_SIZE);
    memset(vd_own[1], 0, VK_SIZE);
    TSEL->p[P_E0] = 1;
    ck("it loads, both kits back", !project_restore_runtime(&p) && !memcmp(dbx_own[0], want, VK_SIZE) &&
       !memcmp(vd_own[1], wv, VK_SIZE) && trk[0].p[P_E0] == DBX_OWN && trk[0].eng_req == ENGI_DBOX);
    db_track(1);
    memcpy(dbx_own[0], DBX_FACTORY[3], VK_SIZE);
    dbx_own[0][DK_KEY(0) + DK_LVL] = 55;
    TSEL->p[P_E0] = DBX_OWN;
    memcpy(want, dbx_own[0], VK_SIZE);
    rc = up_store(6, "MYBOX");
    ck("a user preset of the DBOX sound keeps its kit", (rc == 0 || rc == 3) && !upf_get_of(6, pk, ENGI_DBOX) &&
       !memcmp(pk, want, VK_SIZE) && upf_get(6, pk));
    memset(dbx_own[0], 0, VK_SIZE);
    TSEL->p[P_E0] = 0;
    ck("it loads, its kit back", !up_load(6) && !memcmp(dbx_own[0], want, VK_SIZE) && TSEL->p[P_E0] == DBX_OWN);
}

static void screens(const char *dir)
{
    static const struct { uint32_t g; const char *n; } S[] = {{GR_DBO1, "dbox_osc1"}, {GR_DBO2, "dbox_osc2"},
                                                              {GR_DBHIT, "dbox_sound"}};
    uint32_t i;
    db_track(3);
    strike(38);
    for (i = 0; i < NELEM(S); i++) {
        go_page(S[i].g);
        frame();
        screen_save(dir, S[i].n);
    }
}

static void demo(const char *dir)
{
    static const uint8_t BEAT[16] = {36, 42, 42, 42, 38, 42, 36, 46, 36, 42, 39, 42, 38, 42, 45, 47};
    char path[512];
    uint32_t pr, k, i, q;
    int32_t o[CTL];
    for (pr = 0; pr < NELEM(DBOX_PRESETS); pr++) {
        FILE *f;
        snprintf(path, sizeof path, "%s/dbox_%u_%s.wav", dir, pr + 1u, DBOX_PRESETS[pr].name);
        if (!(f = fopen(path, "wb")))
            return;
        wav_hdr(f, DBX_NK * FS / 2u + FS * 4u);
        db_track(pr);
        for (k = 0; k < DBX_NK; k++) {
            trk_note_on(TSEL, 36u + k, 110);
            for (i = 0; i < FS / 2u; i += CTL) {
                track_render(TSEL, o, CTL);
                for (q = 0; q < CTL; q++)
                    wav_put(f, o[q] / 2, o[q] / 2);
            }
        }
        for (i = 0; i < FS * 4u; i += CTL) {
            if (i % 5504u == 0)
                trk_note_on(TSEL, BEAT[(i / 5504u) % 16u], (i / 5504u) % 4u ? 90 : 120);
            track_render(TSEL, o, CTL);
            for (q = 0; q < CTL; q++)
                wav_put(f, o[q] / 2, o[q] / 2);
        }
        fclose(f);
    }
}

int main(int argc, char **argv)
{
    sound_test();
    pages_test();
    store_test();
    if (argc > 1)
        screens(argv[1]);
    if (argc > 2)
        demo(argv[2]);
    printf(bad ? "dbox: %d FAIL\n" : "dbox: all ok\n", bad);
    return bad != 0;
}
