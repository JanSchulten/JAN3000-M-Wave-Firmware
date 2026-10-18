# How the firmware is put together

## One compilation unit

`firmware/src/felucca.c` `#include`s every `.c` in dependency order; each file may use what the files above it
define. The HAL (`firmware/hal/*.h`) is header-only. Order (abridged):

```
HAL headers → libc.c lcd.c gfx.c
core.h → engines.c (dsp.c, eng_*.c) params.c mod.c voice.c slicer.c fx.c
usb.c midi_uart.c song_chain.c seq.c (chord.c, key.c, fxtrack.c, motion.c) audio.c
panel.c ui.c menu_items.c icons.c ui_graph.c ui_draw.c ui_menu.c ui_input.c ui_layer.c
storage_hw.c storage.c upreset.c project.c ota.c ota_hw.c editor.c console.c master.c main.c
```

The host tests (`tests/hostsim.c`, `tests/ui_test.c`) and the browser build (`web/emu/felucca_web.c`) include
the same sources in the same order with stubbed HAL. Consequence: a new subsystem is easiest to add by including
it from the file that uses it (as `seq.c` includes `key.c`), so every harness picks it up without edits.

Everything is `static`. Names are short and prefixed by subsystem (`key_`, `seq_`, `ui_`, `cv_` for the canvas).

## Two worlds: the audio ISR and the main loop

- **Audio ISR** (`audio.c`): per half buffer, blocks of 32 samples → `mix_block` (fx.c) → `events_block` (seq.c):
  transport requests, the keyboard, MIDI in, `key_tick`, `seq_tick` per track, the arp, the click; then the
  voices render. Everything that makes sound happens here, ending in `trk_note_on` / `trk_note_off`.
