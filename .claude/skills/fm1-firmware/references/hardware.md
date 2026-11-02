# FM-1 hardware, as the firmware sees it

Sources: Felucca's HAL (`firmware/hal/*.h`), `firmware/app.ld`, `firmware/hal/fm1_flash.h`, `firmware/src/storage.c`,
AL-255/FM-1-RE (reverse engineering of the stock firmware), measurements from builds in this repository.

## Chip and memory

- **MCU:** JieLi AC791N / WL82 family, pi32v2 CPU. Some strings in the stock firmware say `JL-BR22`: inherited
  library naming, not the chip.
- **Toolchain:** JieLi Linux toolchain, clang 4.0.1 for pi32v2 (`~/.jieli/toolchain`, from
  `pkgman.jieliapp.com`). Newer toolchain drops produce slightly different code: the stock Felucca 1.5.1 ISR
  already measures 38 871 against its 37 502 budget line with toolchain 20260730.
- **FPU:** the chip has one: JieLi's SDK builds for WL82 with `-mcpu=r3 -mfprev1`. Felucca builds without (soft float,
  and its code is integer only). `__attribute__((target("fprev1")))` turns it on for one function (every float helper it
  calls needs the attribute too, or that helper is not inlined and runs as soft float). Measured on a unit (MENU >
  SYSTEM > INFO's FPU probe): usable after Felucca's start-up, a float multiply-add ~34 ns, a divide ~31 ns; WSHAPE
  HRMNC, PRTCL, WTABLE and SWARM (`wsh_model`, `har_model`, `prt_model`, `wtb_model`, `swm_model`) run on it (docs/PLAITS_PLAN.md); a WSHAPE voice measured ~6.3 % of the audio's time. objdump shows its 4-byte
  instructions as `3f e5` / `3f f5 <unknown instruction>` and decodes their second half as an instruction of its own,
  at times a branch (tests/target_budget.py counts the pair once); `clang -S` prints them (`r0 = r0 * r1 (f)`, `ftoi`, `itof`).
- **SDK:** only three files of `fw-AC79_AIoT_SDK` (Apache-2.0) go into each package: `cpu/wl82/tools/uboot.boot`,
  `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`. The firmware itself does not link the SDK.

### SRAM (512 KB, 0x01C00000 .. 0x01C80000), from app.ld

| Region | Address | Size | What |
| --- | --- | --- | --- |
| RAMTEXT | 0x01C00000 | 24 KB | code that runs while the flash is busy (flash driver) |
| (debug) | 0x01C06000 | 8 KB | kept free for RAM-run debugging |
| RAM | 0x01C08000 | 50 KB | `.data` + `.bss`: what the ISRs touch, small variables |
| POOL | 0x01C14800 | 382 KB | `.pool`: big zero-initialised buffers (FX lines, canvases, engine memory, the song's sources) |
| stacks | 0x01C74000 .. 0x01C7C000 | 32 KB | user and system stacks with 256-byte guard bands |
| NOINIT | 0x01C7C000 | ~15 KB | survives resets and boot-mode entry (panel calibration, boot guard) |
| reserved | 0x01C7FD50 .. | | boot_info, boot-state mailbox, vector table |

The RAM / POOL split is only a budget line in the linker script; both are the same SRAM. It moved twice: Felucca 1.5
96 / 336 KB -> 72 / 360 KB; M-Vave-OP1's 128 steps 72 / 360 -> 56 / 376 KB (the song chain's four sources, 25 KB,
went to the pool); back to 64 steps with the SHIMMER bus and the grouped undo: 50 / 382 KB.

### Budgets (the second core's structure, docs/DUALCORE_PLAN.md step 2; 2026-11-02)

| | Used | Limit | Note |
| --- | --- | --- | --- |
| RAM `.data`+`.bss` | 42 624 B | 51 200 B | before 44 528: the mix sums, SPREAD and Plaits buffers moved to the pool |
| Pool | 377 500 B | 391 168 B | before 373 652 (`mx[2]` 1 800, `plv_w` 1 536, `spread_buf` 512) |
| App image (XIP) | 540 820 B | 581 564 B | before 541 380 |
| Audio ISR static cost `fm1_alnk0_irq` | 37 478 | 44 810 | `tests/target_budget.txt` 40 736 + 10 % |

### Budgets (64 steps, SHIMMER bus, grouped undo, PUMPER; 2026-10-27)

| | Used | Limit | Note |
| --- | --- | --- | --- |
| RAM `.data`+`.bss` | 41 912 B | 51 200 B | build.py demands >= 4 096 B free |
| Pool | 377 836 B | 391 168 B | build.py demands >= 8 192 B free |
| App image (XIP) | 531 436 B | 581 564 B | |
| Audio ISR static cost `fm1_alnk0_irq` | 40 412 | 41 252 | 37 502 + 10 % |

What the 128 steps cost while they were in (27 KB in all): `trk[]` 2 528 B a track (+768), the song chain's sources (`chain`, pool) 25 992 B (~6.5 KB a
pattern), the project's work copy `proj_scratch` 7.7 KB, the extras' staging `xtra_buf` 3 KB; the extras' tail
(steps 65..128, opstore.c `xtra_hi_t`) is read from the flash when needed, not kept with `xtra_slot`.

### Budgets (measured 2026-10, after M-Vave-OP1's step 4; toolchain 20260730)

