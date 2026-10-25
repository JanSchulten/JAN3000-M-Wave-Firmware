/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, MENU > SYSTEM > USB DISK: the FM-1 shows itself as a USB drive (read-only) whose files are its projects,
 * so a phone's Files app or a computer copies them out without any app (usb.c msc on vdisk.c's made-up disk):
 *   README.TXT    what is on the disk: the slots and their names, the firmware
 *   CURRENT.FM1   the music as it is now (written into the autosave first: what a power-on with RESTORE LAST brings)
 *   PROJ_A.FM1 .. PROJ_D.FM1   the project slots that hold one
 * A .FM1 file (FM1X, below) holds the flash as it is, object by object: each one's commit record and payload from its
 * sector's start (storage.c: the record at 0, the payload from 256), its flash offset with it. So nothing is converted,
 * whatever the format: a reader (the browser emulator, web/emu: the same firmware) writes the bytes back where they
 * were and loads the slot, or boots into the autosave. The objects: the project (or the autosave), its extras
 * (opstore.c: FX TRACKS, KEY, the steps 65..128, the scene song ..), its scenes' set, and what a project's sound may
 * need besides: the settings (TUNE's neighbours, SPEAKER EQ ..), the user sample USR1, FM6's voice bank. An object
 * that is not there (never written, damaged) is left out.
 * The disk is made once, when USB DISK starts (CRCs of what is read: a few hundred KB through XIP), then it only reads:
 * nothing writes the flash while it is on (this loop holds the main loop, as CALIBRATION does: the transport stopped,
 * no autosave, no editor). Any button or key ends it: USB comes back as MIDI + audio (+ the console). */
#ifndef DISK_FLASH
#define DISK_FLASH(off) fm1_xip_ptr(off)        /* host tests: their NOR image */
#endif
#define FM1X_VER 1u
#define FM1X_NENT 6u
typedef struct {
    uint8_t magic[4];                           /* "FM1X" */
    uint16_t ver, n;                            /* FM1X_VER; the entries */
    uint32_t size;                              /* the whole file */
    uint8_t slot, rsv0[3];                      /* 0..3 project A..D; 4 the music as it is now (the autosave) */
    char fw[16];                                /* FELUCCA_VERSION */
    char name[16];                              /* the project's name, 0-padded ("" none) */
    uint32_t tcrc;                              /* CRC-32 of the entries that follow */
    uint8_t rsv[12];
} fm1x_hdr_t;
typedef struct {
    uint8_t tag[4];                             /* PROJ XTRA SCNS SETS SMP1 FMVB */
    uint32_t off, len, crc;                     /* the flash extent [off, off + len) and its CRC-32; its bytes follow the
                                                 * table in the entries' order, each padded to 4 */
} fm1x_ent_t;
_Static_assert(sizeof(fm1x_hdr_t) == 64u && sizeof(fm1x_ent_t) == 16u, "FM1X layout");
#define DISK_NF 5u                              /* CURRENT + A..D */
static struct {
    fm1x_hdr_t h[DISK_NF];
    fm1x_ent_t e[DISK_NF][FM1X_NENT];
    char readme[640];
    uint8_t made;                               /* the disk is made: usb.c may show it */
} dsk __attribute__((section(".pool")));

#ifndef USBDISK_SCREEN_ONLY                     /* (tests/vdisk_screen_test.c: the screen alone, on the UI harness) */
static void dsk_ent(fm1x_hdr_t *h, fm1x_ent_t *e, const char *tag, uint32_t off, uint32_t len)
{
    fm1x_ent_t *x = &e[h->n];
    if (!len || h->n >= FM1X_NENT)
        return;
    memcpy(x->tag, tag, 4);
    x->off = off;
    x->len = len;
    x->crc = st_crc32(DISK_FLASH(off), len);
    h->n++;
}
/* a storage object's current copy as an extent: its sector's start, the record and its payload; 0 none */
static uint32_t dsk_obj(uint32_t obj, uint32_t *off)
{
    st_hdr_t h;
    int c = st_current(obj, &h);
    if (c < 0)
        return 0;
    *off = st_sector(obj, (uint32_t)c);
    return ST_PAYLOAD_OFF + h.len;
}
static uint32_t dsk_scenes(uint32_t s, uint32_t *off)   /* scene set s's newest whole copy (opstore.c) */
{
    st_hdr_t h;
    uint32_t seq;
    int c = scb_newest(s, &h, &seq);
    if (c < 0)
        return 0;
    *off = SCB_BASE(s, (uint32_t)c);
    return ST_PAYLOAD_OFF + h.len;
}
/* file k (0 CURRENT, 1..4 slot A..D) into vd: its header, its entries, their bytes */
static void dsk_file(uint32_t k, const char *fname, const char *pname)
{
    static const uint8_t pad[4];
    fm1x_hdr_t *h = &dsk.h[k];
    fm1x_ent_t *e = dsk.e[k];
    uint32_t off = 0, len, i, size;
    memset(h, 0, sizeof *h);
    memcpy(h->magic, "FM1X", 4);
    h->ver = FM1X_VER;
    h->slot = (uint8_t)(k ? k - 1u : 4u);
    str_cpy(h->fw, FELUCCA_VERSION, sizeof h->fw);
    str_cpy(h->name, pname, sizeof h->name);
    len = dsk_obj(k ? OBJ_PROJECT0 + k - 1u : OBJ_AUTOSAVE, &off);
    if (!len)
        return;                                 /* (no project: no file) */
    dsk_ent(h, e, "PROJ", off, len);
    len = dsk_obj(k ? OBJ_XPROJ0 + k - 1u : OBJ_XAUTO, &off);
    dsk_ent(h, e, "XTRA", off, len);
    len = dsk_scenes(k ? k - 1u : 4u, &off);
    dsk_ent(h, e, "SCNS", off, len);
    len = dsk_obj(OBJ_SETTINGS, &off);
    dsk_ent(h, e, "SETS", off, len);
    {                                           /* USR1, as the editor's backup takes it (editor_backup.c id 32) */
        const smp_user_hdr_t *sh = (const smp_user_hdr_t *)smp_user_xip(0);
        if (usr_nz[0] && sh->magic == SMP_USER_MAGIC && sh->data_len <= SMP_USER_SIZE - SMP_USER_DATA)
            dsk_ent(h, e, "SMP1", SMP_USER_BASE, SMP_USER_DATA + sh->data_len);
    }
    {
        st_hdr_t vh;
        if (fvb_valid(FVB_HOME, 0, &vh) == 0)
            dsk_ent(h, e, "FMVB", FVB_HOME, FVB_INFO + FVB_LEN);
    }
    size = sizeof *h + h->n * (uint32_t)sizeof *e;
    for (i = 0; i < h->n; i++)
        size += (e[i].len + 3u) & ~3u;
    h->size = size;
    h->tcrc = st_crc32(e, h->n * (uint32_t)sizeof *e);
    if (vd_begin(fname))
        return;
    vd_add(h, sizeof *h);
    vd_add(e, h->n * (uint32_t)sizeof *e);
    for (i = 0; i < h->n; i++) {
        vd_add(DISK_FLASH(e[i].off), e[i].len);
        vd_add(pad, ((e[i].len + 3u) & ~3u) - e[i].len);
    }
    vd_end();
}
static void dsk_line(char *b, uint32_t n, const char *s)   /* append s to b (n its room) */
{
    uint32_t l = str_len(b);
    str_cpy(b + l, s, n - l);
}
/* the disk: README first, then the music now, then the slots */
static void dsk_make(void)
{
    static const char *const FN[DISK_NF] = {"CURRENT.FM1", "PROJ_A.FM1", "PROJ_B.FM1", "PROJ_C.FM1", "PROJ_D.FM1"};
    char *r = dsk.readme, nm[4][PROJ_NAME_LEN + 1u];
    uint32_t k, n = sizeof dsk.readme, used = 0;
    memset(nm, 0, sizeof nm);
    for (k = 0; k < 4u; k++)
        if (project_used(k)) {                  /* (its name: proj_scratch, as project_used leaves it) */
            used |= 1u << k;
            memcpy(nm[k], proj_scratch.name, PROJ_NAME_LEN);
        }
    r[0] = 0;
    dsk_line(r, n, "M-Vave-OP1 " FELUCCA_VERSION " (based on Felucca): USB DISK, read only\r\n\r\n");
    dsk_line(r, n, "CURRENT.FM1  the music as it is now");
    for (k = 0; k < 5u; k++) {
        const char *p = k ? nm[k - 1u] : proj_name;
        char l[24] = "PROJ_A.FM1   project A";
        if (k && !((used >> (k - 1u)) & 1u))
            continue;
        if (k) {
            l[5] = l[21] = (char)('A' + k - 1u);
            dsk_line(r, n, l);
        }
        if (p[0]) {
            dsk_line(r, n, ": ");
            dsk_line(r, n, p);
        }
        dsk_line(r, n, "\r\n");
    }
    dsk_line(r, n, "\r\nA .FM1 file is a project as the FM-1 keeps it in its flash (FM1X: firmware/src/usbdisk.c).\r\n");
    vd_clear();
    if (!vd_begin("README.TXT")) {
        vd_add(r, str_len(r));
        vd_end();
    }
    dsk_file(0, FN[0], proj_name);
    for (k = 0; k < 4u; k++)
        if ((used >> k) & 1u)
            dsk_file(k + 1u, FN[k + 1u], nm[k]);
    dsk.made = 1;
}
#endif

#ifndef USBDISK_NO_SCREEN                       /* (tests/vdisk_test.c: the disk alone) */
/* ---- the USB DISK screen and its loop (MENU > USB DISK, ui_menu.c) ---- */
static void dsk_title(void)
{
    static const char *const FN[DISK_NF] = {"CURRENT.FM1", "PROJ_A.FM1", "PROJ_B.FM1", "PROJ_C.FM1", "PROJ_D.FM1"};
    uint32_t k, y = 76;
    lcd_fill(0, 0, 240, 240, T_BG);
    {   /* the title with its icon, centred together (as CALIBRATION's) */
        const char *t = "USB DISK";
        int32_t x = (240 - (16 + 6 + text_w(&AF_M, t))) / 2;
        cv_begin(240, 24, T_BG);
        GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H | AL_N(2), "usb disk title centred");
        GFX_HOOK_ALIGN(0, HEAD_MY + AF_M_CAP_Y, 0, HEAD_MY + AF_M_CAP_Y + AF_M_CAP_H, AL_V | AL_PASS,
                       "header icon on its title's line");
        cv_icon_mid(x, 12, 16, ICON_X_FOLDER, T_THEME, T_BG);
        cv_text(x + 22, HEAD_MY, &AF_M, t, T_TEXT);
        cv_blit(0, 5);
    }
    draw_text_box(0, 32, 240, &AF_S, "CONNECT A PHONE OR A COMPUTER", T_MID, 1);
    lcd_fill(16, 56, 208, 1, T_LINE);
    for (k = 0; k < DISK_NF; k++)               /* the files the disk has (README.TXT besides) */
        if (dsk.h[k].n) {
            char l[32];
            str_cpy(l, FN[k], sizeof l);
            if (dsk.h[k].name[0]) {
                str_cpy(l + str_len(l), "  ", sizeof l - str_len(l));
                str_cpy(l + str_len(l), dsk.h[k].name, sizeof l - str_len(l));
            }
            draw_text_box(0, y, 240, &AF_S, l, T_TEXT, 1);
            y += 18;
        }
    lcd_fill(16, 172, 208, 1, T_LINE);
    draw_text_box(0, 206, 240, &AF_S, "ANY BUTTON: BACK", T_DIM, 1);
}
static void dsk_status(const char *s)
{
    lcd_fill(0, 180, 240, 20, T_BG);
    draw_text_box(0, 180, 240, &AF_M, s, T_THEME, 1);
}
#if !defined USBDISK_NO_RUN && !defined USBDISK_SCREEN_ONLY
static void usb_disk_run(void)
{
    uint32_t t = 0, reads = 0, t_read = 0;
    const char *shown = 0;
    ed_flash_stop();                            /* (the transport stopped: an autosave's erase is not heard) */
    if (flash_ok && autosave_sig() != as.saved)
        autosave_write(autosave_sig());         /* CURRENT.FM1: the music as it is now */
    dsk_make();
    scr_wake_now();
    dsk_title();
    usb_disk_switch(1);                         /* (off the bus now; attached as the disk within a second) */
    while (fm1_in.buttons)                      /* OCT+ that started it let go first */
        fm1_wdt_feed();
    fm1_input_edges(0);
    for (;;) {
        const char *s;
        fm1_wdt_feed();
        usb_retry(fm1_ms);
        if (fm1_input_edges(0) || fm1_in.notes)
            break;
        if (fm1_ms - t < 100u)
            continue;
        t = fm1_ms;
        if (msc.reads != reads) {
            reads = msc.reads;
            t_read = fm1_ms;
        }
        s = msc.ejected ? "EJECTED: UNPLUG" : !usb.config || usb.suspended ? "WAITING FOR USB" :
            fm1_ms - t_read < 400u ? "READING" : "CONNECTED";
        if (s != shown)
            dsk_status(s);
        shown = s;
    }
    usb_disk_switch(0);                         /* MIDI + audio again (usb_retry, from the main loop) */
    dsk.made = 0;
    while (fm1_in.buttons || fm1_in.notes)
        fm1_wdt_feed();
    fm1_input_edges(0);
    fm1_input_note_edges();
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.force = 1;
    ui_message("USB DISK OFF");
}
#endif
#endif
