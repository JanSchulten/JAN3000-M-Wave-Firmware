/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's stores: what Felucca's project format has no room for (a project fills its sector, ST_PAYLOAD_MAX), in the
 * flash of the third user sample slot (USR3, 0xC8000..0xDBFFF, 80 KiB): notes and settings were chosen over a third
 * sample slot (eng_sample.c SMP_USER_WRITABLE: the editor uploads into USR1 and USR2 only). Its map:
 *   0xC8000..0xCFFFF  each project slot's extras, A/B (OBJ_XPROJ0 + slot, storage.c): the FX TRACKS' lanes (fxtrack.c)
 *   0xD0000..0xD1FFF  the autosave's extras, A/B (OBJ_XAUTO)
 *   0xD2000..0xDBFFF  the tape (tape.c): copy A at 0xD2000, B at 0xD7000, five sectors each
 * Extras (proj_xtra_t): written after their project, with the id of the project as written (xtra_id: the CRC its commit
 * record holds). A project written any other way (the editor's backups) does not match its slot's extras any more: they
 * are ignored, as a project of Felucca's has none (the lanes load empty). A rename writes the project again: its extras
 * follow it (xtra_renamed). In a song each slot's lanes play with its sections (song_chain.c chain.source[].fx).
 * The tape: one, as the OP-1's (not a project's). Kept a while after its events changed (tape.gen), with the transport
 * stopped and nothing sounding (tape_store_poll, as the autosave), the two copies taking turns; read back at power-on
 * (tape_store_boot, not after a crash). A copy: the commit record (an st_hdr_t of type OBJ_TAPE) at its start, the
 * transport's state at +256, the events from its second sector; written last, the record, so a write cut short leaves
 * the copy before it in charge. Main loop only. */
#define XTRA_MAGIC 0x41525458u                   /* "XTRA" */
typedef struct {
    uint32_t magic;
    uint16_t size, ver;                          /* sizeof (a later firmware's may be longer: what is known is read), 1 */
    uint32_t proj;                               /* the project it belongs to (xtra_id of its store as written) */
    fxt_lane_t fx[FXT_N];                        /* the FX TRACKS */
} proj_xtra_t;
_Static_assert(sizeof(proj_xtra_t) == 12u + FXT_N * sizeof(fxt_lane_t), "project extras layout (stored)");
static proj_xtra_t xtra_slot[4] __attribute__((section(".pool")));   /* each project slot's (magic 0: none) */

