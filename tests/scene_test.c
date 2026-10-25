/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* SCENES (firmware/src/scene.c, the SEQ layer in ui_layer.c), on the firmware's own sources (ui_test.c's harness):
 *   CODEC    an empty scene packs to nothing and unpacks to empty steps with the timing it is reached from; 3000 random
 *            scenes (steps of every kind, automation and locks, FX lanes, timing) round-trip byte for byte, the size
 *            measured = the size written; the densest scene fits the bank; damaged or cut bytes never unpack wrong (each
 *            one unpacks to a scene the project format takes, or is refused) and never read past their end.
 *   SWITCH   stopped: at once, and back, the scenes exactly as they were (steps, LEN DIV SWING GATE QUANTIZE, automation
 *            and locks, FX TRACKS); an empty scene takes the timing; playing: at the end of the pattern (the loop of the
 *            longest track that plays something), not before, every track and lane from step 1 there, the grid kept
 *            (carry); a pick taken back by the scene playing; the transport stopped with one waiting: at once.
 *   MEMORY   picks are refused when the scene leaving would not fit, and one that grew past its room at the pattern's end
 *            is dropped there (SCENE MEMORY FULL): the music as it was, nothing lost.
 *   KEPT     a project saved and loaded brings its scenes and the one playing; another project's bank never applies.
 *   KEY      (M-Vave-OP1) each scene its own GLO > KEY (the title says whose: KEY S2): set on the page it is the scene's;
 *            an empty scene takes the KEY it is reached from, and visited without an edit stays empty; one with only a
 *            KEY of its own is kept; KEY OFF in one scene stays there; a scene packed before (no KEY bytes) takes the KEY
 *            playing and is packed with it after; playing, the switch starts the new scene's run at bar 1 (where its
 *            tracks start step 1); a copy has the KEY copied, each its own after; a project saved and loaded: every
 *            scene's KEY as it was.
 *   COPY     SEQ + OCT+ held 0.7 s: the scene playing copied into the next free scene (the first empty one after it,
 *            round past 16), which then plays: the tracks byte for byte as they were (playing: no step starts again,
 *            nothing released, no switch at the pattern's end), the original in the bank, each scene its own after; once
 *            while held, its release steps no scene; a pick waiting taken back; refused (nothing changed, a pick waiting
 *            still waits): an empty scene, none free, the bank full, a song; the copy kept with the project, the
 *            autosave told.
 *   LAYER    SEQ held: the white keys pick scenes 1..16, OCT+ / OCT- the next / the one before (none past 16 or before 1,
 *            their LEDs lit where there is one), OCT+ held the copy (once, while held; its release steps none),
 *            the black keys are the tools (CLEAR .. lane RANDOM), EDIT puts the track back, PRESETS picks a DRUM lane; the
 *            LEDs (the scene playing lit, one waiting breathing); a song playing: no scenes. The KO-II flow: a punch-in
 *            held after FX is let go keeps sounding while SEQ picks the next scene; let go, it stops.
 *   With a directory: the SEQ layer's screens.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("scene: %-100s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static uint32_t rs = 0x2545F491u;
static uint32_t rnd32(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static uint32_t rnd(uint32_t n) { return n ? rnd32() % n : 0u; }
static void blk(void) { static int32_t o[2 * CTL]; mix_block(o, CTL); }

/* ------------------------------------------------------------ CODEC --- */
static chain_pattern_t pa, pb;
static void rnd_step(step_t *s)
{
    uint32_t j;
    memset(s, 0, sizeof *s);
    if (rnd(3) == 0) {                                 /* (a REST, sometimes with the notes it kept) */
        *s = SCN_REST;
        if (rnd(2))
            s->note[0] = (uint8_t)rnd(128);
        return;
    }
    s->n = (uint8_t)rnd(5);
    for (j = 0; j < 4u; j++)
        s->note[j] = (uint8_t)rnd(128);
    s->time = (uint8_t)rnd(3);
    s->flags = (uint8_t)(rnd(256) & (3u | SF_RATCH | SF_NUDGE));
    s->vel = (uint8_t)rnd(128);
    s->hit = (uint8_t)(rnd(4) ? 0 : rnd(256));
    s->acc = (uint8_t)(s->hit & rnd(256));
    s->probability = (uint8_t)(rnd(102) | (rnd(4) ? 0u : STEP_LEG));   /* (M-Vave-OP1: LEGATO, kept with the step) */
}
static void rnd_pattern(chain_pattern_t *p, uint32_t density)
{
    uint32_t k, i, j, c;
    memset(p, 0, sizeof *p);
    for (k = 0; k < NTRK; k++) {
        for (i = 0; i < NSTEP; i++)
            if (rnd(100) < density)
                rnd_step(&p->step[k][i]);
            else
                p->step[k][i] = SCN_REST;
        p->timing[k][0] = (int16_t)(1 + rnd(NSTEP));
        p->timing[k][1] = (int16_t)(TP[P_SDIV].min + (int32_t)rnd((uint32_t)(TP[P_SDIV].max - TP[P_SDIV].min + 1)));
        p->timing[k][2] = (int16_t)(TP[P_SSWING].min + (int32_t)rnd((uint32_t)(TP[P_SSWING].max - TP[P_SSWING].min + 1)));
        p->timing[k][3] = (int16_t)(TP[P_SGATE].min + (int32_t)rnd((uint32_t)(TP[P_SGATE].max - TP[P_SGATE].min + 1)));
        p->qnt[k] = (uint8_t)rnd(2);
    }
    c = density >= 90u ? MOTION_MAX : rnd(4) ? rnd(40) : 0u;
    p->motion.on = (uint8_t)rnd(16);
    for (i = 0; i < c; i++) {                          /* unique (place, id) records of automatable ids */
        motion_event_t *e = &p->motion.event[p->motion.count];
        uint32_t id, place, dup;
        do {
            id = rnd(P_COUNT);
        } while (!motion_param(id));
        place = rnd(NTRK) << 6 | rnd(NSTEP);
        for (dup = 0, j = 0; j < p->motion.count; j++)
            dup |= p->motion.event[j].place == place && MOTION_ID(&p->motion.event[j]) == id;
        if (dup)
            continue;
        e->place = (uint8_t)place;
        e->param = (uint8_t)(id | (rnd(2) ? MOTION_LOCK : 0u));
        e->value = (int8_t)((int32_t)rnd(192) - 64);
        p->motion.count++;
    }
    for (k = 0; k < FXT_N; k++) {
        if (rnd(3) && density < 90u)
            continue;
        p->fx[k].len = (uint8_t)rnd(NSTEP + 1);
        for (i = 0; i < NSTEP; i++)
            if (density >= 90u || rnd(5) == 0)
                p->fx[k].step[i] = (uint8_t)(1 + rnd(255));
    }
    p->key[0] = (uint8_t)rnd(2);                         /* (M-Vave-OP1: a KEY, its own or not) */
    p->key[1] = (uint8_t)(1 + rnd(KEY_BARS));
    for (i = 0; i < KEY_BARS; i++)
        p->key[2 + i] = rnd(3) ? (uint8_t)rnd(12) : KEY_NONE;
    p->key_own = (uint8_t)rnd(2);
}
static int same_pattern(const chain_pattern_t *a, const chain_pattern_t *b)
{
    return !memcmp(a->step, b->step, sizeof a->step) && !memcmp(a->timing, b->timing, sizeof a->timing) &&
           !memcmp(a->qnt, b->qnt, sizeof a->qnt) && !memcmp(&a->motion, &b->motion, sizeof a->motion) &&
           !memcmp(a->fx, b->fx, sizeof a->fx) && !memcmp(a->key, b->key, sizeof a->key);
}
static uint8_t enc[8192], dmg[8192];
static void test_codec(void)
{
    scn_view_t v;
    uint8_t tm[NTRK][5];
    uint32_t it, n, m, ok = 1, sizes = 0, maxn = 0, k, j;
    memset(&pa, 0, sizeof pa);
    for (k = 0; k < NTRK; k++)
        for (j = 0; j < NSTEP; j++)
            pa.step[k][j] = SCN_REST;
    scn_view_pat(&v, &pa);
    for (k = 0; k < NTRK; k++)
        for (j = 0; j < 5u; j++)
            tm[k][j] = (uint8_t)(j == 0 ? 7u + k : j == 1 ? 3u : j == 2 ? 20u : j == 3 ? 90u : 0u);
    ok = scn_encode(&v, 0, 0) == 0u && scn_decode(enc, 0, &pb, tm);
    for (k = 0; k < NTRK; k++)
        ok &= pb.timing[k][0] == (int16_t)(7 + k) && pb.timing[k][1] == 3 && pb.timing[k][2] == 20 && pb.timing[k][3] == 90 &&
              !pb.qnt[k] && !memcmp(pb.step[k], pa.step[k], sizeof pa.step[k]);
    ck("CODEC: an empty scene packs to no byte; unpacked: empty (REST) steps, the timing it is reached from", ok);
    for (ok = 1, it = 0; it < 3000u; it++) {
        rnd_pattern(&pa, it % 7u == 0u ? 100u : it % 3u ? 15u : 45u);
        scn_view_pat(&v, &pa);
        n = scn_encode(&v, 0, 0);
        m = scn_encode(&v, enc, sizeof enc);
        ok &= n == m && n <= sizeof enc && scn_decode(enc, n, &pb, tm) && same_pattern(&pa, &pb) && pb.key_own;
        sizes += n;
        maxn = n > maxn ? n : maxn;
    }
    printf("  codec: 3000 scenes, %u B on average, the densest %u B (bank %u B)\n", sizes / 3000u, maxn, SCN_CAP);
    ck("  3000 random scenes (steps, automation and locks, FX lanes, timing) round-trip byte for byte, measured = written", ok);
    rnd_pattern(&pa, 100);                              /* the densest: every step, 128 records, both lanes full */
    for (k = 0; k < NTRK; k++)
        for (j = 0; j < NSTEP; j++) {
            pa.step[k][j].n = 4;
            memset(pa.step[k][j].note, 99, 4);
            pa.step[k][j].vel = pa.step[k][j].hit = pa.step[k][j].acc = 77;
            pa.step[k][j].probability = 50;
            pa.step[k][j].flags = 1;
            pa.step[k][j].time = ST_TIE;
        }
    scn_view_pat(&v, &pa);
    n = scn_encode(&v, 0, 0);
    printf("  codec: the densest possible scene %u B\n", n);
    ck("  the densest possible scene fits an empty bank (and two fit it half each at most)", n <= SCN_CAP && n > SCN_CAP / 3u);
    for (ok = 1, it = 0; it < 4000u; it++) {          /* damaged: a byte changed, or the end cut */
        uint32_t cut;
        rnd_pattern(&pa, 20u + rnd(60));
        scn_view_pat(&v, &pa);
        n = scn_encode(&v, enc, sizeof enc);
        memcpy(dmg, enc, n);
        cut = n;
        if (it & 1u)
            dmg[rnd(n ? n : 1)] ^= (uint8_t)(1u + rnd(255));
        else
            cut = rnd(n ? n : 1);
        memset(dmg + cut, 0xA5, sizeof dmg - cut);    /* (what follows must never be read as the scene) */
        if (scn_decode(dmg, cut, &pb, tm)) {
            for (k = 0; k < NTRK; k++)
                for (j = 0; j < NSTEP; j++) {
                    const step_t *s = &pb.step[k][j];
                    ok &= s->n <= 4u && s->time <= ST_REST && (s->probability & ~STEP_LEG) <= 101u &&
                          !(s->flags & ~(3u | SF_RATCH | SF_NUDGE));
                }
            ok &= motion_valid(&pb.motion) && pb.fx[0].len <= NSTEP && pb.fx[1].len <= NSTEP;
            if (cut != n)                            /* (cut short: never the whole scene) */
                ok &= !same_pattern(&pa, &pb) || cut == n;
        }
    }
    ck("  damaged or cut scenes: refused, or a scene the project format takes (never past their end)", ok);
}

/* ------------------------------------------------------------ SWITCH --- */
static void melody(track_t *t, uint32_t base, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        step_clear(&t->step[i]);
    for (i = 0; i < len; i += 2u) {
        uint8_t n = (uint8_t)(base + i % 12u);
        put_step(t, i, 1, &n, ST_NOTE, 0);
    }
    t->p[P_SLEN] = (int16_t)len;
}
static void capture(chain_pattern_t *p)              /* the scene playing, in the RAM form */
{
    uint32_t k;
    memset(p, 0, sizeof *p);
    for (k = 0; k < NTRK; k++) {
        memcpy(p->step[k], trk[k].step, sizeof p->step[k]);
        memcpy(p->timing[k], &trk[k].p[P_SLEN], sizeof p->timing[k]);
        p->qnt[k] = (uint8_t)trk[k].p[P_SQNT];
    }
    p->motion = motion;
    memcpy(p->fx, fxt_lane, sizeof p->fx);
    memcpy(p->key, (const void *)&keyt, sizeof p->key);   /* (M-Vave-OP1: its KEY) */
    p->key_own = key_own;
}
static void scene_setup(void)
{
    ui_power_on();
    stop_transport();
    song.g[G_BPM] = 120;
    melody(&trk[0], 60, 16);
    melody(&trk[1], 48, 16);
    trk[1].p[P_SDIV] = 3;
    trk[2].p[P_SSWING] = 30;
    motion_set_lock(&trk[0], 2, P_REV, 40);
    motion_set_event(&trk[1], 4, P_ATK, 10);
    fxt_lane[0].step[0] = PF_R16 + 1u;
    fxt_lane[0].step[1] = FXT_TIE;
    fxt_lane[1].len = 8;
    fxt_lane[1].step[3] = PF_LPF + 1u;
    frame();
}
static void seq_pick(uint32_t s)                     /* SEQ held + white key s, let go */
{
    btn_down(B_SEQ);
    frame();
    key_down(white(s));
    frame();
    key_up(white(s));
    btn_up(B_SEQ);
    frame();
    frames(64);
}
static uint32_t blocks_to_switch(uint32_t max, uint32_t want)   /* audio blocks until scn.cur == want (max: none) */
{
    uint32_t b;
    for (b = 0; b < max; b++) {
        blk();
        if (scn.cur == want)
            return b;
    }
    return max;
}
static void test_switch(void)
{
    chain_pattern_t s1, s2;
    uint32_t ok, b, k;
    scene_setup();
    capture(&s1);
    seq_pick(1);
    ok = scn.cur == 1u && msg_is("SCENE 2") && scb.len[0] > 0u && !scb.len[1] && seq_is_empty(&trk[0]) &&
         seq_is_empty(&trk[1]) && !motion.count && !fxt_lane[0].step[0] && !fxt_lane[1].len;
    for (k = 0; k < NTRK; k++)                         /* (the timing it was reached from) */
        ok &= !memcmp(&trk[k].p[P_SLEN], s1.timing[k], sizeof s1.timing[k]) && trk[k].p[P_SQNT] == s1.qnt[k];
    ck("SWITCH: stopped, SEQ + A3: scene 2 at once, empty, the timing kept; scene 1 packed in the bank", ok);
    melody(&trk[0], 72, 12);                           /* scene 2: its own */
    trk[0].p[P_SDIV] = 1;
    motion_set_lock(&trk[0], 1, P_DLY, 20);
    fxt_lane[1].step[5] = PF_REV + 1u;
    capture(&s2);
    seq_pick(0);
    capture(&pa);
    ok = scn.cur == 0u && same_pattern(&pa, &s1) && scb.len[1] > 0u && !scb.len[0];
    seq_pick(1);
    capture(&pb);
    ok &= scn.cur == 1u && same_pattern(&pb, &s2);
    ck("  back and forth: each scene exactly as it was (steps, timing, QUANTIZE, automation and locks, FX TRACKS)", ok);

    /* playing: at the pattern's end. Scene 2 (T1 12 steps of 1/8 is 1.5 bars, the longest) -> scene 1 */
    transport_req = 1;
    for (b = 0; b < 1000u; b++)
        blk();
    seq_pick(0);
    ok = msg_is("NEXT: SCENE 1") && scn.req == SCN_END && scn.cur == 1u && scn.clk == 0u;
    {   /* the loop's end from T1's place now: the samples of its steps left (with their swing) less those played of
         * this one; the switch in the block they run out in, the remainder carried into step 1 */
        const track_t *t = &trk[0];
        uint32_t i, left = 0, want, d = div_samples((uint32_t)t->p[P_SDIV]);
        for (i = t->seq_idx; i < (uint32_t)t->p[P_SLEN]; i++)
            left += step_samples(t, d, i);
        left -= t->seq_pos;
        want = (left + CTL - 1u) / CTL - 1u;
        b = blocks_to_switch(20000, 0);
        printf("  switch: %u blocks after the pick (the pattern ends in block %u), step 1 %u samples in\n", b, want,
               trk[0].seq_pos);
        ok &= scn.cur == 0u && b == want && trk[0].seq_idx == 0u && trk[0].seq_pos == (b + 1u) * CTL - left;
    }
    frame();
    capture(&pa);
    ok &= msg_is("SCENE 1") && scb.len[1] > 0u;
    for (k = 0; k < NTRK; k++)
        ok &= !memcmp(pa.step[k], s1.step[k], sizeof s1.step[k]);
    ck("  playing: SEQ + F3 waits (NEXT: SCENE 1) and switches at the end of the longest track's loop, step 1 there", ok);
    {   /* the grid kept: the steps after the switch fall where they would have */
        uint32_t i, pos0 = trk[0].seq_pos;
        for (i = 0; i < 3u; i++)
            blk();
        ok = trk[0].seq_pos == pos0 + 3u * CTL || trk[0].seq_idx > 0u;
        ck("  the grid kept over the switch (the remainder carried; the next steps on time)", ok);
    }
    /* a pick taken back; the transport stopped with one waiting */
    seq_pick(4);
    ok = scn.req == SCN_END && scn.next == 4u;
    seq_pick(0);
    ok &= msg_is("SCENE 1 STAYS") && scn.req == SCN_IDLE;
    for (b = 0; b < 6000u; b++)
        blk();
    ok &= scn.cur == 0u;
    ck("  the scene playing picked again: the pick waiting taken back (SCENE 1 STAYS), nothing switches", ok);
    seq_pick(1);
    transport_req = 2;
    blk();
    frame();
    ok = scn.cur == 1u && !song.playing && scn.req == SCN_IDLE;
    ck("  the transport stopped with a scene waiting: it switches at once", ok);
}

/* ------------------------------------------------------------ MEMORY --- */
static void dense_scene(void)
{
    uint32_t k, i;
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < NSTEP; i++) {
            uint8_t n[4] = {(uint8_t)(40 + i), (uint8_t)(52 + k), (uint8_t)(64 + i % 7u), 90};
            put_step(&trk[k], i, 4, n, ST_NOTE, 1);
            trk[k].step[i].probability = (uint8_t)(1 + i);
            trk[k].step[i].hit = (uint8_t)(i | 1u);
            trk[k].step[i].acc = 1;
        }
}
static void test_memory(void)
{
    chain_pattern_t keep;
    uint32_t ok, s;
    scene_setup();
    for (s = 1; s < 4u; s++) {                         /* dense scenes until the bank is full */
        dense_scene();
        seq_pick(s);
        if (msg_is("SCENE MEMORY FULL"))
            break;
    }
    printf("  memory: %u B used of %u after %u dense scenes\n", scb.used, SCN_CAP, s - 1u);
    capture(&keep);
    ok = msg_is("SCENE MEMORY FULL") && s >= 2u && scn.cur == s - 1u;
    capture(&pa);
    ok &= same_pattern(&pa, &keep);
    ck("MEMORY: dense scenes fill the bank; the next pick says SCENE MEMORY FULL, the music stays as it was", ok);
    /* a pick that fitted, the scene grown past its room before the pattern's end: dropped there, nothing lost */
    scene_setup();
    dense_scene();
    seq_pick(1);                                        /* (scene 1 dense in the bank) */
    dense_scene();
    seq_pick(2);                                        /* (scene 2 too; 3 now, empty: ~1 KB left) */
    melody(&trk[0], 60, 16);
    transport_req = 1;
    for (s = 0; s < 100u; s++)
        blk();
    seq_pick(3);                                        /* fits now: scene 3 is small */
    ok = scn.req == SCN_END;
    dense_scene();                                      /* .. and grows (as recording would) past what is left */
    trk[0].p[P_SLEN] = 16;
    capture(&keep);
    for (s = 0; s < 8000u && scn.req; s++)
        blk();
    frame();
    capture(&pa);
    ok &= scn.cur == 2u && !scn.req && msg_is("SCENE MEMORY FULL") && same_pattern(&pa, &keep);
    ck("  a scene grown past its room at the pattern's end: the switch dropped there (SCENE MEMORY FULL), nothing lost", ok);
    stop_transport();
}

/* ------------------------------------------------------------ KEPT --- */
static void test_kept(void)
{
    chain_pattern_t s1, s2;
    project_store_t slot;
    uint32_t ok;
    scene_setup();
    capture(&s1);
    seq_pick(5);
    melody(&trk[3], 36, 8);
    capture(&s2);
    ok = project_save_as(2, "SCN") == 0;
    slot = proj_slot[2];
    scene_setup();                                     /* (another project now, with a scene of its own; the slot kept) */
    proj_slot[2] = slot;
    seq_pick(3);
    melody(&trk[2], 50, 4);
    ok &= scn.cur == 3u;
    project_load(2);
    capture(&pa);
    ok &= msg_is("LOADED") && scn.cur == 5u && same_pattern(&pa, &s2) && scene_used(0) && !scene_used(1) &&
          !scene_used(3);
    seq_pick(0);
    capture(&pb);
    ok &= same_pattern(&pb, &s1);
    ck("KEPT: a project saved with its scenes loads with them, the one playing too (scene 6), scene 1 as it was", ok);
    scb_ram[2].h.proj ^= 0x55u;                        /* (another project's bank in the slot: never applied) */
    seq_pick(3);
    project_load(2);
    capture(&pa);
    ok = msg_is("LOADED") && scn.cur == 0u && same_pattern(&pa, &s2) && !scene_used(1) && !scene_used(5) && !scb.used;
    ck("  a bank that is another project's: not applied (the project alone, the scene it holds as scene 1)", ok);
    scb_ram[2].h.proj ^= 0x55u;
}

/* ------------------------------------------------------------ COPY --- */
static void seq_copy(void)                           /* SEQ held + OCT+ held past SEQ_COPY_MS, let go */
{
    btn_down(B_SEQ);
    frame();
    btn_down(B_OCTUP);
    frames(SEQ_COPY_MS + 64u);
    btn_up(B_OCTUP);
    frame();
    btn_up(B_SEQ);
    frame();
}
static track_t trk_was[NTRK];
static motion_store_t mo_was;
static fxt_lane_t fx_was[FXT_N];
static void music_keep(void)                         /* what the ISR plays from: the tracks, the automation, the lanes */
{
    memcpy(trk_was, trk, sizeof trk);
    mo_was = motion;
    memcpy(fx_was, fxt_lane, sizeof fx_was);
}
static int music_same(void)
{
    return !memcmp(trk_was, trk, sizeof trk) && !memcmp(&mo_was, &motion, sizeof motion) &&
           !memcmp(fx_was, fxt_lane, sizeof fx_was);
}
static uint32_t packed_now(void)                     /* the bytes the scene playing packs to */
{
    scn_view_t v;
    scn_view_trk(&v);
    return scn_encode(&v, 0, 0);
}
static void test_copy(void)
{
    chain_pattern_t s1, s2;
    project_store_t slot;
    uint32_t ok, b, k, used, gen, sig;
    scene_setup();
    capture(&s1);
    music_keep();
    sig = autosave_sig();
    undo.trk = 1;                                     /* (an undo copy of the scene playing: it goes, as with a switch) */
    seq_copy();
    capture(&pa);
    ok = msg_is("COPIED TO SCENE 2") && scn.cur == 1u && music_same() && same_pattern(&pa, &s1) && scb.len[0] > 0u &&
         !scb.len[1] && scene_used(0) && scene_used(1) && !scene_used(2) && !undo.trk && autosave_sig() != sig;
    ck("COPY: stopped, SEQ + OCT+ held: COPIED TO SCENE 2, the copy plays (the tracks byte for byte), scene 1 in the bank", ok);
    seq_pick(0);
    capture(&pa);
    ok = scn.cur == 0u && same_pattern(&pa, &s1);
    seq_pick(1);
    capture(&pb);
    ok &= scn.cur == 1u && same_pattern(&pb, &s1);
    melody(&trk[0], 72, 12);                           /* the copy changed: the original stays as it was */
    motion_set_lock(&trk[1], 3, P_DLY, 30);
    fxt_lane[1].step[6] = PF_REV + 1u;
    capture(&s2);
    seq_pick(0);
    capture(&pa);
    ok &= same_pattern(&pa, &s1);
    seq_pick(1);
    capture(&pb);
    ok &= same_pattern(&pb, &s2);
    ck("  scene 1 and its copy: each exactly the scene copied, then each its own (steps, automation, FX TRACKS)", ok);

    /* the gesture: the copy once while OCT+ is still held, its release steps no scene; OCT+ / OCT- tapped step */
    scene_setup();
    btn_down(B_SEQ);
    frame();
    btn_down(B_OCTUP);
    frames(SEQ_COPY_MS - 64u);
    ok = scn.cur == 0u && !scb.used;                  /* (not yet) */
    frames(128);
    ok &= msg_is("COPIED TO SCENE 2") && scn.cur == 1u;
    frames(1500);                                      /* (held on: once) */
    ok &= scn.cur == 1u && !scene_used(2);
    btn_up(B_OCTUP);
    frame();
    ok &= scn.cur == 1u && !scn.req && !ui.msg_t;     /* (let go: no step, nothing said) */
    btn_down(B_OCTUP);
    frame();
    btn_up(B_OCTUP);
    frame();
    ok &= scn.cur == 2u && msg_is("SCENE 3");
    btn_down(B_OCTDN);
    frame();
    btn_up(B_OCTDN);
    frame();
    ok &= scn.cur == 1u && msg_is("SCENE 2");
    btn_down(B_OCTDN);                                 /* (OCT+ held with OCT- down: no copy, no step) */
    frame();
    btn_down(B_OCTUP);
    frames(SEQ_COPY_MS + 200u);
    btn_up(B_OCTUP);
    btn_up(B_OCTDN);
    frame();
    ok &= scn.cur == 1u && !scene_used(2) && song.octave == 0;
    btn_up(B_SEQ);
    frame();
    ok &= !ui.layer;
    ck("  OCT+ held 0.7 s: the copy once, while still held; let go: no step; OCT+ / OCT- tapped: the next / the one "
       "before; with OCT- down: nothing", ok);

    /* the next free: the first empty scene after the one playing, past a scene used, round past 16 */
    seq_pick(3);
    melody(&trk[0], 50, 8);                            /* (scene 4 used) */
    seq_pick(1);
    seq_copy();
    ok = msg_is("COPIED TO SCENE 3") && scn.cur == 2u;
    seq_copy();
    ok &= msg_is("COPIED TO SCENE 5") && scn.cur == 4u && scene_used(3);   /* (past scene 4) */
    seq_pick(15);
    melody(&trk[1], 40, 4);
    capture(&s2);
    seq_copy();
    ok &= msg_is("COPIED TO SCENE 6") && scn.cur == 5u && scene_used(15);  /* (16, then 1 .. 5 used) */
    capture(&pa);
    seq_pick(15);
    capture(&pb);
    ok &= same_pattern(&pa, &s2) && same_pattern(&pb, &s2);
    ck("  the next free scene: 2 -> 3, 3 -> 5 (past 4, used), 16 -> 6 (round, past 1 .. 5)", ok);

    /* refused, nothing changed (OCT- dark): the scene playing empty, none free, a song, the bank full */
    seq_pick(9);
    used = scb.used;
    gen = scb.gen;
    ok = !scene_used(9);
    seq_copy();
    ok &= msg_is("NOTHING TO COPY") && scn.cur == 9u && scb.used == used && scb.gen == gen && !scene_used(10);
    for (k = 0; k < SCN_N; k++) {                      /* every scene used */
        seq_pick(k);
        if (!scene_used(k))
            melody(&trk[2], 30 + k, 2);
    }
    used = scb.used;
    gen = scb.gen;
    capture(&s2);
    seq_copy();
    capture(&pa);
    ok &= msg_is("NO FREE SCENE") && scn.cur == 15u && scb.used == used && scb.gen == gen && same_pattern(&pa, &s2);
    scene_setup();
    chain.armed = 1;
    seq_copy();
    ok &= msg_is("STOP THE SONG FIRST") && scn.cur == 0u && !scb.used;
    chain.armed = 0;
    for (k = 1; k < 4u; k++) {                         /* dense scenes until a copy no longer fits */
        dense_scene();
        seq_copy();
        if (msg_is("SCENE MEMORY FULL"))
            break;
    }
    printf("  copy: %u B used of %u after %u dense copies\n", scb.used, SCN_CAP, k - 1u);
    ok &= msg_is("SCENE MEMORY FULL") && k >= 2u && scn.cur == k - 1u && scb.used + packed_now() > SCN_CAP &&
          !scene_used(k);
    used = scb.used;
    gen = scb.gen;
    capture(&s2);
    seq_copy();                                        /* (again: nothing changes) */
    capture(&pa);
    ok &= msg_is("SCENE MEMORY FULL") && scn.cur == k - 1u && scb.used == used && scb.gen == gen && same_pattern(&pa, &s2);
    ck("  refused, nothing changed: NOTHING TO COPY, NO FREE SCENE, STOP THE SONG FIRST, SCENE MEMORY FULL", ok);

    /* playing: the tracks play on (no step starts again, nothing released), no switch at the pattern's end */
    scene_setup();
    transport_req = 1;
    for (b = 0; b < 700u; b++)
        blk();
    btn_down(B_SEQ);
    frame();
    music_keep();
    btn_down(B_OCTUP);
    frames(SEQ_COPY_MS + 64u);
    btn_up(B_OCTUP);
    frame();
    ok = msg_is("COPIED TO SCENE 2") && scn.cur == 1u && music_same() && scn.req == SCN_IDLE && song.playing;
    btn_up(B_SEQ);
    frame();
    ok &= blocks_to_switch(6000, 0) == 6000u && scn.cur == 1u;
    ck("  playing: the copy at once, the music byte for byte as it was (steps, positions, voices), no switch after", ok);
    seq_pick(4);                                       /* a pick waiting: taken back, the copy plays */
    ok = scn.req == SCN_END && scn.next == 4u;
    seq_copy();
    ok &= msg_is("COPIED TO SCENE 3") && scn.cur == 2u && scn.req == SCN_IDLE && blocks_to_switch(6000, 4) == 6000u;
    ck("  a pick waiting (scene 5): taken back by the copy, nothing switches after (the copy plays)", ok);
    seq_pick(9);                                       /* .. refused: the pick still waits, and comes at the pattern's */
    for (b = 0; b < 6000u && scn.req; b++)             /* end (scene 10: empty) */
        blk();
    frame();
    seq_pick(0);
    ok = scn.cur == 9u && !scene_used(9) && scn.req == SCN_END && scn.next == 0u;
    seq_copy();
    ok &= msg_is("NOTHING TO COPY") && scn.req == SCN_END && scn.next == 0u && blocks_to_switch(20000, 0) < 20000u;
    ck("  refused with a pick waiting (NOTHING TO COPY): the pick still waits and switches at the pattern's end", ok);
    stop_transport();
    scene_setup();                                     /* the bank too full for a copy, a pick that fits waiting (its */
    dense_scene();                                     /* own bytes make the room): SCENE MEMORY FULL, the pick waits */
    seq_copy();
    seq_copy();
    transport_req = 1;
    for (b = 0; b < 300u; b++)
        blk();
    seq_pick(0);
    ok = scn.cur == 2u && scn.req == SCN_END && scn.next == 0u;
    seq_copy();
    ok &= msg_is("SCENE MEMORY FULL") && scn.req == SCN_END && scn.next == 0u && blocks_to_switch(20000, 0) < 20000u;
    frame();
    ok &= msg_is("SCENE 1") && scene_used(1) && scene_used(2) && !scene_used(3);
    ck("  refused with a pick waiting (SCENE MEMORY FULL, the pick fits): the pick still waits and switches", ok);
    stop_transport();

    /* kept with the project */
    scene_setup();
    seq_copy();
    melody(&trk[3], 36, 8);                            /* (the copy its own) */
    capture(&s2);
    ok = msg_is("COPIED TO SCENE 2") && project_save_as(1, "CPY") == 0;
    slot = proj_slot[1];
    scene_setup();                                     /* (another project now; the slot kept) */
    proj_slot[1] = slot;
    seq_pick(4);
    project_load(1);
    capture(&pa);
    ok &= msg_is("LOADED") && scn.cur == 1u && same_pattern(&pa, &s2) && scene_used(0) && !scene_used(2);
    seq_pick(0);
    capture(&pb);
    ok &= same_pattern(&pb, &s1);
    ck("  kept: a project saved after a copy loads with both, the copy playing", ok);
}

/* ------------------------------------------------------------ LAYER --- */
static void test_seq_layer(const char *dir)
{
    uint32_t ok, br, m;
    scene_setup();
    btn_down(B_SEQ);
    frames(500);
    ok = ui.layer == LAYER_SEQ && str_eq(layer_head(), "[SEQ] SCENES") && (oct_leds() & 3u) == 2u;   /* (OCT+ lit) */
    if (dir)
        screen_save(dir, "seq_scenes_1");
    btn_down(B_OCTDN);
    frame();
    btn_up(B_OCTDN);
    frame();
    ok &= msg_is("SCENE 1 IS THE FIRST") && scn.cur == 0u;
    key_down(white(15));
    frame();
    key_up(white(15));
    frame();
    ok &= scn.cur == 15u && msg_is("SCENE 16") && (oct_leds() & 3u) == 1u;   /* (OCT- lit, OCT+ dark: none after) */
    btn_down(B_OCTUP);
    frame();
    btn_up(B_OCTUP);
    frame();
    ok &= msg_is("SCENE 16 IS THE LAST") && scn.cur == 15u;
    btn_down(B_OCTDN);
    frame();
    btn_up(B_OCTDN);
    frame();
    ok &= scn.cur == 14u && msg_is("SCENE 15") && song.octave == 0 && (oct_leds() & 3u) == 3u;
    seq_pick(0);
    btn_down(B_SEQ);
    frame();
    btn_down(B_OCTUP);
    frames(SEQ_COPY_MS + 64u);
    ok &= msg_is("COPIED TO SCENE 2") && scn.cur == 1u && song.octave == 0;   /* (scene 1 holds the setup's) */
    if (dir)
        screen_save(dir, "seq_scenes_copied");
    btn_up(B_OCTUP);
    frame();
    btn_up(B_SEQ);
    frame();
    ck("LAYER: SEQ held: [SEQ] SCENES; OCT-: none before 1; G5: scene 16; OCT+: none past it; OCT-: scene 15 (no octave); "
       "OCT+ held: the copy; OCT LEDs", ok);
    seq_pick(0);
    melody(TSEL, 60, 8);
    {
        step_t ref[NSTEP];
        memcpy(ref, TSEL->step, sizeof ref);
        btn_down(B_SEQ);
        frame();
        key_down(black(1));                           /* G#3: REVERSE */
        frame();
        key_up(black(1));
        frame();
        ok = msg_is("REVERSED") && memcmp(ref, TSEL->step, sizeof ref);
        key_down(black(0));                           /* F#3: CLEAR */
        frame();
        key_up(black(0));
        frame();
        ok &= msg_is("SEQUENCE CLEARED") && seq_is_empty(TSEL);
        btn_down(B_EDIT);                             /* EDIT: the track as the layer opened */
        frame();
        btn_up(B_EDIT);
        frame();
        ok &= msg_is("SEQUENCE PUT BACK") && !memcmp(ref, TSEL->step, sizeof ref) && ui.layer == LAYER_SEQ;
        btn_up(B_SEQ);
        frame();
        ck("  the black keys are the tools (G#3 REVERSE, F#3 CLEAR), SEQ + EDIT puts the track back", ok);
    }
    {   /* a DRUM track: BEAT and the lane's tools on the black keys, PRESETS the lane */
        set_engine_of(TSEL, ENGI_DRUM);
        TSEL->engine = TSEL->eng_req;
        track_defaults_steps(TSEL);
        TSEL->p[P_SLEN] = 16;
        ui.lane = 0;
        btn_down(B_SEQ);
        frame();
        turn(EN_PRESET, 1);
        ok = ui.lane == 1u && ui.layer == LAYER_SEQ;
        key_down(black(9));                           /* D#5: the lane's FILL */
        frame();
        key_up(black(9));
        frame();
        {
            uint32_t i, c = 0;
            for (i = 0; i < 16u; i++)
                c += (step_lanes(TSEL->eng_req, &TSEL->step[i]) >> 1) & 1u;
            ok &= c > 0u;
        }
        if (dir)
            screen_save(dir, "seq_scenes_drum");
        btn_up(B_SEQ);
        frame();
        ck("  DRUM: SEQ + PRESETS picks the lane, its FILL on D#5 fills that lane", ok);
    }
    /* the LEDs: the scene playing lit, one waiting breathes; a song playing: no scenes */
    scene_setup();
    transport_req = 1;
    blk();
    btn_down(B_SEQ);
    frame();
    key_down(white(6));
    frame();
    key_up(white(6));
    frame();
    m = layer_leds(&br);
    ok = ((m >> white(0)) & 1u) && ((br >> white(6)) & 1u) && !((m >> white(6)) & 1u) && ((br >> black(0)) & 1u);
    if (dir)
        screen_save(dir, "seq_scenes_waiting");
    btn_up(B_SEQ);
    frame();
    ck("  LEDs: the scene playing lit (F3), the one waiting breathes (E4), the tools breathe", ok);
    stop_transport();
    scene_setup();
    chain.armed = 1;
    seq_pick(2);
    ok = msg_is("STOP THE SONG FIRST") && scn.cur == 0u;
    chain.armed = 0;
    ck("  a song playing or about to: no scene picks (STOP THE SONG FIRST)", ok);
}

/* the KO-II flow: a punch-in held after FX is let go, SEQ picks the next scene meanwhile; the effect ends with its key */
static void test_ko(void)
{
    uint32_t ok, b, g3 = white(1);
    scene_setup();
    transport_req = 1;
    for (b = 0; b < 200u; b++)
        blk();
    btn_down(B_FX);
    frame();
    key_down(g3);                                       /* G3: REPEAT 1/16 */
    frame();
    btn_up(B_FX);
    frames(100);
    ok = (perf_held >> PF_R16) & 1u;
    btn_down(B_SEQ);
    frame();
    key_down(white(4));                                 /* C4: scene 5, at the pattern's end */
    frame();
    key_up(white(4));
    frame();
    ok &= scn.req == SCN_END && scn.next == 4u && ((perf_held >> PF_R16) & 1u) && msg_is("NEXT: SCENE 5");
    btn_down(B_OCTUP);                                  /* OCT+: the one after the one waiting */
    frame();
    btn_up(B_OCTUP);
    frame();
    ok &= scn.next == 5u && ((perf_held >> PF_R16) & 1u);
    btn_up(B_SEQ);
    frame();
    ok &= ui.layer == LAYER_FX;                         /* (SEQ let go: FX's map again, its punch-in still held) */
    for (b = 0; b < 3000u && scn.req; b++)
        blk();
    frame();
    ok &= scn.cur == 5u && ((perf_held >> PF_R16) & 1u);
    key_up(g3);
    frame();
    ok &= !((perf_held >> PF_R16) & 1u) && !ui.layer;
    ck("KO-II: a punch-in held after FX is let go keeps on while SEQ + C4 / OCT+ picks scene 6 (FX's map back after); "
       "let go, it stops", ok);
    stop_transport();
}

/* ------------------------------------------------------------- KEY --- */
static const uint8_t AM_F_C_G[4] = {9, 5, 0, 7}, D_G[2] = {2, 7};
static void key_set(const uint8_t *roots, uint32_t len)   /* GLO > KEY by hand, stopped: LEN, the roots from bar 1, ON */
{
    uint32_t i;
    go_page(GR_KEY);
    frames(400);                                     /* (a knob just after the SEQ layer let go is the layer's) */
    turn(EN_K3, (int32_t)len - (int32_t)keyt.len);
    turn(EN_K1, -16);
    for (i = 0; i < len; i++) {                      /* (a key: the cursor bar's root, the cursor on) */
        key_down((roots[i] + 7u) % 12u);
        frame();
        key_up((roots[i] + 7u) % 12u);
        frame();
    }
    turn(EN_K4, 1);
}
static int key_is(const uint8_t *r, uint32_t len, uint32_t on)
{
    uint32_t i, ok = keyt.len == len && keyt.on == on;
    for (i = 0; i < len; i++)
        ok &= keyt.root[i] == r[i];
    return (int)ok;
}
static void test_key(void)
{
    uint32_t ok, b, off, n, cp, i;
    char ti[20];
    scene_setup();                                   /* no KEY in use: no KEY bytes (a bank firmware before reads) */
    seq_pick(1);
    seq_pick(2);
    ok = scb.len[0] > 0u && scb.len[1] == 0u && !(scb.data[scb_off(0)] & SCN_F_KEY);
    ck("KEY: none in use: the scenes packed without KEY bytes (a bank the firmware before reads)", ok);
    scene_setup();
    key_set(AM_F_C_G, 4);
    page_title(ti);
    ok = key_own && key_is(AM_F_C_G, 4, 1) && str_eq(ti, "KEY S1 1/2");
    seq_pick(1);
    go_page(GR_KEY);
    frame();
    page_title(ti);
    ok &= scn.cur == 1u && key_is(AM_F_C_G, 4, 1) && !key_own && str_eq(ti, "KEY S2 1/2");
    ck("KEY: scene 1's set on GLO > KEY (KEY S1) is its own; an empty scene 2 takes it (KEY S2, not its own yet)", ok);
    key_set(D_G, 2);
    ok = key_own && key_is(D_G, 2, 1);
    seq_pick(0);
    ok &= scn.cur == 0u && key_is(AM_F_C_G, 4, 1) && key_own && scb.len[1] > 0u && scene_used(1);
    seq_pick(1);
    ok &= key_is(D_G, 2, 1) && key_own && seq_is_empty(&trk[0]);
    ck("  scene 2's KEY set (no notes): kept with it (the scene used); scene 1's back on 1, scene 2's on 2", ok);
    seq_pick(2);
    ok = scn.cur == 2u && key_is(D_G, 2, 1) && !key_own;
    seq_pick(0);
    ok &= !scb.len[2] && !scene_used(2) && key_is(AM_F_C_G, 4, 1);
    ck("  an empty scene visited without an edit: the KEY it is reached from, and it stays empty", ok);
    seq_pick(5);
    go_page(GR_KEY);
    frames(400);
    turn(EN_K4, 1);                                  /* (KEY ON already: nothing changes) */
    turn(EN_K3, -16);                                /* (LEN 4 down to 1 and back up to 4: a change each, but */
    ok = key_own;                                    /* the scene's own then) */
    seq_pick(0);
    ok &= scene_used(5);
    seq_pick(5);                                     /* (now its own: a no-op elsewhere checked below) */
    seq_pick(7);
    go_page(GR_KEY);
    frames(400);
    turn(EN_K4, 1);
    ok &= !key_own;
    seq_pick(0);
    ok &= !scene_used(7) && !scb.len[7];
    ck("  a knob turn that changes nothing (KEY ON again): not the scene's own, it stays empty; a change: its own", ok);
    seq_pick(3);
    melody(&trk[0], 64, 8);
    go_page(GR_KEY);
    frames(400);
    turn(EN_K4, -1);
    ok = !keyt.on && key_own;
    seq_pick(0);
    ok &= keyt.on && key_is(AM_F_C_G, 4, 1);
    seq_pick(3);
    ok &= key_is(AM_F_C_G, 4, 0) && key_own;
    seq_pick(0);
    ck("  KEY OFF in scene 4 (with notes): OFF there, ON in scene 1", ok);
    off = scb_off(3);                                /* scene 4 as packed before KEY was a scene's: no KEY bytes */
    n = scb.len[3];
    ok = n > 18u && (scb.data[off] & SCN_F_KEY) && scn.cur == 0u;
    memcpy(enc, scb.data + off, n);
    enc[0] &= (uint8_t)~SCN_F_KEY;
    off = scb_resize(3, n - 18u);
    memcpy(scb.data + off, enc, n - 18u);
    seq_pick(3);
    ok &= scn.cur == 3u && key_is(AM_F_C_G, 4, 1) && !key_own && !seq_is_empty(&trk[0]);
    seq_pick(0);
    off = scb_off(3);
    ok &= scb.len[3] == n && (scb.data[off] & SCN_F_KEY);
    ck("  a scene packed before (no KEY bytes): the KEY playing (a project of before plays as it did); kept with it after", ok);

    /* playing: the switch at the pattern's end starts the new scene's run at bar 1 */
    transport_req = 1;
    for (b = 0; b < 40000u && !(song.playing && key_bar == 2u); b++)
        blk();
    ok = song.playing && key_bar == 2u && scn.cur == 0u;
    seq_pick(1);
    ok &= scn.req == SCN_END && scn.cur == 0u && key_is(AM_F_C_G, 4, 1);
    b = blocks_to_switch(40000u, 1u);
    ok &= b < 40000u && key_is(D_G, 2, 1) && key_own && key_bar == 0u && keyr.step == 0u;
    for (n = 0; n < 40000u && key_bar == 0u; n++)
        blk();
    ok &= key_bar == 1u && n * CTL + CTL >= 16u * div_samples(2) - CTL && n * CTL <= 16u * div_samples(2) + CTL;
    transport_req = 2;
    blk();
    ck("  playing: at the switch (the pattern's end) the new scene's KEY from bar 1, a bar of 1/16s later bar 2", ok);

    /* a copy: the KEY copied, then each its own */
    seq_pick(0);
    seq_copy();
    cp = scn.cur;
    ok = cp != 0u && key_is(AM_F_C_G, 4, 1) && key_own && !seq_is_empty(&trk[0]);
    go_page(GR_KEY);
    frames(400);
    turn(EN_K1, -16);
    turn(EN_K2, -5);                                 /* (bar 1: A -> E) */
    ok &= keyt.root[0] == 4u;
    seq_pick(0);
    ok &= key_is(AM_F_C_G, 4, 1);
    seq_pick(cp);
    ok &= keyt.root[0] == 4u && keyt.root[1] == 5u;
    ck("  COPY: the copy plays the KEY copied; its bar 1 edited, scene 1 keeps its own", ok);

    /* kept with the project */
    {
        project_store_t slot;
        ok = project_save_as(2, "KEYS") == 0;
        slot = proj_slot[2];
        scene_setup();
        proj_slot[2] = slot;
        ok &= key_is_default() && !key_own;
        project_load(2);
        ok &= msg_is("LOADED") && scn.cur == cp && keyt.root[0] == 4u && key_own;
        seq_pick(0);
        ok &= key_is(AM_F_C_G, 4, 1) && key_own;
        seq_pick(1);
        ok &= key_is(D_G, 2, 1) && key_own;
        seq_pick(3);                                 /* (scene 4: the KEY it took as a scene of before, ON) */
        ok &= key_is(AM_F_C_G, 4, 1) && key_own;
        seq_pick(2);
        ok &= keyt.root[0] == 4u;
        ck("KEY KEPT: a project saved and loaded, every scene's KEY as it was (the one playing in the project's extras)", ok);
        seq_pick(6);                                 /* an empty scene, its KEY set to none on purpose: its own */
        go_page(GR_KEY);
        frames(400);
        turn(EN_K4, -1);
        turn(EN_K1, -16);
        for (i = 0; i < 4u; i++) {
            turn(EN_K2, -12);
            turn(EN_K1, 1);
        }
        ok = key_own && key_is_default() && scn.cur == 6u;
        ok &= project_save_as(2, "KEYS") == 0;
        slot = proj_slot[2];
        scene_setup();
        proj_slot[2] = slot;
        project_load(2);
        ok &= scn.cur == 6u && key_own && key_is_default();
        seq_pick(0);
        ok &= key_is(AM_F_C_G, 4, 1);
        seq_pick(6);
        ok &= key_is_default() && key_own;
        ck("  an empty scene's KEY set to none on purpose: its own after a save and a load (not scene 1's)", ok);
    }
}

int main(int argc, char **argv)
{
    test_codec();
    test_switch();
    test_memory();
    test_kept();
    test_key();
    test_copy();
    test_seq_layer(argc > 1 ? argv[1] : 0);
    test_ko();
    if (bad) {
        printf("scene: %d FAILED\n", bad);
        return 1;
    }
    printf("scene: all ok\n");
    return 0;
}
