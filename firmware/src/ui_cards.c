/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, the Alt 2 design (Figma "Alt 2 - Library", section Cards): a card whose parameter has a shape shows
 * the shape instead of the number. The value's number is on the panel's rows, in the footer's hint and in the
 * header's message while the knob turns; the card says at a glance where the knob stands. A symbol lives in a
 * 30 x 24 area in the card's middle (x 13 .. 42, y 10 .. 33; Figma put it at the right under the label, JAN3000 took
 * the label away: the rows under EDIT's cards name it, the header's message while it turns): the value's shape in
 * the value's colour on the card's SURF, nothing behind it (no ghost of the whole range: the shape alone is the
 * value); MOTION driving it, a THEME dot at the top left (draw_column). Figma draws three variants (Low / Mid /
 * High); the firmware draws the value itself, the variants are its points at about 1/6, 1/2 and 5/6. Which shape a card takes is told by its label (card_sym),
 * as icons.c tells the icons; WAVE by the wave picked (its icon, the 24 px Fukiai glyph), VDRUM's TYPE by its
 * resonator (a tube, strings). Everything else is a text card: its value and unit as before. Included by ui_draw.c
 * (draw_column). */
#define SYM_X ((COL_W - SYM_W) / 2)                     /* the symbol's area: the card's middle (x 13, y 10), no label */
#define SYM_Y ((COL_H - SYM_H) / 2)
#define SYM_W 30
#define SYM_H 24
#define SYM_CX (SYM_X + SYM_W / 2)                      /* its centre: x 28, y 22 */
#define SYM_CY (SYM_Y + SYM_H / 2)

enum {
    SYM_NONE, SYM_LEVEL, SYM_PAN, SYM_BIPOLAR, SYM_DEC, SYM_SUS, SYM_GATE, SYM_GLIDE, SYM_RATE, SYM_DEPTH, SYM_DETUNE,
    SYM_ECHO, SYM_SWING, SYM_CHANCE, SYM_NOISE, SYM_CUT, SYM_RES, SYM_ATK, SYM_REL, SYM_WAVE, SYM_SYNC, SYM_SUB,
    SYM_ENV, SYM_DIST, SYM_CRUSH, SYM_HARM, SYM_TIMB, SYM_MORPH, SYM_KTR, SYM_SPREAD, SYM_AUX, SYM_TUNE, SYM_ACC,
    SYM_TUBE, SYM_STRING
};

typedef struct {
    const char *label;
    uint8_t sym;
} card_sym_t;
/* label -> shape (the Figma legend's lists, in the firmware's label words; ENV .. ACC and the TYPEs: JAN3000's
 * sketches, docs/ui-alt2/sketch-1.png and -2.png;
 * every shape at 0 .. 100 %: docs/ui-alt2/symbols.png) */
