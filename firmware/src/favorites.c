/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#define FELUCCA_FAVORITES 1
/* Stable engine/preset references; fixed capacity independent of optional engines. */
typedef struct {
    uint8_t factory[16][32]; /* engine 0..15, preset 0..255 */
    uint32_t user, filter;
} favorites_t;
static favorites_t favorites __attribute__((section(".pool")));
/* engine e's preset k: its row and bit; ANALOG2 (engine 14, M-Vave-OP1) has the upper half of ANALOG's row (presets
 * 128..255 of engine 0, which has 12): row 14 is MIDI LEARN's (midi_control.c), 15 the settings' (settings_persist.c) */
static int fav_at(uint32_t *engine, uint32_t *preset)
{
    if (*engine == 14u) {
        if (*preset >= 128u)
            return 0;
        *engine = 0;
        *preset += 128u;
    }
    return *engine < NENGINES && *engine < 14u && *preset < 256u;
}
static int favorite_has(uint32_t engine, uint32_t preset)
{
    if (engine == NENGINES)
        return preset < UP_SLOTS && ((favorites.user >> preset) & 1u);
    return fav_at(&engine, &preset) && ((favorites.factory[engine][preset / 8u] >> (preset % 8u)) & 1u);
}
static int favorite_set(uint32_t engine, uint32_t preset, int on)
{
    if (engine > NENGINES || (engine == NENGINES ? preset >= UP_SLOTS : !fav_at(&engine, &preset)))
        return 0;
    if (favorite_has(engine, preset) == !!on) return 0;
    if (engine == NENGINES) {
        if (on) favorites.user |= 1u << preset;
        else favorites.user &= ~(1u << preset);
    } else {
        uint8_t *b = &favorites.factory[engine][preset / 8u];
        if (on) *b |= (uint8_t)(1u << (preset % 8u));
        else *b &= (uint8_t)~(1u << (preset % 8u));
    }
    return 1;
}
