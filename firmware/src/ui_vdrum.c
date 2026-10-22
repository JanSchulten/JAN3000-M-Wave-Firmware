/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* VDRUM's part pages (M-Vave-OP1; EDIT family, a VDRUM track only, after EDIT 2; docs/VDRUM_DBOX_SPEC.md): the kit's
 * bytes of one part edited on the device, as FM6's operator pages edit its patch (ui_fm6op.c).
 *   LAYER        LYR  SRC  MOD  EG
 *   LAYER PITCH  LYR  PITCH  MAMT  MRAT
 *   LAYER ENV    LYR  ATK  REL  LVL
 *   PART         BIT  FOLD  DRV  GAIN
 *   PART 2       SEND  CHOK
 * The part is the one last struck by hand on the track (keys, MIDI in: seq.c input_on sets vd_pick; the sequencer's
 * notes never move it), as the OP-1's DBOX takes the key pressed. LYR (one choice for the three layer pages, kept
 * while one goes round them) is L1, L2 or L1+L2: then a detent moves both layers' values by the same step (each held
 * in its range) and the cards show layer 1's. A detent writes one byte of the track's own kit (vd_kit_set: a factory
 * kit is copied into it first and KIT shows OWN), one undo per knob gesture (SAVE held). The bytes are kept by
 * projects and user presets with the kit (eng_vdrum.c vd_pack); nothing here is automatable or lockable. The panel:
 * the part, the layer's sound in words, both layers' envelopes (the picked one in the accent). Included by ui_graph.c
 * (after ui_fm6op.c); the cards: ui_draw.c, the knobs: ui_input.c edit_param. */
static uint8_t vdu_lyr;                              /* 0 L1, 1 L2, 2 L1+L2 */

enum { VU_LYR, VU_SRC, VU_MOD, VU_EG, VU_PIT, VU_MAMT, VU_MRAT, VU_ATK, VU_REL, VU_LVL,
       VU_BIT, VU_FOLD, VU_DRV, VU_GAIN, VU_SEND, VU_CHOK, VU_NONE };
static const uint8_t VU_PAGE[5][4] = {
    {VU_LYR, VU_SRC, VU_MOD, VU_EG},                 /* LAYER */
    {VU_LYR, VU_PIT, VU_MAMT, VU_MRAT},              /* LAYER PITCH */
    {VU_LYR, VU_ATK, VU_REL, VU_LVL},                /* LAYER ENV */
    {VU_BIT, VU_FOLD, VU_DRV, VU_GAIN},              /* PART */
    {VU_SEND, VU_CHOK, VU_NONE, VU_NONE},            /* PART 2 */
};
static const char *const VU_LABEL[VU_NONE] = {"LYR", "SRC", "MOD", "EG", "PITCH", "MAMT", "MRAT", "ATK", "REL", "LVL",
                                              "BIT", "FOLD", "DRV", "GAIN", "SEND", "CHOK"};
static const char *const N_VU_LYR[3] = {"L1", "L2", "L1+L2"};

static int vdu_page(uint32_t g) { return g >= GR_VDL && g <= GR_VDP2; }
static int vdu_ok(void) { return TSEL->eng_req % NENGINES == ENGI_VDRUM; }
static uint32_t vdu_part(void) { uint32_t k = vd_pick[song.sel % NTRK]; return k ? (k - 1u) % VD_NP : 0u; }
static uint32_t vdu_field(uint32_t slot) { return VU_PAGE[(cur_page()->graph - GR_VDL) % 5u][slot & 3u]; }
static int vdu_layered(uint32_t f) { return f >= VU_SRC && f <= VU_LVL; }
static uint32_t vdu_max(uint32_t f) { return f == VU_SRC ? 4u : f == VU_MOD || f == VU_EG ? 2u : f == VU_CHOK ? 3u : 127u; }