static const card_sym_t CARD_SYM[] = {
    {"LEVEL", SYM_LEVEL}, {"LVL", SYM_LEVEL}, {"MLVL", SYM_LEVEL}, {"MIX", SYM_LEVEL}, {"GAIN", SYM_LEVEL},
    {"VOL", SYM_LEVEL}, {"VEL", SYM_LEVEL}, {"BODY", SYM_LEVEL}, {"TOP", SYM_LEVEL}, {"SUB", SYM_LEVEL},
    {"REV", SYM_LEVEL}, {"DLY", SYM_LEVEL}, {"CHOR", SYM_LEVEL}, {"CHO", SYM_LEVEL}, {"SHIM", SYM_LEVEL},
    {"SEND", SYM_LEVEL},
    {"PAN", SYM_PAN},
    {"FINE", SYM_LEVEL}, {"FLT", SYM_BIPOLAR},   /* (FINE: FM6's 0..99; a signed one is bipolar: card_sym) */ {"PIT", SYM_BIPOLAR}, {"SHP", SYM_BIPOLAR},
    {"DEC", SYM_DEC}, {"DECAY", SYM_DEC},
    {"SUS", SYM_SUS}, {"SUSTAIN", SYM_SUS},
    {"GATE", SYM_GATE},
    {"GLD", SYM_GLIDE}, {"GLIDE", SYM_GLIDE}, {"PORTA", SYM_GLIDE}, {"SLIDE", SYM_GLIDE},
    {"RATE", SYM_RATE}, {"RATE2", SYM_RATE},
    {"DEPTH", SYM_DEPTH}, {"DPTH", SYM_DEPTH},
    {"DTN", SYM_DETUNE}, {"DETUNE", SYM_DETUNE}, {"DTUNE", SYM_DETUNE},
    {"FDBK", SYM_ECHO}, {"FEEDBACK", SYM_ECHO},
    {"SWG", SYM_SWING}, {"SWING", SYM_SWING},
    {"CHANCE", SYM_CHANCE}, {"RAND", SYM_CHANCE}, {"PROB", SYM_CHANCE},
    {"NOIS", SYM_NOISE}, {"NOISE", SYM_NOISE},
    {"CUT", SYM_CUT}, {"CUT2", SYM_CUT}, {"CUTOFF", SYM_CUT}, {"TONE", SYM_CUT}, {"BRIT", SYM_CUT},   /* (2: ANLG2) */
    {"RES", SYM_RES}, {"RES2", SYM_RES}, {"RESO", SYM_RES},
    {"ATK", SYM_ATK}, {"ATTACK", SYM_ATK}, {"FADE", SYM_ATK},
    {"REL", SYM_REL}, {"RELEASE", SYM_REL},
    {"ENV", SYM_ENV},                                   /* ACID: how far the envelope opens the filter */
    {"DRV", SYM_DIST}, {"DRIVE", SYM_DIST}, {"DIST", SYM_DIST},
    {"CRSH", SYM_CRUSH},
    {"HARM", SYM_HARM}, {"TIMB", SYM_TIMB}, {"MORPH", SYM_MORPH},   /* the Plaits engines' three (PHYS: HARM) */
    {"KTR", SYM_KTR}, {"TRK", SYM_KTR},
    {"SPRD", SYM_SPREAD},
    {"AUX", SYM_AUX},
    {"TUNE", SYM_TUNE},
    {"ACC", SYM_ACC},
};

/* the card's shape: its label's, when the value is a position on a range (ratio >= 0) and the number says nothing
 * the shape does not (RATE in Hz only: a synced rate is a note value; AMT, GAIN signed: the bipolar block). WAVE: the
 * wave's icon (draw_column's icon: icons.c param_icon), or SYNC / SUB drawn; TYPE: VDRUM's TUBE / STRNG drawn. */
static uint32_t card_sym(const char *label, const char *val, const char *unit, int32_t ratio, uint32_t icon)
{
    uint32_t i, s = SYM_NONE;
    if (!label[0] || !val[0])
        return SYM_NONE;
    if (str_eq(label, "WAVE") || str_eq(label, "WAVE2")) {
        if (str_eq(val, "SYNC"))
            return SYM_SYNC;
        if (str_eq(val, "SUB"))
            return SYM_SUB;
        return FELUCCA_ICONS && icon != ICON_NONE && icon != ICON_LFO_WAVE && icon != ICON_WAVE ? SYM_WAVE : SYM_NONE;
    }
    if (str_eq(label, "TYPE"))                          /* (the filter's, the reverb's, the EQ's ..: text) */
        return str_eq(val, "TUBE") ? SYM_TUBE : str_eq(val, "STRNG") ? SYM_STRING : SYM_NONE;
    if (ratio < 0)
        return SYM_NONE;
    if ((str_eq(label, "AMT") || str_eq(label, "GAIN")) && (val[0] == '-' || val[0] == '+' || str_eq(val, "0")))
        return SYM_BIPOLAR;
    for (i = 0; i < NELEM(CARD_SYM); i++)
        if (str_eq(label, CARD_SYM[i].label)) {
            s = CARD_SYM[i].sym;
            break;
        }
    if (s == SYM_TUNE && !ui.home && cur_page()->scope == SC_GLOBAL)
        return SYM_NONE;                                /* (the project's tuning in cents: its number) */
    if (s == SYM_RATE && !str_eq(unit, "Hz"))          /* (a synced or a step rate: its note value as text) */
        return SYM_NONE;
    if (s == SYM_GLIDE && !str_eq(unit, "ms") && !str_eq(unit, "s") && !str_eq(val, "OFF"))
        return SYM_NONE;
    if ((s == SYM_LEVEL || s == SYM_DIST) && unit[0] && !str_eq(unit, "%") && !str_eq(unit, "dB"))
        return SYM_NONE;
    if (s == SYM_LEVEL && (val[0] == '+' || val[0] == '-' || (str_eq(val, "0") && ratio > 0)))
        return SYM_BIPOLAR;                             /* an offset about 0 (WHEEL's SUB BODY TOP, FM6's MLVL) */
    return s;
}

