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
#define SYM_CUT_KNEE 661                                /* CUT: 1.9 kHz (F_CUTOFF 84 of 127), 1/1000 of the range */

enum {
    SYM_NONE, SYM_LEVEL, SYM_PAN, SYM_BIPOLAR, SYM_DEC, SYM_SUS, SYM_GATE, SYM_GLIDE, SYM_RATE, SYM_DEPTH, SYM_DETUNE,
    SYM_ECHO, SYM_SWING, SYM_CHANCE, SYM_NOISE, SYM_CUT, SYM_RES, SYM_ATK, SYM_REL, SYM_WAVE, SYM_SYNC, SYM_SUB,
    SYM_ENV, SYM_DIST, SYM_CRUSH, SYM_HARM, SYM_TIMB, SYM_MORPH, SYM_KTR, SYM_SPREAD, SYM_AUX, SYM_TUNE, SYM_ACC,
    SYM_TUBE, SYM_STRING, SYM_FILL, SYM_SWEEP, SYM_VIB, SYM_GSIZE, SYM_GPOS, SYM_GDENS, SYM_MODEL, SYM_STRC, SYM_DAMP, SYM_BOW, SYM_EXC, SYM_SNAP, SYM_DRFT, SYM_BELL, SYM_NAME, SYM_FREQ, SYM_ENVBI, SYM_PULSE
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
    {"DEC", SYM_DEC}, {"DECAY", SYM_DEC}, {"DECY", SYM_DEC}, {"DCY", SYM_DEC}, {"DCAY", SYM_DEC},   /* (DRUM ..) */
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
    {"SWP", SYM_SWEEP},
    {"VIB", SYM_VIB},
    {"SIZE", SYM_GSIZE}, {"POS", SYM_GPOS}, {"DENS", SYM_GDENS},
    {"STRC", SYM_STRC}, {"DAMP", SYM_DAMP}, {"BOW", SYM_BOW}, {"EXC", SYM_EXC}, {"SNAP", SYM_SNAP},
    {"DRFT", SYM_DRFT},
    {"MRAT", SYM_BELL},
    {"REG", SYM_NAME}, {"PERC", SYM_NAME}, {"ROTR", SYM_NAME}, {"SLOT", SYM_NAME},   /* WHEEL, FM6: the name alone */
    {"MEG", SYM_ENVBI},
    {"DUTY", SYM_PULSE}, {"PW", SYM_PULSE},              /* a pulse's width */                                 /* FM6: the envelope on the modulators, +- */
    {"FREQ", SYM_FREQ},                                 /* NOISE's filter */                                 /* FM6's modulator ratio: bright .. dark */                                 /* NOISE's drift */             /* PHYS (DAMP: the reverb's too) */            /* GRAIN's grain size and read position */                                   /* LOFI's vibrato */                                 /* LOFI's pitch sweep */
    {"COLR", SYM_FILL},                                 /* NOISE's colour, the delay's */
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
    if (str_eq(label, "MODEL"))                         /* PHYS's models, as pictures (SYM_BM_*) */
        return str_eq(val, "MODAL") || str_eq(val, "STRNG") || str_eq(val, "MEMB") || str_eq(val, "SYMP") ? SYM_MODEL : SYM_NONE;
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
    if (s == SYM_GSIZE && !(ENGINES[TSEL->eng_req % NENGINES] == &ENG_GRAIN && (ui.home || cur_page()->scope == SC_ENGINE)))
        return SYM_NONE;                                /* (SIZE: GRAIN's grains only, not the reverb's or CLOUD's) */
    if (s == SYM_TUNE && !ui.home && cur_page()->scope == SC_GLOBAL)
        return SYM_NONE;                                /* (the project's tuning in cents: its number) */
    if (s == SYM_RATE && !str_eq(unit, "Hz"))          /* (a synced or a step rate: its note value as text) */
        return SYM_NONE;
    if (s == SYM_GLIDE && !str_eq(unit, "ms") && !str_eq(unit, "s") && !str_eq(val, "OFF"))
        return SYM_NONE;
    if ((s == SYM_LEVEL || s == SYM_DIST) && unit[0] && !str_eq(unit, "%") && !str_eq(unit, "dB"))
        return SYM_NONE;
    if (s == SYM_LEVEL && !str_eq(unit, "dB") && (val[0] == '+' || val[0] == '-' || (str_eq(val, "0") && ratio > 0)))
        return SYM_BIPOLAR;                             /* an offset about 0 (WHEEL's SUB BODY TOP, FM6's MLVL); a level
                                                         * in dB stays a fader, -35 dB as much as +7.5 */
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
                int32_t x = px[j] + (yc - py[j]) * (px[i] - px[j]) / (py[i] - py[j]);   /* (32 bits: 1/16 px products stay small; the
                                                         * firmware has no 64-bit divide) */
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
/* a full circle by distance, smooth where sym_arc's segments are not: each pixel of the area whose centre lies
 * within w / 2 of the radius (centre, radius and w in 1/16 px) */
static void sym_ring(int32_t cx, int32_t cy, int32_t rad, int32_t w, uint16_t c)
{
    int32_t x, y;
    for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
        for (x = SYM_X; x < SYM_X + SYM_W; x++) {
            int32_t dx = x * 16 + 8 - cx, dy = y * 16 + 8 - cy, d = (int32_t)sym_isqrt((uint32_t)(dx * dx + dy * dy));
            if (d - rad <= w / 2 && rad - d <= w / 2)
                cv_rect(x, y, 1, 1, c);
        }
}
/* the angle of (dx, dy) from the top, clockwise, in degrees 0 .. 359 (y down; atan by 45 z + 15.6 z (1 - z), within
 * a quarter of a degree) */
static int32_t sym_angle(int32_t dx, int32_t dy)
{
    int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy, z, t;
    if (!ax && !ay)
        return 0;
    z = ax <= ay ? ax * 1024 / ay : ay * 1024 / ax;     /* tan, 0 .. 1024 */
    t = (45 * z + 16 * z * (1024 - z) / 1024) / 1024;    /* atan in degrees, 0 .. 45 */
    if (ax > ay)
        t = 90 - t;                                     /* from the vertical */
    if (dx >= 0)
        return dy <= 0 ? t : 180 - t;                   /* (right half: top .. bottom) */
    return dy > 0 ? 180 + t : (360 - t) % 360;
}
/* sym_ring up to a degrees from the top, clockwise (360: the whole ring; rad 0 and w the diameter: a disc) */
static void sym_ring_to(int32_t cx, int32_t cy, int32_t rad, int32_t w, int32_t a, uint16_t c)
{
    int32_t x, y;
    for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
        for (x = SYM_X; x < SYM_X + SYM_W; x++) {
            int32_t dx = x * 16 + 8 - cx, dy = y * 16 + 8 - cy, d = (int32_t)sym_isqrt((uint32_t)(dx * dx + dy * dy));
            if (d - rad <= w / 2 && rad - d <= w / 2 && (a >= 360 || sym_angle(dx, dy) <= a))
                cv_rect(x, y, 1, 1, c);
        }
}
/* a convex polygon (points in 1/16 px, clockwise on the screen) as a band along its edge, w (1/16 px) wide: each
 * pixel of the area whose centre lies inside it, nearer than w to an edge. A w as wide as the polygon fills it; the
 * band stays even all round, as a fill and an outline drawn apart do not */
static void sym_band(const int32_t *px, const int32_t *py, int32_t n, int32_t w, uint16_t c)
{
    int32_t x, y, i, j, len[32];
    for (i = 0, j = n - 1; i < n; j = i++)
        len[i] = (int32_t)sym_isqrt((uint32_t)((px[i] - px[j]) * (px[i] - px[j]) + (py[i] - py[j]) * (py[i] - py[j])));
    for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
        for (x = SYM_X; x < SYM_X + SYM_W; x++) {
            int32_t mx = x * 16 + 8, my = y * 16 + 8, dmin = 0x7FFFFFFF;
            for (i = 0, j = n - 1; i < n; j = i++)
                if (len[i] > 0) {                       /* (a corner met by its neighbour: no edge) */
                    int32_t d = ((px[i] - px[j]) * (my - py[j]) - (py[i] - py[j]) * (mx - px[j])) / len[i];
                    if (d < dmin) dmin = d;             /* the distance inwards, 1/16 px (negative: outside) */
                }
            if (dmin >= 0 && dmin < w)
                cv_rect(x, y, 1, 1, c);
        }
}

/* JAN3000's pictures, 30 x 24, a row a word (bit 29 - x: the left column the top bit), from
 * tools/symbol_lab/bitmaps.py: PHYS's MODELs (MODAL a person, STRNG strings, MEMB a sheep (a skin), SYMP strings
 * ringing) and STRC's cloud and bolt, MRAT's bell (its outline and its inside); decimal words, as build.py reads a hex word as
 * an address */
static const uint32_t SYM_BM_HUMAN[SYM_H] = {
    73728u, 208896u, 399360u, 264192u, 264192u, 399360u,
    208896u, 122880u, 0u, 522240u, 1838592u, 3146496u,
    6291840u, 4194432u, 12583104u, 8388672u, 8388672u, 8388672u,
    8388672u, 8388672u, 8388672u, 12583104u, 4194432u, 4194432u,
};
static const uint32_t SYM_BM_STRINGS[SYM_H] = {
    0u, 0u, 0u, 4u, 14u, 28u,
    188u, 1016u, 2032u, 8176u, 16352u, 65216u,
    122304u, 506752u, 2025216u, 3786240u, 15961600u, 29826048u,
    126752768u, 238827520u, 1014001664u, 283140096u, 4251648u, 16384u,
};
static const uint32_t SYM_BM_SHEEP[SYM_H] = {
    0u, 0u, 491520u, 850944u, 1574400u, 3146496u,
    6291896u, 29360380u, 16777470u, 50331902u, 33554686u, 100663550u,
    67108988u, 67108984u, 100663488u, 50332032u, 32506112u, 1963776u,
    2039296u, 1574400u, 1574400u, 1574400u, 1574400u, 0u,
};
static const uint32_t SYM_BM_SYMP[SYM_H] = {
    0u, 0u, 0u, 6u, 4u, 24u,
    1016u, 3544u, 7728u, 6592u, 15104u, 10752u,
    1862144u, 6525440u, 4198912u, 13099520u, 9175552u, 26740224u,
    51394560u, 471883776u, 274743296u, 4210688u, 16384u, 0u,
};
static const uint32_t SYM_BM_CLOUD[SYM_H] = {
    126976u, 260096u, 523264u, 4194176u, 16777184u, 16777184u,
    33554416u, 33554416u, 33554416u, 16777184u, 16777184u, 16777184u,
    465920u, 0u, 0u, 0u, 0u, 0u,
    0u, 0u, 0u, 0u, 0u, 0u,
};
static const uint32_t SYM_BM_BOLT[SYM_H] = {
    0u, 0u, 0u, 0u, 0u, 0u,
    0u, 0u, 0u, 0u, 0u, 130816u,
    261632u, 261120u, 523264u, 522240u, 1048448u, 1048064u,
    2096128u, 126976u, 122880u, 229376u, 458752u, 262144u,
};
static const uint32_t SYM_BM_BELL[SYM_H] = {
    0u, 122880u, 473088u, 789504u, 525312u, 1574400u,
    1049088u, 1049088u, 1049088u, 1049088u, 3146496u, 2097408u,
    2097408u, 2097408u, 6291840u, 4194432u, 12583104u, 25165920u,
    50331696u, 134094840u, 135168u, 208896u, 122880u, 49152u,
};
static const uint32_t SYM_BM_BELLIN[SYM_H] = {
    0u, 0u, 49152u, 258048u, 522240u, 522240u,
    1047552u, 1047552u, 1047552u, 1047552u, 1047552u, 2096640u,
    2096640u, 2096640u, 2096640u, 4194048u, 4194048u, 8388480u,
    16777152u, 122880u, 122880u, 49152u, 0u, 0u,
};
/* a picture's rows 0 .. n - 1, each run of set bits one rect */
static void sym_bits(const uint32_t *rows, int32_t n, uint16_t c)
{
    int32_t y, x;
    for (y = 0; y < n && y < SYM_H; y++)
        for (x = 0; x < SYM_W; x++)
            if ((rows[y] >> (29 - x)) & 1u) {
                int32_t x0 = x;
                while (x + 1 < SYM_W && ((rows[y] >> (28 - x)) & 1u))
                    x++;
                cv_rect(SYM_X + x0, SYM_Y + y, x - x0 + 1, 1, c);
            }
}
/* a picture's rows y0 .. SYM_H - 1 */
static void sym_bits_from(const uint32_t *rows, int32_t y0, uint16_t c)
{
    int32_t y, x;
    for (y = y0 < 0 ? 0 : y0; y < SYM_H; y++)
        for (x = 0; x < SYM_W; x++)
            if ((rows[y] >> (29 - x)) & 1u)
                cv_rect(SYM_X + x, SYM_Y + y, 1, 1, c);
}
/* the card's shape s at r (0..1000) in c; val: the value as the card would print it (SWG's ring holds it) */
static void card_symbol(uint32_t s, int32_t r, uint32_t icon, const char *val, uint16_t c)
{
    int32_t x, y, h, i, k, hh, px[32], py[32];
    r = clamp(r, 0, 1000);
    switch (s) {
    case SYM_LEVEL:                                     /* a fader (JAN3000): a 3 px line across a vertical MID track,
                                                         * at the bottom for nothing, at the top for the most */
        cv_rect(SYM_CX - 1, SYM_Y, 2, SYM_H, T_MID);
        cv_rect(SYM_X + 4, SYM_Y + SYM_H - 3 - sym_at(r, 0, SYM_H - 3), SYM_W - 8, 3, c);
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
        /* CUT (JAN3000): from the top down to SYM_CUT_KNEE (F_CUTOFF's 1.9 kHz, 84 of 127) the cutoff moves, 29 .. 16,
         * the flank 10 px; below it the cutoff goes on to -2 and the flank grows to 26 px, a real low pass's long
         * roll-off: at the bottom nearly all of it is flank */
        hh = s == SYM_CUT && r < SYM_CUT_KNEE ? sym_at(r * 1000 / SYM_CUT_KNEE, 26, 10) : 10;   /* (the flank's width) */
        k = s == SYM_CUT ? (r < SYM_CUT_KNEE ? sym_at(r * 1000 / SYM_CUT_KNEE, -2, 16)
                                             : 16 + ((SYM_W - 1 - 16) * (r - SYM_CUT_KNEE) + (1000 - SYM_CUT_KNEE) / 2) / (1000 - SYM_CUT_KNEE))
          : s == SYM_ATK ? sym_at(r, 2, 26) : s == SYM_DEC ? sym_at(r, 3, 22)
          : s == SYM_REL ? sym_at(r, 3, 26) : s == SYM_ENV ? sym_at(r, 3, SYM_H) : sym_at(r, 4, SYM_H - 2);
        for (x = 0; x < SYM_W; x++) {
            int32_t d;
            if (s == SYM_CUT) {                         /* the pass band full up to the cutoff k, then the fall over
                                                         * hh px to nothing */
                d = x - k;
                h = d <= 0 ? SYM_H : d >= hh ? 0 : SYM_H - SYM_H * d * d / (hh * hh);
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
    case SYM_SWING: {                                   /* the swing in per cent (JAN3000), a ring round it closing
                                                         * clockwise from the top as it grows (S, else the 8 px face) */
        const aafont_t *f = text_w(&AF_S, val) > SYM_H - 8 ? &AF_X : &AF_S;   /* the figures alone (JAN3000) */
        int32_t a = 360 * r / 1000;
        const char *b = val;
        if (a >= 10)
            sym_arc(SYM_CX * 16 - 8, SYM_CY * 16 - 8, 168, 168, 0, a, c, 2);   /* (r 10.5: the pen's row inside) */
        cv_text_in(SYM_X, SYM_Y + (f == &AF_S ? CAP_IN(S, SYM_H) : CAP_IN(X, SYM_H)), SYM_W, f, b, c, T_SURF);
        break;
    }
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
            int32_t v = sine_i((uint32_t)x * (0xFFFFFFFFu / (SYM_W - 2)) * 2u) * g / 1000 * 7 / 9;
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
        static const uint8_t RX[4] = {16, 12, 9, 6};   /* each against the outer, 1/16: circles (JAN3000), not ellipses */
        int32_t rx = 48 + 128 * r / 1000;              /* the outer's radius, 1/16 px: 3 .. 11 */
        int32_t xr = SYM_CX * 16 + 48 + ((SYM_X + SYM_W - 1) * 16 - SYM_CX * 16 - 48) * r / 1000, cy = SYM_CY * 16 - 8;
        for (i = 0; i < 4 && (i == 0 || r >= 150 + i * 200); i++)   /* (the inner ones from 35, 55, 75 %) */
            sym_ring(xr - rx * RX[i] / 16, cy + 8, rx * RX[i] / 16, 20, c);   /* (round: by distance) */
        break;
    }
    case SYM_TIMB: {                                    /* the timbre: four blocks in a row, set up and down with it */
        static const int8_t OFF[4] = {-6, 5, -3, 6};
        for (i = 0; i < 4; i++)
            cv_rrect(SYM_X + i * 8, SYM_CY - 6 + OFF[i] * r / 1000, 6, 12, 1, c, T_SURF);
        break;
    }
    case SYM_MORPH: {                                   /* a small solid square becoming a large open hexagon
                                                         * (JAN3000): the square's six points (its corners, the middles
                                                         * of its sides) move out to the hexagon's, its fill hollowed
                                                         * out to a 2 px edge (sym_band) */
        static const int8_t SX[6] = {-4, -4, 4, 4, 4, -4}, SY[6] = {0, -4, -4, 0, 4, 4};   /* about the centre, px */
        static const int8_t TX[6] = {-13, -6, 6, 13, 6, -6}, TY[6] = {0, -11, -11, 0, 11, 11};
        for (i = 0; i < 6; i++) {
            px[i] = (SYM_CX + SX[i]) * 16 + (TX[i] - SX[i]) * 16 * r / 1000;
            py[i] = (SYM_CY + SY[i]) * 16 + (TY[i] - SY[i]) * 16 * r / 1000;
        }
        sym_band(px, py, 6, 32 + (1000 - r) * 160 / 1000, c);   /* the edge 2 px wide, 12 at the bottom: solid */
        break;
    }
    case SYM_KTR: {                                     /* key tracking: a dot at the top, the ring drawn from it
                                                         * clockwise (0 .. 330 degrees), no arrowhead; round and large
                                                         * (JAN3000): r 10, 2 px, by distance (sym_ring_to) */
        int32_t a = 330 * r / 1000;
        sym_ring_to(SYM_CX * 16, SYM_CY * 16, 160, 32, a, c);
        sym_ring_to(SYM_CX * 16, SYM_CY * 16 - 160, 0, 80, 360, c);   /* the dot: a disc of r 2.5 on the top */
        break;
    }
    case SYM_SPREAD:                                    /* two round rings (r 4.5, 1.5 px: sym_ring), overlapping in
                                                         * the middle, moved apart to the area's edges */
        k = 40 + (SYM_W / 2 * 16 - 72 - 12 - 4 - 40) * r / 1000;
        sym_ring(SYM_CX * 16 - k, SYM_CY * 16, 72, 24, c);
        sym_ring(SYM_CX * 16 + k, SYM_CY * 16, 72, 24, c);
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
    case SYM_ACC: {                                     /* the accent as a lava lamp (JAN3000): two soft blobs (a
                                                         * metaball field), one in the bottom left corner; with the
                                                         * value one rises to the top right, stretched on the way, its
                                                         * neck pinching off, slowing as it settles; a small pool stays */
        int32_t t = 1000 - (1000 - r) * (1000 - r) / 1000;   /* eased: slow at the top */
        int32_t sw = sine_i((uint32_t)(r * 2147u)) ;     /* sin(pi r): the stretch on the way, Q15 */
        int32_t ax = (SYM_X + 7) * 16, ay = (SYM_Y + SYM_H - 6) * 16, ar = 80 - 32 * r / 1000;   /* the pool */
        int32_t bx = ax + ((SYM_X + SYM_W - 8) * 16 - ax) * t / 1000, by = ay + ((SYM_Y + 7) * 16 - ay) * t / 1000;
        int32_t rx = 88 - 14 * sw / 32767, ry = 88 + 34 * sw / 32767;   /* the rising blob, 1/16 px */
        for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
            for (x = SYM_X; x < SYM_X + SYM_W; x++) {
                int32_t dx = x * 16 + 8 - ax, dy = y * 16 + 8 - ay, ex = x * 16 + 8 - bx, ey = y * 16 + 8 - by;
                int32_t q = ex * ex * 1024 / (rx * rx) + ey * ey * 1024 / (ry * ry);   /* (the ellipse's d^2, x 1024) */
                int32_t f = ar * ar * 1024 / (dx * dx + dy * dy + 1) + 1048576 / (q + 1);
                if (f >= 1024)
                    cv_rect(x, y, 1, 1, c);
            }
        break;
    }
    case SYM_TUBE: {                                    /* VDRUM's TYPE TUBE: a tube lying (JAN3000): two straight
                                                         * 2 px sides, its closed back end on the left rounded (half an
                                                         * ellipse), the open end on the right the whole ellipse; both
                                                         * drawn by distance (smooth) */
        int32_t lx = (SYM_X + 6) * 16 + 8, ex = (SYM_X + 24) * 16 + 8, ey = SYM_CY * 16, erx = 56, ery = 96;
        cv_rect(SYM_X + 6, SYM_CY - 6, 19, 2, c);
        cv_rect(SYM_X + 6, SYM_CY + 4, 19, 2, c);
        for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
            for (x = SYM_X; x < SYM_X + SYM_W; x++) {
                int32_t dy = y * 16 + 8 - ey, dx = x * 16 + 8 - ex, bx = x * 16 + 8 - lx;
                int32_t v = dx * dx * 64 / (erx * erx) + dy * dy * 64 / (ery * ery);   /* (d / r)^2 x 64 */
                int32_t w = bx * bx * 64 / (erx * erx) + dy * dy * 64 / (ery * ery);
                if ((v >= 36 && v <= 100) || (bx < 0 && w >= 36 && w <= 100))   /* (0.75 .. 1.25 of the radius) */
                    cv_rect(x, y, 1, 1, c);
            }
        break;
    }
    case SYM_FILL:                                      /* COLR (JAN3000): an empty ring filling from the bottom, like
                                                         * a glass, full at the top of the range */
        for (y = SYM_Y; y < SYM_Y + SYM_H; y++)
            for (x = SYM_X; x < SYM_X + SYM_W; x++) {
                int32_t dx = x * 16 + 8 - SYM_CX * 16, dy = y * 16 + 8 - SYM_CY * 16;
                int32_t d = (int32_t)sym_isqrt((uint32_t)(dx * dx + dy * dy));
                if ((d <= 170 && d >= 138) || (d < 138 && y >= SYM_Y + SYM_H - sym_at(r, 0, SYM_H)))
                    cv_rect(x, y, 1, 1, c);             /* (the ring r 10.5, 2 px; inside it the fill) */
            }
        break;
    case SYM_SWEEP: {                                   /* SWP (JAN3000): a 2 px line from the bottom left to the top
                                                         * right, a solid triangle (pointing up) pushed along it from
                                                         * the bottom to the top of the range */
        int32_t x0 = SYM_X + 6, y0 = SYM_Y + SYM_H - 7, x1 = SYM_X + SYM_W - 8, y1 = SYM_Y + 7;   /* (inside) */
        int32_t tx = x0 * 16 + (x1 - x0) * 16 * r / 1000 + 8, ty = y0 * 16 + (y1 - y0) * 16 * r / 1000 + 8;
        sym_seg(SYM_X, SYM_Y + SYM_H - 2, SYM_X + SYM_W - 2, SYM_Y, c);
        px[0] = tx; py[0] = ty - 72;                    /* the tip 4.5 px above the line, the base 4.5 below, 10 wide */
        px[1] = tx + 80; py[1] = ty + 72;
        px[2] = tx - 80; py[2] = ty + 72;
        sym_poly(px, py, 3, c);
        break;
    }
    case SYM_VIB:                                       /* VIB (JAN3000): a straight line becoming a fast sine, many
                                                         * waves (7 across), as tall as the vibrato (0 .. 7 px) */
        sym_sine(112, sym_at(r, 0, 7), 0, c, 2);
        break;
    case SYM_GSIZE:                                     /* SIZE (JAN3000): a triangle (the grain's window) not of lines
                                                         * but of 2 x 2 cells on a 3 px grid, built up column by column
                                                         * from the left as the grain grows (1 .. 10 columns) */
        k = sym_at(r, 1, 10);
        for (i = 0; i < k * 8; i++) {
            int32_t cx = (i / 8) * 3, cy = (i % 8) * 3, m = cx + 1 - SYM_W / 2;
            if ((m < 0 ? -m : m) * SYM_H <= (cy + 2) * (SYM_W / 2) + SYM_W / 4)   /* (inside: apex top middle) */
                cv_rect(SYM_X + cx, SYM_Y + cy, 2, 2, c);
        }
        break;
    case SYM_DRFT: {                                    /* DRFT (JAN3000): a ball carried along a 2 px sine (1.5 waves),
                                                         * left to right with the drift, sitting on the wave */
        int32_t wx = sym_at(r, 4, SYM_W - 5), wy;
        uint32_t p = (uint32_t)(wx * 24) * (0xFFFFFFFFu / (16u * (SYM_W - 1)));
        sym_sine(24, 5, 0, c, 2);
        wy = SYM_Y + SYM_H / 2 - 1 - (sine_i(p) * 5 + 16384) / 32767;
        sym_ring_to((SYM_X + wx) * 16 + 8, (wy - 4) * 16 + 8, 0, 112, 360, c);   /* (a disc of r 3.5 on the wave) */
        break;
    }
    case SYM_GPOS: {                                    /* POS (JAN3000): a 2 px sine (1.5 waves), a solid triangle
                                                         * above it pointing down at it, moving left to right with the
                                                         * read position, its tip 2 px over the wave */
        int32_t wx = sym_at(r, 3, SYM_W - 4), wy;
        uint32_t p = (uint32_t)(wx * 24) * (0xFFFFFFFFu / (16u * (SYM_W - 1)));
        sym_sine(24, 5, 0, c, 2);
        wy = SYM_Y + SYM_H / 2 - 1 - (sine_i(p) * 5 + 16384) / 32767;   /* (the wave's top row there: sym_sine's) */
        px[0] = (SYM_X + wx) * 16 + 8; py[0] = (wy - 2) * 16;
        px[1] = px[0] - 64; py[1] = py[0] - 80;
        px[2] = px[0] + 64; py[2] = py[0] - 80;
        sym_poly(px, py, 3, c);
        break;
    }
    case SYM_GDENS:                                     /* DENS (JAN3000): one grain in the middle, more and more round
                                                         * it until they fill the area: 80 cells (2 x 2, 3 px grid) lit
                                                         * nearest the middle first (1 .. 80) */
        k = sym_at(r, 1, 80);
        for (i = 0; i < 80; i++) {
            int32_t n = 0, j, di, dx = (i % 10) * 6 + 2 - SYM_W, dy = (i / 10) * 6 + 2 - SYM_H + 3;   /* (1/2 px) */
            di = dx * dx + dy * dy * 2;                 /* (the area is wider than tall: rows count a little more) */
            for (j = 0; j < 80; j++) {                  /* (its rank by that distance: 80 x 80 compares, once a draw) */
                int32_t ex = (j % 10) * 6 + 2 - SYM_W, ey = (j / 10) * 6 + 2 - SYM_H + 3, dj = ex * ex + ey * ey * 2;
                n += dj < di || (dj == di && j < i);
            }
            if (n < k)
                cv_rect(SYM_X + (i % 10) * 3, SYM_Y + (i / 10) * 3, 2, 2, c);
        }
        break;
    case SYM_MODEL:                                     /* PHYS's MODEL (JAN3000): its picture */
        sym_bits(str_eq(val, "MODAL") ? SYM_BM_HUMAN : str_eq(val, "STRNG") ? SYM_BM_STRINGS
                 : str_eq(val, "MEMB") ? SYM_BM_SHEEP : SYM_BM_SYMP, SYM_H, c);
        break;
    case SYM_STRC:                                      /* STRC (JAN3000): a cloud, a bolt growing out of it, down to
                                                         * the bottom at the top of the range */
        sym_bits(SYM_BM_CLOUD, SYM_H, c);
        sym_bits(SYM_BM_BOLT, 11 + sym_at(r, 0, SYM_H - 11), c);   /* (the bolt starts under the cloud, row 11) */
        break;
    case SYM_DAMP: {                                    /* DAMP (JAN3000): a bell-shaped 2 px line moving down with the
                                                         * damping, its peak 11 px over its feet */
        int32_t base = SYM_Y + 13 + sym_at(r, 0, SYM_H - 15), py0 = 0;
        for (x = 0; x < SYM_W - 1; x++) {
            int32_t d = x * 2 - (SYM_W - 2), q = 144 + d * d;           /* (1/2 px from the middle; s 6 px) */
            y = base - 11 * 144 * 144 / (q * q);                        /* (a bell: 1 / (1 + d^2 / s^2)^2) */
            if (x)
                sym_seg(SYM_X + x - 1, py0, SYM_X + x, y, c);
            py0 = y;
        }
        break;
    }
    case SYM_BOW:                                       /* BOW (JAN3000): arrows pointing down, more with the bowing
                                                         * (1 .. 5), each a 2 px shaft and a solid head */
        k = sym_at(r, 1, 5);
        for (i = 0; i < k; i++) {
            static const int8_t AX[5] = {14, 7, 21, 1, 27};   /* (the middle first, then outwards) */
            int32_t ax = SYM_X + AX[i];
            cv_rect(ax, SYM_Y + 1, 2, SYM_H - 8, c);
            px[0] = (ax + 1) * 16; py[0] = (SYM_Y + SYM_H) * 16;
            px[1] = px[0] - 56; py[1] = py[0] - 112;
            px[2] = px[0] + 56; py[2] = py[0] - 112;
            sym_poly(px, py, 3, c);
        }
        break;
    case SYM_EXC:                                       /* EXC (JAN3000): an ellipse becoming an octagon: 32 points on
                                                         * the ellipse moving to the octagon's sides, a 2 px edge */
        for (i = 0; i < 32; i++) {
            uint32_t p = SYM_DEG(22) + 0x1000000u + ((uint32_t)i << 27);   /* (22.5 + 11.25 i degrees, clockwise) */
            int32_t sn = sine_i(p), cs = sine_i(p + 0x40000000u), ex = 13 * 16 * sn / 32767, ey = -10 * 16 * cs / 32767;
            int32_t v = i / 4, f = i % 4;               /* the octagon: vertex v (at 22.5 + 45 v degrees), f/4 to v + 1 */
            uint32_t pa = SYM_DEG(22 + 45 * v) + 0x1000000u, pb = SYM_DEG(22 + 45 * ((v + 1) % 8)) + 0x1000000u;
            int32_t ax = 13 * 16 * sine_i(pa) / 32767, ay = -10 * 16 * sine_i(pa + 0x40000000u) / 32767;
            int32_t bx = 13 * 16 * sine_i(pb) / 32767, by = -10 * 16 * sine_i(pb + 0x40000000u) / 32767;
            int32_t ox, oy;
            ox = ax + (bx - ax) * f / 4;
            oy = ay + (by - ay) * f / 4;
            px[i] = SYM_CX * 16 + ex + (ox - ex) * r / 1000;
            py[i] = SYM_CY * 16 + ey + (oy - ey) * r / 1000;
        }
        sym_band(px, py, 32, 32, c);
        break;
    case SYM_SNAP: {                                    /* SNAP (JAN3000): a frequency response, flat, its highs lifted
                                                         * (a shelf from x 10 to 22, 0 .. 9 px) with the snap */
        int32_t lift = sym_at(r, 0, 9), py0 = 0;
        for (x = 0; x < SYM_W - 1; x++) {
            int32_t t = clamp((x - 10) * 1000 / 12, 0, 1000), st = t * t / 1000 * (3000 - 2 * t) / 1000;   /* smoothstep */
            y = SYM_CY + 4 - lift * st / 1000;
            if (x)
                sym_seg(SYM_X + x - 1, py0, SYM_X + x, y, c);
            py0 = y;
        }
        break;
    }
    case SYM_BELL:                                      /* MRAT (JAN3000): a bell, its outline always, filled from the
                                                         * bottom as it gets brighter (dark: empty, bright: full) */
        sym_bits(SYM_BM_BELL, SYM_H, c);
        k = SYM_H - sym_at(r, 0, SYM_H);                /* (the fill's top row) */
        sym_bits_from(SYM_BM_BELLIN, k, c);
        break;
    case SYM_ENVBI: {                                   /* MEG (JAN3000): ENV's sweep for an amount about 0: up from
                                                         * the bottom for +, hanging from the top for -, as tall as it */
        int32_t amt = r > 500 ? (r - 500) * 2 : (500 - r) * 2, d;
        k = sym_at(amt, 1, SYM_H);
        for (x = 0; x < SYM_W; x++) {
            d = SYM_W - 1 - x;
            h = x < 3 ? k * (x + 1) / 3 : k * d * d / ((SYM_W - 4) * (SYM_W - 4));
            if (h < 1) h = 1;
            if (r >= 500)
                sym_col(x, h, c);
            else
                cv_rect(SYM_X + x, SYM_Y, 1, h, c);
        }
        break;
    }
    case SYM_PULSE:                                     /* DUTY, PW (JAN3000): two periods of a pulse (14 px each), high
                                                         * for as much of each as the width (1 .. 13 px), 2 px lines */
        k = sym_at(r, 1, 13);
        for (i = 0; i < 2; i++) {
            int32_t x0 = SYM_X + i * 14, hi = SYM_Y + 4, lo = SYM_Y + SYM_H - 6, xe = i ? SYM_X + SYM_W - 2 : x0 + 14;
            sym_seg(x0, lo, x0, hi, c);                 /* up */
            sym_seg(x0, hi, x0 + k, hi, c);             /* high for the width */
            sym_seg(x0 + k, hi, x0 + k, lo, c);         /* down */
            sym_seg(x0 + k, lo, xe, lo, c);             /* low for the rest (the pen's column: x 29) */
        }
        break;
    case SYM_NAME: {                                    /* REG, PERC (JAN3000): the value's name alone, in the card's
                                                         * middle (M, else S), no label: the name says what it is */
        const aafont_t *f = text_w(&AF_M, val) <= COL_W - 8 ? &AF_M : &AF_S;
        cv_text_in(0, f == &AF_M ? CAP_IN(M, COL_H) : CAP_IN(S, COL_H), COL_W, f, val, c, T_SURF);
        break;
    }
    case SYM_FREQ: {                                    /* FREQ (JAN3000): a frequency response, a low flat line with
                                                         * a peak moving left to right with the frequency */
        int32_t pk = sym_at(r, 2, SYM_W - 3) * 2, py0 = 0;   /* (the peak, 1/2 px) */
        for (x = 0; x < SYM_W - 1; x++) {
            int32_t d = x * 2 - pk, q = 36 + d * d;
            y = SYM_Y + SYM_H - 4 - 16 * 36 * 36 / (q * q);    /* (16 px tall, s 3 px) */
            if (x)
                sym_seg(SYM_X + x - 1, py0, SYM_X + x, y, c);
            py0 = y;
        }
        break;
    }
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
