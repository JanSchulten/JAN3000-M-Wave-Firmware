/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* ACID (M-Vave-OP1, engine 22): a bass line after Roland's TB-303 as Behringer's TD-3 plays it. One oscillator into a
 * four-pole diode-ladder low-pass, its envelope (MEG) on the cutoff, the accent and the slide as the 303 makes them.
 *   WAVE   SAW / SQR (band-limited: dsp.c osc_saw, osc_pulse)
 *   TUNE   -12 .. +12 semitones (the TD-3's TUNE; the octave buttons for more)
 *   CUT    CUTOFF: 30 Hz .. 16 kHz as the other filters show it (F_CUTOFF), the filter's resonant peak there; with the
 *          envelope and the accent it opens up to 6.6 kHz (ACID_B0's last entry); LFO DEST FLT and the matrix's CUT
 *          move it as they move every cutoff (1/127 of the range a step)
 *   RES    RESONANCE: 0 .. 97 % of the ladder's self-oscillation (ACID_KC); a high-pass in the feedback path keeps the
 *          bass a resonant 303 would lose, a soft clip in it squelches
 *   ENV    ENV MOD: the envelope's depth on the cutoff, 0 .. 4.5 octaves
 *   DEC    DECAY: the envelope's, 200 ms .. 2 s (an accented note's is the shortest, as on the 303)
 *   ACC    ACCENT: what an accented note (velocity 110 and up: a step's ACCENT plays 127) adds: louder, its envelope
 *          deeper and the accent sweep, a voltage that charges with each accented note and lets go slowly, so accents
 *          in a row open the filter further (the 303's "wow")
 *   DIST   a drive after the filter (the overdrive a TD-3 is played into), 0 = clean
 * The voice's level is the engine's own (engine_t.ownenv; the track's ADSR does nothing): a fast attack, a slow decay
 * while the key is held, a 10 ms release. A slide (the sequencer's SLIDE, or LEGATO notes with GLIDE) does not
 * restart the envelopes: the pitch glides (voice.c, ~45 ms), the filter stays where it was, the accent follows the
 * note slid to. The presets play MONO (LEGATO): one voice, as the 303; POLY plays up to 4.
 * The filter: Open303's TB-303 structure of the diode ladder (four coupled one-pole stages, the feedback from the
 * last), solved here for its own coefficients: ACID_B0 and ACID_KC put the resonant peak at the cutoff and the edge of
 * self-oscillation, 32 points from 20 Hz a step of 0.27 octaves (numerically, the linear ladder). The model's floats
 * run on the FPU (core.h FM1_FPU). State per voice in the part's engine memory (acid_v_t). */
static const char *const N_ACID_WAVE[] = {"SAW", "SQR"};
static const float ACID_B0[32] = {0.002067f, 0.002491f, 0.003002f, 0.003615f, 0.004352f, 0.005237f, 0.006301f, 0.007582f,
    0.009113f, 0.010947f, 0.013139f, 0.015756f, 0.018879f, 0.022584f, 0.026975f, 0.032161f, 0.038260f, 0.045400f,
    0.053712f, 0.063341f, 0.074383f, 0.086951f, 0.101117f, 0.116954f, 0.134344f, 0.153225f, 0.173388f, 0.194687f,
    0.216635f, 0.238885f, 0.260980f, 0.282439f};
static const float ACID_KC[32] = {17.0707f, 17.0852f, 17.1028f, 17.1240f, 17.1496f, 17.1805f, 17.2177f, 17.2628f,
    17.3172f, 17.3828f, 17.4621f, 17.5579f, 17.6740f, 17.8142f, 17.9841f, 18.1899f, 18.4395f, 18.7425f, 19.1111f,
    19.5605f, 20.1082f, 20.7777f, 21.5982f, 22.6093f, 23.8522f, 25.3885f, 27.2906f, 29.6667f, 32.6220f, 36.3119f,
    40.9163f, 46.6385f};
#define ACID_L0 4.321928f                     /* log2(20 Hz): ACID_B0[0] */
#define ACID_STEP 0.2701205f                  /* octaves between two entries */

typedef struct {
    float y1, y2, y3, y4, hp;                 /* the ladder; the feedback high-pass's low-pass state */
    float dc;                                 /* the output's DC blocker (the 303's output is AC-coupled) */
    float meg, acc, sweep;                    /* the envelope, the accent's envelope, the accent sweep */
    float vca, vdec;                          /* the level, its slow decay while held */
    uint8_t note, accent, on;                 /* the note playing (a slide changes it), accented, sounding */
} acid_v_t;
typedef struct {
    acid_v_t v[NVOICE];
} acid_mem_t;
ENG_MEM_FITS("ACID", sizeof(acid_mem_t));

static acid_v_t *acid_v(const track_t *t, const voice_t *v)
{
    return &((acid_mem_t *)ENG_MEM((uint32_t)(t - trk)))->v[(uint32_t)(v - t->v) % NVOICE];
}

/* 2^x, x in -30 .. 16 (a cubic on the fraction, ~1e-4) */
FM1_FPU static inline float acid_exp2(float x)
{
    union { float f; int32_t i; } u;
    float fl = (float)(int32_t)(x < 0.0f ? x - 1.0f : x), f = x - fl;
    u.f = 1.0f + f * (0.6951786f + f * (0.2261868f + f * 0.0786268f));
    u.i += (int32_t)fl << 23;
    return u.f;
}
/* a soft clip (tanh's shape to 3, then 1): the ladder's feedback, the drive */
FM1_FPU static inline float acid_clip(float x)
{
    if (x > 3.0f)
        return 1.0f;
    if (x < -3.0f)
        return -1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

FM1_FPU static void acid_start(track_t *t, voice_t *v)
{
    acid_v_t *a = acid_v(t, v);
    if (!a->on || a->vca < 1e-4f) {                   /* from rest: the filter empty (a retrigger keeps it) */
        a->y1 = a->y2 = a->y3 = a->y4 = a->hp = a->dc = 0.0f;
        a->vca = 0.0f;
    }
    a->accent = v->vel >= 110u;
    a->note = v->note;
    a->meg = 1.0f;
    a->vdec = 1.0f;
    if (a->accent)
        a->acc = 1.0f;
    a->on = 1;
}
static void acid_note_on(track_t *t, voice_t *v) { acid_start(t, v); }

/* the voice has let go and is silent: it ends (voice.c, engine_t.done) */
FM1_FPU static int acid_done(track_t *t, voice_t *v)
{
    acid_v_t *a = acid_v(t, v);
    if (!a->on)
        return 1;
    if (v->gate || a->vca >= 1e-4f)
        return 0;
    a->on = 0;
    return 1;
}

static int32_t acid_osc[CTL], acid_o[CTL];           /* a block: the oscillator in, the voice out (Q15) */

/* the model on the FPU: the envelopes once a block, then the ladder and the drive over acid_osc -> acid_o */
FM1_FPU __attribute__((noinline)) static void acid_model(const track_t *t, const voice_t *v, acid_v_t *a, uint32_t n,
                                                         const vmod_t *m)
{
    const int16_t *p = t->p;
    const float accamt = (float)p[P_E6] * (1.0f / 127.0f), blk = (float)CTL;
    float lc, pos, fr, b0, k, arel, adec, amd;
    uint32_t i, j;
    adec = a->accent ? 0.2f : 0.2f * acid_exp2((float)p[P_E5] * (3.321928f / 127.0f));   /* 200 ms .. 2 s */
    a->meg *= acid_exp2(-blk * 1.442695f / (adec * 44100.0f));
    a->acc *= acid_exp2(-blk * 1.442695f / (0.2f * 44100.0f));
    if (a->accent && v->gate)                         /* the sweep: charges with the accent, lets go slowly */
        a->sweep += (a->acc - a->sweep) * 0.08f;
    else
        a->sweep -= a->sweep * 0.006f;
    amd = 1.0f + (a->accent ? accamt : 0.0f);         /* an accented note's envelope deeper */
    lc = 4.906891f + (float)p[P_E2] * (9.058894f / 127.0f)               /* CUT: 30 Hz .. 16 kHz (F_CUTOFF) */
       + (float)m->cutoff * (9.058894f / (127.0f * 256.0f))             /* LFO / matrix -> CUT */
       + a->meg * amd * (float)p[P_E4] * (4.5f / 127.0f)                /* ENV MOD */
       + a->sweep * accamt * 2.2f;                                     /* the accent sweep */
    pos = (lc - ACID_L0) * (1.0f / ACID_STEP);
    pos = pos < 0.0f ? 0.0f : pos > 31.0f ? 31.0f : pos;
    j = (uint32_t)pos;
    fr = pos - (float)j;
    if (j >= 31u) {
        j = 30u;
        fr = 1.0f;
    }
    b0 = ACID_B0[j] + (ACID_B0[j + 1u] - ACID_B0[j]) * fr;
    k = (ACID_KC[j] + (ACID_KC[j + 1u] - ACID_KC[j]) * fr) * 0.97f * (float)p[P_E3] * (1.0f / 127.0f);
    /* the level: ~2 ms up, a slow decay held (~3 s), 10 ms down after the key */
    if (v->gate)
        a->vdec *= 0.99976f;
    arel = 1.0f - acid_exp2(-blk * 1.442695f / (0.010f * 44100.0f));
    {
        const float target = v->gate ? a->vdec * (1.0f + (a->accent ? accamt * 0.9f : 0.0f)) : 0.0f;
        a->vca += (target - a->vca) * (v->gate ? 0.35f : arel);
    }
    {
        const float hpa = 0.02114f;                   /* the feedback's high-pass: 150 Hz (1 - e^(-2 pi 150 / 44100)) */
        const float drive = 1.0f + (float)p[P_E7] * (14.0f / 127.0f);
        const float dmk = p[P_E7] ? 0.5f / acid_clip(drive * 0.5f) : 1.0f, lvl = a->vca * (1.0f / 32768.0f);
        const uint32_t dist = p[P_E7] != 0;
        float y1 = a->y1, y2 = a->y2, y3 = a->y3, y4 = a->y4, hl = a->hp, dc = a->dc;
        for (i = 0; i < n; i++) {
            float x = (float)acid_osc[i] * lvl, s;
            hl += (y4 - hl) * hpa;
            x -= k * acid_clip(y4 - hl);              /* the feedback, its bass taken out, soft-clipped */
            y1 += 2.0f * b0 * (x - y1 + y2);
            y2 += b0 * (y1 - 2.0f * y2 + y3);
            y3 += b0 * (y2 - 2.0f * y3 + y4);
            y4 += b0 * (y3 - 2.0f * y4);
            s = y4 * 1.6f;
            if (dist)
                s = acid_clip(s * drive) * dmk;
            dc += (s - dc) * 0.0014f;                /* (~10 Hz, after the drive: a clipped ramp is not centred) */
            s -= dc;
            s = s > 1.0f ? 1.0f : s < -1.0f ? -1.0f : s;
            acid_o[i] = (int32_t)(s * 32767.0f);
        }
        a->y1 = y1; a->y2 = y2; a->y3 = y3; a->y4 = y4; a->hp = hl; a->dc = dc;
    }
}

static void acid_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    acid_v_t *a = acid_v(t, v);
    uint32_t inc, ph = v->ph[0], i;
    if (n > CTL)
        n = CTL;
    if (v->note != a->note) {                         /* slid to another note: the envelopes go on, its accent counts */
        a->note = v->note;
        a->accent = v->mvel >= 110u;
    }
    inc = pitch_inc((uint32_t)clamp(m->pitch16 + t->p[P_E1] * 16, 0, 2047));
    if (m->fine)
        inc += (uint32_t)((int32_t)(inc >> 12) * m->fine);
    if (t->p[P_E0])
        for (i = 0; i < n; i++, ph += inc)
            acid_osc[i] = osc_pulse(ph, inc, 0x80000000u);
    else
        for (i = 0; i < n; i++, ph += inc)
            acid_osc[i] = osc_saw(ph, inc);
    v->ph[0] = ph;
    acid_model(t, v, a, n, m);
    for (i = 0; i < n; i++)
        out[i] += voice_amp(acid_o[i], m, i) << 1;
}

/* {WAVE TUNE CUT RES, ENV DEC ACC DIST}; the ADSR does nothing (the engine's own level), MONO: LEGATO */
static const preset_t ACID_PRESETS[] = {
    {"ACID LINE", {0, 0, 40, 100, 80, 45, 80, 0}, {0, 64, 127, 10}, 0, 1, FX(10, 0, 25, 15), PAT(1)},
    {"SQUELCH", {0, 0, 30, 118, 100, 60, 100, 20}, {0, 64, 127, 10}, 0, 1, FX(10, 0, 25, 15), PAT(14)},
    {"SQR ACID", {1, 0, 45, 90, 70, 40, 70, 0}, {0, 64, 127, 10}, 0, 1, FX(5, 0, 20, 15), PAT(15)},
    {"RUBBER", {0, 0, 55, 60, 50, 25, 60, 0}, {0, 64, 127, 10}, 0, 1, FX(0, 0, 20, 10), PAT(2)},
    {"DIST 303", {0, 0, 42, 105, 85, 55, 90, 90}, {0, 64, 127, 10}, 0, 1, FX(20, 0, 25, 15), PAT(16)},
    {"DEEP SUB", {1, -12, 30, 40, 30, 30, 40, 0}, {0, 64, 127, 10}, 0, 1, FX(0, 0, 10, 10), PAT(8)},
    {"SCREAM", {0, 0, 60, 124, 110, 80, 110, 60}, {0, 64, 127, 10}, 0, 1, FX(20, 0, 30, 20), PAT(17)},
    {"SOFT BASS", {0, 0, 35, 50, 40, 35, 30, 0}, {0, 64, 127, 10}, 0, 1, FX(0, 0, 15, 15), PAT(2)},
};

static const engine_t ENG_ACID = {
    .name = "ACID",
    .page_title = {"ACID", "ENV"},
    .edit = {
        {"WAVE", F_ENUM, 0, 1, 0, N_ACID_WAVE, 0},
        {"TUNE", F_SEMI, -12, 12, 0, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 40, 0, 0},
        {"RES", F_PCT, 0, 127, 100, 0, 0},
        {"ENV", F_PCT, 0, 127, 80, 0, 0},
        {"DEC", F_PCT, 0, 127, 45, 0, 0},
        {"ACC", F_PCT, 0, 127, 80, 0, 0},
        {"DIST", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = ACID_PRESETS,
    .npresets = NELEM(ACID_PRESETS),
    .note_on = acid_note_on,
    .render = acid_render,
    .knob = {P_E2, P_E3, P_E4, P_E5},                 /* HOME: CUTOFF RESONANCE ENV MOD DECAY, as the TD-3's */
    .poly = 4,
    .ownenv = 1,
    .done = acid_done,
    .mem = sizeof(acid_mem_t),
};
