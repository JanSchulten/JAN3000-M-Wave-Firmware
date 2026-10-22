/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* VDRUM and DBOX on the sequencer as DRUM (firmware/src/kitlane.c, M-Vave-OP1), through the UI harness:
 *   MODEL    the lanes: DRUM 8, VDRUM 6 (its parts), DBOX 8 (BELL its cowbell, B 47); every other engine none; a note's
 *            lane on each (VDRUM: its part, DBOX: its key); VDRUM's and DBOX's lanes named as DRUM's, whatever their
 *            P_E0 means (no conga, no clave)
 *   GRID     SEQ > STEP is the grid; black keys 1..6 (VDRUM) / 1..8 (DBOX) pick the lane,
 *            VDRUM's 7 and 8 do nothing and stay dark; white keys toggle hits; the keys play their lanes' notes (DBOX
 *            BELL 47); a DRUM hit on RIM / BELL shows on VDRUM's CLAP / HATOP and an edit there makes it the lane's own;
 *            a lane 7 or 8 picked on DRUM is lane 6 on VDRUM; the lanes' keys flash as they play
 *   REC      live recording writes hits (a lane's note; vel > 110 its accent), other notes stay notes on their lane
 *   MUTE     the EDIT layer's black keys mute the lanes (VDRUM 6, DBOX 8): a muted lane silent, the others as ever, its
 *            name said; DBOX's mute takes every key on the lane (E with SNARE)
 *   TOOLS    SEQ TOOLS' BEAT and the lane row are there; BEAT the basic beat; RANDOM fills VDRUM's six lanes only
 *   LOAD     a VDRUM or DBOX track's notes on its lanes' notes become hits when a project loads, a scene is picked, a
 *            pattern loads; other notes stay; DRUM's steps of today's format stay as stored; nothing sounds different
 *            (the sequencer's output bit for bit, the notes as written against the converted steps)
 *   PRESET   a user preset of a VDRUM track stores its grid
 * Screens of the two grids into argv[1] when given. Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("kitgrid: %-104s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}

static uint32_t kg_lanes_of(const track_t *t, uint32_t l)   /* lane l over steps 1..16, a bit each */
{
    uint32_t i, m = 0;
    for (i = 0; i < 16u; i++)
        m |= ((step_lanes(t->eng_req, &t->step[i]) >> l) & 1u) << i;
    return m;
}

/* track 4 on engine e, settled (its fade done), SEQ > STEP shown */
static track_t *kit_on(uint32_t e)
{
    int32_t o[2u * CTL];
    uint32_t i;
    ui_power_on();
    for (i = 0; i < NTRK; i++)
        track_defaults_steps(&trk[i]);
    track_select(3);
    set_engine_of(TSEL, e);
    for (i = 0; i < 256u && (TSEL->xf_on || TSEL->eng_req != TSEL->engine); i++)
        mix_block(o, CTL);
    go_home();
    frame();
    fam_next(FAM_SEQ);
    frame();
    return TSEL;
}

static void step_notes(step_t *s, uint32_t a, uint32_t b)   /* a NOTE step of one or two notes */
{
    step_clear(s);
    s->time = ST_NOTE;
    s->note[0] = (uint8_t)a;
    s->note[1] = (uint8_t)b;
    s->n = b ? 2u : 1u;
    s->vel = 100;
}

/* the patterns as they were written before the grid: lane notes (in lane order within a step) and others */
static void pat_vdrum(step_t *s)
{
    uint32_t i;
    memset(s, 0, NSTEP * sizeof *s);
    for (i = 0; i < NSTEP; i++)
        step_clear(&s[i]);
    step_notes(&s[0], 36, 0);
    step_notes(&s[2], 42, 0);
    step_notes(&s[3], 46, 42);                           /* (out of lane order: the open hat, then the closed one */
    step_notes(&s[4], 38, 42);                           /* choking it: kept as notes, it plays as it did) */
    step_notes(&s[5], 45, 41);                           /* (two pitches of the TOM part: the last one sounds) */
    step_notes(&s[6], 42, 0);
    step_notes(&s[8], 36, 46);
    s[8].flags = SF_ACCENT;
    step_notes(&s[10], 39, 0);
    step_notes(&s[12], 38, 45);
    step_notes(&s[13], 41, 0);                           /* a low tom: pitched, stays a note */
    step_notes(&s[14], 49, 0);                           /* a crash: VDRUM's HATOP, not its note: stays */
    step_notes(&s[15], 37, 0);                           /* a rim: VDRUM's CLAP part, not its note: stays */
}
static void pat_dbox(step_t *s)
{
    uint32_t i;
    memset(s, 0, NSTEP * sizeof *s);
    for (i = 0; i < NSTEP; i++)
        step_clear(&s[i]);
    step_notes(&s[0], 36, 0);
    step_notes(&s[2], 42, 0);
    step_notes(&s[4], 38, 42);
    step_notes(&s[6], 44, 0);                            /* G#: the pedal hat, not a lane's note */
    step_notes(&s[8], 36, 46);
    s[8].flags = SF_ACCENT;
    step_notes(&s[10], 39, 0);
    step_notes(&s[12], 47, 0);                           /* B: the cowbell, BELL's */
    step_notes(&s[13], 40, 0);                           /* E: snare 2, stays */
    step_notes(&s[14], 37, 0);
    step_notes(&s[15], 45, 0);
}

/* the whole mix over two loops of the 16 steps from the top, hashed (FNV-1a); track 4 plays steps st on engine e */
static uint64_t play_hash(uint32_t e, const step_t *st)
{
    int32_t o[2u * CTL];
    uint64_t h = 1469598103934665603ull;
    uint32_t i, b, p;
    track_t *t = kit_on(e);
    go_home();
    frame();
    memcpy(t->step, st, sizeof t->step);
    t->p[P_SLEN] = 16;
    song.master_q12 = 4096;
    for (p = 0; p < NPART; p++)
        memset(trk[p].v, 0, sizeof trk[p].v);
    memset(eng_mem, 0, sizeof eng_mem);
    transport_req = 1;
    for (b = 0; b < 2u * 16u * step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), 0) / CTL; b++) {
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++)
            h = (h ^ (uint32_t)o[i]) * 1099511628211ull;
    }
    transport_req = 2;
    mix_block(o, CTL);
    return h;
}