/* the value of field f of layer l (0 / 1; the part's fields: l is not used) in kit K's part k */
static uint32_t vdu_get(const uint8_t *K, uint32_t k, uint32_t l, uint32_t f)
{
    const uint8_t *b = K + VK_PART(k % VD_NP), *L = b + 7u * (l & 1u);
    switch (f) {
    case VU_SRC: return (L[VK_LTYPE] & 7u) % 5u;
    case VU_MOD: return ((uint32_t)L[VK_LTYPE] >> 3 & 3u) % 3u;
    case VU_EG: return ((uint32_t)L[VK_LTYPE] >> 5) % 3u;
    case VU_PIT: return L[VK_LPIT] & 127u;
    case VU_MAMT: return L[VK_LMAMT] & 127u;
    case VU_MRAT: return L[VK_LMRAT] & 127u;
    case VU_ATK: return L[VK_LATK] & 127u;
    case VU_REL: return L[VK_LREL] & 127u;
    case VU_LVL: return L[VK_LLVL] & 127u;
    case VU_BIT: return b[VK_PBIT] & 127u;
    case VU_FOLD: return b[VK_PFOLD] & 127u;
    case VU_DRV: return b[VK_PDRV] & 127u;
    case VU_GAIN: return b[VK_PGAIN] & 127u;
    case VU_SEND: return b[VK_PSEND] & 127u;
    case VU_CHOK: return b[VK_PCHOK] & 3u;
    default: return 0;
    }
}
/* .. its byte in the kit, and the byte with value v put in */
static uint32_t vdu_byte(uint32_t k, uint32_t l, uint32_t f)
{
    static const uint8_t OFF[VU_NONE] = {0, VK_LTYPE, VK_LTYPE, VK_LTYPE, VK_LPIT, VK_LMAMT, VK_LMRAT, VK_LATK, VK_LREL,
                                         VK_LLVL, VK_PBIT, VK_PFOLD, VK_PDRV, VK_PGAIN, VK_PSEND, VK_PCHOK};
    return VK_PART(k % VD_NP) + (vdu_layered(f) ? 7u * (l & 1u) : 0u) + OFF[f % VU_NONE];
}
static uint32_t vdu_put(uint32_t old, uint32_t f, uint32_t v)
{
    if (f == VU_SRC)
        return (old & ~7u) | v;
    if (f == VU_MOD)
        return (old & ~0x18u) | v << 3;
    if (f == VU_EG)
        return (old & ~0x60u) | v << 5;
    return v;
}

/* byte b of track tr's own kit = v (main loop): a factory kit becomes the track's own first (KIT OWN), one undo per
 * knob gesture; the ISR takes it at its next block */
static void vd_kit_set(uint32_t tr, uint32_t b, uint32_t v)
{
    track_t *t = &trk[tr % NTRK];
    const uint8_t *K = vd_kit_of(t);
    if (!K || b >= VK_SIZE || K[b] == v)
        return;
    edit_undo_take(t, 0x2000u | b, UNDO_SOUND);      /* (a knob turned on, this byte: one copy) */
    if (K != vd_own[tr % NTRK])
        memcpy(vd_own[tr % NTRK], K, VK_SIZE);       /* (a factory kit, or INIT for an own kit not made yet) */
    vd_own[tr % NTRK][b] = (uint8_t)v;
    vd_kgen[tr % NTRK]++;
    t->p[P_E4] = VD_OWN;
    motion_undo_done(t);
}

/* KNOB slot turned by steps on a part page */
static void vdu_knob(uint32_t slot, int32_t steps)
{
    uint32_t f = vdu_field(slot), k = vdu_part(), tr = song.sel % NTRK, l;
    int32_t max = (int32_t)vdu_max(f), d;
    if (!vdu_ok() || !steps || f == VU_NONE)
        return;
    if (f == VU_LYR) {
        vdu_lyr = (uint8_t)clamp((int32_t)vdu_lyr + steps, 0, 2);
        return;
    }
    d = max < 8 ? (steps > 0 ? 1 : -1) : accel(EN_K1 + slot, steps, max);
    for (l = 0; l < 2u; l++) {
        uint32_t b, v;
        if (vdu_layered(f) ? (vdu_lyr != 2u && l != vdu_lyr) : l)
            continue;
        b = vdu_byte(k, l, f);
        v = (uint32_t)clamp((int32_t)vdu_get(vd_kit_of(TSEL), k, l, f) + d, 0, max);
        vd_kit_set(tr, b, vdu_put(vd_kit_of(TSEL)[b], f, v));
    }
}

/* card slot of a part page: its label, value (12), unit, gauge (0..1000, -1 none), icon; 0 = shown DIM (a level at 0,
 * an empty card) */
