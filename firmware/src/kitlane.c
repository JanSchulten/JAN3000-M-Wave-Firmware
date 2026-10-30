/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: the kits' grid, DRUM's for VDRUM too (included by engines.c after the engines). SEQ > STEP on a
 * kit track is the grid (ui.c), live recording writes hits (seq.c rec_note), the EDIT layer mutes lanes (ui_layer.c
 * EQA), SEQ TOOLS has BEAT and the lane tools (ui_tools.c). A step's lane hits (step_t.hit / acc, bit l: lane l) play
 * each lane's note (kit_notes) on any engine; a kit has a lane per drum:
 *   DRUM   8  KICK SNARE CLAP HATCL HATOP TOM RIM BELL: DRUM_LANE_NOTE, 36 38 39 42 46 45 37 56
 *   VDRUM  6  its parts KICK SNARE CLAP HATCL HATOP TOM (eng_vdrum.c VD_PART_OF) on the same first six notes; black keys
 *             7 and 8 do nothing. Outside the grid the black keys are the lanes too (seq.c kit_key: the drum keys). Hits stored in lanes 7 and 8 (a DRUM beat's RIM 37 and BELL 56) play as always, on
 *             the CLAP and HATOP parts, and show there (kit_fold); an edit of that lane makes them its own (ui.c grid_own)
 * (DBOX, retired, had a grid of eight lanes too.) A hit in lane l stays lane l on another engine, so a beat moves between
 * DRUM and VDRUM (a synth plays the lanes' notes as pitches). A step's notes show on the lane of the drum they strike
 * (lane_of: DRUM's GM map, a low tom 41 on TOM; VDRUM's part, a crash 49 on HATOP). A note that is its lane's
 * own note becomes that lane's hit (step_to_grid: the same note at the same velocity): a pattern loaded into the
 * track (ui.c load_pat16), the grid's edits (grid_own), and on VDRUM every load of its steps (project.c
 * project_restore_runtime, scene.c scene_queue: steps written as notes before this grid), there only where the step
 * plays exactly as it did (step_to_grid_kept: the order of its strikes kept) */
/* the lanes of engine e's grid (DRUM 8, VDRUM 6); 0: no kit (SEQ > STEP is the piano roll) */
static uint32_t kit_lanes(uint32_t e) { return e == ENGI_DRUM ? NLANE : e == ENGI_VDRUM ? VD_NP : 0u; }

/* the notes engine e's lanes play (every engine DRUM's). The audio ISR's seq_step */
static inline const uint8_t *kit_notes(uint32_t e) { (void)e; return DRUM_LANE_NOTE; }

/* the lane a note shows on, on engine e (0 .. its lanes - 1; any other engine: DRUM's GM map) */
static uint32_t lane_of(uint32_t e, uint32_t note)
{
    int32_t st;
    if (e == ENGI_VDRUM)
        return vd_part_note(note, &st);
    return drum_lane(note);
}

/* hit bits b as the lanes they sound on: VDRUM's lanes 7 and 8 (RIM, BELL) on the parts their notes strike */
static uint32_t kit_fold(uint32_t e, uint32_t b)
{
    uint32_t n = kit_lanes(e), m, i;
    if (!n || n >= NLANE)
        return b;
    m = b & ((1u << n) - 1u);
    for (i = n; i < NLANE; i++)
        if ((b >> i) & 1u)
            m |= 1u << lane_of(e, DRUM_LANE_NOTE[i]);
    return m;
}

/* the lanes step s strikes on engine e: its hits, and its notes on their lanes (a NOTE step only) */
static uint32_t step_lanes(uint32_t e, const step_t *s)
{
    uint32_t m = 0, i;
    if (s->time != ST_NOTE)
        return 0;
    for (i = 0; i < s->n && i < 4u; i++)
        m |= 1u << lane_of(e, s->note[i]);
    return m | kit_fold(e, s->hit);
}

/* .. and which of them are accented */
static uint32_t step_accents(uint32_t e, const step_t *s)
{
    return s->flags & SF_ACCENT ? step_lanes(e, s) : kit_fold(e, s->acc) & step_lanes(e, s);
}

/* a step's notes that are a lane's note on engine e become that lane's hits (the same note, the same velocity:
 * nothing sounds different). Other notes (a low tom 41, a crash 49) stay notes, shown on their lane. A step accent
 * of a step left with hits only becomes the hits' accents */
static void step_to_grid(uint32_t e, step_t *s)
{
    const uint8_t *ln = kit_notes(e);
    uint32_t i, k = 0;
    if (s->time != ST_NOTE)
        return;
    for (i = 0; i < s->n && i < 4u; i++) {
        uint32_t l = lane_of(e, s->note[i]);
        if (s->note[i] == ln[l])
            s->hit |= (uint8_t)(1u << l);
        else
            s->note[k++] = s->note[i];
    }
    for (i = k; i < 4u; i++)
        s->note[i] = 0;
    s->n = (uint8_t)k;
    if (!k && (s->flags & SF_ACCENT)) {
        s->acc |= s->hit;
        s->flags &= (uint8_t)~SF_ACCENT;
    }
}

/* step s's lane notes as hits only where they play as they did: seq.c seq_step plays a step's notes, then its hits in
 * lane order, so a step whose notes were in another order ([46 42]: the open hat, then the closed one that chokes it;
 * [45 41]: a part's last pitch) keeps them as notes, shown on their lanes all the same */
static void step_to_grid_kept(uint32_t e, step_t *s)
{
    const uint8_t *ln = kit_notes(e);
    uint8_t a[4 + NLANE], b[4 + NLANE];
    uint32_t na = 0, nb = 0, i;
    step_t c = *s;
    if (s->time != ST_NOTE || !s->n)
        return;
    step_to_grid(e, &c);
    for (i = 0; i < s->n && i < 4u; i++)
        a[na++] = s->note[i];
    for (i = 0; i < NLANE; i++)
        if ((s->hit >> i) & 1u)
            a[na++] = ln[i];
    for (i = 0; i < c.n && i < 4u; i++)
        b[nb++] = c.note[i];
    for (i = 0; i < NLANE; i++)
        if ((c.hit >> i) & 1u)
            b[nb++] = ln[i];
    if (na == nb && !memcmp(a, b, na))
        *s = c;
}

/* a VDRUM track's steps loaded (a project, a scene): their lane notes as hits where they play the same
 * (step_to_grid_kept; DRUM converts only the projects of before its grid: project.c proj_grid) */
static void kit_steps_load(uint32_t e, step_t *s)
{
    uint32_t i;
    if (e != ENGI_VDRUM)
        return;
    for (i = 0; i < NSTEP; i++)
        step_to_grid_kept(e, &s[i]);
}
