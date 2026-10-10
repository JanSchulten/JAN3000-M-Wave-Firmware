# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1-style workflow on top: a four-track tape, and from the
OP-Z two FX tracks with recordable punch-in effects and pattern chains. What is planned next is in the
[roadmap](docs/ROADMAP.md).

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
- The tape is kept: one tape, as on the OP-1, written to flash a few seconds after it changed (stopped and quiet)
  and back after power-on. It is not part of a project.

## FX TRACKS

Two lanes of effects next to the four instrument tracks, after the OP-Z's FX tracks. Two more synth tracks do not
fit the FM-1's RAM; an effect lane costs 66 bytes. The lanes play the FX layer's effects on the master (REPEAT
1/8 1/16 1/32, REVERSE, LPF, HPF, TAPE STOP, FREEZE, OCT UP, OCT DN, FLANGER, PHASER, REV THROW, ECHO THROW, and
the mutes of tracks 1 to 4).

- **FX** opens FX; tap again for **FX TRACKS**, again for **FX TRACKS 2**.
- **KNOB 1** picks the lane (FX1, FX2), **KNOB 2** the step, **KNOB 3** the step's effect (OFF, TIE to hold the effect
  before it on, the effects, the mutes), **KNOB 4** the lane's length (1 to 64 steps of a 1/16). Two lengths drift
  against each other, as the OP-Z's tracks do.
- **Punch-ins, recorded:** REC on FX TRACKS arms the selected lane (stopped: PLAY starts). Hold **FX** and press the
  white keys (effects) or the first four black keys (mutes): what you play goes onto the lane, quantised to the
  1/16, held on as TIEs. Nothing held writes nothing (overdub); a punch inside an older take gives the rest of it
  back. FX LATCH records too.
- **FX TRACKS 2:** KNOB 1 CLEAR the selected lane, KNOB 2 both; OCT+ asks, OCT+ again clears.
- The lanes are saved with the project (SAVE > PROJECT) and with the autosave.

## SONG: pattern chains

GLO > SONG chains the patterns of the project slots A to D (each slot's sequences and FX TRACKS, played with the
current sounds), as the OP-Z chains its patterns.

- **REC** on SONG starts a new chain; the keys **A3 B3 C4 D4** are A B C D. The same key again plays that pattern once
  more, another key adds it after. REC again (or another page) ends the typing.
- A chain typed this way **loops**. KNOB 1 on the last cell (+ ADD), then **KNOB 4 END**: LOOP or STOP.
- **While it plays:** a new chain starts at the end of the pattern playing now; an added pattern plays after the
  ones before it; a cell changed with the knobs plays when its section comes. Nothing has to stop first.
- Each section plays the FX TRACKS saved with its slot, from their first step.

## Try it in the browser

`web/lab` is an emulator page with guided workflows (in German): the whole firmware as WebAssembly, with a
JavaScript fallback. Build it with `sh web/lab/build.sh` (after `./build.sh`, with the emsdk), then
`cd build/lab && python3 -m http.server 8000` and open `http://localhost:8000/local.html`.

## Flash: one sample slot less

The tape and the FX TRACKS need flash that Felucca's project format does not have. They live in the flash of the
third user sample slot (USR3, 80 KiB): the web editor uploads samples into USR1 and USR2 only. Felucca projects
load as before (with empty FX TRACKS).

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
