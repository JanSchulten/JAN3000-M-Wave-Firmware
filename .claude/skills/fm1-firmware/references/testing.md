# Testing

## The suite

```sh
./build.sh                       # first: build/gen (tables, fonts, icons, samples) is needed by every test
. ~/emsdk/emsdk_env.sh           # optional: then run_tests.sh also builds and tests the browser emulator
tests/run_tests.sh > /tmp/t.log 2>&1   # ~15 min; run it in the background
grep -nE " FAIL$|off by 1 px or more|HOST TESTS" /tmp/t.log
```

`run_tests.sh` runs everything even after a failure and ends with `ALL HOST TESTS PASSED` or `HOST TESTS FAILED`.
Most checks print `<suite>: <what> ... ok|FAIL`. The UI render lint reports its alignment as
`alignment: N measurements of M places, K off by 1 px or more`: any K > 0 fails the run without a FAIL line.

One test on its own (the HAL's inline assembly needs optimisation on, so keep `-O1`):

```sh
mkdir -p build/host
cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/key_test tests/key_test.c -lm && build/host/key_test
```

M-Vave-OP1's own tests, good templates: `key_test.c` (an ISR feature driven block by block, its page through the UI harness), `fxtrack_test.c` (an
ISR feature plus its pages, through `mix_block`), `songchain_test.c` (UI gestures while the transport runs: space REC
taps 400 ms apart, two quick taps lock a layer), `opstore_test.c` (flash stores on a NOR image: round trip, power-on,
torn and damaged copies), `scene_test.c` (a codec fuzzed with random and damaged input, an ISR switch timed to the
block from the track's own state, mutations of the code proving the checks fail). A test that presses keys in a layer needs a `frame()` after the button goes down, so the
UI hands the keys to the layer (`kb_mask`), as on the unit.

Register a new test in `tests/run_tests.sh` next to a similar one (a `$CC … -o "$OUT/x" tests/x.c -lm` line and a
`run "description" "$OUT/x"` line) and describe it in the comment block at the top.

## The UI test harness (`tests/ui_test.c`)

Include it with `#define UI_TEST_NO_MAIN 1` / `#include "ui_test.c"` to get the real firmware with stubbed HAL:

- `ui_power_on()` mirrors `main.c`'s init. **Reset any new subsystem state there** (as it does for the KEY),
  or tests leak state into each other.
- `frame()`, `frames(ms)`: UI frames (16 ms each). The audio side does not run by itself: call
  `events_block(CTL)` (or `blocks(n)`) to advance the ISR, or `seq_tick()` for one track.
- `press(B_x)`, `hold(B_x)`, `btn_down` / `btn_up`, `turn(EN_x, steps)`, `tap_key(k)`, `fm1_in.notes` for held keys.
- **Two presses of the same layer button within ~300 ms are a double tap and lock the layer.** Put `frames(400)`
  between taps when a test cycles a family (GLO, SEQ, HOME …).
- `go_page(GR_x)`, `open_family(FAM_x)`, `track_select(k)`, `msg_is("TEXT")`, `check(label, ok)`.
- `screen_save(dir, name)` writes a 240 × 240 PPM of the current screen (grey palette). Convert with Pillow to
  look at it.
- `led_of()` and `led_pos` live in the part excluded by `UI_TEST_NO_MAIN`; test LED logic inside ui_test.c itself.

Prove a new test can fail: introduce a deliberate bug (copy the file aside, `sed` a line, rebuild, run, restore)
and confirm it goes red.

## Lints and budgets that run inside the suite

- **Screens unchanged** (`sh tools/ui_compare.sh [BASE]`, M-Vave-OP1): renders every screen from the working tree and
  from BASE (default `origin/main`) and compares them byte for byte (2868 screens, ~1 min); for UI refactors.
- **UI renders** (`tests/ui_render.c`): every page, every palette; text clipping, overlap, ink on its own colour,
  GREY / MONO purity, and alignment by ink: a centred glyph off by 1 px fails (odd cushion sizes do this).
- **Target budget** (`tests/target_budget.py`): static cost of the render loops and ISRs from the pi32v2
  disassembly (`build/felucca.dis`), +10 % allowed. Run it alone after `./build.sh`.
- **Regression renders** (`tests/regress.c`): golden hashes of every engine and preset, voice budget, routing.

## The browser emulator

`sh web/emu/build.sh` (needs `emcc`) builds `build/emu/{index.html, worklet.js, felucca.wasm}`: the whole
firmware as WebAssembly, the panel on screen, driven by an AudioWorklet. Serve it from localhost (Web MIDI and
AudioWorklet need a secure context): `cd build/emu && python3 -m http.server 8765`.

`scripts/emu_drive.js` drives it with Playwright and saves screenshots:

```sh
NODE_PATH=$(npm root -g) node .claude/skills/fm1-firmware/scripts/emu_drive.js http://127.0.0.1:8765/index.html /tmp/shots
```

Gotchas:

- Click "Switch on" (`#go`) and wait ~3.5 s for the splash before input.
- A focused knob swallows the keyboard: after turning a knob with arrow keys, blur it
  (`document.activeElement.blur()`), or the next button key is ignored.
- Keyboard map: `1`…`6` FX SCL ENV LFO EDIT GLO, `7` `8` `9` `0` `-` `=` HOME SAVE ARP SEQ PLAY REC, `[` `]` OCT− OCT+,
  `Z`… white keys, `S D F H J L ; '` black keys, arrows SELECT / ALGORITHM.
- Chromium in a sandbox may use `/opt/pw-browsers/...`; pass `executablePath` if the bundled one is missing.

## The Artifact page (`web/lab`)

`sh web/lab/build.sh` builds `build/lab`: `index.html` (an Artifact page fragment without doctype),
`local.html` (with doctype, for localhost), `felucca.wasm` and `felucca-js.mjs` (the same firmware through
Binaryen's `wasm2js`, bit-identical output, ~10× slower, used when a host blocks WebAssembly). The page runs the
firmware on its own thread: a ScriptProcessor drives the clock while sound is on, `requestAnimationFrame` while it
is off. Publish `build/lab/index.html` with the other two files as supporting files (`application/wasm`,
`text/javascript`).

`wasm2js` needs the features the build uses:
`--enable-bulk-memory --enable-sign-ext --enable-nontrapping-float-to-int --enable-mutable-globals`.
