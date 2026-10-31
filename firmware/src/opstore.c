/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's stores: what Felucca's project format has no room for (a project fills its sector, ST_PAYLOAD_MAX), in the
 * flash of the second and the third user sample slot (USR2 0xB4000..0xC7FFF, USR3 0xC8000..0xDBFFF, 80 KiB each): notes
 * and settings were chosen over two of the three sample slots (eng_sample.c SMP_USER_WRITABLE: the editor uploads into
 * USR1 only). Its map:
 *   0xB4000..0xC7FFF  the scenes (scene.c): each project slot's bank and the autosave's, two copies of 8 KiB each
 *                     (OBJ_SCENES, below)
 *   0xC8000..0xCFFFF  each project slot's extras, A/B (OBJ_XPROJ0 + slot, storage.c): the FX TRACKS' lanes (fxtrack.c),
 *                     FX1's TYPE and the CLOUD's settings (fx1.c), SCL > CSET's ON, SET and VOICE per track (cset.c),
 *                     the CHORUS's TYPE and FDBK (fx.c), GLO > KEY's run (key.c: the scene in the project's; every other
 *                     scene's KEY is in the scene, scene.c), the LEGATO steps (core.h STEP_LEG: the other scenes' are in
 *                     their steps), the steps 65..128 (core.h NSTEP_FMT: the project holds 64) with their LEN, lanes and
 *                     automation
 *   0xD0000..0xD1FFF  the autosave's extras, A/B (OBJ_XAUTO)
 *   0xD2000..0xDBFFF  free (the TAPE's until it was dropped for GLO > KEY)
 * Extras (proj_xtra_t): written after their project, with the id of the project as written (xtra_id: the CRC its commit
 * record holds). A project written any other way (the editor's backups) does not match its slot's extras any more: they
 * are ignored, as a project of Felucca's has none (the lanes load empty). A rename writes the project again: its extras
 * follow it (xtra_renamed). In a song each slot's lanes play with its sections (song_chain.c chain.source[].fx).
 * Main loop only. */
#define XTRA_MAGIC 0x41525458u                   /* "XTRA" */
typedef struct {
    uint32_t magic;
    uint16_t size, ver;                          /* sizeof (a later firmware's may be longer: what is known is read), 1 */
    uint32_t proj;                               /* the project it belongs to (xtra_id of its store as written) */
    fxt_lane64_t fx[FXT_N];                      /* the FX TRACKS (their steps 1..64; LEN at most 64: XTRA_V7) */
    uint8_t fx1[8];                              /* (E1) FX1 (fx1.c fx1s): TYPE SIZE DENS SPREAD PITCH RAND FDBK MIX */
    int8_t rsv_orc[NTRK];                        /* (SCL > ORCHID's VOICE, before SCL > CSET: written 0, not read) */
    uint8_t rsv_orc_en;                          /* (.. ORCHID ON: written 0, an older firmware's ORCHID off) */
    uint8_t cho[2];                              /* the CHORUS's TYPE and FDBK (fx.c chos; 0 0 before: Felucca's) */
    uint8_t shim_size;                           /* (M-Vave-OP1, 0x80 | SHIMMER SIZE; 0 before: none, see xtra_misc) */
    uint8_t key_on, key_len;                     /* (XTRA_V4) GLO > KEY (key.c keyt): ON, LEN */
    uint8_t key_root[16];                        /* .. each bar's root (KEY_NONE: none) */
    uint8_t key_own;                             /* (M-Vave-OP1, 0x80 | key.c key_own; written 0 before: not known) */
    uint8_t shim_damp;                           /* (.. 0x80 | DAMP) */
    uint8_t leg[NTRK][NSTEP_FMT / 8u];           /* (XTRA_V5) the LEGATO steps (core.h STEP_LEG): step i, bit i % 8 of
                                                  * byte i / 8 (Felucca's step format has no bit left for it) */
    uint8_t cset_en, shim_pitch;                 /* (XTRA_V6) SCL > CSET ON, bit k track k (cset.c); 0x80 | SHIMMER PITCH + 12 */
    uint8_t cset_set[NTRK];                      /* .. each track's set (0 .. 99) */
    uint8_t cset_voice[NTRK];                    /* .. the VOICE before ON (0xFF: none) */
    uint8_t shim_fdbk, fx_serial;                /* 0x80 | SHIMMER FDBK, 0x80 | the serial aux chain (fx.c; were the
                                                  * word's padding, written 0: 0 is an older one's, parallel). The
                                                  * chain is always serial now: written 0x81, its 0x80 the mark that
                                                  * the SHIMMER's four were written */
} proj_xtra_t;
/* (XTRA_V7) M-Vave-OP1's 128 steps: the object goes on after proj_xtra_t with these (size XTRA_V7). Not kept in RAM with
 * the slots' extras (xtra_slot: 2.8 KiB a slot): read from the flash when a project or a song needs them (xtra_hi_of) */
typedef struct {
    uint8_t len[NTRK];                           /* each track's LEN, 1..128 (the project holds at most 64: a longer one
                                                  * 64 there, project.c proj_pack) */
    uint8_t fx_len[FXT_N];                       /* the FX TRACKS' LEN (fxt_lane_t: 0 = 16; proj_xtra_t's at most 64) */
    uint8_t np, rsv;                             /* P_COUNT of the firmware that wrote it (mo's ids from its P_E0 on: as
                                                  * a project's, project.c proj_motion_ids) */
    uint8_t fx[FXT_N][NSTEP - NSTEP_FMT];        /* .. their steps 65..128 */
    uint8_t leg[NTRK][(NSTEP - NSTEP_FMT) / 8u]; /* the LEGATO steps 65..128 (as proj_xtra_t's leg) */
    uint8_t step[NTRK][NSTEP - NSTEP_FMT][PROJ_STEP_BYTES];   /* the steps 65..128, as the project packs its own
                                                  * (project.c proj_step_pack) */
    motion_disk_t mo;                            /* the automation and the locks of steps 65..128 (place: track << 6 |
                                                  * step - 64; PLAY ON is the project's) */
    ssong_t song;                                /* (XTRA_V8) SEQ > SCENE SONG (scene.c) */
    uint8_t eq[NTRK][EQ_NB][4];                  /* (XTRA_V9) HOME > EQ (params.c eq_set): each band's FREQ, GAIN + 24,
                                                  * TYPE, RESO */
    uint8_t dyn[DYN_N];                          /* (XTRA_V10) HOME > MASTER (params.c dyn_set): COMP RELEASE LIMIT GAIN */
    uint8_t lvl[4];                              /* (XTRA_V11) FX > MIX 2: 0x80 | the chain's levels (params.c fx_lvl; [2]
                                                  * the DELAY's is G_DMIX, written 0x80 | 0, not read) */
} xtra_hi_t;
#define XTRA_V1 (12u + FXT_N * (uint32_t)sizeof(fxt_lane64_t))   /* the first extras: the FX TRACKS only */
#define XTRA_V2 (XTRA_V1 + 8u)                   /* .. and FX1's (E1) */
#define XTRA_V3 (XTRA_V2 + 8u)                   /* .. ORCHID's, the CHORUS's */
#define XTRA_V4 (XTRA_V3 + 20u)                  /* .. the KEY's */
#define XTRA_V5 (XTRA_V4 + NTRK * NSTEP_FMT / 8u)   /* .. the LEGATO steps */
#define XTRA_V6 (XTRA_V5 + 4u + 2u * NTRK)       /* .. SCL > CSET's */
#define XTRA_V7 (XTRA_V6 + NTRK + 4u + (NSTEP - NSTEP_FMT) * (FXT_N + NTRK * PROJ_STEP_BYTES) + \
                 NTRK * (NSTEP - NSTEP_FMT) / 8u + (uint32_t)sizeof(motion_disk_t))   /* .. steps 65..128 (xtra_hi_t) */
#define XTRA_V8 (XTRA_V7 + (uint32_t)sizeof(ssong_t))   /* .. the scene song */
#define XTRA_V9 (XTRA_V8 + NTRK * EQ_NB * 4u)    /* .. the EQ */
#define XTRA_V10 (XTRA_V9 + DYN_N)               /* .. the MASTER's dynamics */
#define XTRA_V11 (XTRA_V10 + 4u)                 /* .. the chain's levels (FX > MIX 2) */
_Static_assert(XTRA_V6 == 224u && sizeof(proj_xtra_t) == XTRA_V6 && sizeof(xtra_hi_t) == XTRA_V11 - XTRA_V6 &&
               XTRA_V11 <= PROJ_STORE_SIZE && KEY_BARS == 16u && DYN_N == 4u &&
               NSTEP % 8u == 0u && NSTEP >= NSTEP_FMT, "project extras layout (stored, append-only)");
static proj_xtra_t xtra_slot[4] __attribute__((section(".pool")));   /* each project slot's (magic 0: none) */
static uint8_t xtra_buf[XTRA_V11] __attribute__((aligned(4), section(".pool")));   /* an object as written (main loop) */
#if !FELUCCA_FLASH
static xtra_hi_t xtra_hi_ram[4] __attribute__((section(".pool")));   /* (host builds without the flash: each slot's
                                                  * steps 65..128, as the flash would hold them) */
#endif
static const step_t XTRA_REST = {{0, 0, 0, 0}, 0, ST_REST, 0, 0, 0, 0, 0};   /* an empty step (ui.c step_clear's) */

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
static void xtra_capture(proj_xtra_t *x, xtra_hi_t *h, uint32_t proj)
{
    uint32_t k, i, l;
    memset(x, 0, sizeof *x);
    memset(h, 0, sizeof *h);
    x->magic = XTRA_MAGIC;
    x->size = XTRA_V11;
    x->ver = 1;
    x->proj = proj;
    for (l = 0; l < FXT_N; l++) {                   /* (steps 1..64 and a LEN of at most 64 where firmware of 64 steps */
        memcpy(x->fx[l].step, fxt_lane[l].step, NSTEP_FMT);   /* reads them, the rest after) */
        memcpy(h->fx[l], fxt_lane[l].step + NSTEP_FMT, NSTEP - NSTEP_FMT);
        x->fx[l].len = (uint8_t)(fxt_lane[l].len > NSTEP_FMT ? NSTEP_FMT : fxt_lane[l].len);
        h->fx_len[l] = fxt_lane[l].len;
    }
    h->np = P_COUNT;
    for (k = 0; k < 4u; k++)                        /* MIX 2's levels (XTRA_V11; [2] the DELAY's MIX: G_DMIX, not read) */
        h->lvl[k] = (uint8_t)(0x80u | (k == 2u ? 0u : (uint32_t)clamp(fx_lvl[k], 0, 127)));
    x->fx1[0] = fx1s.type;
    x->fx1[1] = fx1s.size;
    x->fx1[2] = fx1s.dens;
    x->fx1[3] = fx1s.spread;
    x->fx1[4] = (uint8_t)fx1s.pitch;
    x->fx1[5] = fx1s.rnd;
    x->fx1[6] = fx1s.fdbk;
    x->fx1[7] = fx1s.mix;
    x->cset_en = cset_en;
    memcpy(x->cset_set, cset_set, sizeof x->cset_set);
    memcpy(x->cset_voice, cset_voice, sizeof x->cset_voice);
    x->cho[0] = chos.type;
    x->cho[1] = chos.fdbk;
    x->shim_size = (uint8_t)(0x80u | shim_p[0]);
    x->shim_damp = (uint8_t)(0x80u | shim_p[1]);
    x->shim_pitch = (uint8_t)(0x80u | (shim_p[2] + 12));
    x->shim_fdbk = (uint8_t)(0x80u | shim_p[3]);
    x->fx_serial = 0x81u;                        /* (the chain: serial; the SHIMMER's four written) */
    x->key_on = keyt.on;
    x->key_len = keyt.len;
    memcpy(x->key_root, (const void *)keyt.root, sizeof x->key_root);
    x->key_own = (uint8_t)(0x80u | (key_own & 1u));
    for (k = 0; k < NTRK; k++) {
        h->len[k] = (uint8_t)clamp(trk[k].p[P_SLEN], 1, NSTEP);
        for (i = 0; i < NSTEP; i++) {
            const step_t *st = &trk[k].step[i];
            if (st->probability & STEP_LEG) {
                if (i < NSTEP_FMT)
                    x->leg[k][i / 8u] |= (uint8_t)(1u << (i % 8u));
                else
                    h->leg[k][(i - NSTEP_FMT) / 8u] |= (uint8_t)(1u << (i % 8u));
            }
            if (i >= NSTEP_FMT && !proj_step_pack(h->step[k][i - NSTEP_FMT], st))
                proj_step_pack(h->step[k][i - NSTEP_FMT], &XTRA_REST);   /* (one proj_pack would refuse: never kept) */
        }
    }
    motion_to_disk(&h->mo, &motion, 1);
    h->song = ssong;
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < EQ_NB; i++) {
            const eq_band_t *b = &eq_set[k][i];
            h->eq[k][i][0] = (uint8_t)b->f;
            h->eq[k][i][1] = (uint8_t)(b->g + 24);
            h->eq[k][i][2] = (uint8_t)b->ty;
            h->eq[k][i][3] = (uint8_t)b->q;
        }
    for (i = 0; i < DYN_N; i++)
        h->dyn[i] = (uint8_t)dyn_set[i];
}
static int xtra_ok(const proj_xtra_t *x, uint32_t proj)
{
    return x->magic == XTRA_MAGIC && x->size >= XTRA_V1 && proj && x->proj == proj;
}
#if FELUCCA_FLASH
/* object obj's newest copy into x (as much as it knows; st_buf keeps all of it until the next storage call): its
 * length, 0 there is none */
