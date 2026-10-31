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
  (shared budget `NVOICE`), a 128-step pattern (`step_t`; M-Vave-OP1, Felucca's 64: see "128 steps" below), arp and
  LFO state.
- `P_*` (per track) and `G_*` (global) are stored in projects and user presets and spoken by the editor protocol:
  append-only, see SKILL.md.
- `song` holds globals, transport (`playing`), the selected track (`song.sel`, `TSEL`), recording arm bits.
- Engines (`engine_t` in core.h): `note_on`, `render`, optional `amp`, `desc`, `block`, `keys`, `done`, eight
  edit parameters `P_E0..P_E7`, presets, a share of per-part engine memory (`mem`).

## The UI

- **Pages** (`params.c` `PAGES[]`): `{title, family, scope, graph, id[4]}`. The family is the button that opens it
  (`FAM_ENV` … `FAM_TRK`; tapping the button again steps through the family in `PAGES` order). `id[4]` are the
  parameters KNOB 1..4 edit; `0xFF` means the page handles the knob itself (`edit_param` in ui_input.c).
  `GR_*` selects the panel graphic; add new ones before `GR_N`. M-Vave-OP1: a page's own code goes through its
  `page_ops_t` (params.c; the row's `ops`) rather than tests of `pg->graph` in shared functions: flags, the action
  columns, knobs, cards, panel and its hash, defined in `ui_pages.c` (EQ, SONG, PHRASES, AUTOMATION, PRESETS, PROJECT, USER, KEY, FX TRACKS so far).
  `tools/ui_pages.py` writes `docs/UI_PAGES.md`, the special cases still left.
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
- SEQ + OCT+ held copies (`scene_copy`; the hold: ui_layer.c `layer_oct`, `lys.cp`, `SEQ_COPY_MS`): no swap; the
  tracks are copied into `SCN_BUF` with the interrupts off (live recording writes them in the ISR; never pack the live
  tracks into the bank), packed into the bank at `scn.cur`'s place, and `scn.cur` becomes the free scene. Read and
  clear `scn.req` with the interrupts off when a refusal must give a pick back: between a plain read and the clear the
  ISR may have swapped.
- opstore.c: the banks in USR2 (`scb_write`, `scb_load`, `scb_retag`), tagged with `xtra_id` like the extras; the
  autosave's signature hashes the bank's bytes, so a pick and back again writes nothing.

## M2e: SCL > CSET (the Roland J-6's chord sets; replaced M2d's SCL > ORCHID)

- `cset.c` (included by seq.c after the ARP's modes): a track with its `cset_en` bit (SCL > CSET's KNOB 1) that is no
  kit plays on every key (and MIDI note) the chord of the key's note in its set (`cset_set[NTRK]`, KNOB 2; KNOB 3 by
  genre): `cset_make` from `CSET_NOTE` (`cset_data.h`, written by `tools/gen_cset.py` from the J-6 owner's manual's
  Chord Set List), F3 .. E4 in the list's own voicing. It comes before CHRD and QNT. ON makes the track POLY
  (`cset_voice` keeps the VOICE before). The page GR_CSET shows the last chord (`cset_last`).
- ON and the set per track are in the project extras (`opstore.c`; ORCHID's old bytes stay reserved, written 0).
  `orchid.c` and `tests/orchid_test.c` are gone; `tests/cset_test.c` tests the sets.

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

## 128 steps (M-Vave-OP1)

- `NSTEP` 128 (LEN 1..128); `NSTEP_FMT` 64 is what every stored format holds. FUN10 keeps steps 1..64, a LEN of at most
  64 and the motion records of steps 1..64 (`motion_disk_t`: place track << 6 | step); the rest is the extras' tail
  (opstore.c `xtra_hi_t`, `XTRA_V7`, after `proj_xtra_t` in the same flash object): steps 65..128 packed as the project
  packs its own (project.c `proj_step_pack`), each track's true LEN, the lanes' steps 65..128 and LEN, LEGATO, the
  records of steps 65..128. Firmware of 64 steps reads such a project as its first 64 steps.
- The tail is not cached with `xtra_slot` (2.8 KB a slot): `xtra_hi_of` reads it from the flash into `st_buf`, so use
  it before the next storage call (`xtra_proj_id` reads the flash too: take the id first). Applied after a project's
  load (`xtra_apply`), in `chain_prepare` (each slot) and `project_info` (SONG's lengths and pictures).
- In RAM a motion record's place is 16 bits, track << 7 | step: use `MO_TRK`, `MO_STEP`, `MO_PLACE`, never shifts.
- Main-loop step sets (held steps, locks) are `stepset_t` (`ss_add`, `ss_has`, `ss_any`), not `uint64_t`.
- Scenes: a scene with anything past step 64 packs with `SCN_F_WIDE` (16-byte masks, two-byte places); every other
  scene packs byte for byte as before.
- The editor's INFO keeps its step byte at 64 (a 7-bit byte); block `58 01 lo hi` says 128.

## HOME > EQ (four bands a track)

- Settings `eq_set[NTRK][EQ_NB]` (params.c, beside `shim_p`): int16 FREQ step (R40, `EQ_HZ_X10`), GAIN in half dB,
  TYPE (`EQT_*`, stored), RESO (LP / HP Q step). The page is `GR_EQ` in FAM_TRK after CLOCK; its ops' `desc` (`eq_page_desc`, ui_eq.c) hands KNOB 1..4
  `eq_ui_band` and the band's values (KNOB 3: RESO on LP / HP). New formats `F_EQHZ`, `F_HDB`, `F_EQQ` (appended).
- `eq.c` (included by fx.c after fx1.c): RBJ biquads in single float on the FPU (`FM1_FPU`), TDF-II. `eq_poll` (main,
  every ui_input pass) computes a changed track's coefficients from gen_tables.py's `EQ_COS/SIN/A/SQA/QV` and hands them
  over through `eqx.req[k]` (written only while 0; the ISR takes and clears). The ISR glides a quarter a block; flat
  bands are skipped, a track with all flat costs one load (`eqa.run | eqx.req`) in mix_part / mix_spread after the
  INSERT. A silent part (tail run out) calls `eq_rest`: state dropped. The EQ counts for the part's tail.
- Kept in the project extras' tail (`xtra_hi_t.eq`, `XTRA_V9`, 4 bytes a band, validated on load); `autosave_sig`
  hashes `eq_set`; `project_new` and extras of before: `eq_defaults`. Host tests reset with `eq_power_on`.
- The panel (`graph_eq`, ui_eq.c, with its hash `eq_sig`: `EQ_OPS` in ui_pages.c) draws |H| from RBJ's sin^2(w/2) form (`EQ_PHI`): the cos w form cancels in float near 0 Hz.
- Cost (this change): image +6.4 KB, RAM +1.6 KB, `fm1_alnk0_irq` static cost +171. `tests/eq_test.c`.

## HOME > MASTER (the master's dynamics)

- Settings `dyn_set[DYN_N]` (params.c, beside `eq_set`): COMP 0..100 %, RELEASE 0..100 (20 ms .. 1 s, `dyn_rel_ms`),
  LIMIT a step of `DYN_NLIM` (-15.2 .. -0.2 dBFS; `DYN_LIM_DEF` is fx.c `LIM_T`, the threshold of before), GAIN in half dB
  (0 .. +12). New formats `F_DYNMS`, `F_LIMDB` (appended). The page is `GR_DYN` in FAM_TRK after EQ (`DYN_OPS`, ui_dyn.c).
- The master runs at the full level now and MASTER scales what leaves it (fx.c `master_block`, dyn.c): `master_out` is
  `master_pre` (DC blocker, SPEAKER EQ) + `master_lim` (the limiter at `lim_t`, the soft clip); `perf_master` folds the
  buses into the mix (wet 0) before the FX layer. USB LEVEL FIXED still takes the master before the knob (audio.c).
- `dyn.c` (included by fx.c after the mix buffers): `dyn_poll` (main, every ui_input pass) turns changed settings into
  `dynx` (floats, written in place) and `lim_t`, and the ISR's peaks into the page's meters (`dyn_mtr`, half dB, a 0.5 dB
  fall a pass). `master_dyn` (noinline, FPU) only while COMP or GAIN is up: the gain from the block before's peak (a
  log, a power: once a block), ramped over the block, between `master_pre` and `master_lim`.
- At the defaults the output is bit for bit the master of before at MASTER's top (`tests/dyn_test.c` AS BEFORE).
- Kept in the project extras' tail (`xtra_hi_t.dyn`, `XTRA_V10`, validated on load); `autosave_sig` hashes `dyn_set`;
  `project_new` and extras of before: `dyn_defaults`. Host tests reset with `dyn_power_on`.
- Trap: ui_eq.c `eq_log2` is log2 + 1 (it only feeds a difference); dyn.c `dyn_log2` is the true log2.
- Cost: image +2.7 KB, RAM +48 B, `fm1_alnk0_irq` static cost about +0.7 %, `master_dyn` 47.

## The drum keys (DRUM, VDRUM)

- seq.c `kb_press` (noinline: the key loop calls it on a press) sends a kit track's keys to `kit_key`: a black key of a
  lane strikes the lane's note (`kit_notes`) and picks it (eng_drum.c `kit_sel[NTRK]`), a white key strikes the lane
  picked with `kit_kp` = white place - 7 + 12 x OCT (F4 as designed). Black keys past the lanes are `KB_SILENT`.
- `kit_kp` is set around `key_on` only; the engines' note_on take it into the hit: DRUM `drum_lane_t.kp` -> `dv_param_t.key`
  (1/16 semitone, added after TUNE's clamp; the coefficient cache compares three words now), VDRUM `vd_part_t.st16`. Any
  other hit (sequencer, MIDI, the arp later) has 0. On a kit `key_on` strikes even when another key holds the same note.
- Live recording gets the lane's note: the lane's hit, not the pitch. `drum_keys` (the GM map) stays the engine's
  `keys` hook for the arp's latched keys and chord.c. The UI says the lane and pitch from `kit_ev` (ui_input.c `kit_msg`).
- `tests/drumkeys_test.c`.