/* the ramp of a value 0..1000 across lo .. hi (rounded) */
static int32_t sym_at(int32_t r, int32_t lo, int32_t hi) { return lo + ((hi - lo) * r + 500) / 1000; }
/* a column of the area filled from the bottom, h px of SYM_H */
static void sym_col(int32_t x, int32_t h, uint16_t c)
{
    if (h > SYM_H) h = SYM_H;
    if (h > 0)
        cv_rect(SYM_X + x, SYM_Y + SYM_H - h, 1, h, c);
}
/* a stroke of a 2 x 2 pen (cv_line_t thickens downwards only: a steep line would stay 1 px wide) */
static void sym_seg(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    cv_line(x0, y0, x1, y1, c);
    cv_line(x0 + 1, y0, x1 + 1, y1, c);
    cv_line(x0, y0 + 1, x1, y1 + 1, c);
    cv_line(x0 + 1, y0 + 1, x1 + 1, y1 + 1, c);
}
/* a sine across the area: cycles in 1/16, amplitude a px about the middle, 1 or 2 px, phase in 1/1024 of a turn */
static void sym_sine(int32_t cyc16, int32_t a, int32_t ph, uint16_t c, int32_t thick)
{
    int32_t x, py = 0, cy = SYM_Y + SYM_H / 2 - (thick > 1 ? 1 : 0);
    for (x = 0; x < SYM_W; x++) {
        uint32_t p = (uint32_t)(ph * 4194304) + (uint32_t)(x * cyc16) * (0xFFFFFFFFu / (16u * (SYM_W - 1)));
        int32_t y = cy - (sine_i(p) * a + 16384) / 32767;
        if (x)
            cv_line_t(SYM_X + x - 1, py, SYM_X + x, y, c, thick);
        py = y;
    }
}
/* the noise field: 10 x 8 cells of 2 x 2 on a 3 px grid; the cells lit in a fixed shuffled order (the first n lit,
 * so the cells of a low value stay lit at a higher one) */
static uint32_t sym_cell_rank(uint32_t i) { return (i * 2654435761u >> 7) % 997u; }
/* a turn in degrees -> sine_i's phase */
#define SYM_DEG(d) ((uint32_t)(d) * 11930465u)
/* an ellipse's arc from a0 to a1 degrees (0 at the top, clockwise), centre and radii in 1/16 px, 1 px or the 2 px
 * pen, 10 degrees a segment */
static void sym_arc(int32_t cx, int32_t cy, int32_t rx, int32_t ry, int32_t a0, int32_t a1, uint16_t c, int32_t thick)
{
    int32_t a, px = 0, py = 0;
    for (a = a0; ; a += 10) {
        int32_t d = a < a1 ? a : a1, x, y;
        x = (cx + sine_i(SYM_DEG(d)) * rx / 32767 + 8) >> 4;
        y = (cy - sine_i(SYM_DEG(d) + 0x40000000u) * ry / 32767 + 8) >> 4;
        if (a > a0) {
            if (thick > 1)
                sym_seg(px, py, x, y, c);
            else
                cv_line(px, py, x, y, c);
        }
        px = x;
        py = y;
        if (d == a1)
            break;
    }
}
/* a filled polygon (points in 1/16 px), clipped to the symbol's area: each row's crossings at its middle, even-odd */
static void sym_poly(const int32_t *px, const int32_t *py, int32_t n, uint16_t c)
{
    int32_t y, i, j, k, m, xs[16];
    for (y = SYM_Y; y < SYM_Y + SYM_H; y++) {
        int32_t yc = y * 16 + 8;
        for (i = 0, j = n - 1, m = 0; i < n && m < 16; j = i++)
            if ((py[i] <= yc) != (py[j] <= yc)) {
                int32_t x = px[j] + (int32_t)((int64_t)(yc - py[j]) * (px[i] - px[j]) / (py[i] - py[j]));
                for (k = m++; k > 0 && xs[k - 1] > x; k--)
                    xs[k] = xs[k - 1];          /* (sorted as they come) */
                xs[k] = x;
            }
        for (k = 0; k + 1 < m; k += 2) {
            int32_t x0 = (xs[k] + 8) >> 4, x1 = (xs[k + 1] + 8) >> 4;
            if (x0 < SYM_X) x0 = SYM_X;
            if (x1 > SYM_X + SYM_W) x1 = SYM_X + SYM_W;
            if (x1 > x0)
                cv_rect(x0, y, x1 - x0, 1, c);
        }
    }
}