static uint32_t xtra_read(uint32_t obj, proj_xtra_t *x)
{
    st_hdr_t h;
    memset(x, 0, sizeof *x);
    if (!flash_ok || st_current(obj, &h) < 0 || h.len < XTRA_V1)
        return 0;
    memcpy(x, st_buf, h.len < sizeof *x ? h.len : sizeof *x);   /* (an older one: what it has, its size says so) */
    return x->magic == XTRA_MAGIC ? h.len : 0u;
}
#endif
/* the steps 65..128 slot s's extras hold, if they are project proj's: in st_buf (read from the flash: the pointer holds
 * until the next storage call); 0 none (extras of before, another project's, none) */
static const xtra_hi_t *xtra_hi_of(uint32_t s, uint32_t proj)
{
    if (!xtra_ok(&xtra_slot[s & 3u], proj) || xtra_slot[s & 3u].size < XTRA_V7)
        return 0;
#if FELUCCA_FLASH
    {
        proj_xtra_t x;
        return xtra_read(OBJ_XPROJ0 + (s & 3u), &x) >= XTRA_V7 && xtra_ok(&x, proj) ? (const xtra_hi_t *)(st_buf + XTRA_V6)
                                                                                    : 0;
    }
#else
    return &xtra_hi_ram[s & 3u];
#endif
}
/* the lanes x holds (and hi their steps 65..128, 0 none) into dst; x of another project, or none: empty lanes */
static void xtra_lanes(fxt_lane_t *dst, const proj_xtra_t *x, uint32_t proj, const xtra_hi_t *hi)
{
    uint32_t l;
    memset(dst, 0, FXT_N * sizeof *dst);
    if (!xtra_ok(x, proj))
        return;
    for (l = 0; l < FXT_N; l++) {
        memcpy(dst[l].step, x->fx[l].step, NSTEP_FMT);
        dst[l].len = x->fx[l].len <= NSTEP_FMT ? x->fx[l].len : 0u;
        if (!hi)
            continue;
        memcpy(dst[l].step + NSTEP_FMT, hi->fx[l], NSTEP - NSTEP_FMT);
        if (dst[l].len == NSTEP_FMT && hi->fx_len[l] > NSTEP_FMT && hi->fx_len[l] <= NSTEP)
            dst[l].len = hi->fx_len[l];             /* (a longer one is 64 in x) */
    }
}
/* FX1's settings x holds (fx1s); x of another project, an older one without them, or none: Felucca's DELAY, the CLOUD's
 * defaults */
