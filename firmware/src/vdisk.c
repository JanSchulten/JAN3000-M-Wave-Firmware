/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, MENU > USB DISK: a read-only disk made up as it is read (usb.c msc, usbdisk.c fills it). Nothing of it is
 * stored: every sector is computed from a list of files, each a list of pieces (RAM or the flash's XIP window), so the
 * disk costs a few hundred bytes of RAM whatever it shows. Its shape is a small memory card's, the most ordinary there
 * is, so a phone or a computer takes it without asking:
 *   LBA 0          an MBR, one partition: FAT16 (type 06) from LBA VD_PART (32) to the end
 *   VD_PART        the partition's boot sector (BPB: 512-byte sectors, a sector a cluster, 2 FATs of 64 sectors, 512
 *                  root entries, label "FM-1")
 *   then           FAT 1, FAT 2, the root directory (32 sectors), the data: cluster 2 at VD_DATA
 * 16384 sectors (8 MiB) make 16191 clusters, a FAT16 by its count (4085 .. 65524), as every reader decides it. The files
 * lie one after another from cluster 2, each in one run (its FAT entries a straight chain), the rest free. 8.3 names,
 * read-only, one fixed date (the FM-1 has no clock). Pure C: tests/vdisk_test.c reads the image it makes back with an
 * independent reader (tests/vdisk_check.py). */
#define VD_SECTORS 16384u                     /* 8 MiB */
#define VD_PART 32u                           /* the partition's first sector */
#define VD_FATSZ 64u                          /* sectors a FAT */
#define VD_ROOTN 512u                         /* root entries: 32 sectors */
#define VD_FAT1 (VD_PART + 1u)
#define VD_ROOT (VD_FAT1 + 2u * VD_FATSZ)
#define VD_DATA (VD_ROOT + VD_ROOTN * 32u / 512u)
#define VD_CLUSTERS (VD_SECTORS - VD_DATA)    /* data clusters (a sector each) */
#define VD_NFILE 8u
#define VD_NPIECE 72u
_Static_assert(VD_CLUSTERS >= 4085u && VD_CLUSTERS < 65525u, "a FAT16 by its cluster count");
_Static_assert((VD_CLUSTERS + 2u) * 2u <= VD_FATSZ * 512u, "a FAT holds every cluster");
#define VD_DATE ((uint16_t)(((2026u - 1980u) << 9) | (1u << 5) | 1u))   /* 2026-01-01 */
#define VD_TIME ((uint16_t)(12u << 11))                                  /* 12:00 */

typedef struct {
    const uint8_t *p;                         /* 0: that many zero bytes */
    uint32_t n;
} vd_piece_t;
typedef struct {
    uint8_t name[11];                         /* 8.3, space-padded, upper case */
    uint8_t p0, np;                           /* its pieces */
    uint32_t size, clus;                      /* bytes; its first cluster (0: empty) */
} vd_file_t;
static struct {
    vd_file_t f[VD_NFILE];
    vd_piece_t pc[VD_NPIECE];
    uint8_t nf, npc, open;
    uint32_t next;                            /* the next free cluster */
    uint32_t have;                            /* the sector in sec, + 1 (0: none) */
    uint8_t sec[512] __attribute__((aligned(4)));
} vd;

static void vd_clear(void)
{
    memset(&vd, 0, sizeof vd);
    vd.next = 2;
}
/* a new file "NAME.EXT" (8.3); its pieces follow (vd_add), vd_end closes it. 0, or -1: no room */
static int vd_begin(const char *name)
{
    vd_file_t *f;
    uint32_t i = 0, k = 0;
    if (vd.nf >= VD_NFILE)
        return -1;
    f = &vd.f[vd.nf];
    memset(f, 0, sizeof *f);
    memset(f->name, ' ', sizeof f->name);
    for (; name[i] && name[i] != '.' && k < 8u; i++)
        f->name[k++] = (uint8_t)name[i];
    while (name[i] && name[i] != '.')
        i++;
    for (k = 8u, i += name[i] == '.'; name[i] && k < 11u; i++)
        f->name[k++] = (uint8_t)name[i];
    f->p0 = vd.npc;
    vd.open = 1;
    return 0;
}
static int vd_add(const void *p, uint32_t n)  /* (p 0: n zero bytes) */
{
    vd_file_t *f = &vd.f[vd.nf];
    if (!vd.open || vd.npc >= VD_NPIECE)
        return -1;
    if (!n)
        return 0;
    vd.pc[vd.npc].p = (const uint8_t *)p;
    vd.pc[vd.npc].n = n;
    vd.npc++;
    f->np++;
    f->size += n;
    return 0;
}
static int vd_end(void)                       /* 0, or -1: the disk is full (the file is dropped) */
{
    vd_file_t *f = &vd.f[vd.nf];
    uint32_t nc = (f->size + 511u) / 512u;
    if (!vd.open)
        return -1;
    vd.open = 0;
    if (vd.next + nc > VD_CLUSTERS + 2u) {
        vd.npc = f->p0;
        return -1;
    }
    f->clus = nc ? vd.next : 0u;
    vd.next += nc;
    vd.nf++;
    return 0;
}

