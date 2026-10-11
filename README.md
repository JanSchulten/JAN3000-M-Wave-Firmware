# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1-style workflow on top: a four-track tape, and from the
OP-Z two FX tracks with recordable punch-in effects, the OP-Z's punch-ins on a second key map, pattern chains and a
switchable FX1 with a granular CLOUD. What is planned next is in the
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
1/8 1/16 1/32, REVERSE, LPF, HPF, TAPE STOP, FREEZE, OCT UP, OCT DN, FLANGER, PHASER, REV THROW, ECHO THROW, CLOUD
FREEZE, the OP-Z punch-ins below, and the mutes of tracks 1 to 4).

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

## FX1: CLOUD (granular)

The delay bus is FX1, as on the OP-Z: FX > **FX1**, KNOB 1 picks its TYPE: **DELAY** (as before) or **CLOUD**, a granular
cloud after Mutable Instruments' Clouds. It lives in the delay's own 1.5 s line, so it needs no extra RAM. Tracks send
into it with their **DLY** (FX page, KNOB 3).

- **FX1**: KNOB 1 TYPE; with CLOUD KNOB 2 **RAND** (grains detuned at random, up to half of them backwards), KNOB 3
  **FDBK** (the cloud back into itself: with PITCH every pass climbs, a shimmer), KNOB 4 **MIX**.
- **CLOUD** (the page after FX1, in place of DLY): KNOB 1 **SIZE** (10 ms to 1 s), KNOB 2 **DENS** (0.5 to 64 grains a
  second), KNOB 3 **PITCH** (-24 to +24 semitones), KNOB 4 **SPREAD** (how far back the grains come from, how wide).
- **CLOUD FREEZE**: hold FX and press **F5**: nothing new goes in, the cloud plays on in what it holds. It records onto
  the FX TRACKS like every effect of the layer. ECHO THROW throws into the cloud.
- The panel shows the line (1.5 s, now at the right) and each grain where it reads.
- Kept with the project and the autosave. Changing the TYPE fades the old one out and starts the new one empty.

## OP-Z punch-ins: FX MAP B

Hold **FX** and turn **ALGORITHM** right: the white keys play **MAP B**, the OP-Z's punch-ins in the OP-Z's order
(the header says HOLD B; left: MAP A again). A key held while you turn takes the new map's effect at once.

| Key | MAP B | What it does |
| --- | --- | --- |
| F3 | **DUCK** | the master pumps in quarters, as under a kick's sidechain (-20 dB on the beat, back over three quarters) |
| G3 | LPF | filter sweep |
| A3 | REPEAT 1/16 | loop 1 |
| B3 | **STEREO** | the mix panned left and right, a 1/16 each side |
| C4 | REPEAT 1/32 | loop 2 |
| D4 | **PITCH 5TH UP** | the whole mix a fifth higher (the shifted sound alone) |
| E4 | ECHO THROW | follow / echo |
| F4 | **RAMP UP** | the tape stops dead, spins up over a beat and lands in time |
| G4 | **PITCH 4TH DN** | a fourth lower |
| A4 | REVERSE | |
| B4 | TAPE STOP | ramp down |
| C5 | **GLITCH** | from the next 1/16, each 1/16 a random slice: the last 1/32 or 1/64 repeated, the last 1/16 again, backwards, an octave down or up, silence, or the live mix |
| D5 E5 F5 G5 | HPF, REV THROW, CLOUD FREEZE, FREEZE | |

- Any effect on any key, in either map: hold FX and the key, turn PRESETS (EDIT: the key's default). Both maps are
  kept with the settings.
- **KNOB 4 DEPTH** sets how deep DUCK, STEREO and PITCH go, as for the buffer effects.
- PITCH, RAMP UP and GLITCH are buffer effects like REPEAT: the last pressed plays. They all record onto the FX
  TRACKS.

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
- The plan for new effects (granular CLOUD and the OP-Z punch-ins done, track inserts next; German):
  [docs/EFFEKTE_PLAN.md](docs/EFFEKTE_PLAN.md)
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