static void xtra_fx1(const proj_xtra_t *x, uint32_t proj)
{
    static const uint8_t DEF[8] = {FX1_DELAY, 50, 50, 30, 0, 10, 0, 80};
    const uint8_t *v = xtra_ok(x, proj) && x->size >= XTRA_V2 ? x->fx1 : DEF;
    fx1s.type = v[0] < FX1_NTYPE ? v[0] : FX1_DELAY;
    fx1s.size = v[1] > 100u ? 100u : v[1];
    fx1s.dens = v[2] > 100u ? 100u : v[2];
    fx1s.spread = v[3] > 100u ? 100u : v[3];
    fx1s.pitch = (int8_t)clamp((int8_t)v[4], -24, 24);
    fx1s.rnd = v[5] > 100u ? 100u : v[5];
    fx1s.fdbk = v[6] > 100u ? 100u : v[6];
    fx1s.mix = v[7] > 100u ? 100u : v[7];
}
/* SCL > CSET's ON, SET and VOICE before it of each track (cset.c), the CHORUS's TYPE and FDBK (fx.c) and GLO > KEY's
 * run (key.c) x holds; x of another project, an older one, or none: off (set 1), Felucca's chorus, no KEY. An older
 * one's ORCHID (gone): nothing */
static void xtra_misc(const proj_xtra_t *x, uint32_t proj)
{
    uint32_t k, ok = xtra_ok(x, proj) && x->size >= XTRA_V3, ok4 = ok && x->size >= XTRA_V4, ok6 = ok && x->size >= XTRA_V6, sh;
    cset_en = ok6 ? (uint8_t)(x->cset_en & ((1u << NTRK) - 1u)) : 0u;
    for (k = 0; k < NTRK; k++) {
        cset_set[k] = ok6 && x->cset_set[k] < CSET_N ? x->cset_set[k] : 0u;
        cset_voice[k] = ok6 && ((cset_en >> k) & 1u) && x->cset_voice[k] <= V_UNISON ? x->cset_voice[k] : CSET_VNONE;
    }
    chos.type = ok && x->cho[0] < CHO_NTYPE ? x->cho[0] : CHO_CHORUS;   /* (the CHORUS's: fx.c) */
    chos.fdbk = ok && x->cho[1] <= 100u ? x->cho[1] : 0u;
    sh = ok6 && (x->fx_serial & 0x80u);             /* (the SHIMMER page's; extras of before: the defaults. The */
    shim_p[0] = sh ? (int16_t)(x->shim_size & 0x7Fu) : 90;      /* chain is serial in every project) */
    shim_p[1] = sh ? (int16_t)(x->shim_damp & 0x7Fu) : 60;
    shim_p[2] = sh ? (int16_t)clamp((int32_t)(x->shim_pitch & 0x7Fu) - 12, -12, 24) : 12;
    shim_p[3] = sh ? (int16_t)(x->shim_fdbk & 0x7Fu) : 127;
    key_defaults();                                 /* GLO > KEY's run (key.c); an older one: none */
    if (ok4) {
        keyt.on = x->key_on & 1u;
        keyt.len = x->key_len >= 1u && x->key_len <= KEY_BARS ? x->key_len : 4u;
        for (k = 0; k < KEY_BARS; k++)
            keyt.root[k] = x->key_root[k] < 12u ? x->key_root[k] : KEY_NONE;
    }
    key_own = ok4 && (x->key_own & 0x80u) ? x->key_own & 1u   /* (M-Vave-OP1: the scene in the project's own KEY, */
                                           : !key_is_default();   /* as saved; extras of before: one in use is) */
}
/* hi's steps 65..128 of track k onto s (hi 0: none, they stay as loaded: empty) and its LEN onto *len when the
 * project's is 64 (what a longer one holds there); its LEGATO bits come after (xtra_leg) */
