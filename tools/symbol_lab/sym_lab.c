/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* the symbol lab's frames: every card shape at every value of a real parameter, drawn by the firmware's draw_column
 * (grey palette); writes <dir>/<id>.ppm (16 frames a row, 57 x 44 each) and <dir>/data.json; then every other label
 * (the engines', the track's and the global parameters') that has no shape, as its text card, in the group "text" */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
typedef struct { const char *id, *title, *label, *labels; int eng; const param_desc_t *d; } lab_t;
static const param_desc_t *eng_desc(int e, const char *label)
{
    uint32_t i;
    for (i = 0; i < 8u; i++)
        if (ENGINES[e]->edit[i].label && str_eq(ENGINES[e]->edit[i].label, label))
            return &ENGINES[e]->edit[i];
    return 0;
}
static void jstr(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) { if (*s == '"' || *s == '\\') fputc('\\', f); fputc(*s, f); }
    fputc('"', f);
}
static FILE *J;
static int nout;
/* one card's frames: up to 128 values of d (all of them, or evenly picked) */
static void lab_card(const char *dir, const char *id, const char *title, const char *labels, const char *source,
                     const char *group, const param_desc_t *d)
{
    static uint8_t img[8 * 44][16 * 57][3];
    int32_t span = d->max - d->min, frames = span + 1 > 128 ? 128 : span + 1, f;
    uint32_t x, y;
    char path[512], val[32];
    FILE *p;
    memset(img, 0, sizeof img);
    fprintf(J, "%s{\"id\":", nout++ ? ",\n" : "");
    jstr(J, id); fprintf(J, ",\"title\":"); jstr(J, title);
    fprintf(J, ",\"label\":"); jstr(J, d->label); fprintf(J, ",\"labels\":"); jstr(J, labels);
    fprintf(J, ",\"source\":"); jstr(J, source); fprintf(J, ",\"group\":"); jstr(J, group);
    fprintf(J, ",\"min\":%d,\"max\":%d,\"def\":%d,\"values\":[", d->min, d->max, d->def);
    for (f = 0; f < frames; f++) {
        int32_t v = frames > 1 ? d->min + (int32_t)((int64_t)span * f / (frames - 1)) : d->min;
        const char *unit = "";
        char txt[48];
        param_format(d, v, val, &unit);
        snprintf(txt, sizeof txt, "%s%s%s", val, unit && unit[0] ? " " : "", unit ? unit : "");
        fprintf(J, "%s", f ? "," : ""); jstr(J, txt);
        ui.force = 1; ui.hot_t = 0;
        draw_column(0, d->label, val, unit ? unit : "", T_THEME, RATIO(d, enum_rank(d, v)), param_icon(d, v));
        for (y = 0; y < 44u; y++)
            for (x = 0; x < 57u; x++) {
                uint16_t c = swap16(host_screen[(Y_LABEL + y) * 240 + CARD_X(0) + x]);
                uint8_t *o = img[(f / 16) * 44 + y][(f % 16) * 57 + x];
                o[0] = (uint8_t)((c >> 11) * 255u / 31u); o[1] = (uint8_t)(((c >> 5) & 63u) * 255u / 63u);
                o[2] = (uint8_t)((c & 31u) * 255u / 31u);
            }
    }
    fprintf(J, "],\"raws\":[");
    for (f = 0; f < frames; f++)
        fprintf(J, "%s%d", f ? "," : "", frames > 1 ? d->min + (int32_t)((int64_t)span * f / (frames - 1)) : d->min);
    fprintf(J, "],\"frames\":%d}", frames);
    snprintf(path, sizeof path, "%s/%s.ppm", dir, id);
    p = fopen(path, "wb");
    fprintf(p, "P6\n%d %d\n255\n", 16 * 57, ((frames + 15) / 16) * 44);
    fwrite(img, 1, (size_t)16 * 57 * 3 * ((frames + 15) / 16) * 44, p);
    fclose(p);
}
/* has d a shape at any of its values? */
static int lab_has_shape(const param_desc_t *d)
{
    int32_t v;
    char val[32];
    for (v = d->min; v <= d->max; v += d->max - d->min > 64 ? 4 : 1) {
        const char *unit = "";
        param_format(d, v, val, &unit);
        if (card_sym(d->label, val, unit ? unit : "", RATIO(d, enum_rank(d, v)), param_icon(d, v)) != SYM_NONE)
            return 1;
    }
    return 0;
}
static char seen[256][16];
static uint32_t nseen;
static int lab_seen(const char *label)
{
    uint32_t i;
    for (i = 0; i < nseen; i++)
        if (str_eq(seen[i], label))
            return 1;
    if (nseen < 256u)
        str_cpy(seen[nseen++], label, 16);
    return 0;
}
static void lab_text(const char *dir, const param_desc_t *d, const char *source)
{
    char id[40];
    uint32_t i;
    if (!d->label || !d->label[0] || d->label[0] == '-' || d->max <= d->min || lab_seen(d->label) || lab_has_shape(d))
        return;
    snprintf(id, sizeof id, "t_%s", d->label);
    for (i = 0; id[i]; i++)                             /* (a file name: letters and digits) */
        if (!((id[i] >= 'A' && id[i] <= 'Z') || (id[i] >= 'a' && id[i] <= 'z') || (id[i] >= '0' && id[i] <= '9') || id[i] == '_'))
            id[i] = '_';
    lab_card(dir, id, d->label, d->label, source, "text", d);
}
int main(int argc, char **argv)
{
    lab_t L[] = {
        {"cut", "Cutoff", "CUT", "CUT, CUTOFF, TONE, BRIT", 0, 0},
        {"cut2", "Cutoff 2", "CUT2", "CUT2 (ANLG2)", 14, &A2_OSC2[0]},
        {"res", "Resonanz", "RES", "RES, RESO", 0, 0},
        {"env", "Hüllkurven-Menge", "ENV", "ENV (ACID)", 22, 0},
        {"atk", "Attack", "ATK", "ATK, ATTACK, FADE", -1, &TP[P_ATK]},
        {"dec", "Decay", "DEC", "DEC, DECAY", -1, &TP[P_DEC]},
        {"sus", "Sustain", "SUS", "SUS, SUSTAIN, GATE", -1, &TP[P_SUS]},
        {"rel", "Release", "REL", "REL, RELEASE", -1, &TP[P_REL]},
        {"level", "Pegel", "LEVEL", "LEVEL, LVL, MIX, GAIN, VOL, VEL, SEND, REV, DLY, CHO, SHIM ..", -1, &TP[P_LEVEL]},
        {"flt", "Bipolar (± um 0)", "FLT", "FLT, PIT, SHP, AMT, GAIN mit Vorzeichen", -1, &TP[P_ED_FLT]},
        {"pan", "Panorama", "PAN", "PAN", -1, &TP[P_PAN]},
        {"gld", "Glide", "GLD", "GLD, GLIDE, PORTA, SLIDE", -1, &TP[P_GLIDE]},
        {"rate", "Rate (Hz)", "RATE", "RATE in Hz", -1, &TP[P_LRATE]},
        {"depth", "Tiefe", "DEPTH", "DEPTH, DPTH", -1, &TP[P_SLDEPTH]},
        {"dtn", "Detune", "DTN", "DTN, DETUNE, DTUNE", 0, 0},
        {"fdbk", "Feedback / Echo", "FDBK", "FDBK, FEEDBACK", -1, &GP[G_DFDBK]},
        {"swg", "Swing", "SWG", "SWG, SWING", -1, &TP[P_SSWING]},
        {"rand", "Zufall / Chance", "RAND", "RAND, CHANCE, PROB", 8, 0},
        {"nois", "Rauschen", "NOIS", "NOIS, NOISE", 0, 0},
        {"drv", "Drive (Fuzz)", "DRV", "DRV, DRIVE, DIST", 0, 0},
        {"crsh", "Bitcrush", "CRSH", "CRSH", 3, 0},
        {"harm", "Harmonics", "HARM", "HARM (Plaits-Engines, PHYS)", 17, 0},
        {"timb", "Timbre", "TIMB", "TIMB (Plaits-Engines)", 17, 0},
        {"morph", "Morph", "MORPH", "MORPH (Plaits-Engines)", 17, 0},
        {"aux", "Aux", "AUX", "AUX (Plaits-Engines)", 17, 0},
        {"ktr", "Keytracking", "KTR", "KTR, TRK", 0, 0},
        {"sprd", "Spread", "SPRD", "SPRD", 8, 0},
        {"tune", "Tune (Saite)", "TUNE", "TUNE der Engines (nicht die Projekt-Stimmung)", 15, 0},
        {"acc", "Accent", "ACC", "ACC", 22, 0},
        {"type", "Resonator-Typ", "TYPE", "TYPE (VDRUM: TUBE / STRNG)", 15, 0},
        {"wave", "Wellenform", "WAVE", "WAVE, WAVE2", 0, 0},
    };
    uint32_t i, e, k;
    char path[512];
    ui_power_on();
    go_title("ENV"); frame();
    snprintf(path, sizeof path, "%s/data.json", argv[1]);
    J = fopen(path, "w");
    fprintf(J, "[\n");
    for (i = 0; i < NELEM(L); i++) {
        const param_desc_t *d = L[i].d ? L[i].d : eng_desc(L[i].eng, L[i].label);
        if (!d) { fprintf(stderr, "no desc %s\n", L[i].id); return 1; }
        lab_seen(d->label);
        lab_card(argv[1], L[i].id, L[i].title, L[i].labels, L[i].eng >= 0 ? ENGINES[L[i].eng]->name : "Spur / global",
                 "symbol", d);
    }
    for (e = 0; e < NENGINES; e++)                      /* the rest: the engines' labels without a shape .. */
        if (ENGINES[e]->name[0] != '-')
            for (k = 0; k < 8u; k++)
                lab_text(argv[1], &ENGINES[e]->edit[k], ENGINES[e]->name);
    for (k = 0; k < 4u; k++)
        lab_text(argv[1], &A2_OSC2[k], "ANLG2");
    for (k = 0; k < P_COUNT; k++)                       /* .. the track's .. */
        lab_text(argv[1], &TP[k], "Spur");
    for (k = 0; k < G_COUNT; k++)                       /* .. and the global ones */
        lab_text(argv[1], &GP[k], "Global");
    fprintf(J, "\n]\n");
    fclose(J);
    return 0;
}