/* .. from the same state each time: in a child process (as ui_test.c scr_session_fork) */
static uint64_t play_hash_fork(uint32_t e, const step_t *st)
{
    int fd[2];
    uint64_t r = 0;
    pid_t p;
    if (pipe(fd))
        return 0;
    p = fork();
    if (!p) {
        r = play_hash(e, st);
        if (write(fd[1], &r, sizeof r) != sizeof r) _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &r, sizeof r) != sizeof r) r = 0;
    close(fd[0]);
    waitpid(p, 0, 0);
    return r;
}

static void kg_model(void)
{
    track_t *t = &trk[0];
    uint32_t e, ok = 1;
    for (e = 0; e < NENGINES; e++)
        ok &= kit_lanes(e) == (e == ENGI_DRUM || e == ENGI_DBOX ? 8u : e == ENGI_VDRUM ? 6u : 0u);
    ck("the lanes: DRUM 8, VDRUM 6, DBOX 8, every other engine none (the piano roll)", ok);
    ok = kit_notes(ENGI_DBOX)[DV_BELL] == 47u && kit_notes(ENGI_VDRUM) == DRUM_LANE_NOTE && kit_notes(ENGI_DRUM) == DRUM_LANE_NOTE;
    for (e = 0; e < DV_BELL; e++)
        ok &= kit_notes(ENGI_DBOX)[e] == DRUM_LANE_NOTE[e];
    ck("the lanes' notes: DRUM's 36 38 39 42 46 45 37 56; DBOX's the same but BELL on B, its cowbell (47)", ok);
    ok = lane_of(ENGI_VDRUM, 37) == DV_CLAP && lane_of(ENGI_VDRUM, 56) == DV_HATO && lane_of(ENGI_VDRUM, 49) == DV_HATO &&
         lane_of(ENGI_VDRUM, 41) == DV_TOM && lane_of(ENGI_VDRUM, 72) == DV_TOM && lane_of(ENGI_VDRUM, 35) == DV_KICK;
    for (e = 0; e < 6u; e++)
        ok &= lane_of(ENGI_VDRUM, DRUM_LANE_NOTE[e]) == e;
    ck("VDRUM: a note on its part's lane (RIM 37 on CLAP, BELL 56 / crash 49 on HATOP, toms and percussion on TOM)", ok);
    ok = lane_of(ENGI_DBOX, 40) == DV_SNARE && lane_of(ENGI_DBOX, 41) == DV_TOM && lane_of(ENGI_DBOX, 43) == DV_TOM &&
         lane_of(ENGI_DBOX, 44) == DV_HATC && lane_of(ENGI_DBOX, 56) == DV_HATC && lane_of(ENGI_DBOX, 59) == DV_BELL;
    for (e = 0; e < NLANE; e++)
        ok &= lane_of(ENGI_DBOX, DBX_LANE_NOTE[e]) == e;
    ck("DBOX: a note on its key's lane (E SNARE, F G TOM, G# HATCL in any octave, B BELL)", ok);
    ui_power_on();
    set_engine_of(t, ENGI_VDRUM);
    t->p[P_E0] = 1;                                     /* (TYPE STRNG: as DRUM's KIT it would be 66, a conga on TOM) */
    ok = str_eq(drum_lane_name(t, 0), "KICK") && str_eq(drum_lane_name(t, DV_TOM), "TOM") &&
         str_eq(drum_lane_abbr(t, DV_TOM), "TM") && str_eq(drum_lane_name(t, DV_HATC), "HATCL");
    set_engine_of(t, ENGI_DBOX);
    t->p[P_E0] = 3;                                     /* (KIT METAL) */
    ok &= str_eq(drum_lane_name(t, DV_RIM), "RIM") && str_eq(drum_lane_abbr(t, DV_BELL), "CB") &&
          str_eq(drum_lane_name(t, DV_TOM), "TOM");
    ck("the lanes named as DRUM's (VDRUM: its parts), whatever their P_E0 is (no CONGA, no CLAVE, no CYM)", ok);
}

