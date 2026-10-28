/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* FX1's CLOUD (firmware/src/fx1.c), the granular cloud in the delay's line, on the firmware's own sources (hostsim.c):
 *   BUS      the DELAY untouched while it is the type; a TYPE change: the old one's last block fades, FX1 is silent
 *            while the line clears, the new one starts with nothing of the old in it (both ways).
 *   GRAINS   over a long run with the settings changing at random (SIZE, DENS, PITCH +-24, SPREAD, RAND, FREEZE on and
 *            off): no grain ever reads where the line is written or what was never written; at most CL_NG; as many
 *            grains a second as DENS asks; SPREAD 0: no side signal.
 *   SOUND    PITCH +12 / -12 / +7: a sine comes out an octave up / down / a fifth up; the level about the send's,
 *            dense or sparse; FDBK 100 with PITCH: bounded, and it dies away once the send stops; FREEZE: the line
 *            stands, the cloud plays on what it holds after the send stops; let go, it takes the send again.
 *   SONG     through mix_block with tracks sending (DLY), CLOUD FREEZE from the FX layer's F5: bounded, no clicks at the
 *            TYPE change. With a directory argument: WAV demos of the cloud.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("cloud: %-92s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static uint32_t rs = 0x9E3779B9u;
static uint32_t rnd32(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }

/* the buses alone: a send into FX1 (none into the chorus or the reverb), the wet bus and the side out */
static int32_t sc[CTL], sd[CTL], sr[CTL], w[CTL], side[CTL];   /* (the buses' own) */
static void bus(const int32_t *in)
{
    memset(sc, 0, sizeof sc);
    memset(sr, 0, sizeof sr);
    memcpy(sd, in, sizeof sd);
    fx_buses(sc, sd, sr, w, CTL);
    if (hl.side)
        memcpy(side, hl.sd, sizeof side);
    else
        memset(side, 0, sizeof side);
}
static void bus_reset(void)
{
    fx_serial = 0;                              /* isolate FX1 from downstream reverb; chain tested in shimmer_send_test */
    memset(dly_buf, 0, sizeof dly_buf);
    memset(rev_comb, 0, sizeof rev_comb);
    memset(&rev_u, 0, sizeof rev_u);
    memset(&cld, 0, sizeof cld);
    memset(&hl, 0, sizeof hl);
    fx.dly_w = 0;
    fx.dly_lp = 0;
    fx.rtype = 0;
    song.g[G_RTYPE] = 0;
    song.g[G_BPM] = 120;
    song.g[G_DTIME] = 2;
    song.g[G_DFDBK] = 60;
    song.g[G_DMIX] = 90;
    song.g[G_DCOLOR] = 70;
    perf_act = 0;
    fx1s.type = FX1_DELAY;
    fx1s.size = 50; fx1s.dens = 50; fx1s.spread = 30; fx1s.pitch = 0; fx1s.rnd = 10; fx1s.fdbk = 0; fx1s.mix = 80;
}
static void to_cloud(void)
{
    static const int32_t z[CTL];
    uint32_t i;
    fx1s.type = FX1_CLOUD;
    for (i = 0; i < CL_CLEAR + 2u; i++)
        bus(z);
}
/* a sine of period P samples, amplitude A, from sample t0: one block */
static void sine_block(int32_t *b, uint32_t t0, double f, double a)
{
    uint32_t i;
    for (i = 0; i < CTL; i++)
        b[i] = (int32_t)lrint(a * sin(2 * M_PI * f * (double)(t0 + i) / FS));
}
/* the grains' reads against the line: each one inside what was written, never at the write head */
static int grains_sane(void)
{
    uint32_t k, n = 0;
    for (k = 0; k < CL_NG; k++) {
        const cl_grain_t *g = &cld.g[k];
        uint32_t lag;
        if (!g->on)
            continue;
        n++;
        lag = (fx.dly_w - (g->pos >> 16)) & (DLY_LEN - 1u);
        if (lag < 2u || lag > cld.fill + 1u)
            return 0;
    }
    return n <= CL_NG;
}

static void test_bus(void)
{
    static int32_t in[CTL];
    uint32_t t, i, k;
    int32_t peak;
    int ok = 1;
    bus_reset();
    for (t = 0; t < 44100u; t += CTL) {
        sine_block(in, t, 441.0, 12000.0);
        bus(in);
    }
    ok = !cld.busy && cld.type == FX1_DELAY && !cld.fill;
    ck("DELAY (the default): FX1's CLOUD never runs, the line is the delay's", ok);
    fx1s.type = FX1_CLOUD;
    memset(in, 0, sizeof in);
    bus(in);
    peak = 0;
    for (i = 0; i < CTL; i++)
        peak = abs(w[i]) > peak ? abs(w[i]) : peak;
    ok = cld.busy == CL_CLEAR && abs(w[CTL - 1]) < peak / 4 && peak > 0;
    for (k = 0; k < CL_CLEAR; k++) {
        bus(in);
        for (i = 0; i < CTL; i++)
            ok &= !w[i];
    }
    ok &= !cld.busy && cld.type == FX1_CLOUD && !cld.fill;
    for (i = 0; i < DLY_LEN; i++)
        ok &= !dly_buf[i];
    ck("DELAY -> CLOUD: the echoes fade over a block, FX1 silent while the line clears (23 ms), then empty", ok);
    for (t = 0; t < 44100u; t += CTL)
        bus(in);
    ok = 1;
    for (i = 0; i < CTL; i++)
        ok &= !w[i];
    ck("  .. a CLOUD with nothing sent: silence (nothing of the delay's left in it)", ok);
    for (t = 0, peak = 0; t < 22050u; t += CTL) {
        sine_block(in, t, 441.0, 12000.0);
        bus(in);
        for (i = 0; i < CTL; i++)
            peak = abs(w[i]) > peak ? abs(w[i]) : peak;
    }
    ck("  .. a sine sent: the cloud sounds", peak > 1000);
    fx1s.type = FX1_DELAY;
    memset(in, 0, sizeof in);
    bus(in);
    ok = cld.busy == CL_CLEAR;
    for (k = 0; k < CL_CLEAR + 200u; k++) {
        bus(in);
        for (i = 0; i < CTL; i++)
            ok &= !w[i];
    }
    ck("CLOUD -> DELAY: faded, cleared; the delay then has no echo of the cloud (nothing sent: silence)",
          ok && !cld.busy && cld.type == FX1_DELAY);
}

static void test_grains(void)
{
    static int32_t in[CTL];
    uint32_t t, i, starts = 0, sane = 1, most = 0, k;
    int32_t peak = 0;
    bus_reset();
    to_cloud();
    for (t = 0; t < 30u * 44100u; t += CTL) {
        if (t % 22050u < CTL) {                     /* every half second: other settings */
            fx1s.size = (uint8_t)(rnd32() % 101u);
            fx1s.dens = (uint8_t)(rnd32() % 101u);
            fx1s.pitch = (int8_t)((int32_t)(rnd32() % 49u) - 24);
            fx1s.spread = (uint8_t)(rnd32() % 101u);
            fx1s.rnd = (uint8_t)(rnd32() % 101u);
            fx1s.fdbk = (uint8_t)(rnd32() % 101u);
            if (rnd32() % 4u == 0u)
                perf_act ^= PF_BIT(PF_CFRZ);
        }
        for (i = 0; i < CTL; i++)
            in[i] = (int32_t)(rnd32() % 40001u) - 20000;
        {
            uint32_t before = 0;
            for (k = 0; k < CL_NG; k++)
                before |= (uint32_t)cld.g[k].on << k;
            bus(in);
            for (k = 0; k < CL_NG; k++)
                starts += cld.g[k].on && !((before >> k) & 1u);
        }
        sane &= grains_sane();
        most = cld.live > most ? cld.live : most;
        for (i = 0; i < CTL; i++)
            peak = abs(w[i]) > peak ? abs(w[i]) : peak;
    }
    ck("30 s of random settings (SIZE, DENS, PITCH +-24, SPREAD, RAND, FDBK, FREEZE): every grain reads inside what "
          "was written, never at the write head", sane);
    ck("  .. at most CL_NG grains, and grains did play", most <= CL_NG && starts > 100u);
    ck("  .. bounded (no runaway, no wrap)", peak < 4 * 32768);
    /* DENS: grains a second as asked (short grains: always a free slot) */
    perf_act = 0;
    fx1s.size = 0;
    fx1s.rnd = 0;
    fx1s.pitch = 0;
    fx1s.spread = 50;
    fx1s.fdbk = 0;
    for (k = 0; k < 2u; k++) {
        uint32_t want;
        fx1s.dens = k ? 70u : 30u;
        want = 10u * 44100u / cl_gap(fx1s.dens);
        starts = 0;
        for (t = 0; t < 10u * 44100u; t += CTL) {
            uint32_t before = 0;
            for (i = 0; i < CTL; i++)
                in[i] = (int32_t)(rnd32() % 20001u) - 10000;
            for (i = 0; i < CL_NG; i++)
                before |= (uint32_t)cld.g[i].on << i;
            bus(in);
            for (i = 0; i < CL_NG; i++)
                starts += cld.g[i].on && !((before >> i) & 1u);
        }
        ck(k ? "DENS 70: as many grains a second as it says (9.8 / s), +-15 %" : "DENS 30: as many grains a second "
              "as it says (2.1 / s), +-15 %", starts * 100u >= want * 85u && starts * 100u <= want * 115u);
    }
    fx1s.spread = 0;
    fx1s.dens = 60;
    fx1s.size = 50;
    for (t = 0, peak = 0; t < 2u * 44100u; t += CTL) {
        for (i = 0; i < CTL; i++)
            in[i] = (int32_t)(rnd32() % 20001u) - 10000;
        bus(in);
        for (i = 0; i < CTL; i++)
            peak |= side[i];
    }
    ck("SPREAD 0: every grain in the middle (no side signal)", !peak);
}

/* the strongest frequency of b (n samples) between lo and hi Hz, to 0.5 Hz (Goertzel at each step) */
static double peak_freq(const int32_t *b, uint32_t n, double lo, double hi)
{
    double f, best = 0, bf = lo;
    uint32_t i;
    for (f = lo; f <= hi; f += 0.5) {
        double c = 2 * cos(2 * M_PI * f / FS), s1 = 0, s2 = 0, p;
        for (i = 0; i < n; i++) {
            double s0 = b[i] + c * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        p = s1 * s1 + s2 * s2 - c * s1 * s2;
        if (p > best) {
            best = p;
            bf = f;
        }
    }
    return bf;
}
static int32_t rec[3u * 44100u];
static void run_sine(double f, uint32_t frames)
{
    static int32_t in[CTL];
    uint32_t t, i;
    for (t = 0; t < frames; t += CTL) {
        sine_block(in, t, f, 12000.0);
        bus(in);
        for (i = 0; i < CTL && t + i < frames; i++)
            rec[t + i] = w[i];
    }
}
static void test_sound(void)
{
    static const struct { int8_t st; double ratio; const char *what; } P[] = {
        {12, 2.0, "PITCH +12: a 441 Hz sine comes out at 882 Hz"},
        {-12, 0.5, "PITCH -12: .. at 220.5 Hz"},
        {7, 1.4983, "PITCH +7: .. a fifth up (660.8 Hz)"},
        {0, 1.0, "PITCH 0: .. at 441 Hz"}};
    static int32_t in[CTL];
    uint32_t k, t, i, w0;
    double rms0, rms1, e;
    int32_t peak;
    for (k = 0; k < 4u; k++) {
        double f;
        bus_reset();
        to_cloud();
        fx1s.pitch = P[k].st;
        fx1s.size = 75;                             /* grains of ~300 ms, about one a second: each one alone (overlapping */
        fx1s.dens = 15;                             /* grains of a pitched sine meet in other phases: the splice's sidebands) */
        fx1s.spread = 0;
        fx1s.rnd = 0;
        run_sine(441.0, 3u * 44100u);
        f = peak_freq(rec + 22050u, 2u * 44100u, 441.0 * P[k].ratio * 0.8, 441.0 * P[k].ratio * 1.2);
        ck(P[k].what, fabs(f / (441.0 * P[k].ratio) - 1.0) < 0.01);
    }
    /* the level: dense or sparse, about the send's (the line holds half, the return doubles it, MIX 100) */
    for (k = 0; k < 2u; k++) {
        bus_reset();
        to_cloud();
        fx1s.mix = 100;
        fx1s.spread = 40;
        fx1s.size = k ? 70 : 40;
        fx1s.dens = k ? 90 : 30;
        run_sine(441.0, 3u * 44100u);
        for (e = 0, t = 44100u; t < 3u * 44100u; t++)
            e += (double)rec[t] * rec[t];
        e = sqrt(e / (2.0 * 44100u)) / (12000.0 / sqrt(2.0));
        ck(k ? "the level dense (SIZE 70, DENS 90): within 0.3 .. 1.5 x the send's" :
                  "the level sparse (SIZE 40, DENS 30): within 0.2 .. 1.5 x the send's",
              e > (k ? 0.3 : 0.2) && e < 1.5);
    }
    /* FDBK 100 with PITCH +12: bounded while fed, dies away after */
    bus_reset();
    to_cloud();
    fx1s.fdbk = 100;
    fx1s.pitch = 12;
    fx1s.dens = 80;
    fx1s.size = 60;
    fx1s.spread = 60;
    peak = 0;
    for (t = 0; t < 10u * 44100u; t += CTL) {
        for (i = 0; i < CTL; i++)
            in[i] = (int32_t)(rnd32() % 60001u) - 30000;
        bus(in);
        for (i = 0; i < CTL; i++)
            peak = abs(w[i]) > peak ? abs(w[i]) : peak;
    }
    memset(in, 0, sizeof in);
    for (t = 0, rms0 = 0; t < 44100u; t += CTL) {
        bus(in);
        for (i = 0; i < CTL; i++)
            rms0 += (double)w[i] * w[i];
    }
    for (t = 0; t < 20u * 44100u; t += CTL)
        bus(in);
    for (t = 0, rms1 = 0; t < 44100u; t += CTL) {
        bus(in);
        for (i = 0; i < CTL; i++)
            rms1 += (double)w[i] * w[i];
    }
    ck("FDBK 100 with PITCH +12 on full-scale noise: bounded; 20 s after the send stops, 30 dB quieter at least",
          peak < 4 * 32768 && rms1 < rms0 / 1000.0);
    /* FREEZE: the line stands, the cloud plays on in it */
    bus_reset();
    to_cloud();
    fx1s.dens = 80;
    fx1s.size = 60;
    fx1s.spread = 100;
    run_sine(441.0, 44100u);
    perf_act |= PF_BIT(PF_CFRZ);
    w0 = fx.dly_w;
    memset(in, 0, sizeof in);
    for (t = 0, rms0 = 0; t < 5u * 44100u; t += CTL) {
        bus(in);
        if (t >= 4u * 44100u)
            for (i = 0; i < CTL; i++)
                rms0 += (double)w[i] * w[i];
    }
    ck("CLOUD FREEZE held, the send silent: the line stands, 5 s later the cloud still plays what it holds",
          fx.dly_w == w0 && sqrt(rms0 / 44100.0) > 500.0);
    perf_act &= ~PF_BIT(PF_CFRZ);
    for (t = 0; t < 3u * 44100u; t += CTL)
        bus(in);
    for (t = 0, rms1 = 0; t < 44100u; t += CTL) {
        bus(in);
        for (i = 0; i < CTL; i++)
            rms1 += (double)w[i] * w[i];
    }
    ck("  .. let go: it takes the send again (silence: after the line has gone round, silence)", fx.dly_w != w0 &&
          rms1 == 0.0);
}

/* ------------------------------------------------------------------- UI --- */
static void cloud_power_on(void)
{
    ui_power_on();
    bus_reset();
    memset(fxt_lane, 0, sizeof fxt_lane);
}
static uint32_t fx_pages(char *out, uint32_t n)       /* the FX family's titles as FX taps go round, from FX */
{
    uint32_t k;
    out[0] = 0;
    go_title("FX");
    for (k = 0; k < n; k++) {
        if (k) {
            frames(400);
            press(B_FX);
        }
        str_cpy(out + str_len(out), cur_page()->title, 200);
        str_cpy(out + str_len(out), "|", 4);
    }
    return 0;
}
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }
static void test_ui(const char *dir)
{
    char pages[400];
    int ok;
    cloud_power_on();
    fx_pages(pages, 10);
    ck("FX's eight cards: SEND, DRIVE, CHORUS, SHIMMER, DELAY, REVERB, SLICER, FX TRACKS (M-Vave-OP1)",
       strstr(pages, "FX|DRIVE|CHORUS|SHIMMER|DELAY|REVERB|SLICER|FX TRACKS|FX|") != 0);
    go_title("DELAY");
    song.g[G_DFDBK] = 60;
    song.g[G_DMIX] = 90;
    turn(EN_K3, 3);
    turn(EN_K4, -10);
    ok = song.g[G_DFDBK] > 60 && song.g[G_DMIX] < 90 && fx1s.type == FX1_DELAY;
    turn(EN_K1, 1);
    ok &= fx1s.type == FX1_CLOUD && msg_is("FX1: CLOUD (GRANULAR)");
    ck("DELAY: KNOB 2..4 the DELAY's TIME FDBK MIX; KNOB 1 TYPE: CLOUD", ok);
    fx1s.size = 50; fx1s.dens = 50;
    turn(EN_K2, 7);
    turn(EN_K4, -60);
    turn(EN_K3, 7);
    ok = fx1s.size == 57u && fx1s.dens == 0u && fx1s.pitch == 7;
    turn(EN_K3, 40);
    ok &= fx1s.pitch == 24;
    turn(EN_K3, -60);
    ok &= fx1s.pitch == -24;
    ck("DELAY with CLOUD: KNOB 2 SIZE, 3 PITCH (-24..+24), 4 DENS", ok);
    fx1s.pitch = 0;
    /* the FX layer's F5: CLOUD FREEZE */
    ok = PF_DEF[14] == PF_CFRZ && perf_map[14] == PF_CFRZ && str_eq(PF_NAME[PF_CFRZ], "CLOUD FREEZE");
    {
        uint32_t t, w0;
        static int32_t in[CTL];
        transport_req = 1;
        for (t = 0; t < 40u; t++)
            blk();
        btn_down(B_FX);
        frame();
        key_down(white(14));
        blk();
        blk();
        w0 = fx.dly_w;
        for (t = 0; t < 40u; t++)
            blk();
        ok &= (perf_held & PF_BIT(PF_CFRZ)) && (perf_act & PF_BIT(PF_CFRZ)) && fx.dly_w == w0;
        key_up(white(14));
        frame();
        btn_up(B_FX);
        frame();
        frames(400);
        blk();
        blk();
        ok &= fx.dly_w != w0 && !ui.layer;
        ck("FX held + F5: CLOUD FREEZE, the line stands while held; let go, it runs again", ok);
        go_title("DELAY");
        turn(EN_K1, -1);
        for (t = 0; t < CL_CLEAR + 4u; t++)
            blk();
        btn_down(B_FX);
        frame();
        key_down(white(14));
        blk();
        w0 = fx.dly_w;
        blk();
        ok = fx1s.type == FX1_DELAY && (perf_held & PF_BIT(PF_CFRZ)) && !(perf_act & PF_BIT(PF_CFRZ)) &&
             !((perf_avail() >> PF_CFRZ) & 1u) && fx.dly_w != w0;
        key_up(white(14));
        frame();
        btn_up(B_FX);
        frame();
        transport_req = 2;
        blk();
        ck("  .. with the DELAY: CLOUD FREEZE does nothing (not available: dimmed on the map)", ok);
        (void)in;
    }
    /* kept with the project (the extras: opstore.c), Felucca's project: the DELAY */
    cloud_power_on();
    fx1s.type = FX1_CLOUD;
    fx1s.size = 66; fx1s.dens = 77; fx1s.spread = 12; fx1s.pitch = -5; fx1s.rnd = 33; fx1s.fdbk = 44; fx1s.mix = 55;
    project_save(1);
    fx1s.type = FX1_DELAY;
    fx1s.size = fx1s.dens = fx1s.spread = fx1s.rnd = fx1s.fdbk = fx1s.mix = 1;
    fx1s.pitch = 9;
    project_load(1);
    ok = fx1s.type == FX1_CLOUD && fx1s.size == 66u && fx1s.dens == 77u && fx1s.spread == 12u && fx1s.pitch == -5 &&
         fx1s.rnd == 33u && fx1s.fdbk == 44u && fx1s.mix == 55u;
    ck("FX1's TYPE and the CLOUD's settings: saved with the project, back with it", ok);
    memset(&xtra_slot[1], 0, sizeof xtra_slot[1]);
    project_load(1);
    ck("  .. a project without them (Felucca's): FX1 the DELAY, the CLOUD's defaults",
       fx1s.type == FX1_DELAY && fx1s.size == 50u && fx1s.mix == 80u && fx1s.pitch == 0);
    {
        uint32_t a = autosave_sig(), b;
        fx1s.dens = 13;
        b = autosave_sig();
        ck("  .. and the autosave sees them change", a != b);
    }
    if (!dir)
        return;
    cloud_power_on();
    go_title("DELAY");
    ui.force = 1;
    frame();
    screen_save(dir, "fx1-delay");
    {
        uint32_t t, i;
        static int32_t in[CTL];
        transport_req = 1;
        blk();
        for (t = 0; t < 30u; t++)
            blk();
        for (t = 0; t < 22050u; t += CTL) {           /* (the delay's line with something in it) */
            sine_block(in, t, 330.0, 9000.0 * (t % 11025u < 4000u));
            bus(in);
        }
        frame();
        screen_save(dir, "fx1-delay-line");
        turn(EN_K1, 1);
        fx1s.size = 60; fx1s.dens = 70; fx1s.spread = 70; fx1s.rnd = 30;
        to_cloud();
        for (t = 0; t < 2u * 44100u; t += CTL) {
            for (i = 0; i < CTL; i++)
                in[i] = (int32_t)lrint(9000.0 * sin(2 * M_PI * 220.0 * (t + i) / FS) * ((t / 5512u) % 3u != 2u));
            bus(in);
        }
        frame();
        screen_save(dir, "cloud");
        perf_act |= PF_BIT(PF_CFRZ);
        memset(in, 0, sizeof in);
        for (t = 0; t < 22050u; t += CTL)
            bus(in);
        ui.force = 1;
        frame();
        screen_save(dir, "cloud-frozen");
        perf_act = 0;
        go_title("DELAY");
        ui.force = 1;
        frame();
        screen_save(dir, "fx1-cloud");
        transport_req = 2;
    }
}

/* ---------------------------------------------------------------- demos --- */
/* a song with sends into FX1: an acid line (T1), chords (T2), drums (T4); bars of it dry, then through each CLOUD */
static void demo_song(void)
{
    static const uint8_t ACID[16] = {45, 45, 57, 45, 0, 48, 45, 55, 45, 0, 57, 52, 45, 48, 0, 50};
    static const uint8_t CH[2][4] = {{57, 60, 64, 67}, {53, 57, 60, 64}};
    track_t *t1 = &trk[0], *t2 = &trk[1], *td = &trk[3];
    uint32_t i;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_BPM] = 110;
    host_preset(t1, 0, 4);
    host_preset(t2, 1, 5);
    host_drums(td);
    t1->p[P_SLEN] = t2->p[P_SLEN] = td->p[P_SLEN] = 32;
    for (i = 0; i < 32u; i++) {
        uint8_t n = ACID[i % 16u], d[4], k = 0;
        put_step(t1, i, n ? 1u : 0u, &n, n ? ST_NOTE : ST_REST, 0);
        put_step(t2, i, i % 16u == 0u ? 4u : 0u, CH[i / 16u], i % 16u == 0u ? ST_NOTE : ST_TIE, 0);
        if (i % 4u == 0u) d[k++] = 36;
        if (i % 2u == 0u) d[k++] = 42;
        if (i % 8u == 4u) d[k++] = 38;
        put_step(td, i, k, d, k ? ST_NOTE : ST_REST, 0);
    }
    t1->p[P_DLY] = 70;
    t2->p[P_DLY] = 110;
    td->p[P_DLY] = 20;
    t1->p[P_PAN] = -30;
    t2->p[P_PAN] = 30;
}
static void demos(const char *dir)
{
    static const struct {
        const char *name, *what;
        uint8_t size, dens, spread, rnd, fdbk, mix;
        int8_t pitch;
        uint8_t freeze;                               /* CLOUD FREEZE held over the last third, the tracks silent */
    } D[] = {
        {"cloud_dry", "the song without the cloud (MIX 0), to compare", 50, 50, 30, 10, 0, 0, 0, 0},
        {"cloud_wash", "a wash: long grains from anywhere", 72, 80, 100, 25, 20, 95, 0, 0},
        {"cloud_shimmer", "PITCH +12 and FDBK: every pass an octave higher", 62, 75, 60, 10, 70, 90, 12, 0},
        {"cloud_down", "PITCH -12: dark and slow", 70, 60, 40, 15, 25, 95, -12, 0},
        {"cloud_glitch", "short grains, RAND 80: detuned, half of them backwards", 18, 92, 70, 80, 0, 90, 7, 0},
        {"cloud_freeze", "CLOUD FREEZE held, the song stops: the cloud plays on in what it holds", 65, 85, 100, 20, 0, 100, 0, 1},
    };
    static int32_t o[2 * CTL];
    uint32_t k, t, i, frames = 12u * 44100u;
    char path[600];
    for (k = 0; k < NELEM(D); k++) {
        FILE *f;
        cloud_power_on();
        demo_song();
        fx1s.type = FX1_CLOUD;
        fx1s.size = D[k].size; fx1s.dens = D[k].dens; fx1s.spread = D[k].spread; fx1s.rnd = D[k].rnd;
        fx1s.fdbk = D[k].fdbk; fx1s.mix = D[k].mix; fx1s.pitch = D[k].pitch;
        snprintf(path, sizeof path, "%s/%s.wav", dir, D[k].name);
        f = fopen(path, "wb");
        if (!f)
            continue;
        wav_hdr(f, frames);
        transport_req = 1;
        for (t = 0; t < frames; t += CTL) {
            if (D[k].freeze && t >= frames * 2u / 3u && t < frames * 2u / 3u + CTL) {
                perf_held |= PF_BIT(PF_CFRZ);
                transport_req = 2;                    /* (the song stops, the cloud plays on) */
            }
            mix_block(o, CTL);
            for (i = 0; i < CTL && t + i < frames; i++)
                wav_put(f, o[2 * i], o[2 * i + 1]);
        }
        perf_held = 0;
        transport_req = 2;
        mix_block(o, CTL);
        fclose(f);
        printf("cloud: demo %s (%s)\n", path, D[k].what);
    }
}

int main(int argc, char **argv)
{
    test_bus();
    test_grains();
    test_sound();
    test_ui(argc > 1 ? argv[1] : 0);
    if (argc > 2)
        demos(argv[2]);
    if (bad) {
        printf("cloud: %d FAILED\n", bad);
        return 1;
    }
    printf("cloud: all ok\n");
    return 0;
}
