/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* HELP (1.5, Discussion #156; MENU > SYSTEM > HELP, OFF by default): a short, visual hint per page and per layer, key
 * caps and two or three words, never a screen of text.
 *   a page   entered (a page button, HOME): its hint in the footer's first row for HELP_FRAMES (~2 s; in place of the
 *            steps or the OCT+ / OCT- hint, which come back after), ui_draw.c draw_foot
 *   a layer  while its map shows: a second footer row under the layer's own (ui_layer.c draw_layer)
 * A hint is "[KEYCAP]WORDS" up to three times (gfx.c kc_tag names the keycaps); help_parse splits it for cv_key_row.
 * Pages by their title (a page added later has none until it is written here); HOME, HOME LEVELS and the DRUM grid by
 * what they are. Every hint fits its row with every word (tests/ui_test.c test_help: no word dropped). The setting and
 * the timer: ui.c help_on, help_start (PREF_HELP, ui.help_t) */

static const char H_EDIT[] = "[K1-4]SOUND[EDIT]HOLD: ENGINES";
static const char H_LANES[] = "[K1-4]LANE LEVELS[EDIT]HOLD: MUTES";
static const char H_FMOP[] = "[K1]OPERATOR[SAVE]HOLD: UNDO";
static const char H_VDL[] = "[KEYS]PART[K1]LAYER[SAVE]UNDO";
static const char H_VDP[] = "[KEYS]PART[SAVE]HOLD: UNDO";
static const struct { const char *title, *hint; } HELP_PAGE[] = {
    {"ENV", "[K1-4]A D S R[SELECT]ENV DEST"},
    {"ENV DEST", "[K1-4]ENV AMOUNTS[SELECT]ENV"},
    {"LFO", "[K1-4]SHAPE[LFO]HOLD: MOMENTARY"},
    {"LFO 2", "[K1]SYNC TO BPM[SELECT]LFO DEST"},
    {"LFO DEST", "[K1-4]LFO AMOUNTS[SELECT]MOD"},
    {"MOD", "[K1]SLOT[K2]SOURCE[K3]DEST"},
    {"FX", "[K1-4]SENDS[FX]HOLD: PERFORM"},
    {"SHIMMER", "[K1-4]SHIMMER[SELECT]DELAY"},
    {"SLICER", "[K1]ON[K2]PATTERN[SELECT]FX TRACKS"},
    {"INSERT", "[K1]TYPE[K4]MIX[SELECT]INSERT 2"},
    {"INSERT 2", "[K1]PRE DRIVE[SELECT]CHORUS"},
    {"DLY", "[K1-4]ALL TRACKS[SELECT]REVERB"},
    {"REVERB", "[K1]TYPE[K2]SIZE[SELECT]SLICER"},
    {"CHORUS", "[K1]TYPE[K4]FDBK[SELECT]SHIMMER"},
    {"SCL", "[K2]SCALE[K3]QUANT[SELECT]CHORDS"},
    {"CHORD", "[K1]CHORD[K3]GENRE"},        /* (M-Vave-OP1: K1 on into the sets, cset.c) */
    {"EDIT 1", H_EDIT},
    {"EDIT 2", H_EDIT},
    {"FILTER", "[K1]LP BP HP[K2]CUTOFF[SELECT]VOICE"},
    {"LANES", H_LANES},
    {"LANES 2", H_LANES},
    {"SOUNDS", "[KEYS]PICK[K1-4]TUNE DECAY TONE"},   /* (M-Vave-OP1: a DRUM lane's own) */
    {"OPERATOR", H_FMOP},
    {"OP ENV", H_FMOP},
    {"OPERATOR 2", H_FMOP},
    {"LAYER", H_VDL},                                /* (M-Vave-OP1, ui_vdrum.c) */
    {"LAYER PITCH", H_VDL},
    {"LAYER ENV", H_VDL},
    {"PART", H_VDP},
    {"PART 2", H_VDP},
    {"VOICE", "[K1]POLY, MONO[K2]GLIDE"},
    {"VOICE 2", "[K2]DETUNE[K3]PAN[K4]MUTE"},
    {"VOICE 3", "[K1]STEREO SPREAD[SELECT]EDIT 1"},
    {"FX TRACKS", "[REC]ARM[EDIT]CLEAR LANE"},
    {"DELAY", "[K1]DELAY / CLOUD[SELECT]REVERB"},
    {"CLOUD", "[K1]SIZE[K2]DENSITY[K3]PITCH"},
    {"FX TRACKS 2", "[K1]LANE[K2]BOTH[OCT+]CLEAR"},
    {"KEY", "[KEYS]ROOT[REC]RECORD[K4]ON"},
    {"SONG", "[KEYS]F#3:SEC G#3:SONG[PLAY]RUN"},
    {"USER", "[K1]SLOT[OCT+]SAVE[SELECT]PRESETS"},
    {"PRESETS", "[K2]ENGINE[K3]FAV[K4]LIST"},
    {"PHRASES", "[K1]PHRASE[OCT+]LOAD[SELECT]NEXT"},
    {"PROJECT", "[KEYS]PROJECT[OCT+]SAVE[OCT-]LOAD"},
    {"TOOLS", "[K1]CLEAR[K2]INIT SOUND[OCT+]DO"},
    {"ARP", "[K1]MODE[KEYS]CHORD[SELECT]ARP 2"},
    {"ARP 2", "[K3]LATCH[K4]ORDER[SELECT]ARP"},
    {"STEP", "[KEYS]WRITE[K1]STEP[SELECT]RATCH"},   /* (1.5.1: SEQ to DETAIL) */
    {"DETAIL", "[PRESETS]STEP[K1]CHANCE[K2]RATCH"},
    {"AUTOMATION", "[K1]ROW[OCT+]DO[EDIT]DELETE"},
    {"MIXER", "[K4]MUTE[ALGO]TRACK[SELECT]CLOCK"},
    {"CLOCK", "[K1]BPM[GLO]HOLD: TAP"},
    {"EQ", "[K1]BAND[K2]FREQ[K4]TYPE"},
};
#define H_HOME "[PRESETS]SOUND[HOME]HOLD: MENU"
#define H_LEVELS "[K1-4]T1-T4 LEVEL[HOME]MIXER"
#define H_GRID "[KEYS]STEPS[K2]LANE[SELECT]RATCH"
/* a layer's (LAYER_*): what its footer row (LAYERS[].foot) does not say */
static const char *const HELP_LAYER[LAYER_N] = {
    "", "[KEYS]BLACK: MUTES[FX]HOLD", "[KEYS]SOLO MUTE TAP[K1-4]LEVELS", "[KEYS]ROOT[K1-4]ROOT SCL CHORD",
    "[K1]ENGINE[K2]SOUND[K3]FAV", "[OCT-/+]HOLD[SAVE]HOLD: UNDO", "[K1]CLICK[K2]COUNT-IN[K3]LEVEL",   /* (M-Vave-OP1:
                                                     * SEQ's white keys are the scenes, the footer says so; its tools
                                                     * name themselves; the footer says what OCT- / OCT+ held do) */
};
static const char *const HELP_SSONG = "[KEYS]ADD SCENE[EDIT]DELETE";   /* (M-Vave-OP1: SEQ held > SONG) */