static void xtra_hi_steps(step_t *s, int16_t *len, uint32_t k, const xtra_hi_t *hi)
{
    uint32_t i;
    if (!hi)
        return;
    k %= NTRK;
    for (i = NSTEP_FMT; i < NSTEP; i++)
        if (!proj_step_unpack(&s[i], hi->step[k][i - NSTEP_FMT], 1))
            s[i] = XTRA_REST;
    if (*len == (int16_t)NSTEP_FMT && hi->len[k] > NSTEP_FMT && hi->len[k] <= NSTEP)
        *len = hi->len[k];
}
/* .. the automation and locks of steps 65..128 after m's, their ids as today's (hi->np: as a project's); none added if
 * they would not fit or are not valid (extras that do not belong together) */
static void xtra_hi_motion(motion_store_t *m, const xtra_hi_t *hi)
{
    static motion_store_t t __attribute__((section(".pool")));   /* (main loop only) */
    if (!hi || !hi->mo.count)
        return;
    memset(&t, 0, sizeof t);
    if (!motion_from_disk(&t, &hi->mo, 1) || !proj_motion_ids(&t, hi->np ? hi->np : P_COUNT) ||
        t.count > MOTION_MAX - m->count)
        return;
    memcpy(m->event + m->count, t.event, t.count * sizeof t.event[0]);
    m->count = (uint8_t)(m->count + t.count);
    if (!motion_valid(m)) {
        memset(m->event + m->count - t.count, 0, t.count * sizeof t.event[0]);
        m->count = (uint8_t)(m->count - t.count);
    }
}
/* the LEGATO steps x (and hi: 65..128) hold for track k onto its steps s (as loaded: none); x of another project, an
 * older one, or none: none (each note starts its envelope, as that firmware played it) */
