# M-Vave-OP1 roadmap

An OP-1-style workflow on the M-VAVE FM-1, built on Felucca. This page records what the hardware allows,
how OP-1 ideas map onto it, and the order of work.

## What the hardware allows

| | FM-1 | Used by Felucca 1.5.1 |
| --- | --- | --- |
| CPU | JieLi AC791N / WL82, pi32v2 | the audio ISR renders 4 tracks, 8 voices, FX |
| RAM | ~512 KiB SRAM | 72 KiB static (55 KiB used; M-Vave-OP1 56 KiB) + 360 KiB pool (336 KiB used) |
| Flash | 1 MiB SPI NOR | ~570 KiB firmware, the rest: projects, presets, user sample slots (M-Vave-OP1: 2 of 3; the third holds the tape and the FX TRACKS), OTA staging |
| Display | 240 x 240 ST7789, SPI | |
| Controls | 27 keys, 14 buttons, 7 encoders | |
| I/O | stereo out, speaker, USB-C (MIDI, audio in to the host), TRS MIDI in | |

The OP-1 records **audio** to tape: 4 tracks, 6 minutes. One second of 16-bit mono audio is 86 KiB, so the
FM-1 cannot hold even ten seconds of it. The tape here records **notes** instead, with their exact timing,
and plays them back through the engines. Tape tricks that work on time and pitch (loop, varispeed,
reverse, cut and paste) still work. Tricks that work on recorded sound (bouncing, resampling the tape
into a sampler, album mode) do not.

## OP-1 ideas on the FM-1

| OP-1 | M-Vave-OP1 | Status |
| --- | --- | --- |
| Synth engines | Felucca's engines: FM6 (DX7-style), ANALOG, PHASE, LOFI, FORMANT, TRIO, WHEEL, GRAIN, PHYS, NOISE, SAMPLE, SLICE | from Felucca |
| Drum sampler | DRUM engine, SLICE, SAMPLE with user slots | from Felucca |
| Tape: 4 tracks | **TAPE**: 4-track note tape, 128 bars (4:16 at 120 BPM), ~2000 notes | **done (M1)** |
| Tape loop | LOOP 1..32 bars from the bar the head is in, overdub every pass | **done (M1)** |
| Tape speed | SPEED -12..+12 semitones: tempo and pitch together | **done (M1)** |
| Tape reverse | DIR REV, also while recording | **done (M1)** |
| Tape kept when switched off | one tape, as the OP-1's, in USR3's flash (two copies) | **done (M2)** |
| Lift / drop / split | cut, copy and paste a loop region of the tape | planned (M3) |
| Mixer | HOME > MIXER (level, pan, reverb send, mute) | from Felucca |
| Sequencers | step sequencer with chance, ratchets, locks; arpeggiator | from Felucca; Endless and Tombola planned (M5) |
| Frequency modulators | LFO with sync, modulation matrix | from Felucca |
| Playful screens | the TAPE cassette with turning reels | started (M1); per-engine screens planned (M4) |
| Album / bounce | not possible without audio memory | out of scope |

And from the OP-Z (M2b):

| OP-Z | M-Vave-OP1 | Status |
| --- | --- | --- |
| FX1 / FX2 tracks | **FX TRACKS**: two lanes of the FX layer's effects, 64 steps of a 1/16, own lengths | **done (M2b)** |
| Punch-in FX, recorded with REC | REC arms a lane, FX held + keys record (quantised, held on, overdub, punch) | **done (M2b)** |
| Pattern chains, cue the next pattern | SONG: REC + A..D types a chain, LOOP, changes while it plays | **done (M2b)** |
| 16 patterns a project, switched with a key | 4 patterns (the project slots A..D) | planned (M2c) |
| Punch-ins DUCK, RAMP, RANDOMIZE, STEREO .. | not yet | planned (M2c) |

## Milestones

### M1: TAPE (done)

- `firmware/src/tape.c`: the tape. One gap buffer of 4096 events (16 KiB) for all four tracks, so recording
  and playback cost O(1) in the audio ISR. Measured on the target build: +1.8 % static cost in the audio ISR.