/* a project store's id: the CRC its commit record holds (storage.c st_save) */
static uint32_t xtra_id(const void *p, uint32_t n)
{
#if FELUCCA_FLASH
    return st_crc32(p, n);
#else
    return proj_hash(p, n);
#endif
}
/* the project in slot s as written (0: none) */
static uint32_t xtra_proj_id(uint32_t s)
{
#if FELUCCA_FLASH
    st_hdr_t h;
    if (flash_ok)
        return st_current(OBJ_PROJECT0 + (s & 3u), &h) < 0 ? 0u : h.crc;
#endif
    return xtra_id(&proj_slot[s & 3u], sizeof(project_store_t));
}
static void xtra_capture(proj_xtra_t *x, uint32_t proj)
{
    memset(x, 0, sizeof *x);
    x->magic = XTRA_MAGIC;
    x->size = sizeof *x;
    x->ver = 1;
    x->proj = proj;
    memcpy(x->fx, fxt_lane, sizeof x->fx);
}
static int xtra_ok(const proj_xtra_t *x, uint32_t proj)
{
    return x->magic == XTRA_MAGIC && x->size >= sizeof *x && proj && x->proj == proj;
}
/* the lanes x holds into dst; x of another project, or none: empty lanes */
static void xtra_lanes(fxt_lane_t *dst, const proj_xtra_t *x, uint32_t proj)
{
    uint32_t l;
    if (!xtra_ok(x, proj)) {
        memset(dst, 0, FXT_N * sizeof *dst);
        return;
    }
    memcpy(dst, x->fx, FXT_N * sizeof *dst);
    for (l = 0; l < FXT_N; l++)
        if (dst[l].len > NSTEP)
            dst[l].len = 0;
}
#if FELUCCA_FLASH
/* object obj's newest copy into x (as much as it knows); 0: there is none */
static int xtra_read(uint32_t obj, proj_xtra_t *x)
{
    st_hdr_t h;
    memset(x, 0, sizeof *x);
    if (!flash_ok || st_current(obj, &h) < 0 || h.len < sizeof *x)
        return 0;
    memcpy(x, st_buf, sizeof *x);
    return x->magic == XTRA_MAGIC;
}
#endif
/* persist_boot: every slot's extras from flash */
static void xtra_boot(void)
{
#if FELUCCA_FLASH
    uint32_t s;
    for (s = 0; s < 4u; s++)
        xtra_read(OBJ_XPROJ0 + s, &xtra_slot[s]);
#endif
}
/* project_save_as: the project went into slot s as id; its extras after it. 0 saved */
static int xtra_saved(uint32_t s, uint32_t id)
{
    proj_xtra_t *x = &xtra_slot[s & 3u];
    xtra_capture(x, id);
#if FELUCCA_FLASH
    if (flash_ok && st_save(OBJ_XPROJ0 + (s & 3u), x, sizeof *x))
        return 1;
#endif
    return 0;
}
/* project_load: slot s's lanes (none, or another project's: empty) */
static void xtra_loaded(uint32_t s) { xtra_lanes(fxt_lane, &xtra_slot[s & 3u], xtra_proj_id(s)); }
/* project_rename: slot s was written again as id; its extras (if they were its project's: was) follow it */
static void xtra_renamed(uint32_t s, uint32_t was, uint32_t id)
{
    proj_xtra_t *x = &xtra_slot[s & 3u];
    if (!xtra_ok(x, was))
        return;
    x->proj = id;
#if FELUCCA_FLASH
    if (flash_ok)
        st_save(OBJ_XPROJ0 + (s & 3u), x, sizeof *x);
#endif
}
/* the autosave (project.c): written after it as id (0 written); read back with it (n bytes in proj_wire) */
static int xtra_autosaved(uint32_t id)
{
#if FELUCCA_FLASH
    proj_xtra_t x;
    xtra_capture(&x, id);
    return st_save(OBJ_XAUTO, &x, sizeof x);
#else
    (void)id;
    return 0;
#endif
}
static void xtra_autoloaded(uint32_t id)
{
#if FELUCCA_FLASH
    proj_xtra_t x;
    xtra_read(OBJ_XAUTO, &x);
    xtra_lanes(fxt_lane, &x, id);
#else
    (void)id;
#endif
}

/* ------------------------------------------------------------- tape --- */
#if FELUCCA_FLASH
#define OBJ_TAPE 33u                             /* the tape's commit records (as OBJ_FM6VB: no st_load object) */
#define TS_BASE(c) (0xD2000u + (c) * 5u * ST_SECTOR)
#define TS_EV ST_SECTOR                          /* a copy's events: from its second sector */
#define TS_IDLE_MS 3000u                         /* kept this long after the last change, stopped and quiet */
#define TS_POLL_MS 250u                          /* .. checked this often */
#define TS_RETRY_MS 30000u                       /* .. after a failed write, the next try */
typedef struct {
    uint16_t g0, na;
    uint32_t head, loop_in;
    uint8_t loop, rev;
    int8_t speed;
    uint8_t rsv;
} tape_state_t;
#define TS_LEN ((uint32_t)sizeof(tape_state_t) + (uint32_t)sizeof tape_ev)
_Static_assert(sizeof(tape_state_t) == 16u && sizeof tape_ev == 4u * ST_SECTOR, "tape store layout");
static struct {
    uint32_t seq;                                /* the newest copy's (0: none) */
    uint32_t t, poll, last;                      /* fm1_ms: tape.gen seen changed / busy, the last check, the last write */
    uint32_t writes;                             /* (since power-on: the host tests count them) */
    uint16_t saved, seen;                        /* tape.gen written last (or read at power-on), seen last */
    uint8_t cur, err;                            /* the copy holding the newest (0xFF none); the last write failed */
} tps __attribute__((section(".pool")));