static void xtra_leg(step_t *s, uint32_t k, const proj_xtra_t *x, uint32_t proj, const xtra_hi_t *hi)
{
    uint32_t i, ok = xtra_ok(x, proj) && x->size >= XTRA_V5, on;
    for (i = 0; i < NSTEP; i++) {
        on = i < NSTEP_FMT ? ok && ((x->leg[k % NTRK][i / 8u] >> (i % 8u)) & 1u)
                           : ok && hi && ((hi->leg[k % NTRK][(i - NSTEP_FMT) / 8u] >> (i % 8u)) & 1u);
        s[i].probability = (uint8_t)((s[i].probability & ~STEP_LEG) | (on ? STEP_LEG : 0u));
    }
}
/* MIX 2's levels (params.c fx_lvl) from v (XTRA_V11's, each 0x80 | the level); v 0 (extras of before, or none) or a
 * byte without its mark: full, as the chain played before there were levels */
static void xtra_lvl(const uint8_t *v)
{
    uint32_t k;
    for (k = 0; k < 4u; k++)
        fx_lvl[k] = v && (v[k] & 0x80u) ? (int16_t)(v[k] & 0x7Fu) : FX_LVL_DEF;
}
/* a project loaded (its tracks, motion and lanes as the project held them): what x and hi add, for the four tracks */
static void xtra_apply(const proj_xtra_t *x, uint32_t proj, const xtra_hi_t *hi)
{
    uint32_t k, f;
    if (!xtra_ok(x, proj))
        hi = 0;
    xtra_lanes(fxt_lane, x, proj, hi);
    xtra_fx1(x, proj);
    xtra_misc(x, proj);
    for (k = 0; k < NTRK; k++) {
        xtra_hi_steps(trk[k].step, &trk[k].p[P_SLEN], k, hi);
        xtra_leg(trk[k].step, k, x, proj, hi);
    }
    f = motion_guard();
    xtra_hi_motion(&motion, hi);
    motion_unguard(f);
    ssong_clear();                                 /* (XTRA_V8: the scene song; extras of before, or a song no page
                                                    * could make: none) */
    if (hi && x->size >= XTRA_V8 && ssong_ok(&hi->song))
        ssong = hi->song;
    eq_defaults();                                 /* (XTRA_V9: the EQ; extras of before: flat. A band out of range: */
    if (hi && x->size >= XTRA_V9)                  /* its default) */
        for (k = 0; k < NTRK; k++)
            for (f = 0; f < EQ_NB; f++) {
                const uint8_t *e = hi->eq[k][f];
                if (e[0] < EQ_NF && e[1] <= 48u && e[2] < EQT_N && e[3] < EQ_NQ)
                    eq_set[k][f] = (eq_band_t){e[0], (int16_t)(e[1] - 24), e[2], e[3]};
            }
    dyn_defaults();                                /* (XTRA_V10: the MASTER's dynamics; extras of before: no compressor,
                                                    * the limiter as before. A value out of range: its default) */
    if (hi && x->size >= XTRA_V10)
        for (f = 0; f < DYN_N; f++)
            if (hi->dyn[f] >= DYN_DESC[f].min && hi->dyn[f] <= DYN_DESC[f].max)
                dyn_set[f] = hi->dyn[f];
    xtra_lvl(hi && x->size >= XTRA_V11 ? hi->lvl : 0);   /* (XTRA_V11: MIX 2's levels; before: full) */
}
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
    xtra_hi_t *h = (xtra_hi_t *)(xtra_buf + XTRA_V6);
    xtra_capture((proj_xtra_t *)xtra_buf, h, id);
    memcpy(x, xtra_buf, sizeof *x);
