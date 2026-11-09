/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, MENU > USB DISK (firmware/src/usbdisk.c, msc.c, vdisk.c) on a NOR image, as tests/opstore_test.c, driven
 * the way a host drives a USB drive: command blocks (CBW) into msc_out, packets out of msc_in, as usb.c moves them.
 *   SCSI     TEST UNIT READY, INQUIRY (removable disk), READ CAPACITY (16384 blocks of 512), READ FORMAT CAPACITIES,
 *            MODE SENSE 6 / 10 (write-protected), a write refused (DATA PROTECT, its data taken), an unknown command
 *            (ILLEGAL REQUEST, zeros for its data), REQUEST SENSE after each, a read past the end refused, an eject
 *            (not ready), a load; every CSW's tag and residue
 *   DISK     the whole disk read with READ(10) (and a READ(6)) into build/vdisk/disk.img, then read back by
 *            tests/vdisk_check.py, a FAT16 reader of its own: MBR, boot sector, both FATs alike, the root, the files
 *   SONGS    a second set of files in build/vdisk/songs for web/library: scenes, a scene song, a SONG chain
 *   FILES    README.TXT names the slots; CURRENT.FM1 (the music now, written into the autosave first) and PROJ_A /
 *            PROJ_C.FM1 (the slots saved; B and D empty: no file), each an FM1X whose entries' bytes are the flash's
 *            own at their offsets (the checker compares them with build/vdisk/nor.bin), the sample USR1 in them
 * Built and run by tests/run_tests.sh. */
static unsigned char nor[0x100000];
#define SMP_USER_XIP(k) (nor + 0xA0000u + (k) * 0x14000u)
#define FELUCCA_DISK 1
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
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
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
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "TEST"
#endif
#define DISK_FLASH(off) ((const uint8_t *)nor + (off))
#define USBDISK_NO_SCREEN 1
#include "../firmware/src/usbdisk.c"