- **Main loop** (`main.c`): UI frames (`ui_input`, `ui_draw`), storage writes, USB, the editor protocol.
- **Crossing over:** the UI writes `volatile` request fields (`transport_req` 1 start / 2 stop / 3 restart,
  `panic_req`, `aud_req`, the scenes' `scn.req`); the ISR consumes them at a block start. Plain
  settings the ISR only reads (a parameter, the KEY's roots) may be written directly if a torn read is
  harmless. A UI that reads ISR data structures (the arp's note list, a lane) must tolerate a frame of inconsistency.

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

## Worked example: KEY (M6)

`firmware/src/key.c` (GLO > KEY, which replaced the note TAPE of M1) shows the patterns in one small file: settings
the UI writes and the ISR only reads (`keyt`), a clock that mirrors `click_tick` / `fxt_tick` (`key_tick`, called in
`events_block` before the tracks' steps), a `noinline` slow path (`key_shift_to`, once a note), the hooks in seq.c
(the step's notes in `seq_play`, `arp_list`, `seq_start` / `seq_stop`, `input_on` for the page's key entry), storage
in the project extras (`XTRA_V4`) and a page (`GR_KEY`: `key_knob` / `key_poll` in ui_input.c, `draw_columns`,
`graph_key` and its signature). `tests/key_test.c` drives the engine, the sequencer and the page.

## M2b: FX TRACKS, live song chains, the stores

- `fxtrack.c` (included by seq.c after key.c): two effect lanes. The ISR side holds effects through perform.c's
  `perf_fxt` (as keys would) and `perf_fxt_now` (a REPEAT starts on its step). Recording hooks `perf_press` via
  `fxt_press`. Its clock mirrors `click_tick`; in a song `fxt_src()` reads the section's slot's lanes and
  `fxt_section()` restarts them with `chain_apply`.
- song_chain.c: the UI never writes `chain.config`. It edits `chain_config`, then `chain_live(kind)` copies it to
  `chain_next` with IRQs off and sets `chain.req`; `chain_next_row()` (noinline, once a section) takes it.
- opstore.c (included by project.c after `proj_wire`): extras (and, until M6, the tape's store), hooks in `project_save_as`,
  `project_load`, `project_rename`, `persist_boot`, the autosave, `chain_prepare`, and main.c
  (the tape's boot and poll, gone with it in M6).

## M2c: SCENES

- `scene.c` (included by seq.c after motion.c): 16 scenes a project. The scene playing stays in the tracks
  (`trk[].step`, `p[P_SLEN..P_SGATE]`, `P_SQNT`), `motion` and `fxt_lane`; the other 15 wait packed in `scb` (pool,
  7880 B, `scn_encode` / `scn_decode`: a step only where it differs from `SCN_REST`, only the bytes that do). Never
  trust a bank from flash: `scn_decode` checks every field the project format would (`scene_bank_check`).
- A pick: the main loop unpacks into `SCN_BUF` (`chain.source[0]`: songs and scene picks exclude each other) and sets
  `scn.req = SCN_END`; the ISR's `events_block` calls `scene_due` (noinline, only while one waits) at the clock track's
  last step and `scene_swap`s with the carry, as `chain_tick` does. Stopped, the main loop swaps with the interrupts
  off. The swap measures the leaving scene against `scn.room` first and drops the pick if it does not fit.
- `scene_poll` (ui_input.c, every pass) packs the scene left into the bank, recomputes the clock track while one waits
  and reports `SP_SWITCHED` / `SP_FULL`; `scn.moved` tells the UI to drop the undo copy and re-snap the layer's put back
  (`scene_switched_ui`). `scene_settle` before a song starts (`chain_prepare`) and before a save.
- opstore.c: the banks in USR2 (`scb_write`, `scb_load`, `scb_retag`), tagged with `xtra_id` like the extras; the
  autosave's signature hashes the bank's bytes, so a pick and back again writes nothing.

## M2d: SCL > ORCHID (the Orchid's keyboard)

- `orchid.c` (included by seq.c after the AM_* enum): a track with its `orc_en` bit (SCL > ORCHID's KNOB 1) makes
  F3 .. C#4 the chord block (`ORC_KEY`, `orc_press` / `orc_release`: ChoralRoot FM-1's `cu_mod_press`, noinline, once
  a press) and D4 .. G5 roots: `key_on` asks `orc_build` instead of `chord_build`, into the same `kb_chord[k]` (now 7
  wide), so the notes go Felucca's way (voices, the ARP, MIDI OUT, live recording).
- `keyboard_block` marks block keys in `kb_orc` (their release goes back to `orc_release`), only for a track with
  ORCHID on that is not a kit and with no layer or DRUM grid. `orc_lock_req` (KNOB 3) is taken at the start of a block.
- ON (`orc_en`) and VOICE per track (`orc_voice[NTRK]`) are in the extras (`proj_xtra_t`, `XTRA_V2`); LOCK and the
  latch are runtime (`orc_reset` on a project load). The page GR_ORCH: `draw_columns` / `edit_param` special-case it
  (`orc_knob`), `graph_orc` draws it, the signature hashes `orc.gen`, `orc.lgen`, LOCK, VOICE, ON.

## M-Vave-OP1 defaults

- HOME: `ui_home_view` 0 (Felucca's SCOPE) and 1 are TRACKS, 2 LEVELS, 3 SCOPE; the menu steps TRACKS LEVELS SCOPE
  (`HVM_*`).
- STEP, stopped: OCT+ / OCT- add / remove a note of the cursor step (ui_input.c `step_oct`); the octave elsewhere.
- DRUM's preset is the 808 model kit (core.h `DRUM_808_E`); migrations of old data keep `DRUM_KIT_E` (STD).

## E1: FX1 CLOUD

- `fx1.c` (included by fx.c after perform.c): the delay bus's TYPE (`fx1s`, defined at the top of fx.c so perform.c's
  `perf_avail` can read it). `fx_buses` keeps Felucca's chorus + delay loop for the DELAY and branches once to
  `fx1_block` (noinline) otherwise: the chorus copy, the CLOUD (`cloud_render`) or a TYPE change (fade, clear 2048
  samples a block, start empty). The cloud's stereo goes into HALL's side buffer after the reverb (`cloud_side`).
- A grain's start is bounded so it never reads at the write head or what was not written (`cl_spawn`); the cloud has
  its own random numbers (`cl_rng`), so drums' noise renders the same with or without it.
- New settings that do not fit `song.g` (fixed by the formats) go into the project extras (`opstore.c proj_xtra_t`,
  append-only by `size`, older ones read as far as they go).

## E2: the OP-Z punch-ins, MAP B

- perform.c: `PF_DUCK .. PF_GLT` appended before `PF_M1` (stored codes: append-only; `PF_NFX` at most 29 for the
  5-bit key-map code, a `_Static_assert` says so). DUCK and STEREO are stages of their own after PHASER
  (`perf_duck`, `perf_stereo`, noinline, own budget lines). PITCH is the harmonizer with another `hinc` and `hwet`;
  RAMP UP and GLITCH are buffer modes (`BM_RAMP`, `BM_GLT`) on `sl_buf`.
- Trap: a new `else if` branch in `perf_buf` (inlined into `perf_block`'s per-sample loop) made the compiler move a
  block before the loop head, and `target_budget.py` counted it as a nested loop (814 -> 2904). The new modes go
  through the existing `perf_harm` call instead (`perf_harm` dispatches to `perf_buf_e2` when the mode is not
  `BM_HARM`): `perf_block` back to 835. Check the budget after any change to that loop's branches.
- Trap: `mulq16(a, k)` takes k up to 65536 (Q16 1.0); a gain above 1 (STEREO's sqrt 2) overflows its low half. Use
  `mulq16(a, k / 2) << 1`.
- MAP B: `perf_map[2 * PF_KEYS]` (MAP B from `PF_KEYS`), `perf_bank` (main), seq.c `perf_key` reads the bank;
  `perf_remap` makes held keys take the new map's effect. The bytes: `favorites.factory[15][0..9]` (`fx_keys_b`,
  ui_layer.c), the same 16 x 5 bits as MAP A's [14..23]. FX + ALGORITHM switches (ui_input.c, ALGORITHM was unused
  in a layer).
