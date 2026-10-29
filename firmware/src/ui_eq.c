/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: HOME > EQ's panel (the page: params.c PAGES "EQ", its knobs eq_page_desc; the sound: eq.c). Included by
 * ui_graph.c, whose GRAPH_OPS hands it draw_graph's panel and graph_signature's state. */
/* M-Vave-OP1, HOME > EQ: the selected track's response, 20 Hz .. 20 kHz over the panel (a point a FREQ step, 121), +-12 dB
 * around the 0 dB line (beyond: at the edge); the grid at 100 Hz, 1 kHz, 10 kHz; the band picked (KNOB 1) as its own
 * curve, thin, the sum of the four in the theme's colour, thick; each band a numbered point at its FREQ and its own
 * level there (a shelf's or a bell's GAIN; a LP / HP's resonance), the one picked filled. The main loop's floats */
#define EQG_TOP 6                                    /* +12 dB (100 px scale) */
#define EQG_MID 44                                   /* 0 dB */
#define EQG_DB 38                                    /* px for 12 dB */
#define EQG_LAB 88                                   /* the grid's labels */
FM1_FPU static float eq_log2(float x)                /* log2 x for x > 0, ~0.001 (the curve's pixel needs 0.03) */
{
    union { float f; uint32_t u; } v = {x};
    float e = (float)((int32_t)((v.u >> 23) & 255u) - 127), m;
    v.u = (v.u & 0x007FFFFFu) | 0x3F800000u;         /* the mantissa, 1 .. 2 */
    m = v.f;
    return e + (-0.34484843f * m + 2.02466578f) * m - 0.67487759f;
}
/* the level in dB of the coefficients c at FREQ step f: |H|^2 in RBJ's form of phi = sin^2(w / 2) (the one in cos w
 * loses a shelf's few hundredths of a dB near 0 Hz to the float's cancellation: the curve shook below 60 Hz) */
FM1_FPU static float eq_db_at(const eq_coef_t *c, int32_t f)
{
    float p = EQ_PHI[f], sb = c->b0 + c->b1 + c->b2, sa = 1.0f + c->a1 + c->a2;
    float nu = sb * sb - 4.0f * (c->b0 * c->b1 + 4.0f * c->b0 * c->b2 + c->b1 * c->b2) * p + 16.0f * c->b0 * c->b2 * p * p;
    float de = sa * sa - 4.0f * (c->a1 + 4.0f * c->a2 + c->a1 * c->a2) * p + 16.0f * c->a2 * p * p;
    if (nu < 1e-12f) nu = 1e-12f;
    if (de < 1e-12f) de = 1e-12f;
    return 3.01029996f * (eq_log2(nu) - eq_log2(de));   /* 10 log10 = 10 log10(2) log2 */
}
FM1_FPU static int32_t eq_y(float db)
{
    int32_t y = EQG_MID - (int32_t)(db * (float)EQG_DB / 12.0f + (db < 0.0f ? -0.5f : 0.5f));
    return clamp(y, EQG_TOP - 4, EQG_MID + EQG_DB + 4);
}
static int32_t eq_x(int32_t f) { return PANEL_X0 + f * PANEL_W / (EQ_NF - 1); }
FM1_FPU static void graph_eq(const track_t *t, uint16_t c)
{
    static const uint8_t GRID[3] = {28, 68, 108};
    static const char *const GLAB[3] = {"100", "1k", "10k"};
    const eq_band_t *bd = eq_set[song.sel % NTRK];
    uint32_t sel = (uint32_t)eq_ui_band & 3u, b, i;
    eq_coef_t k[EQ_NB];
    int32_t f, px = 0, py = 0, qx = 0, qy = 0;
    (void)t;                                         /* (the selected track's bands: bd) */
    for (b = 0; b < EQ_NB; b++)
        eq_coef(&k[b], &bd[b]);
    for (i = 0; i < 3u; i++) {
        int32_t x = eq_x(GRID[i]);
        cv_rect(x, EQG_TOP, 1, 2 * EQG_DB, T_LINE);
        cv_text(x + 3, EQG_LAB, &AF_S, GLAB[i], T_MID);
    }
    cv_rect(PANEL_X0, EQG_MID, PANEL_W, 1, T_RAISE);  /* 0 dB */
    for (f = 0; f < EQ_NF; f++) {
        float s = 0.0f;
        int32_t x = eq_x(f), y, ys;
        for (b = 0; b < EQ_NB; b++)
            s += eq_db_at(&k[b], f);
        y = eq_y(s);
        ys = eq_y(eq_db_at(&k[sel], f));
        if (f) {
            cv_line(qx, qy, x, ys, T_MID);           /* the band picked, alone */
            cv_line_t(px, py, x, y, c, 2);           /* the sum */
        }
        px = x, py = y, qx = x, qy = ys;
    }
    for (i = 0; i < EQ_NB; i++) {                    /* the points, the one picked last (on top) */
        uint32_t n = (sel + 1u + i) % EQ_NB;
        int32_t x = eq_x(clamp(bd[n].f, 0, EQ_NF - 1)) - 7, y = eq_y(eq_db_at(&k[n], clamp(bd[n].f, 0, EQ_NF - 1))) - 7;
        uint16_t fill = n == sel ? T_THEME : T_RAISE, ink = n == sel ? T_INK : T_TEXT;
        char nm[2] = {(char)('1' + n), 0};
        x = clamp(x, PANEL_X0 - 4, PANEL_X0 + PANEL_W - 10);
        cv_rrect(x, y, 14, 14, 7, fill, T_SURF);
        GFX_HOOK_ALIGN(x, y, x + 14, y + 14, AL_HV, "eq band point number centred");
        cv_text_in(x, y + CAP_IN(S, 14), 14, &AF_S, nm, ink, fill);
    }
}
/* .. everything graph_eq draws: the track's four bands, the one KNOB 1 picked */
static uint32_t eq_sig(uint32_t h)
{
    const eq_band_t *bd = eq_set[song.sel % NTRK];
    uint32_t i;
    for (h ^= (uint32_t)eq_ui_band * 92821u, i = 0; i < EQ_NB; i++)
        h = (h ^ ((uint32_t)(uint16_t)bd[i].f | (uint32_t)(uint16_t)bd[i].g << 8 | (uint32_t)bd[i].ty << 16 |
                  (uint32_t)bd[i].q << 24)) * 16777619u;
    return h;
}
