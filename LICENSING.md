# Felucca licensing

Felucca is free software, licensed under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). That covers the code and its own assets. A few
bundled or ported parts keep their own licences; they are listed below, and their licence
texts are in `LICENSES/`.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments

## What is GPL-3.0-only

Every file in this tree that carries an `SPDX-License-Identifier: GPL-3.0-only` header:

- the firmware: `firmware/` (app, HAL, update loader)
- the build script and tools: `build.sh`, `tools/`
- the web pages (installer, editor, the browser emulator `web/emu/`) and their tests: `web/` (not the Fukiai and DotGothic16 fonts, below)
- the host tests: `tests/`

Eleven source files are ports and keep the licence of their originals:
`firmware/src/phys_dsp.c` (DaisySP, MIT), `firmware/src/phys_symp.c` (Rings, MIT),
`firmware/src/plaits_ws.c`, `firmware/src/plaits_add.c`, `firmware/src/plaits_part.c`, `firmware/src/plaits_wt.c`,
`firmware/src/plaits_swarm.c`, `firmware/src/plaits_tables.h`, `firmware/src/plaits_wt8.h` and
`firmware/src/stmlib_tables.h` (Plaits and stmlib, MIT) and
`firmware/src/fm6_core.c` (msfa, Apache-2.0). Three files of the hardware layer come from felucca-bsp and keep its
MPL-2.0: `firmware/hal/fm1_cpu1.h`, `firmware/hal/fm1_cpu1.S` and `firmware/hal/fm1_cpu1_stop.h` (the second core). The
firmware built with them is GPL-3.0-only as a whole.

You may use, study, change and share Felucca under the GPL. If you distribute Felucca, or
firmware derived from it, you must also give your recipients its complete corresponding
source under the same licence. That includes devices that ship with modified Felucca inside.

## Hügelton Instruments' own work

All by Hügelton Instruments (Leo Kuroshita), in this tree:

