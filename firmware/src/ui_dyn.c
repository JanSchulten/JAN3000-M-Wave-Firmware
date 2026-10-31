/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: HOME > MASTER's panel (the page: params.c PAGES "MASTER", its knobs dyn_page_desc; the sound: dyn.c).
 * Included by ui_pages.c (DYN_OPS). On the left the master's static curve: a peak level into the compressor
 * (-36 .. +12 dBFS across, 2 px a dB) against what leaves the limiter (the same scale up), the knee, GAIN and LIMIT's
 * ceiling in it, the 1:1 line and 0 dBFS thin, the ceiling in REC's red as the limiter's meter. On the right two
 * meters, the compressor's and the limiter's gain reduction (0 .. 12 dB), dyn_poll's peaks with their fall (dyn_mtr).
 * The main loop's floats */
#define DYG_Y 6                                       /* the curve's square: top */
#define DYG_S 96                                      /* .. its side: 48 dB */
#define DYG_TOP 12                                    /* .. dBFS at its top and right edge */
#define DYM_X 126                                     /* the meters: left */
#define DYM_W (PANEL_X0 + PANEL_W - DYM_X)            /* .. their width: 12 dB */
/* the y of an output level in dBFS, on the square */
FM1_FPU static int32_t dyg_y(float db)
{
    int32_t y = DYG_Y + (int32_t)(((float)DYG_TOP - db) * 2.0f + 0.5f);
    return clamp(y, DYG_Y, DYG_Y + DYG_S);
}
/* the master's static curve at input level in (dBFS): the compressor's (dyn.c master_dyn's gain computer), GAIN, the
 * limiter's ceiling */
FM1_FPU static float dyg_out(float in)
{
    float ov = in - dynx.thr, gr, lim = 6.02059991f * (dyn_log2((float)lim_t) - 15.0f), o;
    gr = ov <= -DYN_KNEE ? 0.0f : ov < DYN_KNEE ? dynx.slope * (ov + DYN_KNEE) * (ov + DYN_KNEE) * (0.25f / DYN_KNEE)
                                                : dynx.slope * ov;
    o = in + gr + dynx.mk;
    return o < lim ? o : lim;
}
/* a meter: its name, the reduction in half dB (m), a bar of 12 dB */
static void dym_draw(int32_t y, const char *name, uint32_t m, uint16_t c)
{
    char v[12];
    int32_t w = (int32_t)(m > 24u ? 24u : m) * DYM_W / 24;
    cv_text(DYM_X, y, &AF_S, name, T_MID);
    if (m) {
        v[0] = '-';
        fmt_fix(v + 1, (int32_t)m * 5, 1);
    } else {
        str_cpy(v, "0.0", sizeof v);
    }
    str_cpy(v + str_len(v), "dB", 3);
    cv_text_r(DYM_X + DYM_W, y, &AF_S, v, T_TEXT, cv_bg);
    cv_rect(DYM_X, y + 18, DYM_W, 8, T_LINE);
    if (w)
        cv_rect(DYM_X, y + 18, w, 8, c);
}
FM1_FPU static void graph_dyn(const track_t *t, uint16_t c)
{
    int32_t x, px = 0, py = 0, x0 = PANEL_X0 + 36 * 2, y0 = DYG_Y + DYG_TOP * 2;
    (void)t;
    cv_rect(PANEL_X0, DYG_Y, DYG_S, 1, T_LINE);       /* the square's frame */
    cv_rect(PANEL_X0, DYG_Y + DYG_S, DYG_S + 1, 1, T_LINE);
    cv_rect(PANEL_X0, DYG_Y, 1, DYG_S, T_LINE);
    cv_rect(PANEL_X0 + DYG_S, DYG_Y, 1, DYG_S, T_LINE);
    cv_rect(x0, DYG_Y, 1, DYG_S, T_RAISE);            /* 0 dBFS in, out */
    cv_rect(PANEL_X0, y0, DYG_S, 1, T_RAISE);
    cv_line(PANEL_X0, DYG_Y + DYG_S, PANEL_X0 + DYG_S, DYG_Y, T_MID);   /* 1:1 */
    cv_rect(PANEL_X0 + 1, dyg_y(6.02059991f * (dyn_log2((float)lim_t) - 15.0f)), DYG_S - 1, 1, T_REC);   /* LIMIT */
    for (x = 0; x <= DYG_S; x++) {
        int32_t y = dyg_y(dyg_out((float)x * 0.5f - 36.0f));
        if (x)
            cv_line_t(px, py, PANEL_X0 + x, y, c, 2);
        px = PANEL_X0 + x, py = y;
    }
    dym_draw(DYG_Y, "COMP", dyn_mtr[0], c);
    dym_draw(DYG_Y + 44, "LIMIT", dyn_mtr[1], T_REC);
}
/* .. everything graph_dyn draws: the settings, the meters */
static uint32_t dyn_sig(uint32_t h)
{
    uint32_t i;
    for (i = 0; i < DYN_N; i++)
        h = (h ^ (uint32_t)(uint16_t)dyn_set[i]) * 16777619u;
    return (h ^ (dyn_mtr[0] | (uint32_t)dyn_mtr[1] << 8)) * 16777619u;
}
/* the MASTER page's knobs (page_ops_t desc): COMP RELEASE LIMIT GAIN */
static const param_desc_t *dyn_page_desc(uint32_t slot, int16_t **valp)
{
    *valp = &dyn_set[slot & 3u];
    return &DYN_DESC[slot & 3u];
}
