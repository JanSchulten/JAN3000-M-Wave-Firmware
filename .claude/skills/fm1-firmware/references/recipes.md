# Recipes

Each recipe lists every place that has to change. Grep for the closest existing example (`GR_SONG`, `GR_TOOLS`,
`CF_CLEAR_SONG`, `key_`) and follow its pattern.

## A new page

Keep the page's own code together in a `page_ops_t` (params.c) instead of new `if (pg->graph == GR_…)` tests in
shared functions (`tools/ui_pages.py` lists the ones still there). Declare `MYPAGE_OPS` beside `EQ_OPS` in params.c,
point the `PAGES[]` row at it (`&MYPAGE_OPS`) and define it in its section of `ui_pages.c` (included last by the UI,
so every UI function is there; a bigger page in a file of its own included from there, as `ui_eq.c`). The members,
each optional (0: the shared default):

- `flags`: `PG_LIST` (MENU > LARGE's list layout), `PG_K1` (the PRESETS knob turns KNOB 1), `PG_ACT_FIXED` (an action
  of the page's own: no knob picks one, OCT- goes back), `PG_NO_MOM` (no MOMENTARY on its knobs), `PG_CHAIN_KEYS`
  (the keys type a song chain: SONG), `PG_SLOTS` / `PG_USER_SLOTS` (a slot page: the keys pick, OCT+ saves, OCT- loads;
  user presets or projects: PROJECT, USER), `PG_ENTRY` (the family's button opens it: PROJECT), `PG_ROOT_KEYS` (the
  keys play a key run's roots: KEY), `PG_LANE_REC` (REC arms the FX lane: FX TRACKS).
- `acts`, `act_col`, `act_name`, `act_ready`, `act_do`: an action page (SONG, PHRASES, AUTOMATION are the models).
- `hot`: the columns a turned knob lights; `desc`: KNOB 1..4's values when the page keeps its own (EQ); `knob`: the
  knobs handled whole (SONG); `enter`: the page opened.
- `cards` (draw_columns), `draw` (the panel), `sig` (everything the panel draws, for lazy drawing).
- The footer and the buttons: `icon` (before the title), `title_short` (the title cut where the sound's name would be),
  `title_add` (after the page's name: KEY S3), `edit` / `edit_state` / `edit_word` (EDIT and its key in the footer:
  AUTOMATION's DELETE), `rec` (REC: the page's own recording, -1 when it is the track's: SONG, KEY, FX TRACKS), `play`
  (PLAY while stopped: SONG plays its chain).

Then `sh tools/ui_compare.sh` on any move of existing code: every screen byte for byte as on `origin/main`. A
build without the drawing and input files (the storage tests) sees the ops as zero (tentative definitions).

The steps below cover what has no hook yet:

1. `params.c`: add `GR_MYPAGE` to the `GR_*` enum, before `GR_N`; add `{"TITLE", FAM_x, SC_x, GR_MYPAGE, {ids}}` to `PAGES[]`
   at the place it should take in its family's cycle (the first page of a family opens first). Page indices are
   not stored anywhere, so inserting is safe.
2. `ui.c`: `large_kind()` only where `PG_LIST` does not fit; for an action page without ops `act_cols()`,
   `act_name()`, `act_ready()`; a predicate such as `fxt_page()` if other code must know.
3. `ui_input.c`: `edit_param()` for knobs with `0xFF` ids; exclude the page from MOMENTARY in `mom_turn()`; mark the
   turned column hot (the `hot_col` condition in `ui_input()`); `act_do()` for actions; REC / LED behaviour in
   `rec_tap()` and `ui_leds()` if the page changes them.
4. `ui_draw.c`: the four cards in `draw_columns()` (`draw_column(c, label, value, unit, colour, ratio, icon)`,
   `draw_act_column()` for actions).
5. The panel and its state: the ops' `draw` and `sig` (above), not a `case` in `draw_graph()` and a test in
   `graph_signature()`.
6. `icons.c` `page_icon()`; `ui_help.c` `HELP_PAGE[]` (the hint must fit its row: the UI test checks it).
7. Tests: drive it through `tests/ui_test.c` helpers (see testing.md) and save screens with `screen_save`.
8. Expect existing UI tests to assert the old page layout (for example "GLO opens SONG"). Update those assertions
   to the new design on purpose, never by loosening them.

## A new ISR feature

1. Put it in its own `.c`, included from the file whose ISR path uses it (seq.c for anything note-related).
2. State in a `static struct` (RAM) for small things, big arrays in `.pool`. Make all-zero the valid start state.
3. Per-block work O(1); loops over buffers only on rare events, inside `__attribute__((noinline))` functions.
4. UI → ISR through `volatile` request fields handled at the block start; ISR → UI by fields the UI reads.
5. Hook transport start / stop / restart (`seq_start`, `seq_stop`, `transport_req == 3` in `events_block`) and the
   note inputs (`input_on`, `input_off`, the arp's `arp_fire`, `arp_chord`, `arp_off`, `arp_chord_off`,
   `arp_silence`) as needed. Notes from playback must not be fed back into recording.
6. A note-on recorded or played must always get its note-off (stop, end, jump, direction change, a full buffer):
   reserve room for the offs before accepting an on.
7. Check `./build.sh` (RAM, pool) and `python3 tests/target_budget.py` (ISR cost).

## A new engine

1. `eng_<name>.c` with an `engine_t` (name, page titles, eight `edit` descriptors, presets, `knob[4]` for HOME,
   `note_on`, `render`, optional hooks, `mem` for per-part memory); include it from `engines.c`.
2. Append it to `ENGINES[]` (stored indices: append only) and place it in `ENGINE_ORDER` for display.
3. Add its render function to `FUNCS` in `tests/target_budget.py` and a line in `tests/target_budget.txt`
   (`BUDGET_UPDATE=1` writes it).
4. Golden renders: `tests/regress.c` covers every engine and preset; `GOLDEN_UPDATE=1` rewrites `tests/golden.txt`
   after an intended sound change.

## A confirm dialog

`CF_NEW` appended to the enum in ui.c → its text in `confirm_text()` (ui_draw.c; two lines, ≤ 24 characters) →
`confirm_open(CF_NEW, k)` where the action happens → its execution in the `ui.confirm` block of `ui_input()`
(OCT+ does it, OCT− cancels).

## An icon

Find a Fukiai glyph name, add `name: glyph` to the right map in `tools/gen_aa_icons.py` and to the size lists
that need it (`SMALL_EXTRA` 12 px, `BIG_DEFAULT` 16 px); legacy names from `assets/icons.json` are drawn unless
listed in `NOT_DRAWN`. The enum `ICON_<NAME>` appears in `build/gen/ui_icons.h` after `./build.sh`.

## Persistence

The flash map is full (hardware.md). Felucca's project fills its sector, so M-Vave-OP1 keeps what a project needs
beyond it in `opstore.c`'s extras (`proj_xtra_t`): add a field at the end (its `size` tells a reader what is there),
fill it in `xtra_capture`, apply it in `xtra_lanes` / `xtra_loaded`, hash it into `autosave_sig`. The extras carry the
CRC of their project (`xtra_id`), so a project written another way never picks up stale extras. Writes go through
`storage.c` (A/B copies, CRC, a sequence number) and happen only while the transport is stopped and nothing sounds
(`autosave_quiet`): an erase silences the audio ~40 ms. Something bigger than a sector (the scenes, the dropped tape) writes its own
commit record last over several sectors (as opstore.c's scene sets do) and keeps the ISR off what it writes meanwhile.
Test against a NOR image in the style of `tests/opstore_test.c` (round trip, power-on, torn write, damaged copy).

## Changing the panel graphics

Keep sizes the alignment lint can centre: even sizes for track cushions (`cv_trk(x, y, 12 or 16, …)`), text
centred with the `GFX_HOOK_ALIGN` helpers. Use theme colours (`T_*`), never literals, so all ten palettes work.
