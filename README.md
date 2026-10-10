# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1-style workflow on top, starting with a
four-track tape. What is planned next is in the [roadmap](docs/ROADMAP.md).

> **Status:** early development. Nothing here has been flashed to hardware yet.
> Installing custom firmware is at your own risk. The FM-1 has a single firmware bank and no
> recovery button; if an install fails and the unit will not start, recovery needs extra
> hardware (FM-1 Transporter with an RP2040). Develop and try changes in the host tests and the
> browser emulator first.

## TAPE

A four-track tape in the spirit of the OP-1's. The FM-1 has far too little memory for audio, so the tape
records the **notes** you play, at their exact time, and plays them back through each track's engine.
Change a track's sound later and the take plays with the new sound.

- **GLO** opens **TAPE** (tap GLO again: **TAPE 2**, then SONG).
- **REC** on these pages arms the tape for the selected track (ALGORITHM selects the track). Armed while
  stopped, it starts PLAY (MENU's count-in counts in first, if set). Play the keys, MIDI IN or the arp.
- **PLAY** runs the tape with the sequencer from where the head stands; STOP leaves the head there.
  **GLO + PLAY** starts from the loop's start, or the tape's.
- **KNOB 1 POS** moves the head, a beat a detent (turned fast: a bar). **KNOB 2 LOOP** loops 1 to 32 bars from
  the bar the head is in; recording overdubs every pass. **KNOB 3 SPEED** is varispeed, tempo and pitch
  together (-12 to +12 semitones). **KNOB 4 DIR** plays and records backwards.
- **TAPE 2**: KNOB 1 picks ERASE for the selected track, KNOB 2 ERASE ALL; OCT+ asks, OCT+ again erases.
  NOTES and USED show what the tape holds.
- Length: 128 bars (4:16 at 120 BPM), about 2000 notes over all four tracks.
- The tape is not saved yet: it is lost when the FM-1 is switched off (see the roadmap, M2).

## Try it in the browser

`web/lab` is an emulator page with guided workflows (in German): the whole firmware as WebAssembly, with a
JavaScript fallback. Build it with `sh web/lab/build.sh` (after `./build.sh`, with the emsdk), then
`cd build/lab && python3 -m http.server 8000` and open `http://localhost:8000/local.html`.

## Documents

- Roadmap: [docs/ROADMAP.md](docs/ROADMAP.md)
- OP-1 and FM-1 compared, with the step-by-step plan (German): [docs/OP1_COMPARISON.md](docs/OP1_COMPARISON.md)
- The OP-1 manual 1.2 as Markdown (personal reference, © Teenage Engineering, do not redistribute):
  [docs/reference/op1-manual.md](docs/reference/op1-manual.md)
- Claude Code skill for FM-1 firmware work: [.claude/skills/fm1-firmware](.claude/skills/fm1-firmware/SKILL.md)
- Build: [BUILDING.md](BUILDING.md) (inherited from Felucca)
- Felucca's own README (feature list, controls, history): [docs/FELUCCA_README.md](docs/FELUCCA_README.md)

## Upstream

Felucca's history is merged into this repository, so upstream changes can be pulled in with:

```
git remote add upstream https://github.com/hugelton/Felucca.git
git fetch upstream
git merge upstream/main
```

## Licence

GPL-3.0-only, like Felucca. Bundled and ported parts keep their own licences; see
[LICENSING.md](LICENSING.md) and `LICENSES/`. If you distribute this firmware, you must also
offer its complete corresponding source under the same licence.
