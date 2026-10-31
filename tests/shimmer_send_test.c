/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* Independent SHIMMER: four track sends, simultaneous reverb, exact legacy DSP, UI, project / preset / scene
 * compatibility and automation. No hardware. Optional directory receives the SEND, INSERT and SHIMMER screens. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int bad;
static void ck(const char *s, int ok) { printf("shimmer: %-94s %s\n", s, ok ? "ok" : "FAIL"); bad += !ok; }
static void reset_shimmer(void)
{
    memset(&shim_hl, 0, sizeof shim_hl);
    memset(shim_comb, 0, sizeof shim_comb); memset(shim_ap, 0, sizeof shim_ap); memset(shim_shb, 0, sizeof shim_shb);
    shim_live = shim_quiet = 0;
}
static uint64_t mix_hash(int send, int spread)
{
    int32_t out[2 * CTL]; uint32_t i, k;
    uint64_t h = 1469598103934665603ull;
    ui_power_on(); reset_shimmer();
    memset(&hl, 0, sizeof hl); rev_clear(); memset(&fx, 0, sizeof fx);
    memset(dly_buf, 0, sizeof dly_buf); memset(cho_buf, 0, sizeof cho_buf); memset(&pf, 0, sizeof pf);
    memset(ins, 0, sizeof ins); memset(&mod, 0, sizeof mod); memset(sl, 0, sizeof sl);
    perf_held = perf_latched = 0; lim_env = LIM_T; dc_l = dc_r = dce_l = dce_r = 0;
    vage = 0; rng_state = 0x1234567u; mod_seed = 0x2545F491u;
    host_preset(&trk[0], 0, 0); song.master_q12 = 4096;
    for (i = 0; i < NTRK; i++) { trk[i].p[P_CHOR] = trk[i].p[P_DLY] = trk[i].p[P_REV] = 0; pf.mg[i] = 32768; }
    trk[0].p[P_SHIM] = send; trk[0].p[P_SPRD] = spread; trk[0].p[P_VOICE] = V_POLY;
    trk_note_on(&trk[0], 60, 100); trk_note_on(&trk[0], 67, 100);
    for (k = 0; k < 2000; k++) {
        if (k == 700) { trk_note_off(&trk[0], 60); trk_note_off(&trk[0], 67); }
        mix_block(out, CTL);
        for (i = 0; i < 2 * CTL; i++) h = (h ^ (uint32_t)out[i]) * 1099511628211ull;
    }
    return h;
}
static void test_audio(void)
{
    uint32_t b, i, k;
    int ok = 1, heard = 0;
    int32_t in[CTL], out[CTL], side[CTL], expected[CTL] = {0};
    ck("mixer routes SHIM to the master (mono and SPREAD paths)", mix_hash(0, 0) != mix_hash(100, 0) && mix_hash(0, 100) != mix_hash(100, 100));
    ui_power_on(); reset_shimmer();
    for (i = 0; i < CTL; i++) in[i] = (int32_t)i * 137 - 2000;
    memset(send_sh, 0, sizeof send_sh);
    for (k = 0; k < NTRK; k++) {
        trk[k].p[P_LEVEL] = 100 + k; trk[k].p[P_SHIM] = 30 + 20 * k;
        shimmer_send(&trk[k], in, CTL);
        for (i = 0; i < CTL; i++) expected[i] += mulq15(((in[i] >> 2) * LEVEL_Q12[100 + k]) >> 10, (30 + 20 * k) * 258);
    }
    ck("all four tracks feed one post-LEVEL send", !memcmp(expected, send_sh, sizeof expected));
    /* (M-Vave-OP1: the REVERB TYPE SHIMMER, this bus's reference, is retired.) The bus at PITCH +12: a 440 Hz burst's
     * tail (1 .. 2 s) holds its octave, 880 Hz, near the 440's; a HALL of the same SIZE / DAMP (the REVERB) does not */
    { static int32_t hs[2][2u * FS]; double a[2][2]; uint32_t m, f;
      for (m = 0; m < 2u; m++) {
        memset(&hl, 0, sizeof hl); rev_clear(); reset_shimmer(); fx.rtype = 2;
        song.g[G_RSIZE] = shim_p[0] = 90; song.g[G_RDAMP] = shim_p[1] = 60; shim_p[2] = 12; shim_p[3] = 127;
        for (b = 0; b < 2u * FS / CTL; b++) {
          for (i = 0; i < CTL; i++) in[i] = b < 300u ? (int32_t)(16000.0 * sin((b * CTL + i) * 2.0 * 3.141592653589793 * 440.0 / 44100.0)) : 0;
          memset(out, 0, sizeof out); memset(wet, 0, sizeof wet); memset(mix_l, 0, sizeof mix_l); memset(mix_r, 0, sizeof mix_r);
          if (m) { memcpy(send_sh, in, sizeof in); shim_live = 1; shimmer_bus(wet, CTL); }
          else { rev_hall(in, out, CTL); memcpy(wet, out, sizeof out); }
          for (i = 0; i < CTL; i++) { hs[m][b * CTL + i] = wet[i]; heard |= wet[i]; }
        }
        for (f = 0; f < 2u; f++) {                      /* 440, 880 Hz over 1 .. 2 s (Goertzel, +-10 % in 10 Hz steps) */
          double p = 0, g;
          for (g = (f ? 880.0 : 440.0) * 0.9; g <= (f ? 880.0 : 440.0) * 1.1; g += 10) {
            double w = 2 * 3.141592653589793 * g / FS, c = 2 * cos(w), s0, s1 = 0, s2 = 0; uint32_t t;
            for (t = FS; t < 2u * FS; t++) { s0 = hs[m][t] + c * s1 - s2; s2 = s1; s1 = s0; }
            p += s1 * s1 + s2 * s2 - c * s1 * s2;
          }
          a[m][f] = sqrt(p);
        }
      }
      ok = a[1][1] > 0.3 * a[1][0] && a[0][1] < 0.05 * a[0][0];
      { char w[160]; snprintf(w, sizeof w, "own bus at PITCH +12: its tail's octave 880 / 440 Hz %.2f, a HALL's %.3f", a[1][1] / a[1][0],
                              a[0][1] / a[0][0]);
        ck(w, ok && heard); } }
    { hall_t saved = shim_hl; int16_t comb[sizeof shim_comb / 2], ap[sizeof shim_ap / 2], pitch[SH_LEN];
      memcpy(comb, shim_comb, sizeof comb); memcpy(ap, shim_ap, sizeof ap); memcpy(pitch, shim_shb, sizeof pitch);
      rev_clear(); song.g[G_RTYPE] = 1; song.g[G_RSIZE] = 4; song.g[G_RDAMP] = 126;
      ck("reverb model clear / settings leave the independent shimmer state and buffers intact",
         !memcmp(&saved, &shim_hl, sizeof saved) && !memcmp(comb, shim_comb, sizeof comb) &&
         !memcmp(ap, shim_ap, sizeof ap) && !memcmp(pitch, shim_shb, sizeof pitch)); }
    /* Let the actual new bus decay at the corners (four full-scale sends); it must sleep only after exact silence. */
    for (k = 0; k < 8; k++) {
        static const int16_t pitch[4] = {-12, 0, 12, 24};
        char label[96];
        int32_t peak = 0; reset_shimmer(); shim_p[0] = 127; shim_p[1] = k & 1u ? 127 : 0; shim_p[2] = pitch[k / 2u];
        for (b = 0; b < 60000u; b++) {
            for (i = 0; i < CTL; i++) send_sh[i] = b < 2000u ? ((b * CTL + i) & 31u ? 131068 : -131068) : 0;
            if (b < 2000u) shim_live = 1;
            memset(wet, 0, sizeof wet); memset(mix_l, 0, sizeof mix_l); memset(mix_r, 0, sizeof mix_r);
            shimmer_bus(wet, CTL);
            for (i = 0; i < CTL; i++) { int32_t a = abs(wet[i]) + abs(mix_l[i]); if (a > peak) peak = a; }
        }
        snprintf(label, sizeof label, "SIZE / FDBK max, PITCH %d DAMP %d: bounded, then exact silence and idle", shim_p[2], shim_p[1]);
        ck(label, peak > 0 && peak < 1000000 && !shim_live);
    }
}
static void sh_test_chain(void)
{
    uint32_t i; int ok = 1;
    ui_power_on();
    for (i = 0; i < CTL; i++) { send_d[i] = 100; cho_wet[i] = 1000; shim_wet[i] = 200 + (int32_t)i; }
    chain_delay_in(CTL);
    for (i = 0; i < CTL; i++) ok &= send_d[i] == 1300 + (int32_t)i;
    ck("CHORUS and SHIMMER output feed DELAY before the delay renders", ok);
    for (i = 0; i < CTL; i++) { send_r[i] = 10; wet[i] = 2300; }
    { const int32_t *r = chain_reverb(send_r, wet, CTL);
      for (i = 0; i < CTL; i++) ok &= r[i] == 2510 + (int32_t)i; }
    ck("REVERB receives its own send, the SHIMMER's and what came before (CHORUS, DELAY: fx_buses' wet)", ok);
    (void)mix_hash(100, 0);
    { int d = 0, r = 0;
      for (i = 0; i < DLY_LEN; i++) d |= dly_buf[i];
      for (i = 0; i < sizeof rev_comb / sizeof rev_comb[0]; i++) r |= rev_comb[i];
      ck("SHIM alone reaches both downstream delay and reverb buffers through mix_block", d && r); }
    { hall_t h; int16_t pitch[SH_LEN]; int n, p; double low = 0, high = 0;
      for (p = 0; p < 2; p++) {
        memset(&h, 0, sizeof h); memset(pitch, 0, sizeof pitch); shim_p[2] = 7; shim_p[3] = p ? 127 : 32;
        for (n = 0; n < 1000; n++) {
          for (i = 0; i < HL_NC; i++) hw.l[i] = hw.r[i] = (int32_t)(8000 * sin((n * HL_NC + i) * 2 * 3.141592653589793 * 440 / 22050));
          shimmer_pitch(&h, pitch);
          if (n > 100) for (i = 0; i < HL_NC; i++) { double v = h.sh[i]; if (p) high += v * v; else low += v * v; }
        }
      }
      ck("FEEDBACK changes the pitch-shifted return gain", high > 14 * low && high < 17 * low);
      shim_p[3] = 0; shimmer_pitch(&h, pitch); ok = 1;
      for (i = 0; i < HL_NC; i++) ok &= h.sh[i] == 0;
      ck("FEEDBACK zero removes pitch-shifted feedback exactly", ok);
    }
}

