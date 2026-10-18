/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's stores in USR3's and USR2's flash (firmware/src/opstore.c) against a NOR image, as
 * tests/persistence_test.c:
 *   EXTRAS   a project slot's FX TRACKS saved after it and loaded with it; after a power-on; a project without extras
 *            (Felucca's) or written by the editor since: empty lanes; a rename keeps them; a failed write says so; the
 *            autosave's with it; a song's sections get their slots' lanes; only USR3's sectors written.
 *   KEY      GLO > KEY's run (key.c) saved with a project's extras and loaded with it; an older project's: none.
 *   SCENES   a project slot's other scenes saved after it (USR2's flash, set A..D) and loaded with it, the one it held
 *            playing; after a power-on; the copies take turns, the newest whole one wins; a write cut short: SAVE ERROR,
 *            the copy before in charge (the project as it was: its scenes; another: none); a damaged copy or one whose
 *            bytes are no scenes: not taken; a project written by the editor since or another slot's: none; a rename
 *            keeps them; the autosave's (set 5) with it; only USR2's sectors written, each set its own.
 *   USR2/3   read as empty sample slots whatever their flash holds.
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
static uint32_t erases, lo_w = 0xFFFFFFFFu, hi_w;   /* (the lowest and highest byte erased or written) */
static uint32_t progs, scn_lo = 0xFFFFFFFFu, scn_hi;           /* (the programs; .. the lowest and highest in USR2) */
static void touched(uint32_t a, uint32_t n)
{
    lo_w = a < lo_w ? a : lo_w;
    hi_w = a + n > hi_w ? a + n : hi_w;
    if (a + n > 0xB4000u && a < 0xC8000u) {
        scn_lo = a < scn_lo ? a : scn_lo;
        scn_hi = a + n > scn_hi ? a + n : scn_hi;
    }
}
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    erases++;
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
    progs++;
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
    key_defaults();
    memset(&as, 0, sizeof as);
    memset(&scb, 0, sizeof scb);
    memset((void *)&scn, 0, sizeof scn);
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
    memset(&scb, 0, sizeof scb);                  /* (the pool and .bss zeroed: one scene) */
    memset((void *)&scn, 0, sizeof scn);
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
    {   /* GLO > KEY's run (key.c) with the project; a project's extras from before the KEY (XTRA_V3): none */
        static const uint8_t R[4] = {9, 5, 0, 7};
        uint32_t k;
        key_defaults();
        keyt.on = 1;
        keyt.len = 4;
        for (k = 0; k < 4u; k++)
            keyt.root[k] = R[k];
        ok = project_save(3) == 0;
        key_defaults();
        project_load(3);
        ok &= keyt.on == 1u && keyt.len == 4u && keyt.root[0] == 9u && keyt.root[3] == 7u && keyt.root[4] == KEY_NONE;
        xtra_slot[3].size = XTRA_V3;                  /* (as an older firmware wrote them) */
        key_defaults();
        keyt.on = 1;
        project_load(3);
        ok &= !keyt.on && keyt.len == 4u && keyt.root[0] == KEY_NONE;
        bad += check("KEY: ON, LEN and the roots saved and loaded with the project; older extras: none", ok);
    }
    bad += check("every write of the extras in its USR3 sectors, the projects' or their scenes' (0x97000..0xCFFFF)",
                 lo_w >= 0x97000u && hi_w <= 0xD0000u);
    return bad;
}

