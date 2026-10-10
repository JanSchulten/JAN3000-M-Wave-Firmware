# Recipes

Each recipe lists every place that has to change. Grep for the closest existing example (`GR_SONG`, `GR_TOOLS`,
`CF_CLEAR_SONG`, `tape_`) and follow its pattern.

## A new page

1. `params.c`: append `GR_MYPAGE` to the `GR_*` enum; add `{"TITLE", FAM_x, SC_x, GR_MYPAGE, {ids}}` to `PAGES[]`
   at the place it should take in its family's cycle (the first page of a family opens first). Page indices are
   not stored anywhere, so inserting is safe.
2. `ui.c`: `large_kind()` (list-like pages keep the normal layout: `LK_LABEL`); for an action page `act_cols()`,
   `act_name()`, `act_ready()`; a predicate such as `tape_page()` if other code must know.
3. `ui_input.c`: `edit_param()` for knobs with `0xFF` ids; exclude the page from MOMENTARY in `mom_turn()`; mark the
   turned column hot (the `hot_col` condition in `ui_input()`); `act_do()` for actions; REC / LED behaviour in
   `rec_tap()` and `ui_leds()` if the page changes them.
4. `ui_draw.c`: the four cards in `draw_columns()` (`draw_column(c, label, value, unit, colour, ratio, icon)`,
   `draw_act_column()` for actions).
5. `ui_graph.c`: the panel drawing function and its `case` in `draw_graph()`; hash its state in
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

The flash map is full (hardware.md). Options: extend an existing record within its 4 KB sector (project, settings,
autosave), or repurpose a region with an explicit migration. Writes go through `storage.c` (A/B copies, CRC, a
sequence number) and happen only while the transport is stopped. Add a round-trip test in the style of
`tests/project_test.c` and a torn-write test in the style of `tests/storage_test.c`.

## Changing the panel graphics

Keep sizes the alignment lint can centre: even sizes for track cushions (`cv_trk(x, y, 12 or 16, …)`), text
centred with the `GFX_HOOK_ALIGN` helpers. Use theme colours (`T_*`), never literals, so all ten palettes work.
