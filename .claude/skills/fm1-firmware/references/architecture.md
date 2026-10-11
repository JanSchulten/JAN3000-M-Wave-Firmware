# How the firmware is put together

## One compilation unit

`firmware/src/felucca.c` `#include`s every `.c` in dependency order; each file may use what the files above it
define. The HAL (`firmware/hal/*.h`) is header-only. Order (abridged):

```
HAL headers → libc.c lcd.c gfx.c
core.h → engines.c (dsp.c, eng_*.c) params.c mod.c voice.c slicer.c fx.c
usb.c midi_uart.c song_chain.c seq.c (chord.c, tape.c, motion.c) audio.c
panel.c ui.c menu_items.c icons.c ui_graph.c ui_draw.c ui_menu.c ui_input.c ui_layer.c
storage_hw.c storage.c upreset.c project.c ota.c ota_hw.c editor.c console.c master.c main.c
```

The host tests (`tests/hostsim.c`, `tests/ui_test.c`) and the browser build (`web/emu/felucca_web.c`) include
the same sources in the same order with stubbed HAL. Consequence: a new subsystem is easiest to add by including
it from the file that uses it (as `seq.c` includes `tape.c`), so every harness picks it up without edits.

Everything is `static`. Names are short and prefixed by subsystem (`tape_`, `seq_`, `ui_`, `cv_` for the canvas).

## Two worlds: the audio ISR and the main loop

- **Audio ISR** (`audio.c`): per half buffer, blocks of 32 samples → `mix_block` (fx.c) → `events_block` (seq.c):
  transport requests, the keyboard, MIDI in, `seq_tick` per track, `tape_tick`, the arp, the click; then the
  voices render. Everything that makes sound happens here, ending in `trk_note_on` / `trk_note_off`.
- **Main loop** (`main.c`): UI frames (`ui_input`, `ui_draw`), storage writes, USB, the editor protocol.
- **Crossing over:** the UI writes `volatile` request fields (`transport_req` 1 start / 2 stop / 3 restart,
  `panic_req`, `aud_req`, the tape's `req_seek` / `req_clear`); the ISR consumes them at a block start. Plain
  settings the ISR only reads (a parameter, the tape's `speed`) may be written directly if a torn read is
  harmless. A UI that reads ISR data structures (the tape's gap buffer) must tolerate a frame of inconsistency.

## Tracks, parameters, the song

- `core.h`: `NTRK` = 4 tracks, each a synth part with its own engine, preset, `p[P_COUNT]` parameters, 8 voices
  (shared budget `NVOICE`), a 64-step pattern (`step_t`), arp and LFO state.
- `P_*` (per track) and `G_*` (global) are stored in projects and user presets and spoken by the editor protocol:
  append-only, see SKILL.md.
- `song` holds globals, transport (`playing`), the selected track (`song.sel`, `TSEL`), recording arm bits.
- Engines (`engine_t` in core.h): `note_on`, `render`, optional `amp`, `desc`, `block`, `keys`, `done`, eight
  edit parameters `P_E0..P_E7`, presets, a share of per-part engine memory (`mem`).

## The UI

- **Pages** (`params.c` `PAGES[]`): `{title, family, scope, graph, id[4]}`. The family is the button that opens it
  (`FAM_ENV` … `FAM_TRK`; tapping the button again steps through the family in `PAGES` order). `id[4]` are the
  parameters KNOB 1..4 edit; `0xFF` means the page handles the knob itself (`edit_param` in ui_input.c).
  `GR_*` selects the panel graphic; append new ones at the end of the enum.
- **Screen layout:** header 0..24, four cards 28..72, the panel 76..198 (`Y_GRAPH`, `H_GRAPH` 122), footer 202..240.
  MENU > LARGE changes the cards and panel; `large_kind()` decides per graph.
- **Lazy rendering:** each element remembers a signature; `graph_signature()` must include everything the panel
  draws, `draw_columns()` draws the cards, `draw_foot()` the footer.
- **Action pages:** `act_cols()` marks columns that are actions; turning such a knob picks it (`ui.act`), OCT+
  runs `act_do()`, OCT− cancels or goes HOME. `act_name()`, `act_ready()` and `draw_act_column()` show it.
- **Confirm dialogs:** `CF_*` (ui.c), text in `confirm_text()` (ui_draw.c), execution in the `ui.confirm` branch of
  `ui_input()`.
- **Held layers:** FX, GLO, SCL, EDIT, REC, SEQ held open a layer over the keys and knobs (ui_layer.c); a quick
  double tap locks it open. The FX layer's white keys are performance effects on the master (perform.c).
- **LEDs:** `ui_leds()` in ui_input.c. **Help hints:** `HELP_PAGE[]` in ui_help.c, keyed by page title.
- **Icons:** Fukiai font cells, names in `assets/icons.json`, rasterised by `tools/gen_aa_icons.py` (only the names
  in its lists are drawn; `NOT_DRAWN` skips some).
- **Drawing API (gfx.c):** a 240-wide canvas (`cv_begin`, `cv_rect`, `cv_rrect` with r ≤ 8, `cv_line`,
  `cv_line_t`, `cv_text*`, `cv_icon_in`, `cv_trk` for the track cushions), theme colours `T_*` from the palette.

## Worked example: TAPE (M1)

`firmware/src/tape.c` shows the patterns above in one place: a gap buffer in the pool (all-zero = empty), O(1)
recording and playback in the ISR, `noinline` slow paths, requests from the UI, and the hooks in seq.c
(`input_on` / `input_off`, the arp, `seq_start` / `seq_stop`, `events_block`). The UI side is spread over
params.c, ui.c, ui_input.c, ui_draw.c, ui_graph.c, ui_help.c and icons.c; recipes.md lists each touch point.

## M2b: FX TRACKS, live song chains, the stores

- `fxtrack.c` (included by seq.c after tape.c): two effect lanes. The ISR side holds effects through perform.c's
  `perf_fxt` (as keys would) and `perf_fxt_now` (a REPEAT starts on its step). Recording hooks `perf_press` via
  `fxt_press`. Its clock mirrors `click_tick`; in a song `fxt_src()` reads the section's slot's lanes and
  `fxt_section()` restarts them with `chain_apply`.
- song_chain.c: the UI never writes `chain.config`. It edits `chain_config`, then `chain_live(kind)` copies it to
  `chain_next` with IRQs off and sets `chain.req`; `chain_next_row()` (noinline, once a section) takes it.
- opstore.c (included by project.c after `proj_wire`): extras and the tape store, hooks in `project_save_as`,
  `project_load`, `project_rename`, `persist_boot`, the autosave, `chain_prepare`, and main.c
  (`tape_store_boot`, `tape_store_poll`). web/emu/felucca_web.c calls the same two.

## E1: FX1 CLOUD

- `fx1.c` (included by fx.c after perform.c): the delay bus's TYPE (`fx1s`, defined at the top of fx.c so perform.c's
  `perf_avail` can read it). `fx_buses` keeps Felucca's chorus + delay loop for the DELAY and branches once to
  `fx1_block` (noinline) otherwise: the chorus copy, the CLOUD (`cloud_render`) or a TYPE change (fade, clear 2048
  samples a block, start empty). The cloud's stereo goes into HALL's side buffer after the reverb (`cloud_side`).
- A grain's start is bounded so it never reads at the write head or what was not written (`cl_spawn`); the cloud has
  its own random numbers (`cl_rng`), so drums' noise renders the same with or without it.
- New settings that do not fit `song.g` (fixed by the formats) go into the project extras (`opstore.c proj_xtra_t`,
  append-only by `size`, older ones read as far as they go).
