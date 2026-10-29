/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1: the pages' own code (params.c page_ops_t), one section a page: what the shared functions did for it in
 * a test of its GR_* each (tools/ui_pages.py lists the tests still left). Included last by the UI (ui_layer.c), so
 * every function a page needs is there; PAGES points at the ops declared in params.c. The code is the pages' as it
 * was in ui.c, ui_input.c, ui_draw.c and ui_graph.c; the panels' drawing (graph_song, graph_pats, graph_events) and
 * AUTOMATION's rows (ui_events.c) stay where they are. */

/* ------------------------------------------------------------- HOME > EQ --- */
#include "ui_eq.c"
static const page_ops_t EQ_OPS = {.flags = PG_LIST, .desc = eq_page_desc, .draw = graph_eq, .sig = eq_sig};

/* ------------------------------------------------------------- GLO > SONG --- */
/* the knobs: KNOB 1 SECTION, 2 TRACK, 3 PAT (A..D, -), 4 REPS */
static void song_knob(uint32_t slot, int32_t steps)
{
    chain_row_t *r;
    if (slot == 0u) {
        ui.song_row = (uint8_t)clamp((int32_t)ui.song_row + steps, 0,
            chain_config.count < CHAIN_ROWS ? chain_config.count : CHAIN_ROWS - 1u);
        return;
    }
    if (slot == 1u) {
        ui.song_trk = (uint8_t)clamp((int32_t)ui.song_trk + steps, 0, NTRK - 1);
        return;
    }
    if (ui.song_row >= chain_config.count && slot == 3u) {   /* + ADD, KNOB 4 (M-Vave-OP1): the song's END, */
        if (steps > 0)                           /* LOOP (right) or STOP (left) */
            chain_config.rsv[0] |= CHAIN_LOOP;
        else
            chain_config.rsv[0] &= (uint8_t)~CHAIN_LOOP;
        ui_message(chain_config.rsv[0] & CHAIN_LOOP ? "END: LOOP" : "END: STOP");
        chain_live(CHN_EDIT);
        return;
    }
    if (ui.song_row >= chain_config.count) {     /* + ADD: a turn adds the section (KNOB 3: only that) */
        song_add();
        chain_live(CHN_EDIT);
        if (slot == 2u) return;
    }
    r = &chain_config.row[ui.song_row];
    if (slot == 2u)
        r->slot[ui.song_trk % NTRK] = (uint8_t)clamp((int32_t)r->slot[ui.song_trk % NTRK] + steps, 0, CHAIN_SILENT);
    else
        r->repeat = (uint8_t)clamp((int32_t)r->repeat + steps, 1, 16);
    chain_live(CHN_EDIT);                         /* (playing: where its section ends) */
}
/* the cards: SECTION, TRACK, its PAT (A..D, -), REPS (the section's) */
static void song_cards(void)
{
    char val[12];
    uint32_t row = ui.song_row < CHAIN_ROWS ? ui.song_row : CHAIN_ROWS - 1u, k = ui.song_trk % NTRK, sl;
    int used = row < chain_config.count;
    fmt_int(val, (int32_t)row + 1);
    draw_column(0, "SEC", val, "", VAL(0u), -1, ICON_X_SONG);
    val[0] = 'T';
    val[1] = (char)('1' + k);
    val[2] = 0;
    draw_column(1, "TRACK", val, "", VAL(1u), -1, trk_icon(k, 1));
    sl = used ? chain_config.row[row].slot[k] : CHAIN_SILENT;
    if (used) { val[0] = sl < 4u ? (char)('A' + sl) : '-'; val[1] = 0; }
    else str_cpy(val, "--", sizeof val);
    draw_column(2, "PAT", val, "", used ? VAL(2u) : T_DIM, -1, ICON_X_PATTERN);
    if (!used) {                                 /* + ADD (M-Vave-OP1): the song's END, LOOP or STOP */
        draw_column(3, "END", chain_config.rsv[0] & CHAIN_LOOP ? "LOOP" : "STOP", "", VAL(3u), -1, ICON_LOOP);
        return;
    }
    fmt_int(val, chain_config.row[row].repeat);
    draw_column(3, "REPS", val, "", VAL(3u), -1, ICON_AUTO);
}
static void song_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_song();
}
static uint32_t song_sig(uint32_t h)
{
    uint32_t i;
    h ^= ui.song_row * 40503u + chain_config.count * 7919u + ui.song_trk * 3571u + chain_config.rsv[0] * 611953u;
    h = fnv(h, chain_config.row, sizeof chain_config.row);
    h ^= chain.running ? (chain.row + 1u) * 104729u + chain.remaining * 1299709u : 0u;
    for (i = 0; i < 4u; i++) h ^= (uint32_t)graph_project_used(i) << (24u + i);
    h += graph_pname_sig;
    {                                            /* the timeline: its view (following, ui_graph.c song_follow), the playhead */
        song_lay_t L;
        song_layout(&L);
        h = (h ^ (uint32_t)song_follow(&L)) * 16777619u;
        h = (h ^ (uint32_t)(song_playx(&L) + 1)) * 16777619u;
        h ^= (ui_prefs & PREF_LARGE) ? 0x2545F491u : 0u;
    }
    return h;
}
/* the action, KNOB 1's column: PLAY / STOP (also the PLAY button) */
static const char *song_act_name(uint32_t c)
{
    (void)c;
    return song.playing || chain_busy() ? "STOP" : "PLAY";
}
static int song_act_ready(uint32_t c)
{
    (void)c;
    return song.playing || chain_busy() || chain_config.count;
}
static void song_act_do(uint32_t c)
{
    (void)c;
    if (song.playing || chain_busy() || seq_counting()) transport_req = 2;
    else chain_play_ui();
}
static uint32_t song_icon(void) { return ICON_X_SONG; }   /* the disc */
/* REC: the keys type a chain (song_chain_key), the first key a new one; not the pattern's (a track recording: its) */
static int song_rec(uint32_t bit)
{
    if (song.rec & bit)
        return -1;                                /* (the track recording: its REC, the shared code's) */
    if (!ui.song_rec) record_begin(0, 0, 0, 1);
    ui.song_rec ^= 1u;                            /* (song_chain_key), the first key a new one; not the pattern's */
    ui.song_new = ui.song_rec;
    ui.force = 1;
    ui_message(ui.song_rec ? "NEW CHAIN: TAP A-D" : "CHAIN DONE");
    return 1;
}
static const page_ops_t SONG_OPS = {.flags = PG_LIST | PG_K1 | PG_CHAIN_KEYS, .acts = 1u, .act_col = 1u, .hot = 15u,
                                    .knob = song_knob, .cards = song_cards, .draw = song_panel, .sig = song_sig,
                                    .act_name = song_act_name, .act_ready = song_act_ready, .act_do = song_act_do,
                                    .icon = song_icon, .rec = song_rec, .play = chain_play_ui};