| What | Licence | Where |
| --- | --- | --- |
| Felucca: firmware, tools, web pages, tests | GPL-3.0-only | `LICENSE` |
| The DRUM voices and kit | GPL-3.0-only | `firmware/src/drum_voice.c`, `firmware/src/eng_drum.c` |
| The Hügelton Sample Pack: Felucca's drum sounds, made by `tools/gen_waves.py` (not CC0) | GPL-3.0-only | `tools/gen_waves.py` |
| The controls picture | GPL-3.0-only | `docs/controls.jpg` |
| The Fukiai icon font (<https://github.com/hugelton/Fukiai>): the firmware's icons (rasterised at build time by `tools/gen_aa_icons.py`) and the web editor's | MIT | `web/fukiai.ttf`, `LICENSES/MIT-Fukiai.txt`, `web/FUKIAI-LICENSE.txt` |

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Inter Tight font by The Inter Project Authors: the UI text, rasterised into the firmware at build time (`tools/gen_aa_font.py`; the generated tables are not offered as a font, and the font declares no Reserved Font Name) | SIL OFL 1.1 | `assets/fonts/InterTight[wght].ttf`, `LICENSES/OFL-InterTight.txt` (also `assets/fonts/OFL.txt`) |
| Instrument samples (Versilian Studios VSCO-2 Community Edition, VCSL): the SAMPLE sets | CC0 1.0 | `assets/samples-cc0/`, provenance in `ATTRIBUTION.txt` there |
| DaisySP by Electrosmith, Corp and Emilie Gillet (<https://github.com/electro-smith/DaisySP>): the PHYS engine's modal and string models and the resonator, ported to fixed point | MIT | `firmware/src/phys_dsp.c`, `LICENSES/MIT-DaisySP.txt` |
| Rings by Emilie Gillet (<https://github.com/pichenettes/eurorack>): the PHYS engine's sympathetic strings, ported to fixed point | MIT | `firmware/src/phys_symp.c`, `LICENSES/MIT-Rings.txt` |
| Plaits and stmlib by Emilie Gillet (<https://github.com/pichenettes/eurorack>), M-Vave-OP1: the waveshaping, additive, particle, wavetable and swarm models, the diffuser and their tables (the wavetables in 8 bits; stmlib's pitch ratios too), ported to C for the FPU (the WSHAPE, HRMNC, PRTCL, WTABLE and SWARM engines; the FPU probe, MENU > SYSTEM > INFO) | MIT | `firmware/src/plaits_ws.c`, `firmware/src/plaits_add.c`, `firmware/src/plaits_part.c`, `firmware/src/plaits_wt.c`, `firmware/src/plaits_swarm.c`, `firmware/src/plaits_tables.h`, `firmware/src/plaits_wt8.h`, `firmware/src/stmlib_tables.h`, `LICENSES/MIT-Plaits.txt` |
| msfa by Google Inc. and Pascal Gauthier, from Dexed (<https://github.com/asb2m10/dexed>): the FM6 engine's synthesis, ported to integer C (Dexed itself is GPL-3.0; only msfa is used; the FM6 factory patches are Felucca's own) | Apache-2.0 | `firmware/src/fm6_core.c`, `LICENSES/Apache-2.0-msfa.txt` |
| SCL > CSET's chord sets (M-Vave-OP1): the notes and names of the 100 chord sets listed in the Roland J-6 owner's manual (version 1.02, Chord Set List), transcribed as note data by `tools/gen_cset.py`; Roland and J-6 are Roland Corporation's trademarks, named only as the source | (note data; the code GPL-3.0-only) | `firmware/src/cset_data.h` |
| Open303 by Robin Schmidt (<https://github.com/RobinSchmidt/Open303>): the structure of the ACID engine's diode ladder (four coupled one-pole stages, the feedback from the last), credited as a design; no code taken, the coefficients are this firmware's own | MIT (credited only) | `firmware/src/eng_acid.c` |
| X0X by charlesvestal (<https://github.com/charlesvestal/fm1-x0x>), a Felucca fork: the design of the browser emulator (the worklet, the device clock driven by its audio, the exports); its files credit it in their headers | GPL-3.0 | `web/emu/` |
| Felucca 1.6.1's second core (<https://github.com/hugelton/Felucca>, PR #247, by gospodindilly-ops) after Jangada by zednaked (<https://github.com/zednaked/jangada>), M-Vave-OP1: the design of the parts' render split (fx.c `mix_block`: the sums per core, the split, the LFO's S&H after the parts) and its host test with a thread as CPU1, ported by hand; the test files keep their authors' copyright lines (docs/DUALCORE_PLAN.md) | GPL-3.0 | `firmware/src/fx.c`, `tests/c1_host.h`, `tests/dualcore_test.c` |
| felucca-bsp by Leo Kuroshita, with Melodee's second core by Kerem Kilic (<https://github.com/hugelton/felucca-bsp>, <https://github.com/keremimo/melodee>; MPL-2.0 by agreement, felucca-bsp issue 5) and parts by Charles Vestal, M-Vave-OP1: the second core's start, its mailbox with the check word, its fault path and its safe hold, taken as files (the stack moved into the pool); their fault hook in `firmware/hal/fm1_irq.h`, the resume in `firmware/hal/fm1_vec.S` and the hold before every reset in `firmware/hal/fm1_sys.h` follow felucca-bsp's and say so in their headers. The units whose second core misreads shared RAM were measured by Jangada's maintainer (zednaked) | MPL-2.0 | `firmware/hal/fm1_cpu1.h`, `firmware/hal/fm1_cpu1.S`, `firmware/hal/fm1_cpu1_stop.h`, `LICENSES/MPL-2.0.txt` |
| DotGothic16 font by The DotGothic16 Project Authors (<https://github.com/fontworks-fonts/DotGothic16>): the browser emulator's knob labels, served with its page (cut to printable ASCII; the font declares no Reserved Font Name) | SIL OFL 1.1 | `web/emu/fonts/DotGothic16-subset.woff`, `LICENSES/OFL-DotGothic16.txt` (also `web/emu/fonts/OFL.txt`) |
| graphify by Safi Shamsi and the Graphify contributors (<https://github.com/Graphify-Labs/graphify>), M-Vave-OP1: its Claude Code skill, as `graphify install --project` writes it (a development aid for Claude's code graph: CLAUDE.md; nothing of it is in the firmware or the web pages) | Apache-2.0 (parts also MIT) | `.claude/skills/graphify/` (with its `LICENSE`, `LICENSE-MIT` and `NOTICE`) |
| JieLi AC79 SDK by JieLi Technology: three of its files go into every `.fwsc` package (below); none are in this tree | Apache-2.0 | `LICENSES/Apache-2.0.txt` |

On the device, HOME held > ABOUT opens the information screen; turning PRESETS scrolls on into
CREDITS, a short list of these authors, licences and source URLs.

## JieLi SDK files in the packages (Apache-2.0)

A `.fwsc` package made by `tools/build.py` (with `tools/fm1pkg_make.py`; this is the package the
web installer installs) holds three unmodified files from the JieLi AC79 SDK
(<https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK>, `cpu/wl82/tools/`). They are read from your SDK
checkout at build time (see BUILDING.md); no SDK files are in this tree.

| File in the package | What it is |
| --- | --- |
| `uboot.boot` | the first-stage boot loader (SPL) |
| `cfg_tool.bin` | the chip configuration block |
| `eq_cfg_hw.bin` | the default hardware EQ table |

They are licensed under the Apache License, Version 2.0 (`LICENSES/Apache-2.0.txt`), not under
the GPL, and their copyright stays with JieLi Technology. The SDK has no NOTICE file. Apache-2.0
files may be distributed together with GPL-3.0 code; Felucca itself stays GPL-3.0-only.

Every distribution of a package carries the licence texts with it: the release folder
(`tools/build.py --release`) and the site (`web/make_site.py`, next to the package in
`firmware/` and linked from the installer page).

## No vendor material in the firmware

No M-VAVE material is part of the firmware. It links no vendor code (in particular not M-VAVE's
binary FM1 SDK) and contains no vendor data, and the packages hold only this firmware and the
three SDK files above.

M-Vave-OP1: the installers can return an FM-1 to M-VAVE's official V15 without a separate
download. `vendor/m-vave/FM-1_015.fwsc` is that file, unmodified, from M-VAVE's FM1 Open
development kit, where M-VAVE distributes it under Apache-2.0 (`vendor/m-vave/README.md`,
`LICENSES/Apache-2.0.txt`). The web installer serves it next to the package (`firmware/`) and
checks its size and SHA-256 before using it; a file you downloaded yourself can still be
selected instead. It is never part of a package.

## Contributions

Contributions are welcome under GPL-3.0-only.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments.

"M-VAVE" and "FM-1" are trademarks of their respective owners. Felucca is independent
firmware that runs on FM-1 hardware. It is not affiliated with, endorsed by or supported
by those owners.

## Radio

Felucca never enables the Bluetooth / Wi-Fi radio of the hardware.
