/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DBOX's key pages (M-Vave-OP1; EDIT family, a DBOX track only, after EDIT 2; docs/VDRUM_DBOX_SPEC.md): the sound of
 * one key edited on the device, as on the OP-1's DBOX (oscillator 1 without SHIFT, oscillator 2 with it: the FM-1 has
 * no SHIFT, so they are two pages).
 *   OSC 1   PIT1  WAV1  ENV1  XMOD
 *   OSC 2   PIT2  WAV2  ENV2  CUT
 *   SOUND   MODE  LVL
 * The key is the one last struck by hand on the track (seq.c input_on sets dbx_pick; the sequencer's notes never move
 * it). A detent writes one byte of the track's own kit (dbx_kit_set: a factory kit is copied into it first, KIT shows
 * OWN), one undo per knob gesture (SAVE held); projects and user presets keep it (eng_dbox.c dbx_pack). The panel: the
 * key, its MODE, both oscillators' envelopes (the page's in the accent). Included by ui_graph.c (after ui_vdrum.c). */
enum { DU_PIT1, DU_WAV1, DU_ENV1, DU_XMOD, DU_PIT2, DU_WAV2, DU_ENV2, DU_CUT, DU_MODE, DU_LVL, DU_NONE };
static const uint8_t DU_PAGE[3][4] = {
    {DU_PIT1, DU_WAV1, DU_ENV1, DU_XMOD},            /* OSC 1 */
    {DU_PIT2, DU_WAV2, DU_ENV2, DU_CUT},             /* OSC 2 */
    {DU_MODE, DU_LVL, DU_NONE, DU_NONE},             /* SOUND */
};
static const char *const DU_LABEL[DU_NONE] = {"PITCH", "WAVE", "ENV", "XMOD", "PITCH", "WAVE", "ENV", "CUT", "MODE", "LVL"};
static const uint8_t DU_OFF[DU_NONE] = {DK_PIT1, DK_WAV1, DK_ENV1, DK_XMOD, DK_PIT2, DK_WAV2, DK_ENV2, DK_CUT, DK_MODE,
                                        DK_LVL};

static int dbu_page(uint32_t g) { return g >= GR_DBO1 && g <= GR_DBHIT; }
static int dbu_ok(void) { return TSEL->eng_req % NENGINES == ENGI_DBOX; }
static uint32_t dbu_key(void) { uint32_t k = dbx_pick[song.sel % NTRK]; return k ? (k - 1u) % DBX_NK : 0u; }
static uint32_t dbu_field(uint32_t slot) { return DU_PAGE[(cur_page()->graph - GR_DBO1) % 3u][slot & 3u]; }
static uint32_t dbu_max(uint32_t f) { return f == DU_MODE ? 6u : 127u; }

/* byte b of track tr's own kit = v (main loop): as vd_kit_set */
static void dbx_kit_set(uint32_t tr, uint32_t b, uint32_t v)
{
    track_t *t = &trk[tr % NTRK];
    const uint8_t *K = dbx_kit_of(t);
    if (!K || b >= VK_SIZE || K[b] == v)
        return;
    edit_undo_take(t, 0x3000u | b, UNDO_SOUND);
    if (K != dbx_own[tr % NTRK])
        memcpy(dbx_own[tr % NTRK], K, VK_SIZE);
    dbx_own[tr % NTRK][b] = (uint8_t)v;
    t->p[P_E0] = DBX_OWN;
    motion_undo_done(t);
}

static void dbu_knob(uint32_t slot, int32_t steps)
{
    uint32_t f = dbu_field(slot), b;
    int32_t max = (int32_t)dbu_max(f), d;
    const uint8_t *K = dbx_kit_of(TSEL);
    if (!dbu_ok() || !steps || f == DU_NONE || !K)
        return;
    d = max < 8 ? (steps > 0 ? 1 : -1) : accel(EN_K1 + slot, steps, max);
    b = DK_KEY(dbu_key()) + DU_OFF[f];
    dbx_kit_set(song.sel % NTRK, b, (uint32_t)clamp((int32_t)K[b] + d, 0, max));
}

static int dbu_card(uint32_t slot, const char **label, char *val, const char **unit, int32_t *ratio, uint32_t *icon)
{
    uint32_t f = dbu_field(slot), v;
    const uint8_t *K = dbx_kit_of(TSEL);
    *unit = "";
    *ratio = -1;
    *icon = ICON_AUTO;
    val[0] = 0;
    if (f == DU_NONE || !K) {
        *label = "";
        return 0;
    }
    *label = DU_LABEL[f];
    v = K[DK_KEY(dbu_key()) + DU_OFF[f]] & 127u;
    if (f == DU_MODE) {
        str_cpy(val, N_DBX_MODE[v % 7u], 12);
        return 1;
    }
    if (f == DU_WAV1 || f == DU_WAV2) {
        static const char *const W[5] = {"SIN", "TRI", "SAW", "SQR", "NOIS"};
        str_cpy(val, W[(v + 16u) / 32u > 4u ? 4u : (v + 16u) / 32u], 12);   /* (the nearest shape's name) */
        *ratio = (int32_t)(v * 1000u / 127u);
        return 1;
    }
    fmt_int(val, (int32_t)v);
    *ratio = (int32_t)(v * 1000u / 127u);
    return f != DU_LVL && f != DU_XMOD ? 1 : v != 0u;
}

static uint32_t dbu_sig(void)
{
    const uint8_t *K = dbx_kit_of(TSEL);
    uint32_t h = 2166136261u, i, k = dbu_key();
    if (!K)
        return 0;
    for (i = 0; i < 10u; i++)
        h = (h ^ K[DK_KEY(k) + i]) * 16777619u;
    return h ^ (k + 1u) * 40503u ^ (uint32_t)TSEL->p[P_E0] * 131u;
}
static void graph_dbox(const track_t *t, uint16_t c)
{
    const uint8_t *K = dbx_kit_of(t), *b;
    uint32_t k = dbu_key(), o, g = cur_page()->graph;
    int32_t top = 26, bot = 88;
    char s[24];
    if (!K)
        return;
    b = K + DK_KEY(k);
    cv_rect(PANEL_X0, bot + 2, PANEL_W, 1, T_RAISE);
    for (o = 2u; o-- > 0u;) {                        /* osc 2 first: the page's on top */
        uint32_t env = b[o ? DK_ENV2 : DK_ENV1] & 127u, atk = dbx_atk(env);
        int32_t a = (int32_t)atk * 40 / 60, r = 6 + (int32_t)dbx_dec(env, 64) * 150 / 127, x, px = 12, py = bot, e = 0;
        uint16_t col = (g == GR_DBO2) == (o == 1u) || g == GR_DBHIT ? T_ACCENT : c;
        for (x = 1; x <= 216; x++) {
            if (x <= a)
                e = 1000 * x / (a ? a : 1);
            else
                e = (x == a + 1 && !a) ? 1000 : e - e * 46 / (10 * r) - 1;
            e = clamp(e, 0, 1000);
            cv_line_t(px, py, 12 + x, bot - e * (bot - top) / 1000, col, 2);
            px = 12 + x;
            py = bot - e * (bot - top) / 1000;
        }
    }
    str_cpy(s, "KEY ", sizeof s);
    str_cpy(s + 4, N_DBX_KEY[k], 3);
    cv_text(10, 4 - GOY, &AF_S, s, T_MID);
    cv_text_r(228, 4 - GOY, &AF_S, N_DBX_MODE[(b[DK_MODE] & 7u) % 7u], T_TEXT, T_SURF);
}
