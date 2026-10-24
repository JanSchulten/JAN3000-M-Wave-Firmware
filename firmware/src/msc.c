/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, MENU > USB DISK: USB mass storage, bulk-only transport (BOT 1.0) with the SCSI commands a phone or a
 * computer sends a memory card, on the disk vdisk.c makes up. Pure C, packet in and packet out: usb.c moves the 64-byte
 * packets of EP1 (TIMER5 ISR, usb_poll), tests/vdisk_test.c drives it the same way.
 *   msc_out(p, n)   a packet from the host: a command block wrapper (CBW, 31 bytes), or data of a write
 *   msc_in(p)       the next packet to the host (data, then the status wrapper, CSW, 13 bytes), -1 none
 * One LUN, 512-byte blocks, write-protected: MODE SENSE says so (WP), a write is refused (DATA PROTECT) and its data
 * taken and dropped. A command that fails sends its data phase as zeros (residue: all of it), then the failed status;
 * the host then asks REQUEST SENSE. Data shorter than the host's length is padded with zeros (the residue says how
 * much was not data). An eject (START STOP UNIT) leaves the disk not ready until a load. */
enum { MS_CBW, MS_IN, MS_OUT, MS_CSW };
enum { MO_RESP, MO_READ };                     /* where an IN data phase's bytes come from */
static struct {
    uint8_t st, op, status, ejected;
    uint8_t key, asc;                          /* the sense of the last failed command */
    uint8_t tag[4];
    uint32_t want, len, done, lba;             /* the host's data length, ours, moved so far; READ: the first block */
    uint8_t resp[36];                          /* a short reply (INQUIRY, sense, capacities, mode) */
    volatile uint32_t cmds, reads;             /* (the screen: the host is there, it reads) */
} msc;

