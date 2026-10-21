/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* ANALOG2 (M-Vave-OP1, engine 14): two oscillators, each with a filter and an LFO of its own, summed. EDIT as four
 * rows: WAVE TUNE WAVE2 TUNE2, then WAVE 1's CUT RES LFO RATE, WAVE 2's CUT2 RES2 LFO2 RATE2, then VOICE (VCE GLD
 * GLDMOD PRIO, the track's).
 *   WAVE     SAW SQR TRI SIN PWM NOISE (band-limited as ANALOG's; PWM's width from the SHP routings, as ANALOG's)
 *   TUNE     the oscillator above or below the note, in semitones (OSC 2 also 6 cents sharp: two oscillators that
 *            agree exactly sound as one, and two different filters on one phase cancel in places)
 *   CUT RES  the oscillator's own low-pass (dsp.c's trapezoidal SVF, as ANALOG's); ENV / LFO / matrix -> FLT move
 *            both, and both follow the note as ANALOG at KTR 64
 *   LFO      the depth of the oscillator's own sine LFO on its CUT: up to +-64 of CUT's 127 steps
 *   RATE     that LFO's speed (the track LFO's table, 0.05 .. 30 Hz), restarted with each note (one per voice)
 * Twelve values: EDIT 1 and 2 hold the first eight (P_E0..P_E7), WAVE 2's four live in four of the twenty track
 * parameters DIGITAL's OP ENV pages had (P_FM1_ATK .. P_FM1_REL: stored with every sound already, by projects, user
 * presets and the editor; unused since DIGITAL was retired, and a DIGITAL sound converts to FM6 before it plays).
 * They are ANALOG2's while the track plays it: track_desc / param_desc_of give them its descriptors, its presets set
 * them (a2_preset), the page that shows them is ANALOG2's only (ui.c page_visible). */
static const char *const N_A2_WAVE[] = {"SAW", "SQR", "TRI", "SIN", "PWM", "NOISE"};
enum { A2_SAW, A2_SQR, A2_TRI, A2_SIN, A2_PWM, A2_NOISE };
#define P_A2_CUT2 P_FM1_ATK
#define P_A2_RES2 P_FM1_DEC
#define P_A2_LFO2 P_FM1_SUS
#define P_A2_RATE2 P_FM1_REL
static const param_desc_t A2_OSC2[4] = {
    {"CUT2", F_CUTOFF, 0, 127, 80, 0, 0},
    {"RES2", F_PCT, 0, 127, 20, 0, 0},
    {"LFO2", F_PCT, 0, 127, 0, 0, 0},
    {"RATE2", F_LFOHZ, 0, 127, 60, 0, 0},
};
static int a2_osc2_id(uint32_t id) { return id >= P_A2_CUT2 && id <= P_A2_RATE2; }

static void analog2_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[1] = v->ph[0] + 0x40000000u;
    v->s[0] = v->s[1] = v->s[4] = v->s[5] = 0;       /* the two filters (kept over a retrigger: .keep) */
    v->s[2] = v->s[3] = 0;                            /* the two LFOs: from the start with each note */
    if (!v->s[6])
        v->s[6] = 0x2345671 + (int32_t)v->age;        /* noise state */
}

/* one oscillator of wave w through its filter into acc (add: on top of what is there), n samples. The wave a constant
 * of each case below: one loop per wave, no switch per sample */