- GLO > **TAPE**: KNOB 1 POS, KNOB 2 LOOP, KNOB 3 SPEED, KNOB 4 DIR. REC arms the tape for the selected
  track (the REC LED shows the tape's arming on these pages).
- GLO > **TAPE 2**: ERASE the selected track's notes, or the whole tape (OCT+ asks first). NOTES and USED.
- PLAY runs the tape with the sequencer, from where the head stands; GLO + PLAY (RESTART) starts it from the
  loop's start, or the tape's. A count-in (MENU) also counts in a tape recording.
- `tests/tape_test.c`: 62 checks for recording, playback, overdub, loop, speed, reverse, erase, a full tape,
  the tape's end and the pages.

### M2: keep the tape (done)

Decided: notes and settings over a third sample slot. `firmware/src/opstore.c` uses the flash of the third user
sample slot (USR3, 0xC8000..0xDBFFF, 80 KiB):

- 0xD2000..0xDBFFF: **one tape**, as the OP-1's, in two copies of five sectors that take turns (its own commit
  record after the events: a write cut short leaves the copy before). Written 3 s after the tape changed, with the
  transport stopped and nothing sounding (an erase silences the audio for ~40 ms); read back at power-on, not after
  a crash. While it is written the audio ISR leaves the tape alone (`tape.hold`).
- 0xC8000..0xD1FFF: each project slot's **extras** and the autosave's (the FX TRACKS, see M2b), A/B, tagged with
  the CRC of the project they belong to: a project written another way (the editor's backups) gets empty lanes.
- The web editor uploads samples into USR1 and USR2 only; USR3 reads as an empty slot.

### M2b: OP-Z: FX TRACKS, punch-ins, pattern chains (done)

- `firmware/src/fxtrack.c`: two lanes of 64 steps of a 1/16, each step an effect of the FX layer (or a track's mute)
  or a TIE. Six equal tracks would need ~52 KiB more RAM than there is; a lane costs 66 bytes. The lanes count steps
  as a 1/16 track does (no drift), a lane's REPEAT starts on its step. FX > FX TRACKS, FX TRACKS 2.
- Recording: REC arms a lane, the FX layer's presses go onto it (early in a step onto it, late onto the next, held
  on as TIEs), overdub, a punch inside a take gives the rest back, FX LATCH and the mutes too.
- SONG: `CHAIN_LOOP` in the stored chain's reserved byte (every older song stops at its end as before), REC + A..D
  types a chain, live hand-over at section ends (`chain_live`, `chain_next_row`), all four slots copied at PLAY,
  each section's FX TRACKS from its slot.
- Tests: `tests/fxtrack_test.c`, `tests/songchain_test.c`, `tests/opstore_test.c`; `editor_test.c`,
  `storage_test.c`, `ui_test.c` follow.

### M2c: OP-Z patterns (next)

16 patterns in a project, switched with a key and edited while they play, as the OP-Z's (today a pattern is a
project slot, A..D). Needs ~3.5 KiB of flash per pattern (USR2 or a second extras sector per slot) and a RAM
double buffer for the pattern that comes next. More punch-in effects of the OP-Z's: DUCK, RAMP UP, RANDOMIZE,
STEREO.

### E1..E5: effects (E1 done)

- **E1 (done):** FX1 (the delay bus) gets a TYPE: DELAY or **CLOUD**, a granular cloud in the delay's own line
  (`firmware/src/fx1.c`): up to 8 grains, SIZE DENS PITCH SPREAD, RAND FDBK MIX, CLOUD FREEZE on the FX layer's F5,
  kept in the project extras. `tests/cloud_test.c` with WAV demos.
- Next: OP-Z punch-ins (DUCK, STEREO, PITCH, RAMP UP, GLITCH), new track INSERTs (RING, SHIFT, COMB, VOWEL, WOW), more
  FX1 types (SHIMMER, REVERSE, TAPE ECHO). All on buffers that exist already. The plan, in German:
  [EFFEKTE_PLAN.md](EFFEKTE_PLAN.md).

### M3: tape editing

LIFT (cut the loop region to a clipboard), DROP (paste it at the head), a tape track's mute and level,
merge one tape track into another, quantise a tape track to the grid.

### M4: OP-1 look

A palette in the OP-1's spirit, a playful screen per engine (the FM6 algorithm as a moving diagram, the drum
kit as pads that light up), the tape reels on HOME.

### M5: more sequencers

Endless (a sequence that keeps growing as you play), Tombola (notes bouncing in a turning drum).

## Safety

Nothing here has been flashed to hardware yet. The FM-1 keeps one firmware copy and has no recovery button.
Develop with the host tests (`tests/run_tests.sh`) and the browser emulator (`web/emu/build.sh`) first.
Felucca's installer can return the unit to the official V15 firmware as long as the firmware still starts.
A unit that no longer starts needs FM-1 Transporter and an RP2040 board to recover.
