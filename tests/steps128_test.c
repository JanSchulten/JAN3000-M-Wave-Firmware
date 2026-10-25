/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's 128 steps a track (core.h NSTEP; 64 before), on the real sequencer, stores and UI sources (the stubs of
 * tests/ui_test.c, the RAM stores: FELUCCA_FLASH 0; tests/opstore_test.c has the flash's):
 *   PLAY     LEN 128 plays steps 65..128 and wraps to step 1; automation and a lock past step 64; an FX TRACK of LEN
 *            128; LEN 64 as before
 *   STORE    SAVE / LOAD keep steps 65..128, LEN, LEGATO, automation, locks and the lanes; what a firmware of 64 steps
 *            reads of the slot (steps 1..64, LEN 64, the records of steps 1..64); extras of before (no tail): steps
 *            65..128 empty, LEN 64; a song's slot plays its steps 65..128 (chain_prepare)
 *   SCENES   a scene holding nothing past 64 packs as before (no SCN_F_WIDE, 8-byte masks, one-byte places); one that
 *            does packs wide and round-trips; a wide one cut short or with a step mask past its bytes is refused
 *   UI       STEP's cursor on step 128 (bank 8), PRESETS walks it, the grid's pages; LEN's knob to 128; AUTOMATION's
 *            rows of step 100 and 128; the editor's INFO (64, then 58 01 0 1); screens (with a directory argument)
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int32_t s128_out[2 * CTL];
static void blk(void) { mix_block(s128_out, CTL); }

/* track 1: LEN 128 of 1/16, notes on steps 1, 65 (a TIE on 66), 101 and 128; PAN automated on step 81, REV locked on
 * step 128, the note on 101 LEGATO; FX1 of LEN 128 with an LPF on step 121 */
static track_t *long_pattern(void)
{
    track_t *t;
    ui_power_on();
    song.sel = 0;
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    t = &trk[0];
    track_defaults_steps(t);
    t->p[P_SLEN] = 128;
    t->p[P_SDIV] = 2;
    t->p[P_SSWING] = 0;
    t->p[P_SGATE] = 64;
    t->p[P_VOICE] = V_POLY;
    t->step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0};
    t->step[64] = (step_t){{62}, 1, ST_NOTE, 0, 0};
    t->step[65] = (step_t){{0}, 0, ST_TIE, 0, 0};
    t->step[100] = (step_t){{64}, 1, ST_NOTE, SF_ACCENT, 0};
    t->step[100].probability |= STEP_LEG;
    t->step[127] = (step_t){{67}, 1, ST_NOTE, 0, 0};
    motion_set_event(t, 80, P_PAN, 20);
    motion_set_event(t, 10, P_DLY, 30);
    motion_set_lock(t, 127, P_REV, 90);
    memset(fxt_lane, 0, sizeof fxt_lane);
    fxt_lane[0].len = 128;
    fxt_lane[0].step[120] = fxt_code(PF_LPF);
    return t;
}

static void test_play(void)
{
    track_t *t = long_pattern();
    uint32_t guard = 0, last = 0xFFFFu, wrapped = 0, heard = 0, pan = 0, rev = 0, back = 0, lpf = 0;
    transport_req = 1;
    blk();
    while (!wrapped && guard++ < 400000u) {          /* (128 steps of 1/16 at 120: 16 s) */
        blk();
        if (t->seq_idx != last) {
            if (last == 127u && t->seq_idx == 0u)
                wrapped = 1;
            last = t->seq_idx;
        }
        if (t->seq_n) {
            uint32_t n = t->seq_notes[0];
            heard |= (n == 60u) << 0 | (n == 62u && last >= 64u) << 1 | (n == 64u && last == 100u) << 2 |
                     (n == 67u && last == 127u) << 3;
        }
        if (last >= 81u && last < 127u)
            pan |= t->p[P_PAN] == 20;
        if (last == 127u)
            rev |= t->p[P_REV] == 90;
        if (fxt.idx[0] == 120u)
            lpf |= perf_fxt == PF_BIT(PF_LPF);
    }
    blk();
    back = t->p[P_REV] != 90;
    bad += check("LEN 128: steps 65..128 play (62 on 65, 64 on 101, 67 on 128), then step 1 again",
                 wrapped && heard == 15u);
    bad += check("  automation on step 81 sounds after it, the lock on step 128 there only", pan && rev && back);
    bad += check("  FX1 of LEN 128: its LPF on step 121", lpf);
    transport_req = 2;
    blk();
    t->p[P_SLEN] = 64;
    transport_req = 1;
    blk();
    for (guard = 0, last = 0; guard < 200000u; guard++) {
        blk();
        if (t->seq_idx > last)
            last = t->seq_idx;
        if (t->seq_idx == 0u && last == 63u)
            break;
    }
    bad += check("LEN 64: wraps after step 64, as before", last == 63u && guard < 200000u);
    transport_req = 2;
    blk();
}