static void kg_grid(const char *shots)
{
    track_t *t = kit_on(ENGI_VDRUM);
    uint32_t leds, glow, ok, i;
    ok = grid_on() && song.grid == 1u && cur_page()->scope == SC_STEP && kit_track(t) == 6u;
    ck("VDRUM: SEQ > STEP is the grid (the keys are the UI's)", ok);
    tap_key(key_at(1, 2));
    tap_key(key_at(0, 4));
    ok = ui.lane == DV_CLAP && t->step[4].hit == 1u << DV_CLAP && !t->step[4].n && t->step[4].time == ST_NOTE;
    ck("  black key 3: CLAP; white key 5: its hit on step 5", ok);
    tap_key(key_at(1, 5));
    ok = ui.lane == DV_TOM;
    tap_key(key_at(1, 6));
    ok &= ui.lane == DV_TOM && ui.bank == 0u;
    tap_key(key_at(1, 7));
    ok &= ui.lane == DV_TOM && ui.bank == 0u;
    t->p[P_SLEN] = 32;
    frame();
    leds = grid_leds();
    glow = grid_glow();
    ok &= !((leds | glow) >> key_at(1, 6) & 1u) && !((leds | glow) >> key_at(1, 7) & 1u) && (glow >> key_at(1, 5) & 1u) &&
          (glow >> key_at(1, GK_ACC) & 1u) && (glow >> key_at(1, GK_PGUP) & 1u) && (leds >> key_at(1, DV_TOM) & 1u);
    t->p[P_SLEN] = 16;
    ck("  black keys 1..6 the parts; 7 and 8 nothing (no lane, no page) and dark; ACC and the pages as on DRUM", ok);
    turn(EN_K2, 3);
    ok = ui.lane == DV_TOM;
    turn(EN_K2, -1);
    ok &= ui.lane == DV_HATO;
    ck("  KNOB 2 LANE stops at the sixth (TOM)", ok);
    kb_prev = 0;
    fm1_in.notes = (1u << key_at(1, 0)) | (1u << key_at(1, 6)) | (1u << key_at(1, 7));
    keyboard_block();
    ok = kb_note[key_at(1, 0)] == 36u && kb_note[key_at(1, 6)] == KB_SILENT && kb_note[key_at(1, 7)] == KB_SILENT;
    fm1_in.notes = 0;
    keyboard_block();
    ck("  the keys: black key 1 plays KICK (36), 7 and 8 silent", ok);
    {   /* a struck part flashes its lane's key */
        int32_t o[2u * CTL];
        drum_lit[3] = 0;
        ui.lane = DV_KICK;
        trk_note_on(t, 46, 100);
        mix_block(o, CTL);
        ok = drum_lit[3] == 1u << DV_HATO;
        trk_note_on(t, 49, 100);                        /* (the crash: the same part) */
        ok &= drum_lit[3] == 1u << DV_HATO;
        (void)grid_leds();
        ok &= (grid_leds() >> key_at(1, DV_HATO) & 1u) != 0u;
        frames(200);
        ok &= !(grid_leds() >> key_at(1, DV_HATO) & 1u);
        ck("  a part struck (46, the crash 49 too) flashes its lane's key, then it goes dark", ok);
    }
    /* a DRUM beat's RIM and BELL hits on VDRUM: on CLAP and HATOP, an edit there the lane's own */
    step_clear(&t->step[6]);
    t->step[6].time = ST_NOTE;
    t->step[6].hit = (uint8_t)(1u << DV_RIM | 1u << DV_BELL | 1u << DV_KICK);
    t->step[6].acc = (uint8_t)(1u << DV_BELL);
    ok = step_lanes(ENGI_VDRUM, &t->step[6]) == (1u << DV_KICK | 1u << DV_CLAP | 1u << DV_HATO) &&
         step_accents(ENGI_VDRUM, &t->step[6]) == 1u << DV_HATO;
    grid_hit(t, 6, DV_CLAP, 0);
    ok &= t->step[6].hit == (1u << DV_BELL | 1u << DV_KICK);
    grid_hit(t, 6, DV_HATO, 1);
    ok &= t->step[6].hit == (1u << DV_HATO | 1u << DV_KICK) && t->step[6].acc == 1u << DV_HATO;
    ck("  a DRUM beat's RIM / BELL hits show on CLAP / HATOP; an edit makes them the lane's own (the accent kept)", ok);
    /* lane 8 picked on DRUM, then VDRUM: its sixth */
    set_engine_of(t, ENGI_DRUM);
    frame();
    tap_key(key_at(1, 7));
    ok = ui.lane == DV_BELL;
    set_engine_of(t, ENGI_VDRUM);
    frame();
    ok &= ui.lane == DV_TOM && grid_on();
    ck("  lane 8 picked on DRUM: VDRUM's sixth (TOM) once VDRUM plays", ok);
    if (shots) {
        for (i = 0; i < 16u; i += 4u)
            grid_hit(t, i, DV_KICK, 1);
        for (i = 2; i < 16u; i += 4u)
            grid_hit(t, i, DV_HATC, 1);
        grid_acc(t, 12, DV_SNARE, 1);
        drum_mute[3] = 1u << DV_TOM;
        ui.lane = DV_SNARE;
        ui.cursor = 12;
        frame();
        screen_save(shots, "kitgrid_vdrum");
        drum_mute[3] = 0;
    }

    t = kit_on(ENGI_DBOX);
    ok = grid_on() && kit_track(t) == 8u;
    tap_key(key_at(1, 7));
    tap_key(key_at(0, 2));
    ok &= ui.lane == DV_BELL && t->step[2].hit == 1u << DV_BELL;
    glow = grid_glow();
    ok &= (glow >> key_at(1, 7) & 1u) && (glow >> key_at(1, 6) & 1u);
    kb_prev = 0;
    fm1_in.notes = (1u << key_at(1, 7)) | (1u << key_at(1, 6));
    keyboard_block();
    ok &= kb_note[key_at(1, 7)] == 47u && kb_note[key_at(1, 6)] == 37u;
    fm1_in.notes = 0;
    keyboard_block();
    ck("DBOX: the grid with eight lanes; black key 8 BELL (plays 47, B: the cowbell), its hit on step 3", ok);
    {
        int32_t o[2u * CTL];
        drum_lit[3] = 0;
        trk_note_on(t, 40, 100);                        /* (E: snare 2, on SNARE) */
        trk_note_on(t, 59, 100);                        /* (B an octave up: BELL) */
        mix_block(o, CTL);
        ck("  a key struck flashes its lane (E on SNARE, a B on BELL)", drum_lit[3] == (1u << DV_SNARE | 1u << DV_BELL));
    }
    {   /* the hits play the lanes' notes: lane 8 at 47 */
        step_t st[NSTEP];
        uint64_t hn, hh;
        uint32_t k;
        for (k = 0; k < NSTEP; k++)
            step_clear(&st[k]);
        step_notes(&st[0], 47, 0);
        step_notes(&st[4], 47, 0);
        hn = play_hash_fork(ENGI_DBOX, st);
        for (k = 0; k < NSTEP; k++)
            step_clear(&st[k]);
        st[0].time = st[4].time = ST_NOTE;
        st[0].hit = st[4].hit = 1u << DV_BELL;
        st[0].vel = st[4].vel = 100;
        hh = play_hash_fork(ENGI_DBOX, st);
        ck("  (the render from one state twice: the same)", hh == play_hash_fork(ENGI_DBOX, st) && hh);
        ck("  BELL's hits sound as the note B (47) did, bit for bit", hn == hh);
    }
    if (shots) {
        uint32_t k;
        t = kit_on(ENGI_DBOX);
        for (k = 0; k < 16u; k += 4u)
            grid_hit(t, k, DV_KICK, 1);
        for (k = 4; k < 16u; k += 8u)
            grid_hit(t, k, DV_SNARE, 1);
        for (k = 0; k < 16u; k += 2u)
            grid_hit(t, k, DV_HATC, 1);
        grid_hit(t, 14, DV_BELL, 1);
        grid_acc(t, 14, DV_BELL, 1);
        ui.lane = DV_BELL;
        ui.cursor = 14;
        frame();
        screen_save(shots, "kitgrid_dbox");
    }
}