#if FELUCCA_FLASH
    if (flash_ok && st_save(OBJ_XPROJ0 + (s & 3u), xtra_buf, XTRA_V11))
        return 1;
#else
    xtra_hi_ram[s & 3u] = *h;
#endif
    return 0;
}
/* project_load: slot s's extras (none, or another project's: empty lanes, nothing else) */
static void xtra_loaded(uint32_t s)
{
    uint32_t id = xtra_proj_id(s);
    xtra_apply(&xtra_slot[s & 3u], id, xtra_hi_of(s, id));   /* (the steps 65..128 in st_buf: used at once) */
}
/* project_rename: slot s was written again as id; its extras (if they were its project's: was) follow it */
static void xtra_renamed(uint32_t s, uint32_t was, uint32_t id)
{
    proj_xtra_t *x = &xtra_slot[s & 3u];
    if (!xtra_ok(x, was))
        return;
    x->proj = id;
#if FELUCCA_FLASH
    if (flash_ok) {                                 /* (all it holds, its steps 65..128 too: read back, the id new) */
        proj_xtra_t r;
        uint32_t n = xtra_read(OBJ_XPROJ0 + (s & 3u), &r);
        n = n < sizeof *x ? (uint32_t)sizeof *x : n > sizeof xtra_buf ? (uint32_t)sizeof xtra_buf : n;
        memcpy(xtra_buf, st_buf, n);
        memcpy(xtra_buf, x, sizeof *x);
        st_save(OBJ_XPROJ0 + (s & 3u), xtra_buf, n);
    }
#endif
}
/* the autosave (project.c): written after it as id (0 written); read back with it (n bytes in proj_wire) */
static int xtra_autosaved(uint32_t id)
{
#if FELUCCA_FLASH
    xtra_capture((proj_xtra_t *)xtra_buf, (xtra_hi_t *)(xtra_buf + XTRA_V6), id);
    return st_save(OBJ_XAUTO, xtra_buf, XTRA_V11);
#else
    (void)id;
    return 0;
#endif
}
static void xtra_autoloaded(uint32_t id)
{
#if FELUCCA_FLASH
    proj_xtra_t x;
    uint32_t n = xtra_read(OBJ_XAUTO, &x);
    xtra_apply(&x, id, n >= XTRA_V7 && x.size >= XTRA_V7 ? (const xtra_hi_t *)(st_buf + XTRA_V6) : 0);
#else
    (void)id;
#endif
}

/* ----------------------------------------------------------- scenes --- */
/* SCENES (scene.c): each project slot's bank of scenes and the autosave's, in USR2's flash (0xB4000..0xC7FFF, 80 KiB:
 * the second user sample slot given up for them, as USR3 for the extras; eng_sample.c SMP_USER_WRITABLE). Set s (0..3 the
 * slots A..D, 4 the autosave) has two copies of two sectors each that take turns: the commit record (st_hdr_t, type
 * OBJ_SCENES, rsv[0] the project's id) at a copy's start, written last; from its +256 the bank's header (scb_hdr_t) and
 * its bytes, CRC-checked as a whole. The newest whole copy is the set's; it is a project's only while its id is that of
 * the project as written (xtra_id, as the extras): a project written another way (a rename re-tags it: scene_renamed;
 * the editor's backups: none) loads with only its scene 1, the one in the project itself. Main loop, stopped (as every
 * flash write here). A RAM store stands in for the flash in the host builds without it (FELUCCA_FLASH 0). */
#define OBJ_SCENES 34u
#define SCB_MAGIC 0x314E4353u                    /* "SCN1" */
#define SCB_SETS 5u
#define SCB_BASE(s, c) (0xB4000u + ((s) * 2u + (c)) * 0x2000u)
#define SCB_ROOM (0x2000u - 256u)                /* a copy's payload (two sectors less the commit page): header and bytes */
typedef struct {
    uint32_t magic;
    uint16_t size, ver;                          /* sizeof, 1 */
    uint32_t proj;                               /* the project the scenes belong to */
    uint8_t cur, rsv[3];                         /* the scene the project holds (its own steps: not in the bank) */
    uint16_t len[SCN_N];
    uint16_t used, rsv2;
} scb_hdr_t;
_Static_assert(sizeof(scb_hdr_t) == 52u && sizeof(scb_hdr_t) + SCN_CAP <= SCB_ROOM && SCB_BASE(SCB_SETS, 0) <= 0xC8000u,
               "scene bank layout: five sets of two 8 KiB copies in USR2");