static void as_run(uint32_t ms)
{
    uint32_t end = fm1_ms + ms;
    while ((int32_t)(end - fm1_ms) > 0) {
        fm1_ms += 50u;
        autosave_poll();
    }
}
static int test_autosave(void)
{
    int bad = 0, ok;
    reset();
    autosave_boot(1);
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


/* the scene playing marked v: T1's step 1 the note v, T2's LEN 4 + v % 8 (a scene's timing is its own) */
static void scene_mark(uint32_t v)
{
    uint8_t n = (uint8_t)v;
    put_step(&trk[0], 0, 1, &n, ST_NOTE, 0);
    trk[1].p[P_SLEN] = (int16_t)(4u + v % 8u);
}
static int scene_is(uint32_t v)
{
    return trk[0].step[0].n == 1u && trk[0].step[0].note[0] == v && trk[1].p[P_SLEN] == (int16_t)(4u + v % 8u);
}
static void scenes_demo(void)                    /* scenes 1, 2, 3: 61, 62, 63 (stopped: each pick at once); 3 playing */
{
    scene_reset();
    scene_mark(61);
    scene_queue(1);
    scene_mark(62);
    scene_queue(2);
    scene_mark(63);
}
static int scenes_are(uint32_t a, uint32_t b, uint32_t c)   /* scene 3 playing, scenes 1 2 3 a b c; each picked, back */
{
    int ok = scn.cur == 2u && scene_is(c) && scene_used(0) && scene_used(1) && !scene_used(3) && !scene_used(15);
    ok &= scene_queue(0) == SQ_NOW && scene_is(a);
    ok &= scene_queue(1) == SQ_NOW && scene_is(b);
    ok &= scene_queue(2) == SQ_NOW && scene_is(c);
    return ok;
}
static int scenes_none(uint32_t c)               /* one scene: the project's own (c), playing as scene 1 */
{
    return scn.cur == 0u && !scb.used && scene_is(c) && !scene_used(1) && !scene_used(2);
}
static int test_scenes(void)
{
    int bad = 0, ok;
    uint32_t n, base;
    st_hdr_t *h;
    project_store_t other;
    reset();
    scenes_demo();
    scn_lo = 0xFFFFFFFFu; scn_hi = 0;
    ok = project_save(1) == 0 && msg_is_saved();
    h = (st_hdr_t *)(nor + 0xB8000u);
    ok &= h->magic == ST_MAGIC && h->type == OBJ_SCENES && h->slot == 0u && scn_lo >= 0xB8000u && scn_hi <= 0xBC000u;
    bad += check("SCENES: SAVE to slot B: its other scenes after it, in USR2's flash (set B's copy A, 0xB8000)", ok);
    scene_reset();
    scene_mark(5);
    power_on();
    project_load(1);
    bad += check("  LOAD B after a power-on: scene 3 playing (the project's own), scenes 1 and 2 as they were",
                 scenes_are(61, 62, 63));
    /* saved again: the other copy; the newest wins */
    scene_queue(0);
    scene_mark(71);
    scene_queue(2);
    ok = project_save(1) == 0 && ((st_hdr_t *)(nor + 0xBA000u))->type == OBJ_SCENES;
    power_on();
    project_load(1);
    ok &= scenes_are(71, 62, 63);
    bad += check("  saved again: into copy B (0xBA000); LOAD: the newest", ok);
    /* a write cut short: the scenes' commit record (the save's last program) */
    progs = 0;
    project_save(1);                              /* (as it is: the same programs as the save below) */
    n = progs;
    scene_queue(1);                               /* scene 2 changed, scene 3 playing again: the project the same */
    scene_mark(82);
    scene_queue(2);
    fail_after = (int)n - 1;
    ok = project_save(1) == 2 && msg_is("SAVE ERROR");
    fail_after = -1;
    power_on();
    project_load(1);
    ok &= scenes_are(71, 62, 63);
    bad += check("  a write cut short (its commit record): SAVE ERROR; LOAD: the copy before (scene 2 as it was)", ok);
    scene_mark(64);                               /* (scene 3 changed: the project another) */
    fail_after = (int)n - 1;
    project_save(1);
    fail_after = -1;
    power_on();
    project_load(1);
    bad += check("  .. a project saved anew whose scenes were cut short: the project alone (no scenes of another)",
                 scenes_none(64));
    /* a damaged copy; one whose CRC holds but whose bytes are no scenes */
    scenes_demo();
    project_save(1);
    project_save(1);                              /* (both copies this project's) */
    {
        st_hdr_t hh;
        uint32_t seq;
        int c = scb_newest(1, &hh, &seq);
        uint8_t *pl;
        nor[SCB_BASE(1, (uint32_t)c) + ST_PAYLOAD_OFF + sizeof(scb_hdr_t) + 3u] ^= 0x10u;   /* (a byte of scene 1's) */
        power_on();
        project_load(1);
        bad += check("  the newest copy damaged (a byte): LOAD takes the copy before", c >= 0 && scenes_are(61, 62, 63));
        c = 1 - c;                                /* (the copy taken: its CRC made to hold over bytes that are no scene) */
        h = (st_hdr_t *)(nor + SCB_BASE(1, (uint32_t)c));
        pl = nor + SCB_BASE(1, (uint32_t)c) + ST_PAYLOAD_OFF;
        pl[sizeof(scb_hdr_t)] = 0xFFu;            /* (scene 1's flags: no scene's) */
        h->crc = st_crc32(pl, h->len);
        h->hcrc = st_crc32(h, sizeof *h - 4u);
        power_on();
        project_load(1);
        bad += check("  a copy whose CRC holds but whose bytes are no scenes: not taken (the project alone)", scenes_none(63));
    }
    /* slot B written by the editor since (its backups write the project object): the scenes are another project's */
    scenes_demo();
    project_save(1);
    {
        uint8_t v = 50;
        put_step(&trk[2], 9, 1, &v, ST_NOTE, 0);
    }
    project_capture(&proj_scratch);
    proj_pack(&other, &proj_scratch);
    st_save(OBJ_PROJECT0 + 1, &other, sizeof other);
    power_on();
    project_load(1);
    bad += check("  slot B written since by the editor: its scenes not applied (the project alone)",
                 scenes_none(63) && trk[2].step[9].note[0] == 50u);
    /* each slot its own set: slot D holding the same project as C (the same bytes) has none of C's */
    scenes_demo();
    project_save(2);
    st_save(OBJ_PROJECT0 + 3, &proj_slot[2], sizeof proj_slot[2]);
    power_on();
    project_load(3);
    ok = scenes_none(63);
    project_load(2);
    ok &= scenes_are(61, 62, 63);
    bad += check("  slot D the same project as C (the same bytes): C's scenes stay C's (D: the project alone)", ok);
    /* a rename writes the project again: its scenes follow it */
    scenes_demo();
    project_save(1);
    project_rename(1, "SCENES");
    power_on();
    project_load(1);
    bad += check("  a RENAME writes the project again: its scenes stay its", scenes_are(61, 62, 63) &&
                 str_eq(proj_name, "SCENES"));
    /* the autosave's: set 5 */
    reset();
    autosave_boot(1);
    scenes_demo();
    scn_lo = 0xFFFFFFFFu; scn_hi = 0;
    as_run(11000u);
    ok = as.writes == 1u && ((st_hdr_t *)(nor + SCB_BASE(4, 0)))->type == OBJ_SCENES && scn_lo >= SCB_BASE(4, 0) &&
         scn_hi <= 0xC8000u;
    bad += check("autosave: the scenes changed: written after the project (set 5, 0xC4000)", ok);
    memset(&scb, 0, sizeof scb);
    memset((void *)&scn, 0, sizeof scn);
    scene_mark(5);
    ok = autosave_boot(1) == 1 && scenes_are(61, 62, 63);
    bad += check("  .. power-on: restored with the music, the scene playing and the others", ok);
    as_run(70000u);
    ok = as.writes == 1u;                         /* (scenes_are's picks and back again: nothing changed) */
    scene_queue(0);                               /* (a switch alone: the music and the bank both change) */
    as_run(11000u);
    memset(&scb, 0, sizeof scb);
    memset((void *)&scn, 0, sizeof scn);
    ok &= as.writes == 2u && autosave_boot(1) == 1 && scn.cur == 0u && scene_is(61) && scene_used(2);
    bad += check("  .. scenes picked and back: nothing written; a switch: a write, restored on the scene picked", ok);
    bad += check("every write of the scenes in USR2's flash (0xB4000..0xC7FFF)", scn_lo >= 0xB4000u && scn_hi <= 0xC8000u);
    return bad;
}

static int test_usr3(void)
{
    uint32_t k;
    int ok = 1;
    reset();
    memset((void *)SMP_USER_XIP(0), 0, sizeof(smp_user_hdr_t));
    for (k = 1; k < 3u; k++) {                    /* USR2, USR3: a valid sample's header in their flash */
        smp_user_hdr_t *h = (smp_user_hdr_t *)SMP_USER_XIP(k);
        memset(h, 0, sizeof *h);
        h->magic = SMP_USER_MAGIC;
        h->version = 1;
        h->nz = 1;
        h->data_len = 256;
        h->zone[0].n = 512;
        h->zone[0].le = 511;
        h->zone[0].rate = 65536;
        h->zone[0].hi = 127;
        smp_user_scan(k);
        ok &= !usr_nz[k];
    }
    return check("USR2, USR3: empty sample slots whatever their flash holds (they hold M-Vave-OP1's stores)", ok);
}

int main(void)
{
    int bad = 0;
    bad += test_extras();
    bad += test_autosave();
    bad += test_scenes();
    bad += test_usr3();
    if (bad) {
        printf("opstore: %d FAILED\n", bad);
        return 1;
    }
    printf("opstore: all ok\n");
    return 0;
}