/* ---------------------------------------------------------- SAVE > PHRASES --- */
/* the pattern loader: KNOB 1 the pattern; LOAD, KNOB 2's column, the page's own (no knob picks it) */
static void pats_knob(uint32_t slot, int32_t steps)
{
    if (slot == 0u)
        ui.ppick = (uint8_t)clamp((int32_t)pat_pick() + steps, 0, (int32_t)pat_count() - 1);
}
/* the cards: PAT, then LOAD (a GO button) */
static void pats_cards(void)
{
    uint32_t n = pat_count(), k = pat_pick();
    char tag[4], nm[13];
    pat_label(k, tag, nm);
    draw_column(0, "PAT", tag, "", VAL(0u), (int32_t)k * 1000 / (int32_t)(n > 1u ? n - 1u : 1u), ICON_X_PATTERN);
    draw_act_column(1, "LOAD", T_THEME, ICON_AUTO);
    draw_column(2, "", "", "", T_THEME, -1, ICON_AUTO);
    draw_column(3, "", "", "", T_THEME, -1, ICON_AUTO);
}
static void pats_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_pats();
}
static const char *pats_act_name(uint32_t c)
{
    (void)c;
    return "LOAD";
}
static int pats_act_ready(uint32_t c)
{
    uint32_t s = song.sel;
    (void)c;
    return pat_last[s] != pat_pick() + 1u || steps_sig(TSEL) != pat_sig[s];
}
static void pats_act_do(uint32_t c)
{
    (void)c;
    if (chain_busy()) { ui_message("STOP TO EDIT"); return; }
    if (pat_needs_confirm(TSEL))                      /* the user's steps: the dialog */
        confirm_open(CF_LOAD_PAT, song.sel);
    else                                              /* empty, or a pattern loaded and untouched */
        pat_load_ui(TSEL, pat_pick());
}
static uint32_t pats_icon(void) { return ICON_X_PATTERN; }   /* the pattern loader's */
static const page_ops_t PATS_OPS = {.flags = PG_LIST | PG_K1 | PG_ACT_FIXED, .acts = 2u, .act_col = 2u, .hot = 1u,
                                    .knob = pats_knob, .cards = pats_cards, .draw = pats_panel, .act_name = pats_act_name,
                                    .act_ready = pats_act_ready, .act_do = pats_act_do, .icon = pats_icon};