static int bad;
static int check(const char *what, int ok)
{
    printf("vdisk: %-98s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        bad++;
    return ok;
}

/* ---- a host's side of bulk-only: a CBW, the data, the CSW ---- */
static uint32_t tagn = 0x100;
static uint8_t csw[13];
/* one command: cb (its length 6..16), dir 0x80 IN / 0 OUT, want bytes; IN data into d (room: want); OUT data: want
 * bytes of 0xAA. The CSW in csw; its status returned, -1 a protocol error */
static int cmd(const uint8_t *cb, uint32_t cbn, uint32_t dir, uint32_t want, uint8_t *d)
{
    uint8_t w[31], p[64];
    uint32_t got = 0, t = ++tagn;
    int n;
    memset(w, 0, sizeof w);
    memcpy(w, "USBC", 4);
    w[4] = (uint8_t)t; w[5] = (uint8_t)(t >> 8); w[6] = (uint8_t)(t >> 16); w[7] = (uint8_t)(t >> 24);
    w[8] = (uint8_t)want; w[9] = (uint8_t)(want >> 8); w[10] = (uint8_t)(want >> 16); w[11] = (uint8_t)(want >> 24);
    w[12] = (uint8_t)dir;
    w[14] = (uint8_t)cbn;
    memcpy(w + 15, cb, cbn);
    if (!msc_out(w, 31))
        return -1;
    if (dir == 0x80u) {
        while (got < want) {
            n = msc_in(p);
            if (n <= 0 || (uint32_t)n > 64u || got + (uint32_t)n > want)
                return -1;
            memcpy(d + got, p, (uint32_t)n);
            got += (uint32_t)n;
            if (n < 64)
                break;                          /* a short packet ends the data */
        }
    } else {
        memset(p, 0xAA, sizeof p);
        while (got < want) {
            uint32_t k = want - got < 64u ? want - got : 64u;
            if (!msc_out(p, k))
                return -1;
            got += k;
        }
    }
    n = msc_in(csw);
    if (n != 13 || memcmp(csw, "USBS", 4) || csw[4] != (uint8_t)t || csw[5] != (uint8_t)(t >> 8))
        return -1;
    return csw[12];
}
static uint32_t residue(void) { return csw[8] | (uint32_t)csw[9] << 8 | (uint32_t)csw[10] << 16 | (uint32_t)csw[11] << 24; }
static int sense_is(uint32_t key, uint32_t asc)
{
    static const uint8_t RS[6] = {0x03, 0, 0, 0, 18, 0};
    uint8_t d[18];
    return cmd(RS, 6, 0x80, 18, d) == 0 && d[0] == 0x70 && d[2] == key && d[12] == asc;
}
static int read10(uint32_t lba, uint32_t n, uint8_t *d)
{
    uint8_t cb[10] = {0x28, 0, (uint8_t)(lba >> 24), (uint8_t)(lba >> 16), (uint8_t)(lba >> 8), (uint8_t)lba, 0,
                      (uint8_t)(n >> 8), (uint8_t)n, 0};
    return cmd(cb, 10, 0x80, n * 512u, d);
}

static void write_file(const char *path, const void *p, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(p, 1, n, f);
    fclose(f);
}
static void demo_project(uint32_t base)          /* a few notes on T1, a different one per project */
{
    uint32_t i;
    for (i = 0; i < 16u; i += 4u) {
        uint8_t n = (uint8_t)(base + i);
        put_step(&trk[0], i, 1, &n, ST_NOTE, 0);
    }
}

/* file k of the disk made (0 CURRENT, 1..4 A..D) as the drive hands it out: the header, the table, each entry's bytes */
static void dump_fm1(const char *dir, uint32_t k, const char *name)
{
    static const uint8_t pad[4];
    char path[512];
    uint32_t i;
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(&dsk.h[k], 1, sizeof dsk.h[k], f);
    fwrite(dsk.e[k], 1, dsk.h[k].n * sizeof dsk.e[k][0], f);
    for (i = 0; i < dsk.h[k].n; i++) {
        fwrite(DISK_FLASH(dsk.e[k][i].off), 1, dsk.e[k][i].len, f);
        fwrite(pad, 1, ((dsk.e[k][i].len + 3u) & ~3u) - dsk.e[k][i].len, f);
    }
    fclose(f);
}

static void rhythm(uint32_t mask)                /* T1: a note on each step of the mask's 16, nothing else */
{
    uint32_t i;
    uint8_t n = 60;
    for (i = 0; i < 16u; i++) {
        trk[0].step[i] = SCN_REST;
        if ((mask >> i) & 1u)
            put_step(&trk[0], i, 1, &n, ST_NOTE, 0);
    }
}

static uint8_t img[VD_SECTORS * 512u];

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/vdisk";
    static const uint8_t TUR[6] = {0}, INQ[6] = {0x12, 0, 0, 0, 36, 0}, CAP[10] = {0x25},
                         FMT[10] = {0x23, 0, 0, 0, 0, 0, 0, 0, 12, 0}, MS6[6] = {0x1A, 0, 0x3F, 0, 192, 0},
                         MS10[10] = {0x5A, 0, 0x3F, 0, 0, 0, 0, 0, 192, 0}, WR[10] = {0x2A, 0, 0, 0, 0, 40, 0, 0, 2, 0},
                         ODD[12] = {0xA0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0}, EJ[6] = {0x1B, 0, 0, 0, 2, 0},
                         LD[6] = {0x1B, 0, 0, 0, 3, 0}, PAST[10] = {0x28, 0, 0, 0, 0x40, 0x00, 0, 0, 1, 0},
                         RD6[6] = {0x08, 0, 0, 0, 1, 0};
    uint8_t d[256], s0[512];
    uint32_t lba, ok;
    char path[512];
    smp_user_hdr_t *sh = (smp_user_hdr_t *)SMP_USER_XIP(0);

    memset(nor, 0xFF, sizeof nor);
    host_tracks_init();
    chain_defaults(&chain_config);
    key_defaults();
    panel = PANEL_DEFAULT;
    demo_project(60);
    ok = project_save_as(0, "FIRST") == 0;
    demo_project(64);
    ok &= project_save_as(2, "THIRD") == 0;
    demo_project(67);                            /* the music now: neither slot's */
    str_cpy(proj_name, "LIVE", sizeof proj_name);
    memset(sh, 0, sizeof *sh);                   /* USR1: a header and 1000 bytes (as the editor's upload leaves it) */
    sh->magic = SMP_USER_MAGIC;
    sh->version = 1;
    sh->nz = 1;
    sh->data_len = 1000;
    memset(SMP_USER_XIP(0) + SMP_USER_DATA, 0x5A, 1000);
    usr_nz[0] = 1;
    check("the stage: projects A (FIRST) and C (THIRD) saved, the music now another (LIVE), USR1 holds a sample", ok);

    ok = autosave_sig() != as.saved && autosave_write(autosave_sig()) == 0 && autosave_sig() == as.saved;
    dsk_make();
    check("USB DISK's start: the music now written into the autosave (CURRENT.FM1's), the disk made", ok && dsk.made &&
          vd.nf == 4u);
    check("  the files: README.TXT, CURRENT.FM1, PROJ_A.FM1, PROJ_C.FM1 (B, D empty: none), one run each from cluster 2",
          !memcmp(vd.f[0].name, "README  TXT", 11) && !memcmp(vd.f[1].name, "CURRENT FM1", 11) &&
          !memcmp(vd.f[2].name, "PROJ_A  FM1", 11) && !memcmp(vd.f[3].name, "PROJ_C  FM1", 11) && vd.f[0].clus == 2u &&
          vd.f[1].clus == 2u + (vd.f[0].size + 511u) / 512u);
    check("  CURRENT.FM1: PROJ (the autosave's sector) XTRA SCNS SETS SMP1, the name LIVE; PROJ_A: slot A's, FIRST",
          dsk.h[0].n >= 4u && !memcmp(dsk.e[0][0].tag, "PROJ", 4) && (dsk.e[0][0].off == 0xE5000u ||
          dsk.e[0][0].off == 0xE6000u) && str_eq(dsk.h[0].name, "LIVE") && dsk.h[0].slot == 4u &&
          (dsk.e[1][0].off == 0x97000u || dsk.e[1][0].off == 0x98000u) && str_eq(dsk.h[1].name, "FIRST") &&
          dsk.h[1].slot == 0u && !memcmp(dsk.e[0][dsk.h[0].n - 1u].tag, "SMP1", 4) &&
          dsk.e[0][dsk.h[0].n - 1u].len == SMP_USER_DATA + 1000u);

    msc_reset();
    check("SCSI: TEST UNIT READY: good", cmd(TUR, 6, 0, 0, d) == 0);
    ok = cmd(INQ, 6, 0x80, 36, d) == 0 && d[0] == 0 && d[1] == 0x80 && !memcmp(d + 8, "M-VAVE  ", 8) && residue() == 0;
    check("  INQUIRY: a removable disk, M-VAVE FM-1 OP1 DISK", ok);
    ok = cmd(INQ, 6, 0x80, 255, d) == 0 && residue() == 255u - 36u;
    check("  INQUIRY with 255 asked: 36 bytes of it data (the residue says the rest is not)", ok);
    ok = cmd(CAP, 10, 0x80, 8, d) == 0 && d[0] == 0 && d[1] == 0 && d[2] == 0x3F && d[3] == 0xFF && d[6] == 2 && d[7] == 0;
    check("  READ CAPACITY: the last block 16383, 512 bytes", ok);
    ok = cmd(FMT, 10, 0x80, 12, d) == 0 && d[3] == 8 && d[6] == 0x40 && d[8] == 2 && d[10] == 2;
    check("  READ FORMAT CAPACITIES: one, 16384 blocks, formatted, 512", ok);
    ok = cmd(MS6, 6, 0x80, 192, d) == 0 && d[0] == 3 && (d[2] & 0x80u) && cmd(MS10, 10, 0x80, 192, d) == 0 && d[1] == 6 &&
         (d[3] & 0x80u);
    check("  MODE SENSE 6 / 10: write-protected", ok);
    ok = cmd(WR, 10, 0, 1024, d) == 1 && sense_is(7, 0x27) && msc.st == MS_CBW;
    check("  WRITE(10) of 2 blocks: its 1024 bytes taken, failed, DATA PROTECT (write protected)", ok);
    ok = cmd(ODD, 12, 0x80, 16, d) == 1 && residue() == 16u && sense_is(5, 0x20);
    check("  an unknown command (REPORT LUNS) with 16 bytes asked: zeros, failed, ILLEGAL REQUEST", ok);
    ok = cmd(PAST, 10, 0x80, 512, d) == 1 && sense_is(5, 0x21);
    check("  READ(10) past the end (block 16384): failed, LBA out of range", ok);
    ok = cmd(RD6, 6, 0x80, 512, s0) == 0 && residue() == 0 && s0[510] == 0x55 && s0[511] == 0xAA && s0[446 + 4] == 0x06;
    check("  READ(6) block 0: the MBR, one FAT16 partition", ok);
    ok = cmd(EJ, 6, 0, 0, d) == 0 && cmd(TUR, 6, 0, 0, d) == 1 && sense_is(2, 0x3A) && read10(0, 1, s0) == 1 &&
         cmd(LD, 6, 0, 0, d) == 0 && cmd(TUR, 6, 0, 0, d) == 0;
    check("  START STOP UNIT eject: not ready (medium not present), reads refused; a load: ready again", ok);
    ok = msc_out((const uint8_t *)"USBC", 4) == 1 && msc.st == MS_CBW && cmd(TUR, 6, 0, 0, d) == 0;
    check("  a packet that is no CBW: ignored, the next command as ever", ok);

    for (lba = 0, ok = 1; lba < VD_SECTORS && ok; lba += 128u)
        ok = read10(lba, 128, img + lba * 512u) == 0 && residue() == 0;
    check("DISK: the whole of it, READ(10) 128 blocks at a time (8 MiB)", ok);
    snprintf(path, sizeof path, "mkdir -p %s", dir);
    if (system(path)) return 1;
    snprintf(path, sizeof path, "%s/disk.img", dir);
    write_file(path, img, sizeof img);
    snprintf(path, sizeof path, "%s/nor.bin", dir);
    write_file(path, nor, sizeof nor);
    snprintf(path, sizeof path, "python3 tests/vdisk_check.py %s/disk.img %s/nor.bin %s/files", dir, dir, dir);
    check("  tests/vdisk_check.py: a FAT16 that reads, the files as made, each FM1X entry the flash's bytes",
          system(path) == 0);
    /* M-Vave-OP1, for web/library (its player renders what a project holds): a second disk in dir/songs, with songs,
     * told apart by T1's hits in a bar (a test counts them): project A as it is (4), C saved again with 8;
     * B "SCENES": scenes 1, 2 and 4 with 4, 8 and 2 hits, no song; D "SSONG": the same and a scene song (scene 2
     * twice, 1, 4); the music now "CHAIN": a SONG chain of project A once, then C twice (the chain plays slots: their
     * files are needed with it) */
    rhythm(0x5555);
    ok = project_save_as(2, "THIRD") == 0;
    rhythm(0x1111);
    ok &= scene_copy() == SC_OK && scn.cur == 1u;
    rhythm(0x5555);
    ok &= scene_queue(3) == SQ_NOW && scn.cur == 3u;
    rhythm(0x0101);
    ok &= scene_queue(0) == SQ_NOW && scn.cur == 0u;
    scene_settle();
    ok &= scene_used(0) && scene_used(1) && !scene_used(2) && scene_used(3);
    ok &= project_save_as(1, "SCENES") == 0;
    ssong.count = 3;
    ssong.scene[0] = 1; ssong.repeat[0] = 2;
    ssong.scene[1] = 0; ssong.repeat[1] = 1;
    ssong.scene[2] = 3; ssong.repeat[2] = 1;
    ok &= project_save_as(3, "SSONG") == 0;
    memset(&ssong, 0, sizeof ssong);
    chain_config.count = 2;
    memset(chain_config.row[0].slot, 0, NTRK); chain_config.row[0].repeat = 1;
    memset(chain_config.row[1].slot, 2, NTRK); chain_config.row[1].repeat = 2;
    str_cpy(proj_name, "CHAIN", sizeof proj_name);
    ok &= autosave_write(autosave_sig()) == 0;
    dsk_make();
    check("SONGS (for web/library): B three scenes, D with a scene song, the music now a SONG chain of A and C; the disk again",
          ok && dsk.made && vd.nf == 6u && str_eq(dsk.h[2].name, "SCENES") && str_eq(dsk.h[4].name, "SSONG") &&
          str_eq(dsk.h[0].name, "CHAIN"));
    snprintf(path, sizeof path, "mkdir -p %s/songs", dir);
    if (system(path)) return 1;
    snprintf(path, sizeof path, "%s/songs", dir);
    dump_fm1(path, 0, "CURRENT.FM1");
    dump_fm1(path, 1, "PROJ_A.FM1");
    dump_fm1(path, 2, "PROJ_B.FM1");
    dump_fm1(path, 3, "PROJ_C.FM1");
    dump_fm1(path, 4, "PROJ_D.FM1");
    printf(bad ? "vdisk: %d FAILED\n" : "vdisk: all ok\n", bad);
    return bad != 0;
}