static int ts_head(uint32_t c, st_hdr_t *h)      /* copy c's commit record valid: 0 */
{
    if (st_read(TS_BASE(c), h, sizeof *h))
        return -1;
    return h->magic == ST_MAGIC && h->type == OBJ_TAPE && h->slot == c && h->len == TS_LEN &&
           h->hcrc == st_crc32(h, sizeof *h - 4u) ? 0 : -1;
}
/* the CRC of copy c's state and events as they are in flash (a page at a time) */
static uint32_t ts_crc(uint32_t c)
{
    uint8_t b[256];
    uint32_t off, crc;
    st_read(TS_BASE(c) + ST_PAYLOAD_OFF, b, sizeof(tape_state_t));
    crc = st_crc32_add(0xFFFFFFFFu, b, sizeof(tape_state_t));
    for (off = 0; off < sizeof tape_ev; off += sizeof b) {
        st_read(TS_BASE(c) + TS_EV + off, b, sizeof b);
        crc = st_crc32_add(crc, b, sizeof b);
    }
    return ~crc;
}
/* a tape that keeps the invariants tape.c works by: the gap, the head, events sorted by tick on either side */
static int ts_sane(const tape_state_t *s)
{
    uint32_t i, last = 0;
    if ((uint32_t)s->g0 + s->na > TAPE_EV || s->head > TAPE_END || s->loop >= TAPE_NLOOP || s->loop_in > TAPE_END ||
        s->loop_in % TAPE_BAR || s->rev > 1u || s->speed < -TAPE_SPEED_MAX || s->speed > TAPE_SPEED_MAX)
        return 0;
    for (i = 0; i < s->g0; i++) {
        if (TE_TICK(tape_ev[i]) < last || TE_TICK(tape_ev[i]) > s->head)
            return 0;
        last = TE_TICK(tape_ev[i]);
    }
    for (i = TAPE_EV - s->na; i < TAPE_EV; i++) {
        if (TE_TICK(tape_ev[i]) < last || TE_TICK(tape_ev[i]) < s->head)
            return 0;
        last = TE_TICK(tape_ev[i]);
    }
    return 1;
}
/* the tape (stopped) -> the copy that does not hold the newest; 0 written */
static int tape_store_write(void)
{
    uint32_t c = tps.cur == 0u ? 1u : 0u, base = TS_BASE(c), i, crc;
    tape_state_t s;
    st_hdr_t h, chk;
    int rc = 0;
    tape.hold = 1;                               /* (the ISR leaves it as it is: no seek, erase or recording) */
    memset(&s, 0, sizeof s);
    s.g0 = tape.g0;
    s.na = tape.na;
    s.head = tape.head;
    s.loop_in = tape.loop_in;
    s.loop = tape.loop;
    s.rev = tape.rev;
    s.speed = tape.speed;
    crc = ~st_crc32_add(st_crc32_add(0xFFFFFFFFu, &s, sizeof s), tape_ev, sizeof tape_ev);
    for (i = 0; i < 5u && !rc; i++)
        rc = st_erase(base + i * ST_SECTOR);
    for (i = 0; i < sizeof tape_ev && !rc; i += 256u)
        rc = st_prog(base + TS_EV + i, (const uint8_t *)tape_ev + i, 256u);
    if (!rc)
        rc = st_prog(base + ST_PAYLOAD_OFF, &s, sizeof s);
    if (!rc) {
        h.magic = ST_MAGIC;
        h.type = OBJ_TAPE;
        h.slot = (uint16_t)c;
        h.seq = tps.seq + 1u;
        h.len = TS_LEN;
        h.crc = crc;
        h.rsv[0] = h.rsv[1] = 0xFFFFFFFFu;
        h.hcrc = st_crc32(&h, sizeof h - 4u);
        rc = st_prog(base, &h, sizeof h);        /* the commit record, last */
    }
    tape.hold = 0;
    if (!rc && (ts_head(c, &chk) || chk.seq != h.seq || ts_crc(c) != crc))
        rc = -7;                                 /* (read back: a failing part must not count as kept) */
    if (!rc) {
        tps.cur = (uint8_t)c;
        tps.seq = h.seq;
    }
    return rc;
}
#endif