#if FELUCCA_FLASH
_Static_assert(SCB_ROOM == 2u * ST_SECTOR - ST_PAYLOAD_OFF, "a scene bank's copy: two sectors, the commit page first");
#endif
static void scb_hdr_make(scb_hdr_t *h, uint32_t proj)
{
    memset(h, 0, sizeof *h);
    h->magic = SCB_MAGIC;
    h->size = sizeof *h;
    h->ver = 1;
    h->proj = proj;
    h->cur = scn.cur;
    memcpy(h->len, scb.len, sizeof h->len);
    h->used = scb.used;
}
/* a header read back: the bank it describes is one scene.c can take (scene_bank_check looks at the bytes) */
static int scb_hdr_ok(const scb_hdr_t *h, uint32_t proj)
{
    uint32_t k, sum = 0;
    if (h->magic != SCB_MAGIC || h->size != sizeof *h || h->ver != 1u || h->proj != proj || !proj || h->cur >= SCN_N ||
        h->used > SCN_CAP)
        return 0;
    for (k = 0; k < SCN_N; k++)
        sum += h->len[k];
    return sum == h->used;
}
/* the bank read back from h's bytes (src: the bytes after the header); 0 not taken (the bank emptied) */
static int scb_take(const scb_hdr_t *h)
{
    memcpy(scb.len, h->len, sizeof scb.len);
    scb.used = h->used;
    scb.gen++;
    if (!scene_bank_check(h->cur)) {
        scene_reset();
        return 0;
    }
    scn.cur = h->cur;
    return 1;
}
#if FELUCCA_FLASH
static int scb_head(uint32_t s, uint32_t c, st_hdr_t *h)   /* copy c of set s: a whole commit record: 0 */
{
    if (st_read(SCB_BASE(s, c), h, sizeof *h))
        return -1;
    return h->magic == ST_MAGIC && h->type == OBJ_SCENES && h->slot == c && h->len >= sizeof(scb_hdr_t) &&
           h->len <= SCB_ROOM && h->hcrc == st_crc32(h, sizeof *h - 4u) ? 0 : -1;
}
static uint32_t scb_crc(uint32_t s, uint32_t c, uint32_t len)   /* the payload's CRC as it is in flash */
{
    uint8_t b[256];
    uint32_t off, crc = 0xFFFFFFFFu, n;
    for (off = 0; off < len; off += n) {
        n = len - off < sizeof b ? len - off : sizeof b;
        st_read(SCB_BASE(s, c) + ST_PAYLOAD_OFF + off, b, n);
        crc = st_crc32_add(crc, b, n);
    }
    return ~crc;
}
/* set s's newest whole copy (its CRC checked): 0 or 1, -1 none */
static int scb_newest(uint32_t s, st_hdr_t *out, uint32_t *seq)
{
    st_hdr_t h[2];
    uint32_t c, v = 0;
    int best = -1;
    *seq = 0;
    for (c = 0; c < 2u; c++)
        if (flash_ok && scb_head(s, c, &h[c]) == 0) {
            if (!*seq || h[c].seq - *seq < 0x80000000u)
                *seq = h[c].seq;                 /* (the next write numbers after both) */
            if (scb_crc(s, c, h[c].len) == h[c].crc)
                v |= 1u << c;
        }
    if ((v & 3u) == 3u)
        best = h[1].seq != h[0].seq && h[1].seq - h[0].seq < 0x80000000u ? 1 : 0;   /* (A on a tie) */
    else if (v)
        best = (v & 1u) ? 0 : 1;
    if (best >= 0)
        *out = h[best];
    return best;
}
/* the payload (n bytes of the header h, then from) into copy c of set s, page by page from RAM, and its commit record
 * last (rsv[0]: proj). 0 written and read back */
