/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#define FELUCCA_FAVORITES 1
/* Stable engine/preset references; fixed capacity independent of optional engines. */
typedef struct {
    uint8_t factory[16][32]; /* engine 0..15, preset 0..255 */
    uint32_t user, filter;
} favorites_t;
static favorites_t favorites __attribute__((section(".pool")));
/* the byte of a factory favourite: engine e's row, preset p's bit. Rows 14 and 15 hold other settings (14 MIDI
 * LEARN's map, midi_learn.c; 15 the MENU's and the FX layer's, settings_persist.c), so the engines from 14 on
 * (M-Vave-OP1's VDRUM) keep theirs at the end of ANALOG's row: bytes 24..27 (ANALOG's presets 192..223, which
 * it never has: a _Static_assert in eng_vdrum.c), 32 presets each. 0 = no such favourite */
#define FAV_HI_BYTE 24u
static uint8_t *favorite_cell(uint32_t engine, uint32_t preset)
{
    if (engine < 14u)
        return engine < NENGINES && preset < 256u ? &favorites.factory[engine][preset / 8u] : 0;
    if (engine >= NENGINES || preset >= 32u || FAV_HI_BYTE + 4u * (engine - 14u) + 4u > 32u)
        return 0;
    return &favorites.factory[0][FAV_HI_BYTE + 4u * (engine - 14u) + preset / 8u];
}
static int favorite_has(uint32_t engine, uint32_t preset)
{
    const uint8_t *b;
    if (engine == NENGINES)
        return preset < UP_SLOTS && ((favorites.user >> preset) & 1u);
    b = favorite_cell(engine, preset);
    return b && ((*b >> (preset % 8u)) & 1u);
}
static int favorite_set(uint32_t engine, uint32_t preset, int on)
{
    uint8_t *b = engine == NENGINES ? 0 : favorite_cell(engine, preset);
    if (engine > NENGINES || (engine == NENGINES ? preset >= UP_SLOTS : !b))
        return 0;
    if (favorite_has(engine, preset) == !!on) return 0;
    if (engine == NENGINES) {
        if (on) favorites.user |= 1u << preset;
        else favorites.user &= ~(1u << preset);
    } else {
        if (on) *b |= (uint8_t)(1u << (preset % 8u));
        else *b &= (uint8_t)~(1u << (preset % 8u));
    }
    return 1;
}