/* -------------------------------------------------------- SEQ > AUTOMATION --- */
/* ui_events.c's rows; no knob is an action, OCT+ by the row (ev_oct) */
/* the cards: ROW STEP PARAM VALUE */
static void ev_cards(void)
{
    char val[12];
    const char *unit;
    const track_t *t = TSEL;
    uint16_t rw[EV_ROWS];
    uint32_t n, c = ev_cur(rw, &n), r = ev_row(n), k = c >> 8, step, id;
    int16_t v;
    char nm[12];
    const param_desc_t *d;
    if (k == EVK_TOP) {                            /* PLAY: the counts, PLAY ON / OFF (OCT+: CLEAR) */
        draw_column(0, "ROW", "ALL", "", VAL(0u), -1, ICON_AUTO);
        fmt_int(val, (int32_t)(motion_count(t) - motion_lock_count(t)));
        draw_column(1, "EVENT", val, "", T_MID, -1, ICON_NONE);
        fmt_int(val, (int32_t)motion_lock_count(t));   /* parameter locks (SEQ > STEP: a step held, a knob) */
        draw_column(2, "LOCK", val, "", T_MID, -1, ICON_NONE);
        draw_column(3, "PLAY", motion_enabled(t) ? "ON" : "OFF", "", VAL(3u), -1, motion_icon());
        return;
    }
    if (k == EVK_QNT) {                            /* QUANTIZE (1.2): the steps with a NUDGE, QNTZ ON / OFF */
        draw_column(0, "ROW", "ALL", "", VAL(0u), -1, ICON_AUTO);
        fmt_int(val, (int32_t)ev_nudged(t));
        draw_column(1, "NUDGED", val, "", T_MID, -1, ICON_NONE);
        draw_column(2, "", "", "", T_THEME, -1, ICON_NONE);
        draw_column(3, "QNTZ", t->p[P_SQNT] ? "ON" : "OFF", "", VAL(3u), -1, ICON_QUANTIZE);
        return;
    }
    if (k == EVK_ADD) {
        ev_fix();
        str_cpy(val, "ADD", sizeof val);
    } else {                                       /* "3/7": the rows between QUANTIZE and + ADD */
        fmt_int(val, (int32_t)r - 1);
        str_cpy(val + str_len(val), "/", 4);
        fmt_int(val + str_len(val), (int32_t)n - 3);
    }
    step = ev_step_of(c);
    id = k == EVK_REC ? MOTION_ID(&motion.event[c & 0xFFu]) : k == EVK_ADD ? ui.ev_id :
         k == EVK_CHANCE ? EV_CHANCE : k == EVK_RATCH ? EV_RATCH : k == EVK_VEL ? EV_VEL : EV_NUDGE;
    draw_column(0, "ROW", val, "", VAL(0u), -1, ICON_AUTO);
    fmt_int(val, (int32_t)step + 1);
    draw_column(1, "STEP", val, "", k != EVK_ADD || step < (uint32_t)t->p[P_SLEN] ? VAL(1u) : T_DIM, -1, ICON_AUTO);
    if (!ev_id_ok(t, id)) {
        draw_column(2, "PARAM", "--", "", T_DIM, -1, ICON_AUTO);
        draw_column(3, "VALUE", "--", "", T_DIM, -1, ICON_AUTO);
        return;
    }
    ev_name(t, id, nm);
    draw_column(2, "PARAM", nm, "", k == EVK_REC || k == EVK_ADD ? VAL(2u) : T_MID, -1, ICON_AUTO);
    if (id == EV_NUDGE) {                          /* a step's NUDGE (1.2): "+3 /16", DIM where it does not play */
        const step_t *cs = &t->step[step % NSTEP];
        int32_t sv = k == EVK_ADD ? ev_sadd(EVK_NUDGE) : step_nudge(cs);
        ev_nudge_fmt(val, sv);
        draw_column(3, "VALUE", val, "/16", k != EVK_ADD && ev_nudge_plays(t, cs) ? VAL(3u) : T_DIM, (sv + 8) * 66,
                    ICON_AUTO);
        return;
    }
    if (id == EV_VEL) {                            /* a step's VEL (1.5): 1..127, DIM where it does not play; */
        const step_t *cs = &t->step[step % NSTEP];  /* + ADD: the rows shown or not */
        int32_t sv = ev_sval(cs, EVK_VEL);
        if (k == EVK_ADD) {
            draw_column(3, "ROWS", ui.ev_vel ? "SHOWN" : "HIDDEN", "", T_DIM, -1, ICON_AUTO);
            return;
        }
        fmt_int(val, sv);
        draw_column(3, "VALUE", val, "", ev_vel_plays(cs) ? VAL(3u) : T_DIM, sv * 1000 / 127, ICON_LEVEL);
        return;
    }
    if (id == EV_CHANCE || id == EV_RATCH) {       /* a step's CHANCE (the die) / RATCH; + ADD: what it adds */
        const step_t *cs = &t->step[step % NSTEP];
        uint32_t sk = ev_skind(id), sv = (uint32_t)(k == EVK_ADD ? ev_sadd(sk) : ev_sval(cs, sk));
        int plays = sk == EVK_CHANCE || (cs->time == ST_NOTE && (cs->n || cs->hit));   /* (RATCH does nothing on a
                                                                                        * REST, a TIE, empty: DIM) */
        if (sk == EVK_CHANCE) {
            fmt_int(val, (int32_t)sv);
        } else {
            val[0] = 'x';                          /* x1 .. x4 */
            val[1] = (char)('0' + sv);
            val[2] = 0;
        }
        draw_column(3, "VALUE", val, sk == EVK_CHANCE ? "%" : "", k != EVK_ADD && plays ? VAL(3u) : T_DIM,
                    sk == EVK_CHANCE ? (int32_t)sv * 10 : ((int32_t)sv - 1) * 333, sk == EVK_CHANCE ? ICON_PROB : ICON_X_REPEAT);
        return;
    }
    v = k == EVK_REC ? motion.event[c & 0xFFu].value : motion_base_value(t, id);
    d = track_desc(t, id);
    param_format(d, v, val, &unit);
    draw_column(3, k == EVK_ADD ? "VALUE" : (motion.event[c & 0xFFu].param & MOTION_LOCK) ? "LOCK" : "AUTO", val, unit,
                k != EVK_ADD ? VAL(3u) : T_DIM, RATIO(d, enum_rank(d, v)), param_icon(d, v));
}
static void ev_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_events();
}
/* the rows, the one selected, + ADD's place, PLAY, QUANTIZE */
static uint32_t ev_page_sig(uint32_t h) { return (h ^ ev_sig()) * 16777619u; }
static const char *ev_page_act_name(uint32_t c)
{
    (void)c;
    return ev_act_name();
}
static int ev_page_act_ready(uint32_t c)
{
    (void)c;
    return ev_act_ready();
}
static void ev_page_act_do(uint32_t c)
{
    (void)c;
    ev_oct();
}
/* #93: "AUTOMATION 6/6" -> "AUTO 6/6" in the footer where the sound's name would be cut (MENU > LARGE) */
static void ev_title_short(char *ti, uint32_t n) { str_cpy(ti + 4, ti + 10, n - 4); }
/* EDIT deletes the row's record; its key in the footer: dim on PLAY, QUANTIZE and + ADD (nothing to delete) */
static uint32_t ev_edit_state(void)
{
    uint16_t rw[EV_ROWS];
    uint32_t k = ev_cur(rw, 0) >> 8;
    return k == EVK_TOP || k == EVK_QNT || k == EVK_ADD ? 1u : 2u;
}
static const page_ops_t EVENTS_OPS = {.flags = PG_LIST | PG_K1, .acts = 16u, .act_col = 5u, .knob = ev_knob,
                                      .enter = ev_enter, .cards = ev_cards, .draw = ev_panel, .sig = ev_page_sig,
                                      .act_name = ev_page_act_name, .act_ready = ev_page_act_ready,
                                      .act_do = ev_page_act_do, .icon = motion_icon, .title_short = ev_title_short,
                                      .edit = ev_delete, .edit_state = ev_edit_state, .edit_word = "DELETE"};