/* the hint of the page shown, 0 = none */
static const char *help_page(void)
{
    uint32_t i;
    const char *t;
    if (ui.home)
        return home_levels() ? H_LEVELS : H_HOME;
    if (grid_on() && !detail_on())
        return H_GRID;
    t = cur_page()->title;
    for (i = 0; i < NELEM(HELP_PAGE); i++)
        if (str_eq(HELP_PAGE[i].title, t))
            return HELP_PAGE[i].hint;
    return 0;
}
/* "[K1]SLOT[OCT+]SAVE" -> kh[] (at most 3), their words in b (bn bytes); the number of hints */
static uint32_t help_parse(const char *s, khint_t *kh, char *b, uint32_t bn)
{
    uint32_t n = 0, i = 0, len;
    int32_t id;
    while (s && n < 3u && (id = kc_tag(s, &len)) >= 0) {
        s += len;
        kh[n].key = (uint8_t)id;
        kh[n++].act = b + i;
        while (*s && *s != '[' && i + 2u < bn)
            b[i++] = *s++;
        b[i++] = 0;
    }
    return n;
}
/* a hint as a row of key hints from x 8 to 232 at y (the footer's rows: 2, 21); 0 = nothing to draw */
static int help_row(const char *s, int32_t y)
{
    khint_t kh[3];
    char b[48];
    uint32_t n = help_parse(s, kh, b, sizeof b);
    if (n)
        cv_key_row(8, 232, y, kh, n, 7u, T_BG);
    return n != 0u;
}