static void test_ui(const char *dir)
{
    int ok; uint32_t k, sig;
    static const uint8_t ids[4] = {P_CHOR, P_SHIM, P_DLY, P_REV};
    ui_power_on(); frames(400); go_page(GR_FX);
    ok = !memcmp(cur_page()->id, ids, 4);
    for (k = 0; k < 4; k++) { int16_t v = TSEL->p[ids[k]]; turn(EN_K1 + k, 1); ok &= TSEL->p[ids[k]] > v; }
    ck("SEND faders are CHO / SHIM / DLY / REV; every knob edits its own send", ok);
    TSEL->p[P_SHIM] = 77; frame(); if (dir) screen_save(dir, "send-shimmer");
    track_select(1); ck("SHIM send belongs to the selected track", TSEL->p[P_SHIM] == 0 && trk[0].p[P_SHIM] == 77);
    go_page(GR_FX); turn(EN_SELECT, 1); frame();
    ok = str_eq(cur_page()->title, "MIX 2");
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "DRIVE");
    TSEL->p[P_ITYPE] = IT_SOFT;
    { int16_t a = TSEL->p[P_IA], b = TSEL->p[P_IB], c = TSEL->p[P_IC]; turn(EN_K2, 1); turn(EN_K3, 1); turn(EN_K4, 1);
      ok &= TSEL->p[P_IA] > a && TSEL->p[P_IB] > b && TSEL->p[P_IC] > c && TSEL->p[P_ITYPE] == IT_SOFT; }
    turn(EN_SELECT, 1); frame();
    ok &= str_eq(cur_page()->title, "CHORUS");
    for (k = 0; k < NPAGES; k++) ok &= !str_eq(PAGES[k].title, "DIST") && !str_eq(PAGES[k].title, "INSERT 2");
    ck("MIX 2, then the DRIVE after the sends: TYPE DRIVE TONE LEVEL on K1..K4, CHORUS next; no DIST or INSERT 2 page", ok);
    if (dir) screen_save(dir, "drive");
    go_page(GR_SHIM); sig = graph_signature();
    { int16_t sz = song.g[G_RSIZE], damp = song.g[G_RDAMP]; turn(EN_K1, 1); turn(EN_K2, -1);
      ck("SHIMMER SIZE / DAMP redraw independently of REVERB", shim_p[0] > 90 && shim_p[1] < 60 && sig != graph_signature() && song.g[G_RSIZE] == sz && song.g[G_RDAMP] == damp); }
    { int16_t pitch = shim_p[2], fb = shim_p[3]; sig = graph_signature();
      turn(EN_K3, -1); turn(EN_K4, -1);
      ck("PITCH and FEEDBACK have their own knobs and redraw", shim_p[2] < pitch && shim_p[3] < fb && graph_signature() != sig); }
    frame(); if (dir) screen_save(dir, "shimmer");
    track_select(2); ck("SHIMMER SIZE / DAMP are shared across tracks", shim_p[0] > 90 && shim_p[1] < 60);
}
static void test_stored(void)
{
    static project_t a, q; static project_store_t st; static uint8_t bytes[SCN_CAP]; static chain_pattern_t pat;
    up_rec_t u; proj_xtra_t x; static xtra_hi_t xh; scn_view_t view;
    int16_t v[P_COUNT], defs[P_COUNT]; uint32_t i, n, sig, pos, dst, span = P_COUNT + 2 + NSTEP_FMT * 9;
    ui_power_on();
    for (i = 0; i < NTRK; i++) trk[i].p[P_SHIM] = 20 + i * 25;
    shim_p[0] = 113; shim_p[1] = 47; shim_p[2] = -7; shim_p[3] = 98;
    motion_set_event(&trk[0], 2, P_SHIM, 97); motion_set_lock(&trk[1], 5, P_SHIM, 44);
    ck("SHIM supports automation and step locks", motion.count == 2 && motion_valid(&motion));
    project_capture(&a); ck("FUN10 round trip keeps all sends, automation and locks", proj_pack(&st, &a) && proj_import(&q, &st, sizeof st) && !memcmp(&a, &q, sizeof a));
    ck("project with extras saves", project_save_as(0, "SHIMMER") == 0);
    shim_p[0] = shim_p[1] = 0; trk[0].p[P_SHIM] = 0; project_load(0);
    ck("project load restores independent sends and global settings", trk[0].p[P_SHIM] == 20 && shim_p[0] == 113 && shim_p[1] == 47 && shim_p[2] == -7 && shim_p[3] == 98);
    sig = autosave_sig(); shim_p[1]++; ck("global SHIMMER edits trigger autosave", sig != autosave_sig());
    xtra_capture(&x, &xh, 1); x.shim_size = x.shim_damp = x.shim_pitch = x.shim_fdbk = x.fx_serial = 0; xtra_misc(&x, 1);   /* (written 0 before) */
    ck("older extras load safe SHIMMER defaults", shim_p[0] == 90 && shim_p[1] == 60 && shim_p[2] == 12 && shim_p[3] == 127);
    memset(&u, 0, sizeof u); u.np = P_COUNT; u.ver = UP_VER;
    for (i = 0; i < P_COUNT; i++) { defs[i] = TP[i].def; up_set_value(&u, i, trk[0].p[i]); }
    up_params(&u, v, defs); ck("user preset keeps the SHIM send", v[P_SHIM] == 20);
    /* Recreate the previous FUN10 layout: 111 parameters, no SHIM field; engine ids and their motion are old. */
    for (i = 0; i < NTRK; i++) motion_clear(&trk[i]); motion_set_event(&trk[0], 1, P_E0, param_desc_of(trk[0].eng_req, P_E0)->def); project_capture(&a); proj_pack(&st, &a);
    for (i = 0; i < NTRK; i++) {
        pos = 68 + i * span; dst = 68 + i * (span - 1);
        memmove(st.raw + dst, st.raw + pos, P_SHIM);
        memmove(st.raw + dst + P_SHIM, st.raw + pos + P_SHIM + 1, span - P_SHIM - 1);
    }
    pos = 68 + NTRK * span; dst = pos - NTRK;
    memmove(st.raw + dst, st.raw + pos, sizeof a.chain + sizeof(motion_disk_t));
    st.raw[dst + sizeof a.chain + 4 + 1] = 103;  /* motion_disk_t's 4-byte header, first record's id */
    st.raw[66] = 111; n = proj_hash(st.raw, sizeof st - 4); memcpy(st.raw + sizeof st - 4, &n, 4);
    ck("old 111-parameter project: SHIM zero, engine values and automation retain their meaning",
       proj_import(&q, &st, sizeof st) && !q.t[0].p[P_SHIM] && q.t[0].p[P_E0] == a.t[0].p[P_E0] && MOTION_ID(&q.motion.event[0]) == P_E0);
    for (i = 0; i < NTRK; i++) motion_clear(&trk[i]); motion_set_event(&trk[0], 1, P_E0, param_desc_of(trk[0].eng_req, P_E0)->def); motion_set_lock(&trk[1], 2, P_SHIM, 37);
    scn_view_trk(&view); n = scn_encode(&view, bytes, sizeof bytes);
    ck("scene codec keeps original engine wire ids and new SHIM locks", scn_param_put(P_E0) == 103 && scn_param_get(103) == P_E0 &&
       scn_param_put(P_SHIM | MOTION_LOCK) == (111 | MOTION_LOCK) && motion.count == 2 && scn_decode(bytes, n, &pat, view.tm) && !memcmp(&pat.motion, &motion, sizeof motion));
}

#ifndef SHIMMER_TEST_NO_MAIN
int main(int argc, char **argv)
{
    test_audio(); sh_test_chain(); test_ui(argc > 1 ? argv[1] : 0); test_stored();
    printf("shimmer: %s\n", bad ? "FAIL" : "all passed"); return !!bad;
}

#endif