static void kg_rec(void)
{
    uint32_t e, ok;
    for (e = 0; e < 2u; e++) {
        uint32_t eng = e ? ENGI_DBOX : ENGI_VDRUM;
        track_t *t = kit_on(eng);
        song.rec = 1u << 3;
        song.playing = 1;
        t->seq_idx = 5;
        t->seq_pos = 0;
        fm1_in.notes = 1u << key_at(1, 3);              /* the grid's HATCL key */
        keyboard_block();
        fm1_in.notes = 0;
        keyboard_block();
        ok = t->step[5].hit == 1u << DV_HATC && !t->step[5].n;
        t->seq_idx = 9;
        rec_note(t, 36, 120, 0);                        /* (a lane's note, hard: accented) */
        rec_note(t, e ? 40u : 49u, 90, 0);              /* (VDRUM a crash, DBOX snare 2: notes on their lanes) */
        ok &= t->step[9].hit == 1u << DV_KICK && t->step[9].acc == 1u << DV_KICK && t->step[9].n == 1u &&
              t->step[9].note[0] == (e ? 40u : 49u) &&
              step_lanes(eng, &t->step[9]) == (1u << DV_KICK | 1u << (e ? DV_SNARE : DV_HATO));
        song.rec = 0;
        song.playing = 0;
        ck(e ? "REC on DBOX: a lane's key a hit, a lane's note hard an accented hit, E a note on SNARE"
             : "REC on VDRUM: a lane's key a hit, a lane's note hard an accented hit, a crash a note on HATOP", ok);
    }
}