static int vdu_card(uint32_t slot, const char **label, char *val, const char **unit, int32_t *ratio, uint32_t *icon)
{
    uint32_t f = vdu_field(slot), v;
    const uint8_t *K = vd_kit_of(TSEL);
    *unit = "";
    *ratio = -1;
    *icon = ICON_AUTO;
    val[0] = 0;
    if (f == VU_NONE || !K) {
        *label = "";
        return 0;
    }
    *label = VU_LABEL[f];
    if (f == VU_LYR) {
        str_cpy(val, N_VU_LYR[vdu_lyr % 3u], 12);
        *ratio = (int32_t)vdu_lyr * 500;
        return 1;
    }
    v = vdu_get(K, vdu_part(), vdu_lyr == 1u ? 1u : 0u, f);
    if (f == VU_SRC || f == VU_MOD || f == VU_EG) {
        str_cpy(val, (f == VU_SRC ? N_VD_SRC : f == VU_MOD ? N_VD_MOD : N_VD_EG)[v], 12);
        return 1;
    }
    if (f == VU_CHOK) {
        if (v)
            fmt_int(val, (int32_t)v);
        else
            str_cpy(val, "OFF", 12);
        return v != 0u;
    }
    fmt_int(val, (int32_t)v);
    *ratio = (int32_t)(v * 1000u / 127u);
    return f != VU_LVL && f != VU_GAIN && f != VU_SEND ? 1 : v != 0u;
}

/* the panel: "3 CLAP" and the layer in words, both layers' envelopes over 216 px (ATK REL as the engine's times on a
 * log scale), the layer picked (L1+L2: both) in the accent, a silent one DIM */
static uint32_t vdu_sig(void)
{
    const uint8_t *K = vd_kit_of(TSEL);
    uint32_t h = 2166136261u, i, k = vdu_part();
    if (!K)
        return 0;
    for (i = 0; i < 21u; i++)
        h = (h ^ K[VK_PART(k) + i]) * 16777619u;
    return h ^ (k + 1u) * 40503u ^ (vdu_lyr + 1u) * 7919u ^ (uint32_t)TSEL->p[P_E4] * 131u;
}
static void graph_vdrum(const track_t *t, uint16_t c)
{
    const uint8_t *K = vd_kit_of(t);
    uint32_t k = vdu_part(), l;
    int32_t top = 26, bot = 88;
    char b[24];
    if (!K)
        return;
    cv_rect(PANEL_X0, bot + 2, PANEL_W, 1, T_RAISE);
    for (l = 2u; l-- > 0u;) {                        /* layer 2 first: the picked one on top */
        const uint8_t *L = K + VK_PART(k) + 7u * l;
        uint32_t eg = ((uint32_t)L[VK_LTYPE] >> 5) % 3u, lv = L[VK_LLVL] & 127u;
        int32_t a = (L[VK_LATK] & 127) * 40 / 127, r = 8 + (L[VK_LREL] & 127) * 150 / 127, x, px = 12, py = bot;
        int32_t e = 0, hgt = (int32_t)(lv * lv) * (bot - top) / 16129;
        uint16_t col = !lv ? T_DIM : (vdu_lyr == 2u || vdu_lyr == l) ? T_ACCENT : c;
        for (x = 1; x <= 216; x++) {
            if (eg == VE_MULT) {                     /* three strikes ATK apart, short decays, then REL's */
                int32_t s = 3 + a / 2;
                e = x == 1 || x == 1 + s || x == 1 + 2 * s ? 1000 : x < 1 + 2 * s ? e * 3 / 4 : e - e * 46 / (10 * r) - 1;
            } else if (x <= a) {                     /* the attack: linear (AD), curved (EXP) */
                e = eg == VE_EXP ? 1000 * x * x / (a * a) : 1000 * x / a;
            } else {                                 /* the decay: ~99 % in REL's width */
                e = (x == a + 1 && !a) ? 1000 : e - e * 46 / (10 * r) - 1;
            }
            e = clamp(e, 0, 1000);
            cv_line_t(px, py, 12 + x, bot - e * hgt / 1000, col, 2);
            px = 12 + x;
            py = bot - e * hgt / 1000;
        }
    }
    b[0] = (char)('1' + k);
    b[1] = ' ';
    str_cpy(b + 2, N_VD_PART[k], sizeof b - 2u);
    cv_text(10, 4 - GOY, &AF_S, b, T_MID);
    l = vdu_lyr == 1u ? 1u : 0u;
    str_cpy(b, N_VU_LYR[vdu_lyr % 3u], sizeof b);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), N_VD_SRC[vdu_get(K, k, l, VU_SRC)], 5);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), N_VD_MOD[vdu_get(K, k, l, VU_MOD)], 5);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), N_VD_EG[vdu_get(K, k, l, VU_EG)], 5);
    cv_text_r(228, 4 - GOY, &AF_S, b, T_TEXT, T_SURF);
}
