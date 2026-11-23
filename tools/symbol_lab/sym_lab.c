/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* the symbol lab's frames: every card shape at every value of a real parameter, drawn by the firmware's draw_column
 * (grey palette); writes <dir>/<id>.ppm (16 frames a row, 57 x 44 each) and <dir>/data.json */
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
    uint32_t i, x, y;
    char path[512], val[32];
    FILE *j;
    ui_power_on();
    go_title("ENV"); frame();
    snprintf(path, sizeof path, "%s/data.json", argv[1]);
    j = fopen(path, "w");
    fprintf(j, "[\n");
    for (i = 0; i < NELEM(L); i++) {
        const param_desc_t *d = L[i].d ? L[i].d : eng_desc(L[i].eng, L[i].label);
        int32_t v, n, f;
        static uint8_t img[8 * 44][16 * 57][3];
        FILE *p;
        if (!d) { fprintf(stderr, "no desc %s\n", L[i].id); return 1; }
        n = d->max - d->min + 1;
        memset(img, 0, sizeof img);
        fprintf(j, "%s{\"id\":", i ? ",\n" : "");
        jstr(j, L[i].id); fprintf(j, ",\"title\":"); jstr(j, L[i].title);
        fprintf(j, ",\"label\":"); jstr(j, L[i].label); fprintf(j, ",\"labels\":"); jstr(j, L[i].labels);
        fprintf(j, ",\"source\":"); jstr(j, L[i].eng >= 0 ? ENGINES[L[i].eng]->name : "Spur / global");
        fprintf(j, ",\"min\":%d,\"max\":%d,\"def\":%d,\"values\":[", d->min, d->max, d->def);
        for (v = d->min, f = 0; v <= d->max && f < 128; v++, f++) {
            const char *unit = "";
            char txt[48];
            param_format(d, v, val, &unit);
            snprintf(txt, sizeof txt, "%s%s%s", val, unit && unit[0] ? " " : "", unit ? unit : "");
            fprintf(j, "%s", f ? "," : ""); jstr(j, txt);
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
        fprintf(j, "],\"frames\":%d}", f);
        snprintf(path, sizeof path, "%s/%s.ppm", argv[1], L[i].id);
        p = fopen(path, "wb");
        fprintf(p, "P6\n%d %d\n255\n", 16 * 57, ((f + 15) / 16) * 44);
        fwrite(img, 1, (size_t)16 * 57 * 3 * ((f + 15) / 16) * 44, p);
        fclose(p);
        (void)n;
    }
    fprintf(j, "\n]\n");
    fclose(j);
    return 0;
}