static step_t s128_ref[NSTEP];
static motion_store_t s128_mo;
static fxt_lane_t s128_fx[FXT_N];
static void keep_ref(void)
{
    memcpy(s128_ref, trk[0].step, sizeof s128_ref);
    s128_mo = motion;
    memcpy(s128_fx, fxt_lane, sizeof s128_fx);
}
static void wipe(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        track_defaults_steps(&trk[k]);
        trk[k].p[P_SLEN] = 16;
    }
    memset(fxt_lane, 0, sizeof fxt_lane);
}
static int same_motion(const motion_store_t *a, const motion_store_t *b)
{
    uint32_t i, j, n = 0;
    if (a->count != b->count || a->on != b->on)
        return 0;
    for (i = 0; i < a->count; i++)                   /* (the order may differ: the project's records come first) */
        for (j = 0; j < b->count; j++)
            n += a->event[i].place == b->event[j].place && a->event[i].param == b->event[j].param &&
                 a->event[i].value == b->event[j].value;
    return n == a->count;
}
static uint32_t hi_empty(const step_t *s)
{
    uint32_t i, n = 0;
    for (i = NSTEP_FMT; i < NSTEP; i++)
        n += s[i].time == ST_REST && !s[i].n && !s[i].hit;
    return n == NSTEP - NSTEP_FMT;
}

static void test_store(void)
{
    project_t q;
    uint32_t i, lo = 0;
    int ok;
    long_pattern();
    keep_ref();
    ok = project_save(1) == 0;
    wipe();
    motion_clear(&trk[0]);
    project_load(1);
    ok &= !memcmp(trk[0].step, s128_ref, sizeof s128_ref) && trk[0].p[P_SLEN] == 128 &&
          same_motion(&motion, &s128_mo) && !memcmp(fxt_lane, s128_fx, sizeof s128_fx);
    bad += check("SAVE / LOAD: steps 65..128, LEN 128, LEGATO, automation and locks past 64, FX1's LEN 128 kept", ok);
    ok = proj_import(&q, &proj_slot[1], sizeof proj_slot[1]) && q.t[0].p[P_SLEN] == 64 && hi_empty(q.t[0].step) &&
         !memcmp(q.t[0].step, s128_ref, NSTEP_FMT * sizeof(step_t));
    for (i = 0; i < q.motion.count; i++)
        lo += MO_STEP(&q.motion.event[i]) < NSTEP_FMT;
    ok &= q.motion.count == 1u && lo == 1u && xtra_slot[1].fx[0].len == NSTEP_FMT;
    bad += check("  what a firmware of 64 steps reads there: steps 1..64, LEN 64, the record on step 11, FX1 LEN 64", ok);
    xtra_slot[1].size = XTRA_V6;                     /* (extras of before: no steps 65..128) */
    wipe();
    project_load(1);
    ok = hi_empty(trk[0].step) && trk[0].p[P_SLEN] == 64 && !memcmp(trk[0].step, s128_ref, NSTEP_FMT * sizeof(step_t)) &&
         motion.count == 1u && fxt_lane[0].len == NSTEP_FMT && !fxt_lane[0].step[120];
    bad += check("extras of before (no tail): steps 65..128 empty, LEN 64, the records of steps 1..64", ok);
    /* a song on slot B: its steps 65..128 and LEN with it */
    long_pattern();
    project_save(1);
    wipe();
    motion_clear(&trk[0]);
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(1, 1);
    ok = chain_prepare() == 0 && chain.source[1].timing[0][0] == 128 &&
         !memcmp(chain.source[1].step[0], s128_ref, sizeof s128_ref) && chain.source[1].fx[0].len == 128u &&
         chain.source[1].fx[0].step[120] == fxt_code(PF_LPF);
    for (i = lo = 0; i < chain.source[1].motion.count; i++)
        lo += MO_STEP(&chain.source[1].motion.event[i]) >= NSTEP_FMT;
    ok &= lo == 2u;
    bad += check("a SONG on slot B: its steps 65..128, LEN 128, FX1's and the records past 64 (chain_prepare)", ok);
    transport_req = 2;
    blk();
    chain_stop();
    chain_config.count = 0;
}