static void kg_mute(void)
{
    static const uint8_t NOTE[2][3] = {{36, 38, 46}, {36, 38, 40}};
    uint32_t e, ok, a, b2, i;
    for (e = 0; e < 2u; e++) {
        uint32_t eng = e ? ENGI_DBOX : ENGI_VDRUM, n = e ? 8u : 6u;
        track_t *t = kit_on(eng);
        double en[3];
        go_home();
        frame();
        btn_down(B_EDIT); frames(500);
        a = leds_at(0); b2 = leds_at(250);
        ok = eqa_count(t) == n;
        for (i = 0; i < LY_INIT_B; i++)
            ok &= (((a & b2) >> black(i)) & 1u) == (i < n);
        key_down(black(1)); frame(); key_up(black(1)); frame();
        ok &= drum_mute[3] == 1u << DV_SNARE && msg_is("SNARE MUTED");
        btn_up(B_EDIT); frame();
        ck(e ? "DBOX: EDIT + black key 2 mutes SNARE (8 lane keys lit)" : "VDRUM: EDIT + black key 2 mutes SNARE (6 part keys lit)", ok);
        for (i = 0; i < 3u; i++) {                      /* each struck alone: its energy, SNARE muted */
            int32_t o[CTL];
            uint32_t b, k, p;
            for (p = 0; p < NPART; p++)
                memset(trk[p].v, 0, sizeof trk[p].v);
            memset(eng_mem, 0, sizeof eng_mem);
            en[i] = 0;
            trk_note_on(t, NOTE[e][i], 100);
            for (b = 0; b < 64u; b++) {
                track_render(t, o, CTL);
                for (k = 0; k < CTL; k++)
                    en[i] += (double)o[k] * o[k];
            }
            trk_note_off(t, NOTE[e][i]);
        }
        drum_mute[3] = 0;
        ok = en[0] > 0.0 && en[1] == 0.0 && (e ? en[2] == 0.0 : en[2] > 0.0);
        ck(e ? "  DBOX: the snare (D) silent and snare 2 (E, on SNARE) too; the kick sounds"
             : "  VDRUM: the snare part silent (its send too: no resonance); the kick and the open hat sound", ok);
    }
}