/* ---------------------------------------------------------- SAVE > PRESETS --- */
/* the browser: KNOB 1 one preset, 2 the next / previous engine, 3 FAV, 4 LIST (the PRESETS knob: preset_step, as on
 * every page without one of its own) */
static void browse_knob(uint32_t slot, int32_t steps)
{
    if (slot == 0u) {
        preset_step(steps);
    } else if (slot == 1u) {
        select_engine(eng_step(TSEL->eng_req, steps));
    } else if (slot == 2u) {
        preset_mark(steps > 0);
    } else if (slot == 3u) {                          /* LIST: ALL FAV BASS .. OTHER (ui.c list_mode), no wrap */
        uint32_t m = (uint32_t)clamp((int32_t)list_mode() + steps, 0, (int32_t)LIST_N - 1);
        if (m != list_mode()) {
            list_set(m);
            ui.force = 1;
            settings_later();                         /* (#197: not on every detent; ui.c) */
        }
    }
}
/* the cards: No. (of the list), ENG, FAV, LIST */
static void browse_cards(void)
{
    char val[12];
    uint32_t total, cur = preset_pos(&total);
    char u[8];
    if (cur < total) fmt_int(val, (int32_t)cur + 1);
    else str_cpy(val, "--", 8);
    str_cpy(u, "/", 8);
    fmt_int(u + 1, (int32_t)total);
    draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
    draw_column(1, "ENG", ENGINES[TSEL->eng_req]->name, "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
    draw_column(2, "FAV", preset_favorite() ? "ON" : "OFF", "", VAL(2u), -1, ICON_X_STAR);
    draw_column(3, "LIST", list_name(list_mode()), "", VAL(3u), -1, ICON_X_FOLDER);
}
static void browse_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_browse();
}
static const page_ops_t BROWSE_OPS = {.flags = PG_LIST | PG_NO_MOM, .knob = browse_knob, .cards = browse_cards,
                                      .draw = browse_panel};