static inline __attribute__((always_inline)) void a2_run(uint32_t w, int32_t *acc, uint32_t n, int add, uint32_t *ph,
                                                         uint32_t inc, uint32_t pw, int32_t *nst, const tsvf_t *f,
                                                         int32_t *ic1, int32_t *ic2)
{
    uint32_t p = *ph, i;
    int32_t c1 = *ic1, c2 = *ic2, s;
    for (i = 0; i < n; i++) {
        switch (w) {
        case A2_SQR: s = osc_pulse(p, inc, 0x80000000u); break;
        case A2_TRI: s = osc_tri(p); break;
        case A2_SIN: s = osc_sine(p); break;
        case A2_PWM: s = osc_pulse(p, inc, pw); break;
        case A2_NOISE: s = (int32_t)(noise32(nst) >> 16) - 32768; break;
        default: s = osc_saw(p, inc); break;
        }
        p += inc;
        s = tsvf_lp(f, s >> 2, &c1, &c2);             /* each at half ANALOG's level: the two sum to its one */
        acc[i] = add ? acc[i] + s : s;
    }
    *ph = p;
    *ic1 = c1;
    *ic2 = c2;
}
static __attribute__((noinline)) void a2_osc(uint32_t w, int32_t *acc, uint32_t n, int add, uint32_t *ph, uint32_t inc,
                                             uint32_t pw, int32_t *nst, const tsvf_t *f, int32_t *ic1, int32_t *ic2)
{
    switch (w) {
    case A2_SQR: a2_run(A2_SQR, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    case A2_TRI: a2_run(A2_TRI, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    case A2_SIN: a2_run(A2_SIN, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    case A2_PWM: a2_run(A2_PWM, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    case A2_NOISE: a2_run(A2_NOISE, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    default: a2_run(A2_SAW, acc, n, add, ph, inc, pw, nst, f, ic1, ic2); break;
    }
}

/* oscillator k's cutoff this block: CUT, the routings to FLT, the note, its LFO (advanced a block) */
static int32_t a2_cut(const int16_t *p, voice_t *v, const vmod_t *m, uint32_t k)
{
    int32_t cut = (k ? p[P_A2_CUT2] : p[P_E4]) << 8, dep = k ? p[P_A2_LFO2] : p[P_E6];
    uint32_t ph = (uint32_t)v->s[2 + k];
    cut += m->cutoff + (v->pitch16 - 60 * 16) * 4;    /* ANALOG's KTR 64 */
    if (dep)
        cut += (sine_i(ph) * dep) >> 8;
    v->s[2 + k] = (int32_t)(ph + LFO_INC[(k ? p[P_A2_RATE2] : p[P_E7]) & 127]);
    return cut;
}

static void analog2_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    int32_t acc[CTL];
    tsvf_t f1, f2;
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], i;       /* state in locals: out[] may alias v->s[] */
    int32_t a1 = v->s[0], a2 = v->s[1], b1 = v->s[4], b2 = v->s[5], nst = v->s[6];
    uint32_t pw = 0x80000000u + (uint32_t)((m->shape - (64 << 8)) << 15);
    uint32_t inc1 = p[P_E1] ? cents_inc(m->pitch16 + p[P_E1] * 16, 0, m->fine) : m->inc;
    uint32_t inc2 = cents_inc(m->pitch16 + p[P_E3] * 16, 6, m->fine);
    n = n < CTL ? n : CTL;
    tsvf_coef(&f1, a2_cut(p, v, m, 0), p[P_E5]);
    tsvf_coef(&f2, a2_cut(p, v, m, 1), p[P_A2_RES2]);
    a2_osc((uint32_t)p[P_E0], acc, n, 0, &ph0, inc1, pw, &nst, &f1, &a1, &a2);
    a2_osc((uint32_t)p[P_E2], acc, n, 1, &ph1, inc2, pw, &nst, &f2, &b1, &b2);
    for (i = 0; i < n; i++)                           /* linear up to half scale, then a soft knee (ANALOG's) */
        out[i] += voice_amp(soft_knee(acc[i], 16000) << 1, m, i) << 1;
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->s[0] = a1;
    v->s[1] = a2;
    v->s[4] = b1;
    v->s[5] = b2;
    v->s[6] = nst;
}

/* the presets: {WAVE TUNE WAVE2 TUNE2, CUT RES LFO RATE}, and WAVE 2's {CUT2 RES2 LFO2 RATE2} (A2_OSC2_PRESET, the
 * same order) */
static const preset_t ANALOG2_PRESETS[] = {
    {"DUAL SAW", {0, 0, 0, 0, 85, 25, 0, 60}, {2, 70, 100, 50}, 15, 0, FX(0, 30, 30, 35), PAT(4)},
    {"WOBBLE", {0, -12, 1, -12, 55, 70, 70, 48}, {0, 60, 100, 30}, 0, 1, FX(10, 0, 20, 15), PAT(2)},
    {"SWEEP PAD", {4, 0, 0, 7, 60, 30, 50, 20}, {80, 90, 110, 95}, 0, 0, FX(0, 60, 25, 70), PAT(5)},
    {"OCT BASS", {1, -12, 0, 0, 45, 60, 0, 60}, {0, 55, 40, 25}, 35, 1, FX(10, 0, 10, 10), PAT(2)},
    {"BELL SIN", {3, 0, 2, 19, 127, 0, 0, 60}, {0, 85, 20, 70}, 0, 0, FX(0, 25, 35, 45), PAT(7)},
    {"NOISE HAT", {5, 0, 5, 0, 120, 60, 0, 60}, {0, 30, 0, 25}, 0, 0, FX(0, 0, 10, 15), PAT(0)},
    {"STEREO LD", {0, 0, 4, 12, 90, 40, 25, 70}, {4, 70, 100, 50}, 20, 1, FX(0, 20, 40, 30), PAT(4)},
    {"WIND", {5, 0, 3, 12, 50, 100, 60, 25}, {60, 90, 70, 80}, 0, 0, FX(0, 30, 30, 70), PAT(5)},
};
static const uint8_t A2_OSC2_PRESET[NELEM(ANALOG2_PRESETS)][4] = {
    {70, 25, 20, 30}, {55, 70, 70, 52}, {75, 20, 40, 30}, {80, 30, 0, 60},
    {100, 0, 0, 60}, {90, 80, 30, 90}, {80, 40, 60, 72}, {70, 110, 50, 33},
};

/* preset pi's WAVE 2 values into the track (ui.c apply_preset_to, after the rest of the preset), ANALOG2 only */
static void a2_preset(track_t *t, uint32_t pi)
{
    uint32_t k;
    for (k = 0; k < 4u; k++)
        t->p[P_A2_CUT2 + k] = A2_OSC2_PRESET[pi % NELEM(ANALOG2_PRESETS)][k];
}

static const engine_t ENG_ANALOG2 = {
    .name = "ANALOG2",
    .page_title = {"OSC", "WAVE 1"},
    .edit = {
        {"WAVE", F_ENUM, 0, 5, 0, N_A2_WAVE, 0},
        {"TUNE", F_SEMI, -24, 24, 0, 0, 0},
        {"WAVE2", F_ENUM, 0, 5, 0, N_A2_WAVE, 0},
        {"TUNE2", F_SEMI, -24, 24, 0, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 85, 0, 0},
        {"RES", F_PCT, 0, 127, 25, 0, 0},
        {"LFO", F_PCT, 0, 127, 0, 0, 0},
        {"RATE", F_LFOHZ, 0, 127, 60, 0, 0},
    },
    .presets = ANALOG2_PRESETS,
    .npresets = NELEM(ANALOG2_PRESETS),
    .note_on = analog2_note_on,
    .render = analog2_render,
    .knob = {P_E4, P_A2_CUT2, P_ATK, P_REL},
    .keep = 0x73,                /* the filters (s[0] s[1], s[4] s[5]) and the noise */
};