static int scb_prog(uint32_t s, uint32_t c, uint32_t seq, const scb_hdr_t *hd, const uint8_t *from, uint32_t n,
                    uint32_t proj)
{
    uint8_t pg[256];
    uint32_t base = SCB_BASE(s, c), len = (uint32_t)sizeof *hd + n, off, i, crc = 0xFFFFFFFFu;
    st_hdr_t h, chk;
    int rc = st_erase(base);
    if (!rc)
        rc = st_erase(base + ST_SECTOR);
    for (off = 0; off < len && !rc; off += sizeof pg) {
        uint32_t m = len - off < sizeof pg ? len - off : sizeof pg;
        for (i = 0; i < m; i++)
            pg[i] = off + i < sizeof *hd ? ((const uint8_t *)hd)[off + i] : from[off + i - sizeof *hd];
        crc = st_crc32_add(crc, pg, m);
        rc = st_prog(base + ST_PAYLOAD_OFF + off, pg, m);
    }
    if (!rc) {
        h.magic = ST_MAGIC;
        h.type = OBJ_SCENES;
        h.slot = (uint16_t)c;
        h.seq = seq + 1u;
        h.len = len;
        h.crc = ~crc;
        h.rsv[0] = proj;
        h.rsv[1] = 0xFFFFFFFFu;
        h.hcrc = st_crc32(&h, sizeof h - 4u);
        rc = st_prog(base, &h, sizeof h);        /* the commit record, last */
    }
    if (!rc && (scb_head(s, c, &chk) || chk.seq != h.seq || scb_crc(s, c, len) != h.crc))
        rc = -7;                                 /* (read back: a failing part must not count as kept) */
    return rc;
}
/* the bank as it is now -> set s, the project id proj; 0 written */
static int scb_write(uint32_t s, uint32_t proj)
{
    st_hdr_t h;
    scb_hdr_t hd;
    uint32_t seq;
    int c;
    if (!flash_ok)
        return -1;
    c = scb_newest(s, &h, &seq);
    scb_hdr_make(&hd, proj);
    return scb_prog(s, c == 0 ? 1u : 0u, seq, &hd, scb.data, scb.used, proj);
}
/* set s's bank, if it is project proj's, into the bank (the scene it holds playing: scene.c); 0 none */
static int scb_load(uint32_t s, uint32_t proj)
{
    st_hdr_t h;
    scb_hdr_t hd;
    uint32_t seq;
    int c = scb_newest(s, &h, &seq);
    scene_reset();
    if (c < 0 || h.rsv[0] != proj || h.len < sizeof hd)
        return 0;
    st_read(SCB_BASE(s, (uint32_t)c) + ST_PAYLOAD_OFF, &hd, sizeof hd);
    if (!scb_hdr_ok(&hd, proj) || h.len != sizeof hd + hd.used)
        return 0;
    st_read(SCB_BASE(s, (uint32_t)c) + ST_PAYLOAD_OFF + sizeof hd, scb.data, hd.used);
    return scb_take(&hd);
}
/* set s's newest copy, if it is project was's, copied to the other copy as project id's (a rename: the bank in RAM may
 * be another project's); 0 done or nothing to do */
static int scb_retag(uint32_t s, uint32_t was, uint32_t id)
{
    st_hdr_t h, n, chk;
    uint8_t pg[256];
    uint32_t seq, off, crc = 0xFFFFFFFFu, base;
    int c = scb_newest(s, &h, &seq), rc;
    if (c < 0 || h.rsv[0] != was || !id)
        return 0;
    base = SCB_BASE(s, c == 0 ? 1u : 0u);
    rc = st_erase(base);
    if (!rc)
        rc = st_erase(base + ST_SECTOR);
    for (off = 0; off < h.len && !rc; off += sizeof pg) {
        uint32_t m = h.len - off < sizeof pg ? h.len - off : sizeof pg;
        st_read(SCB_BASE(s, (uint32_t)c) + ST_PAYLOAD_OFF + off, pg, m);
        if (!off)                                /* (the header's id: in the first page; pg need not be aligned) */
            memcpy(pg + __builtin_offsetof(scb_hdr_t, proj), &id, sizeof id);
        crc = st_crc32_add(crc, pg, m);
        rc = st_prog(base + ST_PAYLOAD_OFF + off, pg, m);
    }
    if (!rc) {
        n = h;
        n.slot = (uint16_t)(c == 0 ? 1u : 0u);
        n.seq = seq + 1u;
        n.crc = ~crc;
        n.rsv[0] = id;
        n.hcrc = st_crc32(&n, sizeof n - 4u);
        rc = st_prog(base, &n, sizeof n);
    }
    if (!rc && (scb_head(s, n.slot, &chk) || chk.seq != n.seq || scb_crc(s, n.slot, h.len) != n.crc))
        rc = -7;
    return rc;
}
#else
static struct {                                  /* (host builds without the flash: the sets in RAM) */
    scb_hdr_t h;
    uint8_t data[SCN_CAP];
} scb_ram[SCB_SETS];
static int scb_write(uint32_t s, uint32_t proj)
{
    scb_hdr_make(&scb_ram[s].h, proj);
    memcpy(scb_ram[s].data, scb.data, scb.used);
    return 0;
}
static int scb_load(uint32_t s, uint32_t proj)
{
    scene_reset();
    if (!scb_hdr_ok(&scb_ram[s].h, proj))
        return 0;
    memcpy(scb.data, scb_ram[s].data, scb_ram[s].h.used);
    return scb_take(&scb_ram[s].h);
}
static int scb_retag(uint32_t s, uint32_t was, uint32_t id)
{
    if (scb_ram[s].h.magic == SCB_MAGIC && scb_ram[s].h.proj == was && id)
        scb_ram[s].h.proj = id;
    return 0;
}
#endif
/* project_save_as: the project went into slot s as id; its scenes after it (and its extras). 0 kept */
static int scene_saved(uint32_t s, uint32_t id)
{
    scene_settle();
    return scb_write(s & 3u, id);
}
/* project_load: slot s's scenes, if they are its project's (the scene playing: the one the project held) */
static void scene_loaded(uint32_t s) { scb_load(s & 3u, xtra_proj_id(s)); scene_swing_folded(); }
static void scene_renamed(uint32_t s, uint32_t was, uint32_t id) { scb_retag(s & 3u, was, id); }
static int scene_autosaved(uint32_t id)
{
    scene_settle();
    return scb_write(4u, id);
}
static void scene_autoloaded(uint32_t id) { scb_load(4u, id); scene_swing_folded(); }