static void kg_tools(void)
{
    uint32_t r, i, hits;
    track_t *t = kit_on(ENGI_VDRUM);
    int ok = tl_cell(t, TL_BEAT) && tl_cell(t, TL_LCLR) && tl_cell(t, TL_LRND);
    tl_do(t, TL_BEAT, 0);
    ok &= kg_lanes_of(t, DV_KICK) == 0x1111u && kg_lanes_of(t, DV_SNARE) == 0x1010u && kg_lanes_of(t, DV_HATC) == 0x5555u;
    ck("VDRUM: SEQ TOOLS has BEAT and the lane row; BEAT: kick on the beats, the backbeat, 8th hats", ok);
    for (r = 0, ok = 1; r < 40u; r++) {
        tl_do(t, TL_RANDOM, 0);
        for (i = 0, hits = 0; i < 16u; i++)
            hits |= t->step[i].hit | step_lanes(ENGI_VDRUM, &t->step[i]);
        ok &= !(hits >> 6) && (kg_lanes_of(t, DV_KICK) & 1u);
        tl_do(t, TL_COOK, 0);
        for (i = 0, hits = 0; i < 16u; i++)
            hits |= t->step[i].hit;
        ok &= !(hits >> 6);
    }
    ck("  RANDOM and COOK (40 times): beats in the six lanes only, the kick on step 1", ok);
    t = kit_on(ENGI_DBOX);
    tl_do(t, TL_LFILL, DV_BELL);
    ck("DBOX: FILL LANE on BELL: every step", kg_lanes_of(t, DV_BELL) == 0xFFFFu && t->step[3].hit == 1u << DV_BELL);
}

