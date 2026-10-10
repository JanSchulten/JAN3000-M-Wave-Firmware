/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's stores in USR3's flash (firmware/src/opstore.c) against a NOR image, as tests/persistence_test.c:
 *   EXTRAS   a project slot's FX TRACKS saved after it and loaded with it; after a power-on; a project without extras
 *            (Felucca's) or written by the editor since: empty lanes; a rename keeps them; a failed write says so; the
 *            autosave's with it; a song's sections get their slots' lanes; only USR3's sectors written.
 *   TAPE     kept once changed and rested 3 s, stopped and quiet (not while playing, a key held, a note sounding); read
 *            back at power-on as it was (events, head, loop, speed, direction); the copies take turns, the newest wins;
 *            a write cut short or a damaged copy leaves the one before; one that breaks the tape's rules is not taken;
 *            none after a crash; the ISR leaves the tape alone while it is written.
 *   USR3     reads as an empty sample slot whatever its flash holds.
 * Built and run by tests/run_tests.sh. */
static unsigned char nor[0x100000];
#define SMP_USER_XIP(k) (nor + 0xA0000u + (k) * 0x14000u)
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
static void panel_setup(void) {}

static uint8_t flash_ok = 1;
static int fail_after = -1;
static uint32_t erases, tape_erases, lo_w = 0xFFFFFFFFu, hi_w;   /* (the lowest and highest byte erased or written) */
static void touched(uint32_t a, uint32_t n) { lo_w = a < lo_w ? a : lo_w; hi_w = a + n > hi_w ? a + n : hi_w; }
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    erases++;
    tape_erases += off >= 0xD2000u && off < 0xDC000u;
    touched(off, 4096);
    memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (fail_after == 0) return -9;
    if (fail_after > 0) fail_after--;
    touched(off, n);
    for (i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"

static int check(const char *what, int ok)
{
    printf("opstore: %-96s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

static int msg_is(const char *m) { return ui.msg_t && str_eq(ui.msg, m); }
static int msg_is_saved(void) { return msg_is("SAVED"); }
static void reset(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(xtra_slot, 0, sizeof xtra_slot);
    memset(&ui, 0, sizeof ui);
    memset(fxt_lane, 0, sizeof fxt_lane);
    memset(&tape, 0, sizeof tape);
    memset(tape_ev, 0, sizeof tape_ev);
    memset(&tps, 0, sizeof tps);
    memset(&as, 0, sizeof as);
    panel = PANEL_DEFAULT;
    host_tracks_init();
    fm1_ms = 0;
    transport_req = 0;
    flash_ok = 1;
    fail_after = -1;
    erases = 0;
}
static void lanes_demo(void)
{
    memset(fxt_lane, 0, sizeof fxt_lane);
    fxt_lane[0].step[0] = fxt_code(PF_LPF);
    fxt_lane[0].step[1] = FXT_TIE;
    fxt_lane[0].len = 8;
    fxt_lane[1].step[3] = fxt_code(PF_M1 + 2u);
    fxt_lane[1].len = 12;
}
static int lanes_are_demo(void)
{
    return fxt_lane[0].step[0] == fxt_code(PF_LPF) && fxt_lane[0].step[1] == FXT_TIE && fxt_lane[0].len == 8u &&
           fxt_lane[1].step[3] == (FXT_MUTE | 2u) && fxt_lane[1].len == 12u;
}
static int lanes_empty(void)
{
    static const fxt_lane_t z[FXT_N];
    return !memcmp(fxt_lane, z, sizeof z);
}
static void power_on(void)                       /* RAM lost: the stores as persist_boot reads them */
{
    uint32_t i;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(xtra_slot, 0, sizeof xtra_slot);
    memset(fxt_lane, 0, sizeof fxt_lane);
    for (i = 0; i < 4u; i++)
        proj_fetch(i);
    xtra_boot();
}

static int test_extras(void)
{
    int bad = 0, ok;
    project_store_t other;
    reset();
    lanes_demo();
    lo_w = 0xFFFFFFFFu; hi_w = 0;
    ok = project_save(1) == 0 && msg_is_saved();
    ok &= st_sector(OBJ_XPROJ0 + 1, 0) == 0xCA000u && ((st_hdr_t *)(nor + 0xCA000u))->type == OBJ_XPROJ0 + 1;
    bad += check("SAVE to slot B: the project, then its FX TRACKS at 0xCA000 (USR3's flash)", ok);
    memset(fxt_lane, 0, sizeof fxt_lane);
    project_load(1);
    bad += check("LOAD B: its FX TRACKS back", lanes_are_demo());
    power_on();
    project_load(1);
    bad += check("  .. after a power-on too", lanes_are_demo());
    memset(fxt_lane, 0, sizeof fxt_lane);
    project_save(2);                              /* (C: lanes empty) */
    lanes_demo();
    project_load(2);
    bad += check("a slot saved with empty lanes: LOAD empties them", lanes_empty());
    /* a project of Felucca's: written without extras (the project object alone) */
    lanes_demo();
    project_capture(&proj_scratch);
    proj_pack(&other, &proj_scratch);
    st_save(OBJ_PROJECT0 + 3, &other, sizeof other);
    power_on();
    lanes_demo();
    project_load(3);
    bad += check("a project with no extras (Felucca's): LOAD gives empty lanes", lanes_empty() && song.g[G_BPM] == proj_scratch.g[G_BPM]);
    /* slot B written by the editor (its backups write the project object): B's extras no longer belong to it */
    trk[0].step[5].note[0] = 77;
    trk[0].step[5].n = 1;
    project_capture(&proj_scratch);
    proj_pack(&other, &proj_scratch);
    st_save(OBJ_PROJECT0 + 1, &other, sizeof other);
    power_on();
    lanes_demo();
    project_load(1);
    bad += check("slot B written since by the editor: its old extras ignored, empty lanes", lanes_empty() &&
                 trk[0].step[5].note[0] == 77);
    lanes_demo();
    project_save(1);
    project_rename(1, "RENAMED");
    power_on();
    project_load(1);
    bad += check("a RENAME writes the project again: its FX TRACKS stay its", lanes_are_demo() && str_eq(proj_name, "RENAMED"));
    lanes_demo();
    fail_after = 2;                               /* (the project's writes pass, the extras' fail) */
    ok = project_save(0) == 2 && msg_is("SAVE ERROR");
    fail_after = -1;
    bad += check("the extras' write fails: SAVE ERROR", ok);
    /* a song gets each slot's lanes */
    lanes_demo();
    project_save(0);
    memset(fxt_lane, 0, sizeof fxt_lane);
    project_save(2);
    chain_config.count = 2;
    chain_config.row[0] = chain_row_of(0, 1);
    chain_config.row[1] = chain_row_of(2, 1);
    ok = chain_prepare() == 0 && chain.source[0].fx[0].step[0] == fxt_code(PF_LPF) && chain.source[0].fx[1].len == 12u &&
         !chain.source[2].fx[0].step[0] && chain.source[1].fx[0].len == 8u && !chain.source[3].fx[0].len &&
         !chain.source[3].fx[1].step[3];
    transport_req = 0;
    chain.armed = 0;
    bad += check("SONG: each slot's FX TRACKS copied (A, B: the demo; C: empty; D, Felucca's: empty)", ok);
    bad += check("every write of the extras in its USR3 sectors or the projects' (0x97000..0x9EFFF, 0xC8000..0xCFFFF)",
                 lo_w >= 0x97000u && hi_w <= 0xD0000u);
    return bad;
}

static void as_run(uint32_t ms)
{
    uint32_t end = fm1_ms + ms;
    while ((int32_t)(end - fm1_ms) > 0) {
        fm1_ms += 50u;
        autosave_poll();
        tape_store_poll();
    }
}
static int test_autosave(void)
{
    int bad = 0, ok;
    reset();
    autosave_boot(1);
    tape_store_boot(1);
    lanes_demo();
    as_run(11000u);
    ok = as.writes == 1u && ((st_hdr_t *)(nor + 0xD0000u))->type == OBJ_XAUTO;
    bad += check("autosave: the FX TRACKS changed: written (the project, then its extras at 0xD0000)", ok);
    memset(fxt_lane, 0, sizeof fxt_lane);
    ok = autosave_boot(1) == 1 && lanes_are_demo();
    bad += check("  .. power-on: restored with the music", ok);
    as_run(70000u);
    fxt_lane[0].step[5] = fxt_code(PF_FRZ);
    as_run(11000u);
    memset(fxt_lane, 0, sizeof fxt_lane);
    ok = as.writes == 2u && autosave_boot(1) == 1 && fxt_lane[0].step[5] == fxt_code(PF_FRZ);
    bad += check("  .. a lane's step alone changed: a write, restored", ok);
    return bad;
}

/* a tape with a few notes on tracks 1 and 3, the head in the middle, a loop, SPEED and DIR set */
static void tape_demo(void)
{
    uint32_t i;
    memset(tape_ev, 0, sizeof tape_ev);
    for (i = 0; i < 6u; i++)
        tape_ev[i] = te_make(i * 96u, i & 2u, 60u + i, 100, i & 1u);
    tape.g0 = 6;
    for (i = 0; i < 4u; i++)
        tape_ev[TAPE_EV - 4u + i] = te_make(1000u + i * 48u, 0, 70u + i, 90, i & 1u);
    tape.na = 4;
    tape.head = 800;
    tape.loop = 2;
    tape.loop_in = TAPE_BAR;
    tape.speed = -3;
    tape.rev = tape.dir = 0;
    tape.gen++;
}
static uint32_t tape_snap[TAPE_EV];
static int tape_is(const uint32_t *ev, uint32_t g0, uint32_t na, uint32_t head)
{
    uint32_t i;
    if (tape.g0 != g0 || tape.na != na || tape.head != head)
        return 0;
    for (i = 0; i < g0; i++)
        if (tape_ev[i] != ev[i]) return 0;
    for (i = TAPE_EV - na; i < TAPE_EV; i++)
        if (tape_ev[i] != ev[i]) return 0;
    return 1;
}
static void tape_power_on(void)
{
    memset(&tape, 0, sizeof tape);
    memset(tape_ev, 0, sizeof tape_ev);
    memset(&tps, 0, sizeof tps);
}
static int test_tape(void)
{
    int bad = 0, ok;
    uint32_t e0;
    reset();
    tape_power_on();
    ok = tape_store_boot(1) == 0 && tps.cur == 0xFFu;
    tape_erases = 0;
    as_run(20000u);
    bad += check("tape: none in flash: an empty tape; nothing changes: nothing written", ok && !tps.writes && !tape_erases);
    tape_demo();
    memcpy(tape_snap, tape_ev, sizeof tape_snap);
    song.playing = 1;
    as_run(20000u);
    ok = !tps.writes;
    song.playing = 0;
    fm1_in.notes = 1u << 3;
    as_run(20000u);
    ok &= !tps.writes;
    fm1_in.notes = 0;
    trk[2].v[0].active = 1;
    as_run(20000u);
    ok &= !tps.writes;
    trk[2].v[0].active = 0;
    bad += check("tape changed: no write while playing, a key held or a note sounding", ok);
    lo_w = 0xFFFFFFFFu; hi_w = 0;
    e0 = tape_erases;
    as_run(2800u);
    ok = !tps.writes;
    as_run(500u);
    bad += check("  .. stopped and quiet 3 s: written once (five sectors, in 0xD2000..0xD6FFF), the ISR let go of it",
                 ok && tps.writes == 1u && tape_erases - e0 == 5u && lo_w >= 0xD0000u && hi_w <= 0xD7000u && !tape.hold &&
                 tps.cur == 0u && ((st_hdr_t *)(nor + 0xD2000u))->type == OBJ_TAPE);
    as_run(60000u);
    bad += check("  .. unchanged: no more writes", tps.writes == 1u);
    tape_power_on();
    ok = tape_store_boot(1) == 1 && tape_is(tape_snap, 6, 4, 800) && tape.loop == 2u && tape.loop_in == TAPE_BAR &&
         tape.speed == -3 && !tape.rev;
    bad += check("power-on: the tape as it was kept (events, head, LOOP, SPEED, DIR)", ok);
    tape.rev = tape.dir = 1;
    tape_ev[2] = te_make(2u * 96u, 1, 99, 127, 0);
    tape.gen++;
    memcpy(tape_snap, tape_ev, sizeof tape_snap);
    lo_w = 0xFFFFFFFFu; hi_w = 0;
    as_run(4000u);
    ok = tps.writes == 1u && tps.cur == 1u && ((st_hdr_t *)(nor + 0xD7000u))->type == OBJ_TAPE && hi_w <= 0xDC000u;
    tape_power_on();
    ok &= tape_store_boot(1) == 1 && tape_ev[2] == tape_snap[2] && tape.rev == 1u && tape.dir == 1u && tps.cur == 1u;
    bad += check("the next change into copy B (0xD7000..0xDBFFF); power-on: the newest", ok);
    tape_ev[3] = te_make(3u * 96u, 3, 50, 64, 1);
    tape.gen++;
    fail_after = 30;                              /* (cut short among the events: copy A half written) */
    as_run(4000u);
    fail_after = -1;
    ok = tps.err && !tape.hold;
    tape_power_on();
    ok &= tape_store_boot(1) == 1 && tape_is(tape_snap, 6, 4, 800) && tape.rev == 1u;
    bad += check("a write cut short: an error, the ISR let go; power-on: the copy before (B)", ok);
    nor[0xD7000u + 4096u + 100u] ^= 0x10u;        /* B's events damaged, A torn: nothing whole */
    tape_power_on();
    ok = tape_store_boot(1) == 0 && !tape.g0 && !tape.na && !tape.head;
    bad += check("  .. B damaged too: an empty tape", ok);
    reset();
    tape_power_on();
    tape_store_boot(1);
    tape_demo();
    tape.gen++;
    as_run(4000u);
    tape_ev[1] = te_make(5000u, 0, 61, 100, 0);   /* out of order before the head: breaks the tape's rules */
    tape.gen++;
    memcpy(tape_snap, tape_ev, sizeof tape_snap);
    as_run(4000u);                                /* (B: whole, its CRC right, but not a tape) */
    tape_power_on();
    ok = tape_store_boot(1) == 1 && tape_ev[1] != tape_snap[1] && tps.cur == 0u;
    bad += check("a copy whose events break the tape's order is not taken: the one before", ok);
    tape_power_on();
    ok = tape_store_boot(0) == 0 && !tape.g0 && !tape.na && tps.cur == 0u;
    bad += check("after a crash (not allowed): an empty tape, nothing read; the next write goes to the other copy", ok);
    tape.hold = 1;
    tape.req_clear = 1;
    tape.req_seek = 100u + 1u;
    events_block(CTL);
    ok = tape.req_clear && tape.req_seek && !tape_rolling();
    tape.hold = 0;
    events_block(CTL);
    ok &= !tape.req_clear && !tape.req_seek && tape.head == 100u;
    bad += check("while being written (hold): the ISR leaves the tape (its requests wait, it does not roll)", ok);
    return bad;
}

static int test_usr3(void)
{
    smp_user_hdr_t *h = (smp_user_hdr_t *)SMP_USER_XIP(2);
    uint32_t k;
    reset();
    for (k = 0; k < 2u; k++) {
        smp_user_hdr_t *u = (smp_user_hdr_t *)SMP_USER_XIP(k);
        memset(u, 0, sizeof *u);
    }
    memset(h, 0, sizeof *h);
    h->magic = SMP_USER_MAGIC;
    h->version = 1;
    h->nz = 1;
    h->data_len = 256;
    h->zone[0].n = 512;
    h->zone[0].le = 511;
    h->zone[0].rate = 65536;
    h->zone[0].hi = 127;
    smp_user_scan(2);
    return check("USR3: an empty sample slot whatever its flash holds (it holds M-Vave-OP1's stores)", !usr_nz[2]);
}

int main(void)
{
    int bad = 0;
    bad += test_extras();
    bad += test_autosave();
    bad += test_tape();
    bad += test_usr3();
    if (bad) {
        printf("opstore: %d FAILED\n", bad);
        return 1;
    }
    printf("opstore: all ok\n");
    return 0;
}