/* floor(sqrt(v)) */
static uint32_t sym_isqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v)
        b >>= 2;
    for (; b; b >>= 2)
        if (v >= r + b) {
            v -= r + b;
            r = (r >> 1) + b;
        } else
            r >>= 1;
    return r;
}
/* a convex polygon (points in 1/16 px, clockwise on the screen) as a band along its edge, w (1/16 px) wide: each
 * pixel of the area whose centre lies inside it, nearer than w to an edge. A w as wide as the polygon fills it; the
 * band stays even all round, as a fill and an outline drawn apart do not */
static void sym_band(const int32_t *px, const int32_t *py, int32_t n, int32_t w, uint16_t c)
{
    int32_t x, y, i, j, len[8];
    for (i = 0, j = n - 1; i < n; j = i++)
        len[i] = (int32_t)sym_isqrt((uint32_t)((px[i] - px[j]) * (px[i] - px[j]) + (py[i] - py[j]) * (py[i] - py[j])));
    for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
        for (x = SYM_X; x < SYM_X + SYM_W; x++) {
            int32_t mx = x * 16 + 8, my = y * 16 + 8, dmin = 0x7FFFFFFF;
            for (i = 0, j = n - 1; i < n; j = i++)
                if (len[i] > 0) {                       /* (a corner met by its neighbour: no edge) */
                    int32_t d = (int32_t)(((int64_t)(px[i] - px[j]) * (my - py[j]) - (int64_t)(py[i] - py[j]) * (mx - px[j])) / len[i]);
                    if (d < dmin) dmin = d;             /* the distance inwards, 1/16 px (negative: outside) */
                }
            if (dmin >= 0 && dmin < w)
                cv_rect(x, y, 1, 1, c);
        }
}

