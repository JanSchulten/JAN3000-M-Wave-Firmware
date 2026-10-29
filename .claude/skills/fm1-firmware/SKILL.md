---
name: fm1-firmware
description: Build, extend, test and safely ship custom firmware for the M-VAVE FM-1 pocket synthesizer (JieLi AC791N / WL82, pi32v2) on the Felucca / M-Vave-OP1 code base. Use this skill whenever someone works on FM-1 firmware - a new page or screen, a sound engine, sequencer, KEY or arp features, OP-1 / OP-Z-style workflows, RAM / flash / audio-ISR budgets, the JieLi toolchain or AC79 SDK, build.sh or tests/run_tests.sh failures, the browser emulator or its Artifact page, flashing a .fwsc package, or recovering an FM-1 that no longer starts - even when they only say "FM1", "M-Vave", "Felucca", "fwsc" or "meine Firmware".
---

# FM-1 firmware

Custom firmware for the M-VAVE FM-1 is written in C against a header-only HAL, built with JieLi's clang for pi32v2,
packaged as a `.fwsc` and installed over USB-MIDI. This repository (M-Vave-OP1) is a fork of Felucca by Leo
Kuroshita (GPL-3.0-only): keep that licence and its headers on every file, and credit Felucca in anything you
publish.

The FM-1 has one firmware bank and no recovery button. A firmware that does not boot needs extra hardware to
recover. So the working order is always: **host tests → browser emulator → careful install**, never the reverse.

## Where things are

| Need | Read |
| --- | --- |
| Chip, memory map, flash map, panel, display, audio, USB | [references/hardware.md](references/hardware.md) |
| How the code is put together: unity build, audio ISR vs main loop, tracks, pages, lazy drawing | [references/architecture.md](references/architecture.md) |
| Step-by-step recipes: a page, an ISR feature, an engine, an icon, a confirm dialog, persistence | [references/recipes.md](references/recipes.md) |
| Host tests, the UI test harness, lints, budgets, emulator, Playwright, the Artifact page | [references/testing.md](references/testing.md) |
| Who calls what, what a change reaches (the code graph: `graphify query / path / explain / affected`) | `CLAUDE.md` |
| OP-1 → FM-1 mapping and the rebuild plan | [references/op1-mapping.md](references/op1-mapping.md), `docs/OP1_COMPARISON.md`, `docs/reference/op1-manual.md` |

Read the reference that matches the task before editing code; they record the traps that cost time.

## Set up once per machine or container

```sh
sh .claude/skills/fm1-firmware/scripts/setup_env.sh          # toolchain, AC79 SDK files, Python deps
sh .claude/skills/fm1-firmware/scripts/setup_env.sh --emsdk  # + Emscripten for the browser emulator
```

The script retries the Gitee clone (it resets connections now and then) and fetches only the three SDK files the
package needs. Linux x86-64 runs the JieLi toolchain natively; macOS needs Docker (see BUILDING.md).

## The loop for every change

1. **Plan against the budgets** before writing code (numbers in hardware.md). The audio ISR, the 72 KB RAM and
   the 360 KB pool are the usual limits; the flash map is full, so persistence needs a deliberate trade.
2. **Write the change in the code's own style.** Felucca's comments are dense prose that say *why* and name the
   numbers; new code should read the same. One `.c` per subsystem, included by `firmware/src/felucca.c` or by the
   file that uses it (see architecture.md for the order).
3. **Write or extend a host test first or alongside** (`tests/<feature>_test.c`, registered in
   `tests/run_tests.sh`). Prove the test can fail: break the code on purpose once and watch it go red.
4. **Build:** `./build.sh`. It refuses an image over its limits and reports RAM, pool and image size.
   `sh .claude/skills/fm1-firmware/scripts/budget_report.sh` prints those plus the audio ISR cost.
5. **Run the whole suite** in the background (about 15 minutes): `tests/run_tests.sh`. It keeps going after a
   failure; the last line is `ALL HOST TESTS PASSED` or `HOST TESTS FAILED`. Find failures with
   `grep -nE " FAIL$|off by 1 px or more|HOST TESTS" <log>`: the UI alignment lint fails without printing FAIL.
6. **Look at it:** screenshots from the test harness (`screen_save`) and the browser emulator
   (`sh web/emu/build.sh`, then `scripts/emu_drive.js`). Check the colour palette in the emulator, the grey one
   in the harness.
7. **Install only when all of the above is green**, and only with the user's go-ahead (see Safety).

## Rules that keep the firmware healthy

- **Register access only in `firmware/hal/`.** The build checks it.
- **The audio ISR must stay cheap.** Code it runs goes through `events_block` (seq.c) once per 32-sample block.
  Keep new ISR work O(1) per block, put rarely used paths in `__attribute__((noinline))` functions, and check
  `python3 tests/target_budget.py`: `fm1_alnk0_irq` may grow at most 10 % over `tests/target_budget.txt`.
- **Main loop ↔ ISR:** the UI never edits ISR state mid-flight. It sets a `volatile` request field that the ISR
  takes at the start of a block (`transport_req`, `panic_req`, `aud_req`).
- **Big buffers go to the pool:** `static T buf[N] __attribute__((section(".pool")));` (zeroed at boot). Design
  state so all-zero is a valid empty state, because nothing else initialises it.
- **Stored formats are append-only.** Track parameters are mapped by count (new `P_*` just before `P_E0`), global
  `G_*` ids and `G_COUNT` are fixed by project files and the editor protocol, setting bits and enum values are
  never reused. Never reorder stored enums.
- **`track_t` stays a multiple of 16 bytes** (a `_Static_assert` guards it; use the `rsv_sz` pad).
- **UI drawing is lazy:** every value a panel shows must be hashed into `graph_signature()` (ui_graph.c), or the
  screen will not update.
- **Do not touch `firmware/loader/`, the boot guard in main.c, the package identity or the OTA path** without the
  user asking: they decide whether a bad image can still be replaced over USB.

## Safety: installing and recovering

- Ask before installing anything on a real FM-1, and say what is being installed.
- Install: `python3 tools/fm1_install.py build/felucca.fwsc` (needs `mido` and `python-rtmidi`), or serve the web
  installer locally: `python3 web/make_site.py build/felucca.fwsc dev /tmp/site`, then
  `http://localhost:8000/webapp/installer/` in Chrome or Edge.
- Way back while the firmware still starts: the installer's "Return to official V15" or M-VAVE's updater.
- If it no longer starts: the boot guard sends the unit to the chip's boot mode (USB device `WL80UBOOT`) after two
  failed boots; from there FM-1 Transporter with a Seeed XIAO RP2040 on the FM-1's USB lines reads and writes the
  flash. Back the flash up first, then write the official `FM-1.fwsc`.
- Without that recovery hardware, treat every install as possibly final: emulator and host tests first.

## Reporting back

Tell the user what changed, what was tested (suite result, budgets before → after, screenshots), and what was
not (anything only real hardware can show: timing feel, LED brightness, the speaker). Translate for the user's
language; code, comments and docs in the repository stay in English unless the user asks otherwise.