static chain_pattern_t s128_pat;
static void test_scenes(void)
{
    static uint8_t buf[SCN_CAP];
    scn_view_t v;
    uint8_t tm0[NTRK][5] = {{0}};
    uint32_t n, k;
    track_t *t;
    int ok;
    ui_power_on();
    for (k = 0; k < NTRK; k++) {
        track_defaults_steps(&trk[k]);
        trk[k].p[P_SLEN] = 16;
    }
    memset(fxt_lane, 0, sizeof fxt_lane);
    t = &trk[0];
    t->step[3] = (step_t){{60}, 1, ST_NOTE, 0, 0};
    scn_view_trk(&v);
    n = scn_encode(&v, buf, sizeof buf);
    ok = !(buf[0] & SCN_F_WIDE) && n == 1u + NTRK * 5u + 8u + 2u + 3u;
    bad += check("a scene of steps 1..64 packs as before: no SCN_F_WIDE, an 8-byte mask (34 bytes)", ok);
    motion_set_event(t, 5, P_PAN, 10);
    scn_view_trk(&v);
    n = scn_encode(&v, buf, sizeof buf);
    ok = !(buf[0] & SCN_F_WIDE) && buf[n - 3u] == (0u << 6 | 5u) && buf[n - 2u] == P_PAN;
    bad += check("  its automation record: one place byte, track << 6 | step", ok);
    motion_clear(t);
    t->step[3] = (step_t){{0}, 0, ST_REST, 0, 0};
    t->step[69] = (step_t){{60}, 1, ST_NOTE, 0, 0};
    t->p[P_SLEN] = 100;
    scn_view_trk(&v);
    n = scn_encode(&v, buf, sizeof buf);
    ok = (buf[0] & SCN_F_WIDE) && n == 1u + NTRK * 5u + 16u + 2u + 3u && buf[1] == 100u;
    bad += check("a step past 64: the wide form (16-byte masks), LEN 100 in its timing", ok);
    long_pattern();
    scn_view_trk(&v);
    n = scn_encode(&v, buf, sizeof buf);
    ok = (buf[0] & SCN_F_WIDE) && scn_decode(buf, n, &s128_pat, tm0) && s128_pat.timing[0][0] == 128 &&
         !memcmp(s128_pat.step[0], trk[0].step, sizeof s128_pat.step[0]) && same_motion(&s128_pat.motion, &motion) &&
         !memcmp(s128_pat.fx, fxt_lane, sizeof s128_pat.fx);
    bad += check("  the long pattern round-trips: steps, LEN 128, records past 64, FX1 of LEN 128", ok);
    ok = !scn_decode(buf, n - 1u, &s128_pat, tm0);
    buf[0] &= (uint8_t)~SCN_F_WIDE;                  /* (its masks read as 8 bytes: the bytes after are no scene) */
    ok &= !scn_decode(buf, n, &s128_pat, tm0);
    bad += check("  cut short, or read without its wide flag: refused", ok);
    motion_clear(&trk[0]);
}

static void test_ui(const char *dir)
{
    track_t *t = long_pattern();
    uint16_t rw[EV_ROWS];
    uint32_t n, r, s100 = 0, s128 = 0, r128 = 0;
    stop_transport();
    go_page(GR_ROLL);
    cursor_set(127);
    frame();
    bad += check("STEP: the cursor on step 128, bank 8", ui.cursor == 127u && ui.bank == 7u);
    if (dir)
        screen_save(dir, "steps128-step-128");
    turn(EN_PRESET, 1);
    frame();
    bad += check("  PRESETS on: step 1 again (LEN 128), bank 1", ui.cursor == 0u && ui.bank == 0u);
    t->p[P_SLEN] = 120;
    go_page(GR_ROLL);
    cursor_set(0);
    turn(EN_PRESET, -1);
    frame();
    bad += check("  LEN 120, back from step 1: step 120 (bank 8)", ui.cursor == 119u && ui.bank == 7u);
    t->p[P_SLEN] = 128;
    go_auto_top();
    frame();
    n = ev_list(rw);
    for (r = 0; r < n; r++)
        if (rw[r] >> 8 == EVK_REC) {
            uint32_t s = MO_STEP(&motion.event[rw[r] & 0xFFu]);
            s100 |= s == 80u;
            if (s == 127u) {
                s128 = 1;
                r128 = r;
            }
        }
    bad += check("AUTOMATION: the rows of steps 81 and 128", s100 && s128);
    if (dir) {
        ui.ev_row = (uint16_t)r128;
        ui.force = 1;
        frame();
        screen_save(dir, "steps128-automation");
    }
    go_page(GR_FXT);
    ui.fxt_cur = 120;
    frame();
    if (dir)
        screen_save(dir, "steps128-fxtracks");
    ui.home = 1;
    ui.force = 1;
    frame();
    if (dir)
        screen_save(dir, "steps128-home");
    motion_clear(t);
}

int main(int argc, char **argv)
{
    test_play();
    test_store();
    test_scenes();
    test_ui(argc > 1 ? argv[1] : 0);
    if (bad) {
        printf("steps128: %d FAILED\n", bad);
        return 1;
    }
    printf("steps128: all ok\n");
    return 0;
}
