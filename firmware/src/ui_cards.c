/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, the Alt 2 design (Figma "Alt 2 - Library", section Cards): a card whose parameter has a shape shows
 * the shape instead of the number. The value's number is on the panel's rows, in the footer's hint and in the
 * header's message while the knob turns; the card says at a glance where the knob stands. A symbol lives in a
 * 30 x 24 area at the card's right (x 21 .. 50, y 15 .. 38: Figma's 19, 15 on its 55 px card, 6 px from the right
 * edge): the value's shape in the value's colour over a LINE ghost of the whole range. Figma draws three variants
 * (Low / Mid / High); the firmware draws the value itself, the variants are its points at about 1/6, 1/2 and 5/6.
 * Which shape a card takes is told by its label (card_sym), as icons.c tells the icons; WAVE by the wave picked (its
 * icon, the 24 px Fukiai glyph). Everything else is a text card: its value and unit as before. Included by
 * ui_draw.c (draw_column). */
#define SYM_X 21                                        /* the symbol's area in the card */
#define SYM_Y 15
#define SYM_W 30
#define SYM_H 24

enum {
    SYM_NONE, SYM_LEVEL, SYM_PAN, SYM_BIPOLAR, SYM_DEC, SYM_SUS, SYM_GATE, SYM_GLIDE, SYM_RATE, SYM_DEPTH, SYM_DETUNE,
    SYM_ECHO, SYM_SWING, SYM_CHANCE, SYM_NOISE, SYM_CUT, SYM_RES, SYM_ATK, SYM_REL, SYM_WAVE, SYM_SYNC, SYM_SUB
};

typedef struct {
    const char *label;
    uint8_t sym;
} card_sym_t;
/* label -> shape (the Figma legend's lists, in the firmware's label words) */
static const card_sym_t CARD_SYM[] = {
    {"LEVEL", SYM_LEVEL}, {"LVL", SYM_LEVEL}, {"MLVL", SYM_LEVEL}, {"MIX", SYM_LEVEL}, {"DRV", SYM_LEVEL},
    {"DRIVE", SYM_LEVEL}, {"DIST", SYM_LEVEL}, {"GAIN", SYM_LEVEL}, {"VOL", SYM_LEVEL}, {"VEL", SYM_LEVEL},
    {"KTR", SYM_LEVEL}, {"BODY", SYM_LEVEL}, {"TOP", SYM_LEVEL}, {"SUB", SYM_LEVEL}, {"REV", SYM_LEVEL},
    {"DLY", SYM_LEVEL}, {"CHOR", SYM_LEVEL}, {"CHO", SYM_LEVEL}, {"SHIM", SYM_LEVEL}, {"SEND", SYM_LEVEL},
    {"PAN", SYM_PAN},
    {"FINE", SYM_LEVEL}, {"FLT", SYM_BIPOLAR},   /* (FINE: FM6's 0..99; a signed one is bipolar: card_sym) */ {"PIT", SYM_BIPOLAR}, {"SHP", SYM_BIPOLAR},
    {"DEC", SYM_DEC}, {"DECAY", SYM_DEC},
    {"SUS", SYM_SUS}, {"SUSTAIN", SYM_SUS},
    {"GATE", SYM_GATE},
    {"GLD", SYM_GLIDE}, {"GLIDE", SYM_GLIDE}, {"PORTA", SYM_GLIDE}, {"SLIDE", SYM_GLIDE},
    {"RATE", SYM_RATE},
    {"DEPTH", SYM_DEPTH}, {"DPTH", SYM_DEPTH},
    {"DTN", SYM_DETUNE}, {"DETUNE", SYM_DETUNE}, {"DTUNE", SYM_DETUNE}, {"SPRD", SYM_DETUNE},
    {"FDBK", SYM_ECHO}, {"FEEDBACK", SYM_ECHO},
    {"SWG", SYM_SWING}, {"SWING", SYM_SWING},
    {"CHANCE", SYM_CHANCE}, {"RAND", SYM_CHANCE}, {"PROB", SYM_CHANCE},
    {"NOIS", SYM_NOISE}, {"NOISE", SYM_NOISE},
    {"CUT", SYM_CUT}, {"CUTOFF", SYM_CUT}, {"TONE", SYM_CUT}, {"BRIT", SYM_CUT},
    {"RES", SYM_RES}, {"RESO", SYM_RES},
    {"ATK", SYM_ATK}, {"ATTACK", SYM_ATK}, {"FADE", SYM_ATK},
    {"REL", SYM_REL}, {"RELEASE", SYM_REL},
};