static void msc_reset(void)                    /* a bus reset, BOT reset, a new configuration */
{
    msc.st = MS_CBW;
    msc.want = msc.len = msc.done = 0;
}
static uint32_t msc_be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static void msc_put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static void msc_fail(uint32_t key, uint32_t asc)
{
    msc.status = 1;
    msc.key = (uint8_t)key;
    msc.asc = (uint8_t)asc;
    msc.len = 0;
}
static void msc_reply(const uint8_t *p, uint32_t n)
{
    memcpy(msc.resp, p, n);
    msc.op = MO_RESP;
    msc.len = n;
}
static void msc_read(uint32_t lba, uint32_t n)  /* READ(6) / READ(10): n blocks from lba */
{
    if (msc.ejected)
        msc_fail(0x02, 0x3A);                  /* NOT READY, medium not present */
    else if (lba >= VD_SECTORS || n > VD_SECTORS - lba)
        msc_fail(0x05, 0x21);                  /* ILLEGAL REQUEST, LBA out of range */
    else {
        msc.op = MO_READ;
        msc.lba = lba;
        msc.len = n * 512u;
        msc.reads++;
    }
}
/* the command in cb (SCSI CDB): its reply, or its failure */
static void msc_command(const uint8_t *cb)
{
    static const uint8_t INQ[36] = {0x00, 0x80, 0x02, 0x02, 31, 0, 0, 0,   /* disk, removable, SPC-2 */
                                    'M', '-', 'V', 'A', 'V', 'E', ' ', ' ',
                                    'F', 'M', '-', '1', ' ', 'O', 'P', '1', ' ', 'D', 'I', 'S', 'K', ' ', ' ', ' ',
                                    '0', '.', '1', ' '};
    uint8_t r[18];
    msc.status = 0;
    msc.len = 0;
    msc.op = MO_RESP;
    memset(r, 0, sizeof r);
    switch (cb[0]) {
    case 0x00:                                 /* TEST UNIT READY */
        if (msc.ejected)
            msc_fail(0x02, 0x3A);
        return;
    case 0x03:                                 /* REQUEST SENSE: the last failure's, then cleared */
        r[0] = 0x70;
        r[2] = msc.key;
        r[7] = 10;
        r[12] = msc.asc;
        msc_reply(r, 18);
        msc.key = msc.asc = 0;
        return;
    case 0x12:                                 /* INQUIRY: the standard data; EVPD pages: the list, none else */
        if (cb[1] & 1u) {
            r[1] = cb[2];
            if (cb[2] != 0x00)
                msc_fail(0x05, 0x24);          /* ILLEGAL REQUEST, invalid field in CDB */
            else
                msc_reply(r, 4);               /* supported pages: only this one */
            return;
        }
        msc_reply(INQ, sizeof INQ);
        return;
    case 0x1A:                                 /* MODE SENSE(6): the header, write-protected */
        r[0] = 3;
        r[2] = 0x80;
        msc_reply(r, 4);
        return;
    case 0x5A:                                 /* MODE SENSE(10) */
        r[1] = 6;
        r[3] = 0x80;
        msc_reply(r, 8);
        return;
    case 0x1E:                                 /* PREVENT / ALLOW MEDIUM REMOVAL */
    case 0x2F:                                 /* VERIFY(10) */
    case 0x35:                                 /* SYNCHRONIZE CACHE(10) */
        return;
    case 0x1B:                                 /* START STOP UNIT: LoEj with START 0 ejects, with 1 loads */
        if (cb[4] & 2u)
            msc.ejected = !(cb[4] & 1u);
        return;
    case 0x23:                                 /* READ FORMAT CAPACITIES: one, formatted */
        r[3] = 8;
        msc_put_be32(r + 4, VD_SECTORS);
        r[8] = 0x02;
        r[10] = 0x02;                          /* 512 */
        msc_reply(r, 12);
        return;
    case 0x25:                                 /* READ CAPACITY(10): the last block, the block length */
        if (msc.ejected) {
            msc_fail(0x02, 0x3A);
            return;
        }
        msc_put_be32(r, VD_SECTORS - 1u);
        msc_put_be32(r + 4, 512);
        msc_reply(r, 8);
        return;
    case 0x08:                                 /* READ(6) */
        msc_read((uint32_t)(cb[1] & 0x1Fu) << 16 | (uint32_t)cb[2] << 8 | cb[3], cb[4] ? cb[4] : 256u);
        return;
    case 0x28:                                 /* READ(10) */
        msc_read(msc_be32(cb + 2), (uint32_t)cb[7] << 8 | cb[8]);
        return;
    case 0x0A:                                 /* WRITE(6), WRITE(10), WRITE(12): DATA PROTECT, write protected */
    case 0x2A:
    case 0xAA:
        msc_fail(0x07, 0x27);
        return;
    default:                                   /* ILLEGAL REQUEST, invalid command */
        msc_fail(0x05, 0x20);
        return;
    }
}
/* a packet from the host; 1 taken, 0 not now (it waits, NAKed: the status of the last command is still to go) */
static int msc_out(const uint8_t *p, uint32_t n)
{
    if (msc.st == MS_OUT) {                    /* a write's data: dropped */
        msc.done += n;
        if (msc.done >= msc.want)
            msc.st = MS_CSW;
        return 1;
    }
    if (msc.st != MS_CBW)
        return 0;
    if (n != 31u || p[0] != 'U' || p[1] != 'S' || p[2] != 'B' || p[3] != 'C')
        return 1;                              /* not a CBW: ignored */
    memcpy(msc.tag, p + 4, 4);
    msc.want = (uint32_t)p[8] | (uint32_t)p[9] << 8 | (uint32_t)p[10] << 16 | (uint32_t)p[11] << 24;
    msc.done = 0;
    msc.cmds++;
    msc_command(p + 15);
    if (msc.len > msc.want)
        msc.len = msc.want;
    msc.st = !msc.want ? MS_CSW : (p[12] & 0x80u) ? MS_IN : MS_OUT;
    if (msc.st == MS_OUT && !msc.status) {     /* data from the host for a command that takes none */
        msc_fail(0x05, 0x20);
    }
    return 1;
}
/* the next packet to the host into p (64 bytes room): its length, -1 nothing to send */
static int msc_in(uint8_t *p)
{
    if (msc.st == MS_IN) {
        uint32_t k = msc.want - msc.done < 64u ? msc.want - msc.done : 64u, i;
        if (msc.op == MO_READ && msc.done < msc.len)
            vd_read(msc.lba * 512u + msc.done, p, msc.done + k <= msc.len ? k : msc.len - msc.done);
        for (i = 0; i < k; i++)                /* (past our length: zeros) */
            if (msc.op != MO_READ || msc.done + i >= msc.len)
                p[i] = msc.done + i < msc.len ? msc.resp[msc.done + i] : 0u;
        msc.done += k;
        if (msc.done >= msc.want)
            msc.st = MS_CSW;
        return (int)k;
    }
    if (msc.st == MS_CSW) {
        uint32_t res = msc.want - msc.len;
        p[0] = 'U'; p[1] = 'S'; p[2] = 'B'; p[3] = 'S';
        memcpy(p + 4, msc.tag, 4);
        p[8] = (uint8_t)res; p[9] = (uint8_t)(res >> 8); p[10] = (uint8_t)(res >> 16); p[11] = (uint8_t)(res >> 24);
        p[12] = msc.status;
        msc.st = MS_CBW;
        return 13;
    }
    return -1;
}
