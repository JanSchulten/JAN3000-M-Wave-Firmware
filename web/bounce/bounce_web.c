/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1's offline bounce: the browser emulator's firmware (web/emu/felucca_web.c, the same sources as the FM-1's)
 * with taps on the mixer (firmware/src/fx.c FX_PART_TAP, FX_MIX_TAP), run as fast as the CPU goes instead of along an
 * AudioWorklet. The page (web/bounce/bounce.js) writes a .FM1 file's flash extents into the NOR image (web_nor), boots,
 * loads the project (bounce_load), starts it (bounce_play) and runs the device a slice at a time (bounce_run),
 * taking what the taps gathered (bounce_take):
 *   0      the master, as USB audio has it (USB LEVEL FIXED: the MASTER knob out of it; SPEAKER EQ FLAT; no click)
 *   1..4   each track's share of the mix before the master (its level, pan, its own effects; dry of the buses)
 *   5      the rest of what goes into the master: the FX buses' returns (reverb, delay, chorus, HALL's side) and the
 *          FX layer's punch-ins, so 1..5 add up to the master before its limiter
 * Each stem through a DC blocker as the master's (fx.c master_out): the sum's comes off there, so the stems' too
 * Stereo floats, 1.0 = full scale (a stem may go past it: the master's limiter comes after). The device's clock is
 * the emulator's: fm1_ms and the audio halves as web_ms runs them (main loop every 15 ms, as on the FM-1). */
#include <stdint.h>
#define BN_STEMS 6u
#define BN_CAP 8192u                               /* frames gathered between two bounce_take (a slice: <= 4096) */
static float bn_buf[BN_STEMS][BN_CAP * 2u];
static uint32_t bn_n, bn_on;
static int32_t bn_part[4][32 * 2];                 /* this block's parts (CTL frames), L R */
static int32_t bn_prev_l[32], bn_prev_r[32];
static int32_t bn_dc[BN_STEMS][2], bn_dce[BN_STEMS][2];   /* each stem's DC blocker (master_out's, fx.c dc_block) */
static void bn_part_tap(uint32_t t, uint32_t n);
static void bn_mix_tap(const int32_t *out, uint32_t n, uint32_t perf);
#define FX_PART_TAP(t, n) bn_part_tap((t), (n))
#define FX_MIX_TAP(out, n, perf) bn_mix_tap((out), (n), (perf))
#include "../emu/felucca_web.c"

/* after part t's mix_part: its share is what mix_l / mix_r grew by */
static void bn_part_tap(uint32_t t, uint32_t n)
{
    uint32_t i;
    if (t == 0u)
        for (i = 0; i < n; i++)
            bn_prev_l[i] = bn_prev_r[i] = 0;
    if (t >= 4u)
        return;
    for (i = 0; i < n; i++) {
        bn_part[t][2u * i] = mix_l[i] - bn_prev_l[i];
        bn_part[t][2u * i + 1u] = mix_r[i] - bn_prev_r[i];
        bn_prev_l[i] = mix_l[i];
        bn_prev_r[i] = mix_r[i];
    }
}
/* the block done: the master, the parts and the rest (what master_out was given, less the parts): mix + wet, or with
 * the FX layer at work (perf) mix_l / mix_r themselves, which perf_master made that (scaled: x 1 at USB LEVEL FIXED) */
static void bn_mix_tap(const int32_t *out, uint32_t n, uint32_t perf)
{
    uint32_t i, k, keep = bn_on && bn_n + n <= BN_CAP;   /* (the DC blockers run on, gathered or not) */
    for (i = 0; i < n; i++) {
        uint32_t w = 2u * (bn_n + i);
        int32_t sl = 0, sr = 0;
        if (keep) {
            bn_buf[0][w] = (float)out[2u * i] / 32768.0f;
            bn_buf[0][w + 1u] = (float)out[2u * i + 1u] / 32768.0f;
        }
        for (k = 0; k < 5u; k++) {             /* (each through the master's DC blocker: the sum's DC goes there) */
            int32_t l = k < 4u ? bn_part[k][2u * i] : (perf ? mix_l[i] : mix_l[i] + wet[i]) - sl;
            int32_t r = k < 4u ? bn_part[k][2u * i + 1u] : (perf ? mix_r[i] : mix_r[i] + wet[i]) - sr;
            if (k < 4u) {
                sl += l;
                sr += r;
            }
            l = dc_block(l, &bn_dc[k][0], &bn_dce[k][0]);
            r = dc_block(r, &bn_dc[k][1], &bn_dce[k][1]);
            if (keep) {
                bn_buf[1u + k][w] = (float)l / 32768.0f;
                bn_buf[1u + k][w + 1u] = (float)r / 32768.0f;
            }
        }
    }
    if (keep)
        bn_n += n;
}

EXPORT float *bounce_buf(uint32_t stem) { return bn_buf[stem % BN_STEMS]; }
EXPORT uint32_t bounce_cap(void) { return BN_CAP; }
/* the frames gathered since the last take (read bounce_buf(0..5) up to them), then gathering starts again */
EXPORT uint32_t bounce_take(void)
{
    uint32_t n = bn_n;
    bn_n = 0;
    return n;
}
/* the device runs ms milliseconds (<= 90: a slice fits BN_CAP); gathered only while bounce_gather(1) */
EXPORT void bounce_run(uint32_t ms)
{
    while (ms--)
        web_ms();
}
EXPORT void bounce_gather(uint32_t on)
{
    bn_on = on != 0u;
    bn_n = 0;
}
/* after the boot (the splash run through): the music to bounce, 0..3 project slot A..D, 4 the autosave (CURRENT.FM1:
 * restored whatever RESTORE LAST says); then the bounce's own settings over the file's: USB LEVEL FIXED, SPEAKER EQ
 * FLAT, the click and the count-in off. 1 loaded */
EXPORT uint32_t bounce_load(uint32_t slot)
{
    uint32_t ok = 1;
    if (slot < 4u) {
        ok = proj_import(&proj_scratch, &proj_slot[slot], sizeof(project_store_t)) != 0;
        if (ok)
            project_load(slot);
    } else {
        ui_prefs &= ~PREF_RESTORE_OFF;
        ok = autosave_boot(1) != 0;
    }
    ui_prefs |= PREF_USB_FIXED;
    fx_usb_fixed = 1;
    settings.lowcut = 0;
    fx_lowcut = 0;
    rp_put(RP_CLICK, 0);
    rp_put(RP_COUNTIN, 0);
    ui.msg_t = 0;
    return ok;
}
/* start: 0 PLAY (the scene as it is: its patterns looping), 1 the scene song from its first row to its end (END
 * STOP for the bounce), 2 the SONG chain (the SONG page's PLAY: refused as there when a project it plays is not in
 * its slot, bounce_chain(1)), 3 every scene that holds something, in their order, one pass each: a scene song of them
 * (so the FM-1's own change from scene to scene: at the end of the longest track's loop, the sample's remainder
 * carried, scene.c), made here in place of the project's; a project with nothing in any scene: the one playing, once;
 * 4 an order of scenes of the page's own (bounce_order_row): a scene song of those rows, as 3. 1 started */
static uint8_t bn_ord_n, bn_ord_sc[SSG_ROWS], bn_ord_rep[SSG_ROWS];
/* the page's own order (mode 4), row by row: row i plays scene (0..15) repeat times (1..16); n rows in all (<= 16) */
EXPORT void bounce_order_clear(void) { bn_ord_n = 0; }
EXPORT uint32_t bounce_order_row(uint32_t i, uint32_t scene, uint32_t repeat)
{
    if (i >= SSG_ROWS || scene >= SCN_N)
        return 0;
    bn_ord_sc[i] = (uint8_t)scene;
    bn_ord_rep[i] = (uint8_t)(repeat < 1u ? 1u : repeat > 16u ? 16u : repeat);
    bn_ord_n = (uint8_t)(i + 1u > bn_ord_n ? i + 1u : bn_ord_n);
    return 1;
}
EXPORT uint32_t bounce_play(uint32_t mode)
{
    if (mode == 4u) {
        uint32_t i;
        if (!bn_ord_n)
            return 0;
        memset(&ssong, 0, sizeof ssong);
        for (i = 0; i < bn_ord_n; i++) {
            ssong.scene[i] = bn_ord_sc[i];
            ssong.repeat[i] = bn_ord_rep[i];
        }
        ssong.count = bn_ord_n;
        return ssong_start(0) == SQ_NOW;
    }
    if (mode == 3u) {
        uint32_t k;
        memset(&ssong, 0, sizeof ssong);
        for (k = 0; k < SCN_N; k++)
            if (scene_used(k)) {
                ssong.scene[ssong.count] = (uint8_t)k;
                ssong.repeat[ssong.count++] = 1;
            }
        if (!ssong.count) {
            ssong.scene[0] = scn.cur;
            ssong.repeat[0] = 1;
            ssong.count = 1;
        }
        return ssong_start(0) == SQ_NOW;
    }
    if (mode == 1u) {
        if (!ssong.count)
            return 0;
        ssong.flags &= (uint8_t)~SSG_LOOP;
        return ssong_start(0) == SQ_NOW;
    }
    if (mode == 2u) {
        if (!chain_config.count)
            return 0;
        chain_config.rsv[0] &= (uint8_t)~CHAIN_LOOP;   /* (END STOP for the bounce, as the scene song) */
        return chain_prepare() == 0u;
    }
    transport_req = 1;
    return 1;
}
/* the scenes that hold something: bit k scene k + 1 */
EXPORT uint32_t bounce_scenes(void)
{
    uint32_t k, m = 0;
    for (k = 0; k < SCN_N; k++)
        m |= scene_used(k) ? 1u << k : 0u;
    return m;
}
/* the SONG chain's projects: bit s slot A + s, of the slots its sections play (what 0), of those the ones that hold
 * no project here (what 1: the chain does not start, "PATTERN x EMPTY" on the FM-1) */
EXPORT uint32_t bounce_chain(uint32_t what)
{
    uint32_t i, k, m = 0;
    for (i = 0; i < chain_config.count && i < CHAIN_ROWS; i++)
        for (k = 0; k < NTRK; k++) {
            uint32_t s = chain_config.row[i].slot[k];
            if (s < 4u && (!what || !project_used(s)))
                m |= 1u << s;
        }
    return m;
}
/* the mixer: track t's LEVEL (0..127: the track's LVL, params.c P_LEVEL), as the project has it, and set for this
 * bounce (after bounce_load, before bounce_play; the file is not touched); the gain a value stands for (Q12, fx.c) */
EXPORT uint32_t bounce_level(uint32_t t) { return (uint32_t)trk[t % NTRK].p[P_LEVEL] & 127u; }
EXPORT void bounce_set_level(uint32_t t, uint32_t v) { trk[t % NTRK].p[P_LEVEL] = (int16_t)(v > 127u ? 127u : v); }
EXPORT uint32_t bounce_level_q12(uint32_t v) { return (uint32_t)LEVEL_Q12[v & 127u]; }
EXPORT void bounce_stop(void) { transport_req = 2; }   /* (as PLAY while playing: the tails ring on) */
EXPORT uint32_t bounce_busy(void) { return song.playing || chain_busy() || seq_counting() || transport_req == 1u; }
EXPORT uint32_t bounce_bpm10(void) { return (uint32_t)song.g[G_BPM] * 10u; }   /* (G_BPM: whole BPM) */
EXPORT uint32_t bounce_has(uint32_t mode) { return mode == 1u ? ssong.count != 0u : mode == 2u ? chain_config.count != 0u : 1u; }
/* the longest track's loop, in 1/16 steps of the tempo's (LEN x DIV): a pattern's natural length */
EXPORT uint32_t bounce_loop_ms(void)
{
    uint32_t t, best = 0;
    for (t = 0; t < NTRK; t++) {
        uint32_t ms = (uint32_t)((uint64_t)trk[t].p[P_SLEN] * div_samples((uint32_t)trk[t].p[P_SDIV]) * 1000u / FS);
        best = ms > best ? ms : best;
    }
    return best;
}
EXPORT const char *bounce_name(void) { return proj_name; }