/* the card's shape: its label's, when the value is a position on a range (ratio >= 0) and the number says nothing
 * the shape does not (RATE in Hz only: a synced rate is a note value; AMT, GAIN signed: the bipolar block). WAVE: the
 * wave's icon (draw_column's icon: icons.c param_icon), or SYNC / SUB drawn. */
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
    if (ratio < 0)
        return SYM_NONE;
    if ((str_eq(label, "AMT") || str_eq(label, "GAIN")) && (val[0] == '-' || val[0] == '+' || str_eq(val, "0")))
        return SYM_BIPOLAR;
    for (i = 0; i < NELEM(CARD_SYM); i++)
        if (str_eq(label, CARD_SYM[i].label)) {
            s = CARD_SYM[i].sym;
            break;
        }
    if (s == SYM_RATE && !str_eq(unit, "Hz"))          /* (a synced or a step rate: its note value as text) */
        return SYM_NONE;
    if (s == SYM_GLIDE && !str_eq(unit, "ms") && !str_eq(unit, "s") && !str_eq(val, "OFF"))
        return SYM_NONE;
    if (s == SYM_LEVEL && unit[0] && !str_eq(unit, "%") && !str_eq(unit, "dB"))
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

static void card_symbol(uint32_t s, int32_t r, uint32_t icon, uint16_t c)
{
    int32_t x, h, i, k;
    uint16_t g = T_LINE;                                /* the ghost */
    r = clamp(r, 0, 1000);
    switch (s) {
    case SYM_LEVEL:                                     /* a block filled from the bottom (Figma: 6 / 13 / 20 of 24) */
        cv_rrect(SYM_X, SYM_Y, SYM_W, SYM_H, 3, g, T_SURF);
        h = sym_at(r, 3, SYM_H);
        cv_rrect(SYM_X, SYM_Y + SYM_H - h, SYM_W, h, h > 5 ? 3 : 1, c, g);
        break;
    case SYM_SUS:                                       /* a full-height block from the left, as wide as the level */
    case SYM_GATE:
        cv_rrect(SYM_X, SYM_Y, SYM_W, SYM_H, 3, g, T_SURF);
        x = sym_at(r, 4, SYM_W);
        cv_rrect(SYM_X, SYM_Y, x, SYM_H, 2, c, g);
        break;
    case SYM_BIPOLAR:                                   /* a block down (negative) or up from the centre line */
        cv_rrect(SYM_X, SYM_Y, SYM_W, SYM_H, 3, g, T_SURF);
        h = (r - 500) * (SYM_H / 2) / 500;
        if (h > 0)
            cv_rect(SYM_X, SYM_Y + SYM_H / 2 - h, SYM_W, h, c);
        else if (h < 0)
            cv_rect(SYM_X, SYM_Y + SYM_H / 2, SYM_W, -h, c);
        cv_rect(SYM_X, SYM_Y + SYM_H / 2 - 1, SYM_W, 2, c);
        break;
    case SYM_PAN:                                       /* a dot on a short track */
        cv_rect(SYM_X + 2, SYM_Y + SYM_H / 2 - 1, SYM_W - 4, 2, g);
        x = sym_at(r, 0, SYM_W - 8);
        cv_rrect(SYM_X + x, SYM_Y + SYM_H / 2 - 4, 8, 8, 4, c, T_SURF);
        break;
    case SYM_CUT:                                       /* the low pass: flat, then a round fall at the cutoff */
    case SYM_RES:                                       /* a peak on a low floor, taller with the resonance */
    case SYM_ATK:                                       /* the rise, then the level held */
    case SYM_DEC:                                       /* a plateau, then the fall to a sustain */
    case SYM_REL:                                       /* the level held, then the fall to nothing */
        cv_rrect(SYM_X, SYM_Y, SYM_W, SYM_H, 3, g, T_SURF);
        k = s == SYM_CUT ? sym_at(r, 3, 24) : s == SYM_ATK ? sym_at(r, 2, 26) : s == SYM_DEC ? sym_at(r, 3, 22)
          : s == SYM_REL ? sym_at(r, 3, 26) : sym_at(r, 4, SYM_H - 2);
        for (x = 0; x < SYM_W; x++) {
            int32_t d;
            if (s == SYM_CUT) {                         /* a quarter ellipse from the cutoff to the right edge */
                d = x - k;
                h = d <= 0 ? SYM_H : SYM_H - SYM_H * d * d / ((SYM_W - k) * (SYM_W - k));
            } else if (s == SYM_RES) {                  /* a floor of 4, the peak's flanks falling as 1 / (1 + d^2) */
                d = x * 2 - (SYM_W - 1);
                h = 4 + k * 16 / (16 + d * d);
            } else if (s == SYM_ATK) {
                h = x >= k ? SYM_H : SYM_H * (x + 1) / (k + 1);
            } else if (s == SYM_DEC) {                  /* full for 5 px, the fall over k px to a third */
                d = x - 5;
                h = d < 0 ? SYM_H : d >= k ? SYM_H / 3 : SYM_H / 3 + (SYM_H * 2 / 3) * (k - d) * (k - d) / (k * k);
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
    case SYM_DEPTH:                                     /* a 2 px sine as tall as the value, its ghost at the most */
        sym_sine(24, 9, 0, g, 2);
        sym_sine(24, sym_at(r, 1, 9), 0, c, 2);
        break;
    case SYM_DETUNE:                                    /* two thin sines drifting apart */
        sym_sine(32, 7, 0, c, 1);
        sym_sine(32 + sym_at(r, 0, 12), 7, sym_at(r, 0, 200), c, 1);
        break;
    case SYM_ECHO:                                      /* 7 bars falling in height: the slower, the more repeats */
        k = sym_at(r, 300, 900);                        /* the fall a repeat, 1/1000 */
        for (i = 0, h = 1000; i < 7; i++, h = h * k / 1000) {
            cv_rect(SYM_X + i * 5, SYM_Y, 2, SYM_H, g);
            cv_rect(SYM_X + i * 5, SYM_Y + SYM_H - SYM_H * h / 1000, 2, SYM_H * h / 1000, c);
        }
        break;
    case SYM_SWING:                                     /* two bars on a LINE grid, the second shifted by the swing */
        for (x = 0; x < SYM_W; x += 6)
            cv_rect(SYM_X + x, SYM_Y, 1, SYM_H, g);
        cv_rect(SYM_X + 6, SYM_Y, 2, SYM_H, c);
        cv_rect(SYM_X + 18 + sym_at(r, 0, 8), SYM_Y, 2, SYM_H, c);
        break;
    case SYM_CHANCE:                                    /* 3 x 3 dots, the share filled (Figma: 2 / 5 / 8 of 9) */
        k = sym_at(r, 1, 9);
        for (i = 0; i < 9; i++) {
            static const uint8_t ORDER[9] = {4, 0, 8, 2, 6, 1, 7, 3, 5};   /* (the centre, the corners, the edges) */
            int32_t j = ORDER[i];
            cv_rrect(SYM_X + 5 + (j % 3) * 9, SYM_Y + 3 + (j / 3) * 8, 4, 4, 2, i < k ? c : g, T_SURF);
        }
        break;
    case SYM_NOISE:                                     /* 80 cells, the share lit (Figma: 8 / 32 / 68) */
        k = sym_at(r, 2, 80);
        for (i = 0; i < 80; i++) {
            int32_t n = 0, j;
            for (j = 0; j < 80; j++)                    /* (its rank in the shuffled order: 80 x 80 compares, once a draw) */
                n += sym_cell_rank((uint32_t)j) < sym_cell_rank((uint32_t)i) ||
                     (sym_cell_rank((uint32_t)j) == sym_cell_rank((uint32_t)i) && j < i);
            cv_rect(SYM_X + (i % 10) * 3, SYM_Y + (i / 10) * 3, 2, 2, n < k ? c : g);
        }
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