| | Used | Limit | Note |
| --- | --- | --- | --- |
| RAM `.data`+`.bss` | 52 152 B | 73 728 B | before step 4 (PHASE VOICE TRIO SLICE DBOX retired): 56 616; WSHAPE +384 |
| Pool | 339 548 B | 368 640 B | build.py demands ≥ 8 192 B free |
| App image (XIP) | 576 648 B | 581 564 B | **4 916 B free**; before step 4: 569 516 (the FPU probe and its Plaits tables in it: 12 192 B); WSHAPE +1 904, HRMNC +2 228, PRTCL +5 704, WTABLE +30 688 (25.5 KB of 8-bit tables), SWARM +2 860, SCL > CSET +8 876 (from claude/cset-chords) |
| Audio ISR static cost `fm1_alnk0_irq` | 37 802 | 41 252 | 37 502 + 10 % (tests/target_budget.py) |
| Key-scan ISR `fm1_timer5_irq` | 33 887 | 36 725 | |

What things cost (measured with `nm` on the target build): a synth track ~23 KB (its `trk[]` entry 1 824 B, engine
memory 12 888 B, SLICER buffer 8 KB), an FX TRACK lane 66 B (130 B with 128 steps), a song pattern in RAM (`chain_pattern_t`) ~3.4 KB, the
scene bank (15 scenes packed, scene.c `scb`) 7.9 KB of pool.

Run `scripts/budget_report.sh` to get the current numbers.

### Flash (1 MB SPI NOR, XIP at 0x02000000)

| Range | What |
| --- | --- |
| 0x00000 .. ~0x96FFF | boot code, app image |
| 0x97000 .. 0x9EFFF | projects A–D (two 4 KB copies each) |
| 0x9F000 | FM6 patches of the user presets, copy A |
| 0xA0000 .. 0xB3FFF | user sample slot USR1 (80 KB, ADPCM, ~7 s at 22.05 kHz) |
| 0xB4000 .. 0xC7FFF | M-Vave-OP1 (opstore.c): the scenes, set A..D + autosave, two 8 KB copies each (commit record `OBJ_SCENES`) |
| 0xC8000 .. 0xCFFFF | M-Vave-OP1 (opstore.c): the projects' extras A/B (`OBJ_XPROJ0 + slot`: the FX TRACKS, steps 65..128) |
| 0xD0000 .. 0xD1FFF | the autosave's extras A/B (`OBJ_XAUTO`) |
| 0xD2000 .. 0xDBFFF | free since M6 (the dropped TAPE kept its copies A/B here) |
| 0xDC000 .. 0xDFFFF | user preset banks |
| 0xE0000 .. 0xE4FFF | OTA staging (the update loader) |
| 0xE5000 .. 0xE6FFF | autosave A/B (RESTORE LAST) |
| 0xE7000 | FM6 voice bank |
| 0xE8000 | the stock OTA's VM backup: never written |
| 0xFC000 .. 0xFFFFF | settings, FM6 patches copy B (0xFE000) |

Every sector is spoken for. USR3 and USR2 (the third and second sample slots) were given up for notes and settings:
the editor uploads into USR1 only (`SMP_USER_WRITABLE` 1), USR2 and USR3 read empty. New persistent data means packing
it into the extras (`proj_xtra_t`, append-only by its `size` field) or the scene sets' spare bytes, or giving up USR1
too. Flash is erased and written only while the transport is stopped: an erase stalls XIP.

## Panel

- **Keys:** 27, F3 .. G5 (key k = k semitones above F3); 16 white, 11 black. One 11-column × 6-row diode matrix
  behind two 74HC595 (PA4 SER, PA3 SRCLK, PA1 RCLK), scanned by a ~10 kHz TIMER5 ISR, asymmetric debounce.
- **Buttons (14):** FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT− OCT+ (`panel.c` B_*; the matrix ids are
  mapped through `panel_t`, recalibrated by holding OCT− and OCT+ at power-on).
- **Encoders (7):** SELECT, ALGORITHM, PRESETS, KNOB 1..4 (`EN_*`); one detent = one quadrature cycle.
- **MASTER:** a potentiometer on the ADC (0..1023), read before the audio starts.
- **LEDs:** one per button and key, on the matrix columns; levels dark, dim glow, breath, mid, lit
  (`fm1_led`, `fm1_led_dim`, `fm1_led_breath`, `fm1_led_mid`).

## Display, audio, I/O

- **Display:** ST7789-class 240 × 240 RGB565 on SPI1 at 12 MHz with DMA (PC9 CLK, PC10 DO, PC7 CS, PC8 D/C,
  backlight PA2 active low, which is also the key matrix's /OE). A full 240 × 122 panel blit is ~53 KB, ~36 ms:
  redraw only what changed.
- **Audio:** I²S at 44.1 kHz, half buffers of 128 frames (2.9 ms), rendered in blocks of 32 (`CTL`).
- **USB:** MIDI, USB audio *to the host* (the master output, 44.1 / 48 kHz), CDC serial console (optional).
- **MIDI:** TRS MIDI IN on UART1 (PH8).
- **No audio input** is used by any known firmware: no mic, line-in or radio sampling as on the OP-1.
- **Battery:** ADC channel 3 through a divider.

## Packages and identity

`build.py` writes `build/felucca.fwsc`: app + loader + the three SDK files, identity `FM-1_900` (development) or
`FM-1_910` with `./build.sh --release X.Y`. The installers check the identity; keep it unless the user decides
otherwise.