static void vd_le16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void vd_le32(uint8_t *p, uint32_t v) { vd_le16(p, v); vd_le16(p + 2, v >> 16); }
/* bytes [off, off + n) of file f, beyond its size zeros */
static void vd_file_read(const vd_file_t *f, uint32_t off, uint8_t *d, uint32_t n)
{
    uint32_t i, at = 0;
    memset(d, 0, n);
    for (i = 0; i < f->np && n; i++) {
        const vd_piece_t *pc = &vd.pc[f->p0 + i];
        if (off < at + pc->n) {
            uint32_t o = off - at, k = pc->n - o < n ? pc->n - o : n;
            if (pc->p)
                memcpy(d, pc->p + o, k);
            d += k;
            off += k;
            n -= k;
        }
        at += pc->n;
    }
}
/* cluster c's FAT entry: the next of its file's run, 0xFFFF its last, 0 free */
static uint32_t vd_fat(uint32_t c)
{
    uint32_t i;
    if (c < 2u)
        return c ? 0xFFFFu : 0xFFF8u;
    for (i = 0; i < vd.nf; i++) {
        const vd_file_t *f = &vd.f[i];
        uint32_t nc = (f->size + 511u) / 512u;
        if (f->clus && c >= f->clus && c < f->clus + nc)
            return c + 1u < f->clus + nc ? c + 1u : 0xFFFFu;
    }
    return 0;
}
static void vd_dirent(uint32_t e, uint8_t *d)   /* root entry e: 0 the label, then the files */
{
    memset(d, 0, 32);
    if (e == 0u) {
        memcpy(d, "FM-1       ", 11);
        d[11] = 0x08;                           /* volume label */
    } else if (e <= vd.nf) {
        const vd_file_t *f = &vd.f[e - 1u];
        memcpy(d, f->name, 11);
        d[11] = 0x21;                           /* read-only, archive */
        vd_le16(d + 14, VD_TIME);
        vd_le16(d + 16, VD_DATE);
        vd_le16(d + 18, VD_DATE);
        vd_le16(d + 22, VD_TIME);
        vd_le16(d + 24, VD_DATE);
        vd_le16(d + 26, f->clus);
        vd_le32(d + 28, f->size);
        return;
    }
}
static void vd_sector(uint32_t lba, uint8_t *s)   /* a whole sector (vd_read takes the part it needs) */
{
    uint32_t i;
    memset(s, 0, 512);
    if (lba == 0u) {                            /* the MBR: one partition */
        uint8_t *p = s + 446;
        p[1] = 0x00; p[2] = 0x21; p[3] = 0x00;  /* (CHS: not used by anything that reads LBA) */
        p[4] = 0x06;                            /* FAT16 */
        p[5] = 0xFE; p[6] = 0xFF; p[7] = 0xFF;
        vd_le32(p + 8, VD_PART);
        vd_le32(p + 12, VD_SECTORS - VD_PART);
        vd_le32(s + 440, 0x314D4646u);          /* the disk signature */
        s[510] = 0x55; s[511] = 0xAA;
    } else if (lba == VD_PART) {                /* the boot sector */
        s[0] = 0xEB; s[1] = 0x3C; s[2] = 0x90;
        memcpy(s + 3, "MVAVEOP1", 8);
        vd_le16(s + 11, 512);
        s[13] = 1;                              /* a sector a cluster */
        vd_le16(s + 14, 1);                     /* reserved sectors */
        s[16] = 2;                              /* FATs */
        vd_le16(s + 17, VD_ROOTN);
        vd_le16(s + 19, VD_SECTORS - VD_PART);
        s[21] = 0xF8;
        vd_le16(s + 22, VD_FATSZ);
        vd_le16(s + 24, 32);                    /* sectors a track, heads: made up, as on every card */
        vd_le16(s + 26, 2);
        vd_le32(s + 28, VD_PART);               /* hidden: the sectors before the partition */
        s[36] = 0x80;
        s[38] = 0x29;
        vd_le32(s + 39, 0x0F310001u);
        memcpy(s + 43, "FM-1       ", 11);
        memcpy(s + 54, "FAT16   ", 8);
        s[62] = 0xF4;                           /* (not bootable: hlt) */
        s[510] = 0x55; s[511] = 0xAA;
    } else if (lba >= VD_FAT1 && lba < VD_ROOT) {
        uint32_t c = ((lba - VD_FAT1) % VD_FATSZ) * 256u;
        for (i = 0; i < 256u; i++)
            if (c + i < VD_CLUSTERS + 2u)
                vd_le16(s + 2u * i, vd_fat(c + i));
    } else if (lba >= VD_ROOT && lba < VD_DATA) {
        uint32_t e = (lba - VD_ROOT) * 16u;
        for (i = 0; i < 16u; i++)
            vd_dirent(e + i, s + 32u * i);
    } else if (lba >= VD_DATA && lba < VD_SECTORS) {
        uint32_t c = lba - VD_DATA + 2u;
        for (i = 0; i < vd.nf; i++) {
            const vd_file_t *f = &vd.f[i];
            if (f->clus && c >= f->clus && (c - f->clus) * 512u < f->size) {
                vd_file_read(f, (c - f->clus) * 512u, s, 512);
                break;
            }
        }
    }
}
/* the disk's bytes [off, off + n), n within one sector (usb.c asks 64 at a time): a sector is computed once, when its
 * first part is asked for (a FAT sector walks the files 256 times: still microseconds), its other parts copied */
static void vd_read(uint32_t off, uint8_t *d, uint32_t n)
{
    uint32_t lba = off / 512u, o = off % 512u;
    if (lba >= VD_SECTORS || o + n > 512u) {
        memset(d, 0, n);
        return;
    }
    if (lba + 1u != vd.have) {                  /* (the sector's next part comes from vd.sec) */
        vd_sector(lba, vd.sec);
        vd.have = lba + 1u;
    }
    memcpy(d, vd.sec + o, n);
}