/* ------------------------------------------------- SAVE > PROJECT, SAVE > USER --- */
/* the slot pages (PG_SLOTS): the keys pick a slot, OCT+ saves, OCT- loads, F#3 renames, G#3 erases (ui_input.c
 * slots_keys, slots_oct, ui_name.c name_rename); PROJECT's four projects, USER's two banks of sixteen user presets
 * (PG_USER_SLOTS: slots_user). PROJECT is SAVE's first (PG_ENTRY) and keeps F#5 NEW */
/* the cards: USER's BANK, the SLOT, where SAVE and LOAD are */
static void slots_cards(void)
{
    char val[12];
    uint32_t user = slots_user();
    if (user) {
        draw_column(0, "BANK", ui.uslot & 16u ? "2" : "1", "/2", VAL(0u), ui.uslot & 16u ? 1000 : 0, ICON_AUTO);
        up_slot_label(val, ui.uslot);
    } else {
        draw_column(0, "", "", "", T_THEME, -1, ICON_NONE);
        val[0] = (char)('A' + (((uint32_t)song.g[G_SLOT] - 1u) & 3u)); val[1] = 0;
    }
    draw_column(1, "SLOT", val, "", T_THEME, -1, ICON_AUTO);
    draw_column(2, "SAVE", "OCT+", "", T_THEME, -1, ICON_AUTO);
    draw_column(3, "LOAD", "OCT-", "", T_THEME, -1, ICON_AUTO);
}
static void slots_knob(uint32_t slot, int32_t steps)   /* PROJECT: the keys pick, no knob does anything */
{
    (void)slot;
    (void)steps;
}
static void slots_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_slots();
}
static uint32_t slots_sig(uint32_t h)
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        h ^= (uint32_t)graph_project_used(i) << (20u + i);
    h += graph_pname_sig;
    return h;
}
static const page_ops_t SLOTS_OPS = {.flags = PG_LIST | PG_SLOTS | PG_ENTRY, .knob = slots_knob, .cards = slots_cards,
                                     .draw = slots_panel, .sig = slots_sig};