static void kg_load(void)
{
    step_t raw[NSTEP], conv[NSTEP];
    uint32_t e, ok, i;
    for (e = 0; e < 2u; e++) {
        uint32_t eng = e ? ENGI_DBOX : ENGI_VDRUM;
        track_t *t;
        uint64_t h0, h1;
        if (e)
            pat_dbox(raw);
        else
            pat_vdrum(raw);
        memcpy(conv, raw, sizeof conv);
        kit_steps_load(eng, conv);
        ok = conv[0].hit == 1u << DV_KICK && !conv[0].n && conv[4].hit == (1u << DV_SNARE | 1u << DV_HATC) &&
             conv[8].hit == (1u << DV_KICK | 1u << DV_HATO) && conv[8].acc == conv[8].hit && !(conv[8].flags & SF_ACCENT) &&
             conv[12].hit == (e ? 1u << DV_BELL : (1u << DV_SNARE | 1u << DV_TOM)) &&
             conv[13].n == 1u && conv[13].note[0] == raw[13].note[0];
        ok &= e ? conv[6].n == 1u && conv[6].note[0] == 44u && step_lanes(eng, &conv[6]) == 1u << DV_HATC &&
                      step_lanes(eng, &conv[13]) == 1u << DV_SNARE && conv[14].hit == 1u << DV_RIM && !conv[14].n &&
                      conv[15].hit == 1u << DV_TOM
                : conv[14].n == 1u && !conv[14].hit && step_lanes(eng, &conv[14]) == 1u << DV_HATO &&
                      conv[15].note[0] == 37u && step_lanes(eng, &conv[15]) == 1u << DV_CLAP &&
                      step_lanes(eng, &conv[13]) == 1u << DV_TOM && !memcmp(&conv[3], &raw[3], sizeof conv[3]) &&
                      !memcmp(&conv[5], &raw[5], sizeof conv[5]) &&
                      step_lanes(eng, &conv[3]) == (1u << DV_HATC | 1u << DV_HATO);
        ck(e ? "DBOX: its lane notes (36 38 39 42 46 45 37 47) hits, G# E stay notes on their lanes"
             : "VDRUM: its lane notes (36 38 39 42 46 45) hits, the accent the hits'; 41 49 37 stay notes on their lanes; "
               "a step out of lane order ([46 42], [45 41]) stays as it was", ok);
        h0 = play_hash_fork(eng, raw);
        h1 = play_hash_fork(eng, conv);
        conv[2].hit = 0;                                 /* (a hat less: another sound) */
        ok = play_hash_fork(eng, conv) != h1;
        conv[2].hit = 1u << DV_HATC;
        ck("  the sequencer's output with the notes as written and as hits: the same, bit for bit", h0 == h1 && ok);

        /* a project of the notes (saved before this grid) loads as hits */
        t = kit_on(eng);
        memcpy(t->step, raw, sizeof t->step);
        set_engine_of(&trk[2], ENGI_DRUM);
        memcpy(trk[2].step, raw, sizeof trk[2].step);
        ok = project_save(0) == 0;
        memset(t->step, 0, sizeof t->step);
        project_load(0);
        ok &= trk[3].eng_req == eng && !memcmp(trk[3].step, conv, sizeof conv) && !memcmp(trk[2].step, raw, sizeof raw);
        ck("  a project's VDRUM / DBOX steps load as hits (DRUM's of today's format as stored)", ok);

        /* a scene of the notes: picked, it is hits */
        t = kit_on(eng);
        memcpy(t->step, raw, sizeof t->step);
        ok = scene_copy() == SC_OK && scn.cur == 1u;
        for (i = 0; i < NSTEP; i++)
            step_clear(&t->step[i]);
        ok &= scene_queue(0) == SQ_NOW && scn.cur == 0u && !memcmp(t->step, conv, sizeof conv);
        ck("  a scene of them (written before), picked: its steps hits", ok);

        /* a pattern: SAVE > PHRASES' 12 BEAT */
        t = kit_on(eng);
        load_pat16(t, PATTERNS[11].note, PATTERNS[11].flags);
        for (i = 0, ok = 1; i < 16u; i++)
            ok &= t->step[i].time != ST_NOTE || t->step[i].hit || t->step[i].n;
        ok &= (t->step[0].hit >> DV_KICK) & 1u;
        for (i = 0; i < 16u; i++)
            if (t->step[i].time == ST_NOTE && t->step[i].n)
                ok &= t->step[i].note[0] != kit_notes(eng)[lane_of(eng, t->step[i].note[0])];
        ck("  a phrase (12 BEAT) into the track: its lane notes hits, the kick on step 1", ok);
    }
}

static void kg_preset(void)
{
    track_t *t = kit_on(ENGI_VDRUM);
    const up_rec_t *r;
    grid_hit(t, 0, DV_KICK, 1);
    grid_hit(t, 4, DV_SNARE, 1);
    grid_acc(t, 4, DV_SNARE, 1);
    t->step[6].time = ST_NOTE;
    t->step[6].hit = 1u << DV_BELL;                     /* (a DRUM BELL: on HATOP) */
    up_store_cat(5, "KITGRID", CAT_DRUM);
    r = up_rec(5);
    ck("a VDRUM track's user preset stores its grid (lanes as shown, accents)", r->ver == UP_VER_GRID &&
       r->note[0] == 1u << DV_KICK && r->note[4] == 1u << DV_SNARE && r->flags[4] == 1u << DV_SNARE &&
       r->note[6] == 1u << DV_HATO);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    kg_model();
    kg_grid(argc > 1 ? argv[1] : 0);
    kg_rec();
    kg_mute();
    kg_tools();
    kg_load();
    kg_preset();
    printf(bad ? "kitgrid: %d FAILED\n" : "kitgrid: all passed\n", bad);
    return bad != 0;
}