static void card_symbol(uint32_t s, int32_t r, uint32_t icon, uint16_t c)
{
    int32_t x, y, h, i, k, px[32], py[32];
    r = clamp(r, 0, 1000);
    switch (s) {
    case SYM_LEVEL:                                     /* a block filled from the bottom (Figma: 6 / 13 / 20 of 24) */
        h = sym_at(r, 3, SYM_H);
        cv_rrect(SYM_X, SYM_Y + SYM_H - h, SYM_W, h, h > 5 ? 3 : 1, c, T_SURF);
        break;
    case SYM_SUS:                                       /* a full-height block from the left, as wide as the level */
    case SYM_GATE:
        x = sym_at(r, 4, SYM_W);
        cv_rrect(SYM_X, SYM_Y, x, SYM_H, 2, c, T_SURF);
        break;
    case SYM_BIPOLAR:                                   /* a block down (negative) or up from the centre line */
        h = (r - 500) * (SYM_H / 2) / 500;
        if (h > 0)
            cv_rect(SYM_X, SYM_Y + SYM_H / 2 - h, SYM_W, h, c);
        else if (h < 0)
            cv_rect(SYM_X, SYM_Y + SYM_H / 2, SYM_W, -h, c);
        cv_rect(SYM_X, SYM_Y + SYM_H / 2 - 1, SYM_W, 2, c);
        break;
    case SYM_PAN:                                       /* a dot on a short track */
        cv_rect(SYM_X + 2, SYM_Y + SYM_H / 2 - 1, SYM_W - 4, 2, T_LINE);
        x = sym_at(r, 0, SYM_W - 8);
        cv_rrect(SYM_X + x, SYM_Y + SYM_H / 2 - 4, 8, 8, 4, c, T_SURF);
        break;
    case SYM_CUT:                                       /* the low pass: flat, then a round fall at the cutoff */
    case SYM_RES:                                       /* a peak on a low floor, taller with the resonance */
    case SYM_ATK:                                       /* the rise, then the level held */
    case SYM_DEC:                                       /* a plateau, then the fall to a sustain */
    case SYM_REL:                                       /* the level held, then the fall to nothing */
    case SYM_ENV:                                       /* the envelope's sweep: a quick rise, the decay to nothing */
        k = s == SYM_CUT ? sym_at(r, -9, SYM_W - 1) : s == SYM_ATK ? sym_at(r, 2, 26) : s == SYM_DEC ? sym_at(r, 3, 22)
          : s == SYM_REL ? sym_at(r, 3, 26) : s == SYM_ENV ? sym_at(r, 3, SYM_H) : sym_at(r, 4, SYM_H - 2);
        for (x = 0; x < SYM_W; x++) {
            int32_t d;
            if (s == SYM_CUT) {                         /* the pass band full up to the cutoff k (-9 .. 29: at the
                                                         * bottom only the fall's tail at the left edge), then the
                                                         * fall over 10 px to nothing: the shape closes as it goes */
                d = x - k;
                h = d <= 0 ? SYM_H : d >= 10 ? 0 : SYM_H - SYM_H * d * d / 100;
            } else if (s == SYM_RES) {                  /* a floor of 4, the peak's flanks falling as 1 / (1 + d^2) */
                d = x * 2 - (SYM_W - 1);
                h = 4 + k * 16 / (16 + d * d);
            } else if (s == SYM_ATK) {
                h = x >= k ? SYM_H : SYM_H * (x + 1) / (k + 1);
            } else if (s == SYM_DEC) {                  /* full for 5 px, the fall over k px to a third */
                d = x - 5;
                h = d < 0 ? SYM_H : d >= k ? SYM_H / 3 : SYM_H / 3 + (SYM_H * 2 / 3) * (k - d) * (k - d) / (k * k);
            } else if (s == SYM_ENV) {                  /* k tall: up in 3 px, then a square fall over the rest; the
                                                         * tail 1 px (where the sweep ends) */
                d = SYM_W - 1 - x;
                h = x < 3 ? k * (x + 1) / 3 : k * d * d / ((SYM_W - 4) * (SYM_W - 4));
                if (h < 1) h = 1;
            } else {                                    /* REL: the fall over the last k px */
                d = x - (SYM_W - k);
                h = d < 0 ? SYM_H : SYM_H * (k - d) * (k - d) / (k * k);
            }
            sym_col(x, h, c);
        }
        break;
    case SYM_GLIDE:                                     /* a low step, a ramp as wide as the time, a high step */
        k = sym_at(r, 1, 22);
        x = SYM_X + (SYM_W - k) / 2;
        cv_line_t(SYM_X, SYM_Y + 17, x, SYM_Y + 17, c, 2);
        cv_line_t(x, SYM_Y + 17, x + k, SYM_Y + 5, c, 2);
        cv_line_t(x + k, SYM_Y + 5, SYM_X + SYM_W - 1, SYM_Y + 5, c, 2);
        break;
    case SYM_RATE:                                      /* a 2 px sine, more cycles as it rises: 1 .. 5 */
        sym_sine(sym_at(r, 16, 80), 6, 0, c, 2);
        break;
    case SYM_DEPTH:                                     /* a 2 px sine as tall as the value */
        sym_sine(24, sym_at(r, 1, 9), 0, c, 2);
        break;
    case SYM_DETUNE:                                    /* two thin sines drifting apart */
        sym_sine(32, 7, 0, c, 1);
        sym_sine(32 + sym_at(r, 0, 12), 7, sym_at(r, 0, 200), c, 1);
        break;
    case SYM_ECHO:                                      /* 7 bars falling in height: the slower, the more repeats */
        k = sym_at(r, 300, 900);                        /* the fall a repeat, 1/1000 */
        for (i = 0, h = 1000; i < 7; i++, h = h * k / 1000)   /* (x 0 .. 28: the seventh inside the area) */
            if (SYM_H * h / 1000 > 0)
                cv_rect(SYM_X + i * (SYM_W - 2) / 6, SYM_Y + SYM_H - SYM_H * h / 1000, 2, SYM_H * h / 1000, c);
        break;
    case SYM_SWING:                                     /* two bars, the second shifted by the swing */
        cv_rect(SYM_X + 6, SYM_Y, 2, SYM_H, c);
        cv_rect(SYM_X + 18 + sym_at(r, 0, 8), SYM_Y, 2, SYM_H, c);
        break;
    case SYM_CHANCE:                                    /* 3 x 3 dots, the share drawn (Figma: 2 / 5 / 8 of 9) */
        k = sym_at(r, 1, 9);
        for (i = 0; i < k; i++) {
            static const uint8_t ORDER[9] = {4, 0, 8, 2, 6, 1, 7, 3, 5};   /* (the centre, the corners, the edges) */
            int32_t j = ORDER[i];
            cv_rrect(SYM_X + 5 + (j % 3) * 9, SYM_Y + 3 + (j / 3) * 8, 4, 4, 2, c, T_SURF);
        }
        break;
    case SYM_NOISE:                                     /* 80 cells, the share lit (Figma: 8 / 32 / 68) */
        k = sym_at(r, 2, 80);
        for (i = 0; i < 80; i++) {
            int32_t n = 0, j;
            for (j = 0; j < 80; j++)                    /* (its rank in the shuffled order: 80 x 80 compares, once a draw) */
                n += sym_cell_rank((uint32_t)j) < sym_cell_rank((uint32_t)i) ||
                     (sym_cell_rank((uint32_t)j) == sym_cell_rank((uint32_t)i) && j < i);
            if (n < k)
                cv_rect(SYM_X + (i % 10) * 3, SYM_Y + (i / 10) * 3, 2, 2, c);
        }
        break;
    case SYM_DIST: {                                    /* fuzz: a sine driven into its rails, rougher as it goes */
        int32_t g = 1000 + r * 7, py0 = 0;              /* the gain, 1/1000: 1 .. 8; at 1 the sine is 7 of its 9 px */
        for (x = 0; x < SYM_W - 1; x++) {               /* (the pen's second column: x 29) */
            int32_t v = (int32_t)((int64_t)sine_i((uint32_t)x * (0xFFFFFFFFu / (SYM_W - 2)) * 2u) * g * 7 / 9000);
            uint32_t hs = (uint32_t)(x + 1) * 2654435761u;
            v = clamp(v, -32767, 32767);
            y = SYM_CY - 1 - (v * 9 + 16384) / 32767;
            y += (int32_t)((hs >> 28) % 5u) * r * r / 1000000 - 2 * r * r / 1000000;   /* the grit: 0 .. +-2 px */
            y = clamp(y, SYM_Y, SYM_Y + SYM_H - 2);
            if (x)
                sym_seg(SYM_X + x - 1, py0, SYM_X + x, y, c);
            py0 = y;
        }
        break;
    }
    case SYM_CRUSH: {                                   /* bitcrush: one sine cycle in stairs, fewer and longer ones
                                                         * with the crush (12 levels held 1 px .. 2 levels held 6) */
        int32_t lv = sym_at(r, 12, 2), hold = sym_at(r, 1, 6), py0 = 0;
        for (x = 0; x < SYM_W - 2; x++) {               /* (each step to x + 1, the pen's column after it) */
            int32_t xs = x - x % hold, v = sine_i((uint32_t)xs * (0xFFFFFFFFu / (SYM_W - 3)));
            int32_t q = (v * lv + (v < 0 ? -32767 : 32767)) / 65534;   /* -lv/2 .. lv/2, rounded */
            y = SYM_CY - 1 - q * 20 / lv;
            if (x && y != py0)
                sym_seg(SYM_X + x, py0, SYM_X + x, y, c);
            sym_seg(SYM_X + x, y, SYM_X + x + 1, y, c);
            py0 = y;
        }
        break;
    }
    case SYM_HARM: {                                    /* the harmonics: a small circle in the middle growing into up
                                                         * to four ellipses nested at one point on the right */
        static const uint8_t RX[4] = {16, 12, 9, 6}, RY[4] = {16, 11, 7, 4};   /* each against the outer, 1/16 */
        int32_t rx = 48 + 176 * r / 1000, ry = 48 + 128 * r / 1000;   /* the outer's radii, 1/16 px: 3 .. 14, 3 .. 11 */
        int32_t xr = SYM_CX * 16 + 48 + ((SYM_X + SYM_W - 1) * 16 - SYM_CX * 16 - 48) * r / 1000, cy = SYM_CY * 16 - 8;
        for (i = 0; i < 4 && (i == 0 || r >= 150 + i * 200); i++)   /* (the inner ones from 35, 55, 75 %) */
            sym_arc(xr - rx * RX[i] / 16, cy, rx * RX[i] / 16, ry * RY[i] / 16, 0, 360, c, 1);
        break;
    }
    case SYM_TIMB: {                                    /* the timbre: four blocks in a row, set up and down with it */
        static const int8_t OFF[4] = {-6, 5, -3, 6};
        for (i = 0; i < 4; i++)
            cv_rrect(SYM_X + i * 8, SYM_CY - 6 + OFF[i] * r / 1000, 6, 12, 1, c, T_SURF);
        break;
    }
    case SYM_MORPH: {                                   /* a small solid square becoming a large open triangle (play):
                                                         * its right corners meet at the tip, its fill hollowed out to
                                                         * a 2 px edge (sym_band) */
        static const int8_t SX[4] = {-4, 4, 4, -4}, SY[4] = {-4, -4, 4, 4};   /* the square, about the centre, px */
        static const int8_t TX[4] = {3, SYM_W - 3, SYM_W - 3, 3}, TY[4] = {1, SYM_H / 2, SYM_H / 2, SYM_H - 1};
        for (i = 0; i < 4; i++) {
            px[i] = (SYM_CX + SX[i]) * 16 + (SYM_X + TX[i] - SYM_CX - SX[i]) * 16 * r / 1000;
            py[i] = (SYM_CY + SY[i]) * 16 + (SYM_Y + TY[i] - SYM_CY - SY[i]) * 16 * r / 1000;
        }
        sym_band(px, py, 4, 32 + (1000 - r) * 160 / 1000, c);   /* the edge 2 px wide, 12 at the bottom: solid */
        break;
    }
    case SYM_KTR: {                                     /* key tracking: a dot at the top, the ring drawn from it
                                                         * clockwise (0 .. 330 degrees), an arrowhead at its end */
        int32_t a = 330 * r / 1000, rr = 9 * 16, cx = SYM_CX * 16, cy = SYM_CY * 16;
        uint32_t p = SYM_DEG(a);
        cv_rrect(SYM_CX - 2, SYM_CY - 11, 4, 4, 2, c, T_SURF);
        if (a >= 10)
            sym_arc(cx - 8, cy - 8, rr, rr, 0, a, c, 2);
        if (a >= 30) {
            int32_t sn = sine_i(p), cs = sine_i(p + 0x40000000u);   /* along the ring (cs, sn), outwards (sn, -cs) */
            int32_t ex = cx + sn * rr / 32767, ey = cy - cs * rr / 32767;
            px[0] = ex + cs * 64 / 32767;                /* the tip 4 px ahead, the wings 4 back, 4 to each side */
            py[0] = ey + sn * 64 / 32767;
            px[1] = ex - cs * 64 / 32767 + sn * 64 / 32767;
            py[1] = ey - sn * 64 / 32767 - cs * 64 / 32767;
            px[2] = ex - cs * 64 / 32767 - sn * 64 / 32767;
            py[2] = ey - sn * 64 / 32767 + cs * 64 / 32767;
            sym_poly(px, py, 3, c);
        }
        break;
    }
    case SYM_SPREAD:                                    /* two circles, overlapping in the middle, moved apart */
        k = sym_at(r, 2, SYM_W / 2 - 6) * 16;
        sym_arc(SYM_CX * 16 - 8 - k, SYM_CY * 16 - 8, 64, 64, 0, 360, c, 2);
        sym_arc(SYM_CX * 16 - 8 + k, SYM_CY * 16 - 8, 64, 64, 0, 360, c, 2);
        break;
    case SYM_AUX: {                                     /* a striker coming at a plate, the plate's dome growing as
                                                         * it meets it (at the top, the dome alone) */
        int32_t pl = SYM_X + 22, rd = sym_at(r, 1, 7), gap = (900 - r) * 10 / 900;
        cv_rect(pl, SYM_Y + 1, 4, SYM_H - 2, c);
        for (i = 0; i <= 12; i++) {                     /* the dome: a half disc left of the plate */
            uint32_t p = SYM_DEG(180 + i * 15);
            px[i] = pl * 16 + sine_i(p) * rd * 16 / 32767;
            py[i] = SYM_CY * 16 - sine_i(p + 0x40000000u) * rd * 16 / 32767;
        }
        sym_poly(px, py, 13, c);
        if (r < 900) {                                  /* the striker: its head 5 x 8, its handle 2 px */
            x = pl - rd - gap - 5;
            cv_rrect(x, SYM_CY - 4, 5, 8, 2, c, T_SURF);
            if (x > SYM_X)
                cv_rect(SYM_X, SYM_CY - 1, x - SYM_X, 2, c);
        }
        break;
    }
    case SYM_TUNE: {                                    /* a string: slack, a wide meander, at the bottom of the range;
                                                         * tight, a straight line, at the top */
        int32_t a = sym_at(1000 - r, 0, 11), px0 = 0;
        for (y = 0; y < SYM_H - 1; y++) {
            x = SYM_CX - 1 + (sine_i((uint32_t)y * (0xFFFFFFFFu / (SYM_H - 2)) * 2u) * a + 16384) / 32767;
            if (y)
                sym_seg(px0, SYM_Y + y - 1, x, SYM_Y + y, c);
            px0 = x;
        }
        break;
    }
    case SYM_ACC: {                                     /* the accent: a ball in the bottom left corner thrown to the
                                                         * top right, splashing more the further it goes */
        int32_t bx = SYM_X * 16 + ((SYM_X + SYM_W - 7) * 16 - SYM_X * 16) * r / 1000;
        int32_t by = (SYM_Y + SYM_H) * 16 + ((SYM_Y + 4) * 16 - (SYM_Y + SYM_H) * 16) * r / 1000;
        int32_t rad = 8 * 16 + 16 * r / 1000;
        for (i = 0; i < 32; i++) {
            uint32_t p = (uint32_t)i << 27;              /* 32 points round it */
            int32_t w = (sine_i(p * 3u + (1u << 25)) * 6 + sine_i(p * 5u + (9u << 24)) * 4 + sine_i(p * 7u) * 3) / 13;
            int32_t d = rad + (int32_t)((int64_t)rad * w * r / (2000LL * 32767));   /* the splash: +-r/2000 of it */
            px[i] = bx + sine_i(p) * d / 32767;
            py[i] = by - sine_i(p + 0x40000000u) * d / 32767;
        }
        sym_poly(px, py, 32, c);
        break;
    }
    case SYM_TUBE:                                      /* VDRUM's TYPE TUBE: a tube lying, open at the right */
        cv_rect(SYM_X + 5, SYM_CY - 6, 19, 2, c);
        cv_rect(SYM_X + 5, SYM_CY + 4, 19, 2, c);
        sym_arc((SYM_X + 5) * 16, SYM_CY * 16 - 8, 64, 88, 180, 360, c, 2);
        sym_arc((SYM_X + 24) * 16, SYM_CY * 16 - 8, 64, 88, 0, 360, c, 2);
        break;
    case SYM_STRING:                                    /* .. STRNG: three strings slanting up to the right */
        sym_seg(SYM_X, SYM_Y + 20, SYM_X + 23, SYM_Y + 6, c);
        sym_seg(SYM_X + 6, SYM_Y + 21, SYM_X + 26, SYM_Y + 5, c);
        sym_seg(SYM_X + 14, SYM_Y + 22, SYM_X + 28, SYM_Y + 3, c);
        break;
    case SYM_WAVE:                                      /* the wave's icon, large */
        cv_icon_in(SYM_X - 2, SYM_Y - 4, SYM_W + 4, SYM_H + 4, 24, icon, c, T_SURF);
        break;
    case SYM_SYNC:                                      /* hard sync: a saw restarted before its end */
        for (i = 0; i < 2; i++) {
            x = SYM_X + i * 15;
            cv_line_t(x, SYM_Y + 20, x + 10, SYM_Y + 4, c, 2);
            cv_line_t(x + 10, SYM_Y + 4, x + 10, SYM_Y + 20, c, 2);
            cv_line_t(x + 10, SYM_Y + 20, x + 15, SYM_Y + 12, c, 2);
        }
        break;
    case SYM_SUB:                                       /* the octave below: a square of one cycle */
        cv_line_t(SYM_X, SYM_Y + 4, SYM_X + 14, SYM_Y + 4, c, 2);
        cv_line_t(SYM_X + 14, SYM_Y + 4, SYM_X + 14, SYM_Y + 19, c, 2);
        cv_line_t(SYM_X + 14, SYM_Y + 19, SYM_X + 29, SYM_Y + 19, c, 2);
        break;
    default:
        break;
    }
}
