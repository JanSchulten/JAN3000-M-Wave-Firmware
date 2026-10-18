# M-VAVE FM-1 firmware development spec

A self-contained brief for writing custom firmware for the **M-VAVE FM-1** pocket synthesizer, e.g. to hand to Claude
in another project. Everything here was learned while building M-Vave-OP1, a fork of **Felucca** (GPL-3.0-only).

- Felucca (upstream): https://github.com/hugelton/Felucca
- M-Vave-OP1 (this fork): https://github.com/janschulten/m-vave-op1, branch `op1-on-felucca`

Measured numbers are from October 2026 builds of M-Vave-OP1 and will drift as code changes. Re-measure with
`./build.sh` and `python3 tests/target_budget.py`.

---

## 1. Ground rules (read first)

1. **The FM-1 has one firmware bank and no recovery button.** A firmware that does not boot needs extra hardware to
   recover (see §6). Always work in this order: **host tests → browser emulator → careful install**, never the
   reverse.
2. **Never install on a real unit without the owner's explicit go-ahead.** Say exactly which file is installed.
3. **Licence:** Felucca is GPL-3.0-only. A firmware built on it stays GPL-3.0-only, keeps every licence header, and
   credits Felucca (and ported parts such as ChoralRoot FM-1, also GPL-3.0-only) wherever it is published.
4. **Do not touch** `firmware/loader/`, the boot guard in `main.c`, the package identity or the OTA path unless the
   owner asks. They decide whether a broken image can still be replaced over USB.
5. **Register access only in `firmware/hal/`.** The build checks this.

---

## 2. Hardware

### Chip
- **MCU:** JieLi AC791N / WL82 family, **pi32v2** CPU. Some strings in the stock firmware say `JL-BR22`; that is
  inherited library naming, not the chip.
- **SRAM:** 512 KB. **Flash:** 1 MB SPI NOR, executed in place (XIP) at `0x02000000`.

### SRAM map (from `firmware/app.ld`)

| Region | Address | Size | Use |
| --- | --- | --- | --- |
| RAMTEXT | 0x01C00000 | 24 KB | code that must run while the flash is busy (flash driver) |
| debug | 0x01C06000 | 8 KB | kept free for RAM-run debugging |
| RAM | 0x01C08000 | 72 KB | `.data` + `.bss`: everything the ISRs touch, small variables |
| POOL | 0x01C1A000 | 360 KB | `.pool`: big zero-initialised buffers (FX lines, canvases, engine memory) |
| stacks | 0x01C74000..0x01C7C000 | 32 KB | user + system stacks, 256-byte guard bands |
| NOINIT | 0x01C7C000 | ~15 KB | survives resets and boot-mode entry (panel calibration, boot guard) |
| reserved | 0x01C7FD50.. | | boot_info, boot-state mailbox, vector table |

RAM vs POOL is only a budget line in the linker script: both are the same SRAM. Pool buffers are declared
`static T buf[N] __attribute__((section(".pool")));` and are **zeroed at boot and never initialised otherwise**:
design state so that all-zero is a valid empty state.

### Current budgets (M-Vave-OP1, Oct 2026)

| | Used | Limit |
| --- | --- | --- |
| RAM `.data`+`.bss` | 56 440 B | 73 728 B |
| Pool | 355 244 B | 368 640 B (build demands ≥ 8 KB free) |
| App image (XIP code) | 535 124 B | 581 564 B |
| Audio ISR static cost `fm1_alnk0_irq` | 37 289 | budget 37 502, hard limit +10 % |
| Key-scan ISR `fm1_timer5_irq` | 34 803 | budget 33 387 (+10 %) |

Rough costs: a synth track is about 23 KB, of which the `trk[]` entry is 1.8 KB, engine memory 12.9 KB and the
SLICER buffer 8 KB. One second of 16-bit mono audio is 86 KB, so **there is no room for audio recording or
looping**. Record notes, not sound.

### Flash map (1 MB, every sector is spoken for)