/* main loop, every pass (after autosave_poll): the tape kept when it changed and has rested TS_IDLE_MS, stopped and
 * quiet (autosave_quiet: an erase stops the audio for its ~40 ms, five of them here) */
static int autosave_quiet(void);
static void tape_store_poll(void)
{
#if FELUCCA_FLASH
    uint16_t g = tape.gen;
    if (!flash_ok || fm1_ms - tps.poll < TS_POLL_MS)
        return;
    tps.poll = fm1_ms;
    if (g != tps.seen || !autosave_quiet() || tape.req_clear || tape.req_seek) {
        tps.seen = g;
        tps.t = fm1_ms;
        return;
    }
    if (g == tps.saved || fm1_ms - tps.t < TS_IDLE_MS || (tps.err && fm1_ms - tps.last < TS_RETRY_MS))
        return;
    tps.last = fm1_ms;
    tps.err = tape_store_write() != 0;
    if (!tps.err) {
        tps.saved = g;
        tps.writes++;
    }
#endif
}

/* power-on (main.c, after the autosave): the newest whole copy becomes the tape when `allowed` (no crash, hang or failed
 * boot before); none, or one that does not keep the tape's rules: an empty tape. 1 = read back */
static int tape_store_boot(int allowed)
{
    int rc = 0;
#if FELUCCA_FLASH
    st_hdr_t h[2];
    uint32_t c, v = 0, n, order[2];
    tape_state_t s;
    tps.cur = 0xFF;
    tps.seq = 0;
    for (c = 0; c < 2u; c++)
        if (flash_ok && ts_head(c, &h[c]) == 0) {
            v |= 1u << c;
            if (!tps.seq || h[c].seq - tps.seq < 0x80000000u)
                tps.seq = h[c].seq;               /* (the next write numbers after both) */
        }
    order[0] = (v & 3u) == 3u && h[0].seq != h[1].seq && h[1].seq - h[0].seq < 0x80000000u ? 1u : (v & 1u) ? 0u : 1u;
    order[1] = order[0] ^ 1u;                    /* (the newest first: B if newer, wrap included; A on a tie) */
    for (n = 0; n < 2u && !rc; n++) {
        c = order[n];
        if (!((v >> c) & 1u) || ts_crc(c) != h[c].crc)
            continue;                            /* (none there, or a write cut short) */
        st_read(TS_BASE(c) + ST_PAYLOAD_OFF, &s, sizeof s);
        st_read(TS_BASE(c) + TS_EV, tape_ev, sizeof tape_ev);
        if (!ts_sane(&s))
            continue;
        tps.cur = (uint8_t)c;                     /* (the next write goes to the other one) */
        if (!allowed)
            break;
        tape.g0 = s.g0;
        tape.na = s.na;
        tape.head = s.head;
        tape.loop_in = s.loop_in;
        tape.loop = s.loop;
        tape.rev = tape.dir = s.rev;
        tape.speed = s.speed;
        rc = 1;
    }
    if (!rc) {
        memset(tape_ev, 0, sizeof tape_ev);
        tape.g0 = tape.na = 0;
        tape.head = 0;
    }
    tps.saved = tps.seen = tape.gen;
    tps.t = fm1_ms;
#else
    (void)allowed;
#endif
    return rc;
}