/* USER: KNOB 1 picks one of the two banks; the keys its slots */
static void user_knob(uint32_t slot, int32_t steps)
{
    if (slot == 0u)
        ui.uslot = (uint8_t)((ui.uslot & 15u) + (steps > 0 ? 16u : 0u));
}
static void user_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_user();
}
static const page_ops_t USER_OPS = {.flags = PG_LIST | PG_SLOTS | PG_USER_SLOTS | PG_NO_MOM, .hot = 1u, .knob = user_knob,
                                    .cards = slots_cards, .draw = user_panel};

/* ------------------------------------------------------------------- GLO > KEY --- */
/* key.c's run of roots (a bar each): KNOB 1 the bar, 2 its ROOT, 3 LEN, 4 ON (key_knob); the keys play the roots and
 * REC arms their recording (PG_ROOT_KEYS) */
/* the cards: BAR, its ROOT (the chord it makes in the selected track's scale), LEN, ON */
static void key_cards(void)
{
    char val[12];
    uint32_t len = keyt.len >= 1u && keyt.len <= KEY_BARS ? keyt.len : 1u, r = keyt.root[key_cur % KEY_BARS];
    fmt_int(val, (int32_t)key_cur + 1);
    draw_column(0, "BAR", val, "", VAL(0u), len > 1u ? (int32_t)(key_cur * 1000u / (len - 1u)) : 0, ICON_TIME);
    key_chord_name(TSEL, r, val);
    draw_column(1, "ROOT", val, "", r < 12u ? VAL(1u) : T_DIM, r < 12u ? (int32_t)(r * 1000u / 11u) : -1, ICON_SCALE);
    fmt_int(val, (int32_t)len);
    draw_column(2, "LEN", val, len == 1u ? "BAR" : "BARS", VAL(2u), (int32_t)((len - 1u) * 1000u / (KEY_BARS - 1u)),
                ICON_LENGTH);
    draw_column(3, "KEY", keyt.on ? "ON" : "OFF", "", keyt.on ? VAL(3u) : T_DIM, -1, ICON_TRANSPOSE);
}
static void key_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_key();
}
/* the run, the cursor, the bar playing, the scale */
static uint32_t key_sig(uint32_t h)
{
    uint32_t i;
    for (i = 0; i < KEY_BARS; i++)
        h = (h ^ keyt.root[i]) * 16777619u;
    h = (h ^ (keyt.on | (uint32_t)keyt.len << 8 | (uint32_t)key_cur << 16 | (uint32_t)key_bar << 24)) * 16777619u;
    h = (h ^ ((uint32_t)song.playing | (uint32_t)TSEL->p[P_ROOT] << 8 | (uint32_t)TSEL->p[P_SCALE] << 16 |
              (uint32_t)key_arm << 24 | (uint32_t)(key_live & 31u) << 25)) * 16777619u;
    h = (h ^ (scn.cur + 1u)) * 16777619u;        /* (M-Vave-OP1: the scene in the title) */
    return h;
}
static void key_title(char *ti)                   /* the scene whose KEY it is ("KEY S3", scene.c) */
{
    str_cpy(ti + str_len(ti), " S", 4);
    fmt_int(ti + str_len(ti), (int32_t)scn.cur + 1);
}
static uint32_t key_icon(void) { return ICON_SCALE; }   /* the chord progression */
/* REC: its recording, not the pattern's; armed only: PLAY starts it (the count-in first, if set) */
static int key_rec(uint32_t bit)
{
    (void)bit;
    if (ui.menu || ui.confirm || ui.layer)
        return -1;
    if (!key_arm) record_begin(0, 0, 1, 0);
    key_arm ^= 1u;                                /* armed only: PLAY starts it (the count-in first, if set) */
    ui.force = 1;
    ui_message(!key_arm ? "KEY REC OFF" : song.playing || seq_counting() ? "KEY REC: PLAY THE ROOTS"
                                                                         : "KEY REC: PRESS PLAY");
    return 1;
}
static const page_ops_t KEY_OPS = {.flags = PG_LIST | PG_NO_MOM | PG_ROOT_KEYS, .hot = 15u, .knob = key_knob,
                                   .cards = key_cards, .draw = key_panel, .sig = key_sig, .icon = key_icon,
                                   .title_add = key_title, .rec = key_rec};