| Range | Use |
| --- | --- |
| 0x00000..~0x96FFF | boot code + app image |
| 0x97000..0x9EFFF | projects A–D (two 4 KB copies each) |
| 0x9F000 | FM6 patches of user presets, copy A |
| 0xA0000..0xB3FFF | user sample slot USR1 (80 KB ADPCM, ~7 s at 22.05 kHz) |
| 0xB4000..0xC7FFF | (M-Vave-OP1, former USR2) scenes: sets A..D + autosave, two 8 KB copies each |
| 0xC8000..0xCFFFF | (M-Vave-OP1) project extras A/B (`proj_xtra_t`) |
| 0xD0000..0xD1FFF | the autosave's extras A/B |
| 0xD2000..0xDBFFF | free (the dropped TAPE's copies were here) |
| 0xDC000..0xDFFFF | user preset banks |
| 0xE0000..0xE4FFF | OTA staging (the update loader) |
| 0xE5000..0xE6FFF | autosave A/B |
| 0xE7000 | FM6 voice bank |
| 0xE8000 | the stock OTA's VM backup: **never write** |
| 0xFC000..0xFFFFF | settings; FM6 patches copy B (0xFE000) |

Flash is erased and written **only while the transport is stopped and nothing sounds**. An erase stalls XIP and
silences audio for about 40 ms. Writes go through `storage.c`: A/B copies, a CRC, a sequence number, and the
commit record written last.

### Panel
- **27 keys** F3..G5 (key k = k semitones above F3): 16 white, 11 black. An 11-column × 6-row diode matrix behind
  two 74HC595 shift registers (PA4 SER, PA3 SRCLK, PA1 RCLK), scanned by a ~10 kHz TIMER5 ISR with asymmetric
  debounce.
- **14 buttons:** FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT− OCT+. Matrix ids are mapped through
  `panel_t`; holding OCT− and OCT+ at power-on recalibrates.
- **7 encoders:** SELECT, ALGORITHM, PRESETS, KNOB 1..4. One detent is one quadrature cycle.
- **MASTER:** a potentiometer on the ADC (0..1023).
- **LEDs:** one per button and per key, on the matrix columns. Levels: dark, dim glow, breath, mid, lit.

### Display, audio, I/O
- **Display:** ST7789-class, 240 × 240 RGB565.
  - Interface: SPI1 at 12 MHz with DMA (PC9 CLK, PC10 DO, PC7 CS, PC8 D/C).
  - Backlight: PA2, active low; PA2 is also the key matrix's /OE.
  - A full 240 × 122 blit is ~53 KB and takes ~36 ms, so **redraw only what changed**.
- **Audio out:**
  - Format: I²S, 44.1 kHz, stereo, half buffers of 128 frames (2.9 ms).
  - Rendering: blocks of 32 frames (`CTL`).
  - Outputs: headphone/line jack and speaker.
- **No audio input:** no mic or line-in is usable.
- **USB:** class-compliant MIDI, USB audio *to the host* (the master output, 44.1 or 48 kHz), and an optional CDC
  serial console.
- **MIDI:** the TRS jack is **MIDI IN only** (UART1, PH8); there is no DIN/TRS MIDI out.
- **Clock and sync:**
  - Out: USB MIDI clock, or a Pocket Operator–style sync pulse on the left audio channel (M-Vave-OP1 "SYNC OUT PO").
  - In: USB MIDI clock, or MIDI clock on the TRS input.
- **Battery:** ADC channel 3 through a divider.

---

## 3. Toolchain and environment

- **Compiler:** JieLi Linux toolchain, clang 4.0.1 for pi32v2, in `~/.jieli/toolchain`; `tools/get_toolchain.sh`
  fetches it from `pkgman.jieliapp.com`. Native on Linux x86-64; macOS needs Docker (see `BUILDING.md`). Newer
  toolchain drops change code size and ISR cost slightly.
- **SDK:** only three files of `fw-AC79_AIoT_SDK` (Apache-2.0, tag `AC79NN_SDK_V1.2.1_2023-12-13`, Gitee) go into a
  package:
  - `cpu/wl82/tools/uboot.boot`
  - `cpu/wl82/tools/cfg_tool.bin`
  - `cpu/wl82/tools/cfg/eq_cfg_hw.bin`

  The firmware itself does **not** link the SDK; it uses its own header-only HAL. Clone it sparsely; Gitee resets
  connections, so retry.
- **Python:** Pillow and fontTools for the generated fonts, icons and tables; mido and python-rtmidi only for the
  USB installer.
- **Emscripten** (optional): builds the browser emulator and the wasm2js fallback.
- **One-shot setup:**

  ```sh
  sh .claude/skills/fm1-firmware/scripts/setup_env.sh          # toolchain, SDK files, Python deps
  sh .claude/skills/fm1-firmware/scripts/setup_env.sh --emsdk  # + Emscripten
  ```

---

## 4. Build, package, install

```sh
./build.sh                     # -> build/felucca.bin, build/loader/ota.bin, build/felucca.fwsc
./build.sh --release X.Y       # release identity
sh .claude/skills/fm1-firmware/scripts/budget_report.sh   # RAM, pool, image, ISR costs
```

- `build.sh` / `tools/build.py` refuses an image over its limits and reports RAM, pool and image size. It also
  generates `build/gen/*` (fonts, icons, tables, samples), which every test needs, so **build once before testing**.
- **Package:** `.fwsc` = app + update loader + the three SDK files. Identity `FM-1_900` for development and
  `FM-1_910` for release. The installers check the identity; keep it.
- **Install** (only with the owner's go-ahead), either way:
  - Over USB-MIDI: `python3 tools/fm1_install.py build/felucca.fwsc`.
  - With the web installer:
    1. `python3 web/make_site.py build/felucca.fwsc dev /tmp/site`
    2. Serve `/tmp/site` (e.g. `python3 -m http.server 8000`).
    3. Open `http://localhost:8000/webapp/installer/` in Chrome or Edge (needs Web MIDI).

---

## 5. Recovery

- **While the firmware still starts:** the installer's "Return to official V15", or M-VAVE's own updater.
- **If it no longer starts:**
  1. The boot guard sends the unit into the chip's boot mode (USB device `WL80UBOOT`) after two failed boots.
  2. From there, "FM-1 Transporter" with a Seeed XIAO RP2040 wired to the FM-1's USB lines can read and write the
     flash.
  3. Back up the flash first, then write the official `FM-1.fwsc`.
- Without that recovery hardware, treat every install as possibly final.

---

## 6. Architecture of a Felucca-based firmware

### One compilation unit
`firmware/src/felucca.c` `#include`s every `.c` in dependency order; each file may use what the files above define.
The HAL (`firmware/hal/*.h`) is header-only. The include order, abridged:

```
HAL → libc.c lcd.c gfx.c → core.h → engines.c (dsp.c, eng_*.c) params.c mod.c voice.c slicer.c fx.c
→ usb.c midi_uart.c song_chain.c seq.c (chord.c, orchid.c, motion.c, scene.c, fxtrack.c …) audio.c
→ panel.c ui.c menu_items.c icons.c ui_graph.c ui_draw.c ui_menu.c ui_input.c ui_layer.c
→ storage_hw.c storage.c upreset.c project.c (opstore.c) ota.c editor.c console.c master.c main.c
```

The host tests (`tests/hostsim.c`, `tests/ui_test.c`) and the browser build (`web/emu/felucca_web.c`) include the
same sources with a stubbed HAL. To add a subsystem, **include its `.c` from the file that uses it** (as seq.c
includes fxtrack.c); then every harness picks it up without edits. Everything is `static`, and names are short
with a subsystem prefix (`seq_`, `ui_`, `cv_`, `fx_`).

### Two worlds: the audio ISR and the main loop
- **Audio ISR** (`audio.c`): for each half buffer, blocks of 32 samples go through `events_block` (seq.c) and then
  `mix_block` (fx.c).
  - `events_block` handles transport requests, keyboard, MIDI in, `seq_tick` per track, the arp and the click.
  - `mix_block` renders the voices and runs the per-track DIST / INSERT / SLICER, the send buses (chorus, delay,
    reverb) and the master.
  - Every note ends in `trk_note_on` / `trk_note_off`.
- **Main loop** (`main.c`): UI frames (`ui_input`, `ui_draw`), storage writes, USB, the editor protocol.
- **Crossing over:** the UI never edits ISR state mid-flight. It sets `volatile` request fields (`transport_req`,
  `panic_req`, `aud_req`, …) that the ISR takes at the start of a block. Plain settings the ISR only reads may be
  written directly if a torn read is harmless. UI code reading ISR structures must tolerate one frame of
  inconsistency.
- **ISR cost rules:**
  - Per-block work is O(1).
  - Rare or long paths go into `__attribute__((noinline))` functions.
  - No divides in sample loops (use shifts or precomputed reciprocals).
  - `tests/target_budget.py` measures each listed function statically from the disassembly and allows at most
    +10 % over `tests/target_budget.txt`. Add every new ISR function to its `FUNCS` list and give it a budget line.

### Tracks, parameters, the song
- `NTRK` = 4 tracks. Each is a synth part with its own engine, preset, `p[P_COUNT]` parameters, voices from a shared
  `NVOICE` pool, a 64-step pattern (`step_t`), and arp and LFO state. `track_t` must stay a multiple of 16 bytes
  (a `_Static_assert` guards it).
- `P_*` (per track) and `G_*` (global) are stored in projects and presets and spoken by the editor protocol:
  - New `P_*` go just before `P_E0`.
  - `G_*` ids and `G_COUNT` are fixed.
  - Setting bits and enum values are never reused.
  - **Stored formats are append-only.**
- Engines (`engine_t`, core.h): `note_on`, `render`, optional `amp`, `desc`, `block`, `keys`, `done`; eight edit
  parameters `P_E0..P_E7`; presets; a share of per-part memory. `ENGINES[]` is append-only, and display order
  lives in `ENGINE_ORDER`.

### UI system
- **Pages** (`params.c PAGES[]`): `{title, family, scope, graph, id[4]}`.
  - The family is the button that opens the page; tapping it again steps through the family.
  - `id[4]` are the parameters KNOB 1..4 edit; `0xFF` means the page handles that knob itself (`edit_param` in
    ui_input.c).
  - Append new `GR_*` graph values at the end of the enum.
- **Screen layout:** header 0..24, four value cards 28..72, panel 76..198 (`Y_GRAPH`, `H_GRAPH` = 122), footer
  202..240. Canvas buffers hold at most 240 × 124; draw bigger things in parts with `cv_oy`.
- **Lazy drawing:** every value a panel shows must be hashed into `graph_signature()` (ui_graph.c), or the screen
  does not update.
- **Drawing API (gfx.c):** `cv_begin`, `cv_rect`, `cv_rrect`, `cv_line`, `cv_line_t`, `cv_text*`, `cv_icon_*`.
  Use theme colours `T_*`, never literals, so all ten palettes work. GREY and MONO palettes must render strictly
  grey/neutral (a lint checks this).
- **Text and alignment:** centre with `GFX_HOOK_ALIGN` hooks. The render lint fails at 1 px off; odd sizes cause
  this.
- **Other building blocks:**
  - Confirm dialogs: `CF_*` + `confirm_text()`; OCT+ confirms, OCT− cancels.
  - Help hints: `HELP_PAGE[]` in ui_help.c, length-checked.
  - LEDs: `ui_leds()`.
  - Held-button layers: ui_layer.c.
  - Menu items: menu_items.c, where the `MI_*` order is the menu order.

### Persistence
The flash map is full. New per-project data goes into the **project extras** (`opstore.c proj_xtra_t`).
- Append fields at the end. The `size` field tells a reader what is there, and older extras are read as far as
  they go.
- Fill the fields in `xtra_capture`, apply them on load, and hash them into the autosave signature.
- The extras carry the CRC of their project (`xtra_id`), so stale extras are never applied.
- New global settings bits go into spare bits of existing settings bytes.
- Test against a NOR image in the style of `tests/opstore_test.c`: round trip, power-on, torn write, damaged copy.

---

## 7. Testing

```sh
./build.sh                                   # first: build/gen is needed by every test
. ~/emsdk/emsdk_env.sh                       # optional: also tests the browser emulator
tests/run_tests.sh > /tmp/t.log 2>&1         # ~15 min, run in the background
grep -nE " FAIL$|off by 1 px or more|HOST TESTS" /tmp/t.log
```

- The last line is `ALL HOST TESTS PASSED` or `HOST TESTS FAILED`. The alignment lint fails without printing
  "FAIL".
- **Do not edit sources or `run_tests.sh` while the suite runs.**
- **Single test:** `cc -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/x tests/x.c -lm`. `-O1` is required, because
  the HAL's inline assembly must be optimised away on the host.
- **UI harness** (`tests/ui_test.c`):
  - Startup and frames: `ui_power_on()`, `frame()` / `frames(ms)`.
  - Input: `press`, `hold`, `btn_down` / `btn_up`, `turn(EN_x, n)`, `key_down` / `key_up`.
  - Running the audio side: `events_block(CTL)`.
  - Navigation and checks: `go_title("PAGE")`, `check(label, ok)`.
  - Screenshots: `screen_save()`.
  - Two presses of a layer button within ~300 ms count as a double tap, so put `frames(400)` between taps. Reset
    any new subsystem's state in `ui_power_on()`.
- **Render lint** (`tests/ui_render.c`): every screen in every palette; checks clipping, overlap, ink on its own
  colour, GREY/MONO purity and 1-px alignment. Add a named scenario for each new screen.
- **Golden files:** `tests/golden.txt` (regenerate with `GOLDEN_UPDATE=1` only after an intended sound change),
  `tests/union_golden.txt`, `tests/target_budget.txt` (`BUDGET_UPDATE=1`). Web editor tables are checked against
  `build/host/desc.json` by `node web/test_web.mjs`.
- **Prove a new test can fail:** break the code on purpose once and watch it go red.
- **Browser emulator:**
  - `sh web/emu/build.sh` builds the whole firmware as WebAssembly, driven by an AudioWorklet; serve it from
    localhost. `scripts/emu_drive.js` drives it with Playwright.
  - `sh web/lab/build.sh` builds a self-contained page (wasm plus a wasm2js fallback) that can be published as a
    claude.ai Artifact.

---

## 8. Traps learned the hard way

- A new `else if` inside an inlined per-sample loop can make the compiler restructure the loop, and the budget tool
  then counts it as nested: one change took a cost from 814 to 2904. Dispatch through an existing call instead,
  and **re-check the budget after every change to an ISR loop's branches**.
- `mulq16(a, k)` takes k up to 65536 (Q16 1.0); a gain above 1 overflows. Use `mulq16(a, k / 2) << 1`.
- Feedback paths (chorus, flanger, shimmer, reverbs) must **round towards zero**, e.g.
  `(x * g + (x < 0 ? 32767 : 0)) >> 15`. Plain `>> 15` floors, which leaves a −1 limit cycle (hum, DC) after
  the sound stops. Test that tails end in exact silence.
- Pitch shifting inside a reverb loop: feed it **before the input low cut**, or DC and rumble circulate and the loop
  runs away.
- Every note-on needs its note-off: on stop, a jump, a direction change, a full buffer. Store the sounding pitch
  (after any transposition) so the off matches.
- Changing a `G_*` range changes the editor protocol tables. Update `web/editor.html`'s mock and `web/test_web.mjs`.
- `sed -i` on core headers may be blocked by permission classifiers; a short Python replace works.
- UI test state leaks between tests. Set the state a test depends on explicitly.

---

## 9. Ideas already explored (M-Vave-OP1)

These live in this repository and can be studied or reused:
- FX TRACKS: punch-in lanes.
- OP-Z punch-in effects, MAP B.
- SONG pattern chains.
- 16 SCENES per project.
- SCL > ORCHID: the Telepathic Instruments Orchid's chord keyboard, after ChoralRoot.
- FX1 CLOUD: a granular effect on the delay bus.
- Effect pages drawn as guitar pedals, with SEND as mixer faders.
- CHORUS types CHORUS / FLANGER / VIBRATO with FDBK.
- REVERB ROOM / SPRING / HALL / SHIMMER.
- PO sync out.
- A one-screen ARP with a keyboard view.
- A KEY track (chord progression transposing the tracks inside their scales; played live, recorded with REC, as the OP-Z's).
- A notes-only TAPE was built and dropped as not good enough.

---

## 10. Prompt template for a new firmware project

> You are writing custom firmware for the M-VAVE FM-1 (JieLi AC791N/WL82, pi32v2, 512 KB SRAM, 1 MB flash), based
> on Felucca (GPL-3.0-only, https://github.com/hugelton/Felucca). Follow the attached FM1_FIRMWARE_SPEC.md.
> - Plan against the budgets in §2 before writing code (RAM, pool, flash, audio ISR +10 %).
> - Keep stored formats append-only; new persistent data goes into the project extras.
> - Write host tests alongside the code. Run the full suite before calling anything done; check UI changes in
>   screenshots and the browser emulator.
> - Never install on the hardware without my explicit go-ahead; tell me what only real hardware can confirm
>   (timing feel, levels, the speaker).
> - Keep Felucca's licence and credit it.
> - Goal of this firmware: <describe it>.