/* ------------------------------------------------------------- FX > FX TRACKS --- */
/* fxtrack.c's two lanes of punch-in effects: KNOB 1 the lane, 2 STEP, 3 its EFFECT, 4 LEN; REC arms the lane
 * (PG_LANE_REC), EDIT clears it */
static void fxt_knob(uint32_t slot, int32_t steps)
{
    fxt_lane_t *ln = &fxt_lane[ui.fxt_sel % FXT_N];
    uint32_t len = FXT_LEN(ln);
    if (slot == 0u) {
        ui.fxt_sel = (uint8_t)clamp((int32_t)ui.fxt_sel + steps, 0, FXT_N - 1);
        if (ui.fxt_cur >= FXT_LEN(fxt_src(ui.fxt_sel)))
            ui.fxt_cur = (uint8_t)(FXT_LEN(fxt_src(ui.fxt_sel)) - 1u);
    } else if (slot == 1u) {
        ui.fxt_cur = (uint8_t)clamp((int32_t)ui.fxt_cur + steps, 0, (int32_t)FXT_LEN(fxt_src(ui.fxt_sel)) - 1);
    } else if (chain_busy()) {                    /* (a song plays its slots' lanes: they are its patterns') */
        ui_message("STOP TO EDIT");
    } else if (slot == 2u) {                      /* (the list: OFF, TIE, the effects, the mutes; not round) */
        static const char *const MUTE_NAME[NTRK] = {"MUTE T1", "MUTE T2", "MUTE T3", "MUTE T4"};
        uint8_t *st = &ln->step[ui.fxt_cur % NSTEP];
        uint32_t e;
        *st = fxt_of_choice((uint32_t)clamp((int32_t)fxt_choice(*st) + steps, 0, FXT_CHOICES - 1u));
        e = fxt_eff(*st);
        ui_message(*st == FXT_TIE ? "TIE: THE EFFECT GOES ON" : e >= PF_N ? "OFF" : e >= PF_M1 ? MUTE_NAME[e - PF_M1]
                                                                                             : PF_NAME[e]);
    } else {
        ln->len = (uint8_t)clamp((int32_t)len + steps, 1, NSTEP);
        if (ui.fxt_cur >= ln->len)
            ui.fxt_cur = (uint8_t)(ln->len - 1u);
    }
}
/* the cards: the lane, STEP, its EFFECT, LEN */
static void fxt_cards(void)
{
    char val[12];
    uint32_t l = ui.fxt_sel % FXT_N, len = FXT_LEN(fxt_src(l)), c = fxt_src(l)->step[ui.fxt_cur % NSTEP];
    uint32_t e = fxt_eff(c);
    const char *unit = "";
    draw_column(0, "LANE", l ? "FX2" : "FX1", "", (fxt.arm >> l) & 1u ? T_REC : VAL(0u), -1, ICON_X_REPEAT);
    fmt_int(val, (int32_t)ui.fxt_cur + 1);
    draw_column(1, "STEP", val, "", VAL(1u), (int32_t)(ui.fxt_cur * 1000u / (len > 1u ? len - 1u : 1u)), ICON_STEPS);
    if (c == FXT_TIE) {
        str_cpy(val, "TIE", sizeof val);
    } else if (e >= PF_N) {
        str_cpy(val, "OFF", sizeof val);
    } else if (e >= PF_M1) {
        str_cpy(val, "T1", sizeof val);
        val[1] = (char)('1' + e - PF_M1);
        unit = "MUTE";
    } else {
        str_cpy(val, PF_CELL[e], sizeof val);
        unit = e <= PF_R32 ? "RPT" : e == PF_RTHR || e == PF_ETHR ? "THROW" : "";
    }
    draw_column(2, "EFFECT", val, unit, e < PF_N || c == FXT_TIE ? VAL(2u) : T_DIM, -1, ICON_SWEEP);
    fmt_int(val, (int32_t)len);
    draw_column(3, "LEN", val, "", VAL(3u), (int32_t)(len * 1000u / NSTEP), ICON_LENGTH);
}
static void fxt_panel(const track_t *t, uint16_t c)
{
    (void)t;
    (void)c;
    cv_oy = 0;
    graph_fxt();
}
/* the lanes, the cursor, the play heads */
static uint32_t fxt_sig(uint32_t h)
{
    uint32_t i;
    uint32_t l;
    for (l = 0; l < FXT_N; l++) {
        const fxt_lane_t *ln = fxt_src(l);       /* (in a song: the section's) */
        for (i = 0; i < NSTEP; i++)
            h = (h ^ ln->step[i]) * 16777619u;
        h = (h ^ (ln->len | (uint32_t)fxt.idx[l] << 8 | (uint32_t)fxt.cur[l] << 16)) * 16777619u;
    }
    h = (h ^ (fxt.arm | (uint32_t)ui.fxt_sel << 4 | (uint32_t)ui.fxt_cur << 8 | (uint32_t)song.playing << 16 |
              (uint32_t)fxt_follow() << 17)) * 16777619u;
    return h;
}
static uint32_t fxt_icon(void) { return ICON_X_REPEAT; }   /* a punch-in's */
/* REC: the selected lane's ARM, not the pattern's (armed only, PLAY starts it) */
static int fxt_rec(uint32_t bit)
{
    (void)bit;
    uint8_t lb = (uint8_t)(1u << (ui.fxt_sel % FXT_N));
    if (chain_busy() && !(fxt.arm & lb)) {
        ui_message("STOP TO RECORD");
        return 0;
    }
    if (!(fxt.arm & lb)) record_begin(0, lb, 0, 0);
    fxt.arm ^= lb;                                /* (M-Vave-OP1: armed only, PLAY starts it) */
    ui.force = 1;
    ui_message(fxt.arm & lb ? (ui.fxt_sel ? "FX2 ARMED: HOLD [FX]" : "FX1 ARMED: HOLD [FX]")
                            : (ui.fxt_sel ? "FX2 DISARMED" : "FX1 DISARMED"));
    return 1;
}
/* EDIT: the selected lane cleared, after the dialog */
static void fxt_edit(void)
{
    if (chain_busy()) ui_message("STOP TO EDIT");
    else if (!fxt_used(ui.fxt_sel)) ui_message("NOTHING TO CLEAR");
    else confirm_open(CF_FXT_LANE, ui.fxt_sel);
}
static const page_ops_t FXT_OPS = {.flags = PG_LIST | PG_NO_MOM | PG_LANE_REC, .hot = 15u, .knob = fxt_knob,
                                   .cards = fxt_cards, .draw = fxt_panel, .sig = fxt_sig, .icon = fxt_icon, .rec = fxt_rec,
                                   .edit = fxt_edit};
