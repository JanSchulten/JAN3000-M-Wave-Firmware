# M-Vave-OP1 roadmap

An OP-1-style workflow on the M-VAVE FM-1, built on Felucca. This page records what the hardware allows,
how OP-1 ideas map onto it, and the order of work.

## What the hardware allows

| | FM-1 | Used by Felucca 1.5.1 |
| --- | --- | --- |
| CPU | JieLi AC791N / WL82, pi32v2 | the audio ISR renders 4 tracks, 8 voices, FX |
| RAM | ~512 KiB SRAM | 72 KiB static (55 KiB used) + 360 KiB pool (328 KiB used) |
| Flash | 1 MiB SPI NOR | ~570 KiB firmware, the rest: projects, presets, 3 user sample slots, OTA staging |
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
| Lift / drop / split | cut, copy and paste a loop region of the tape | planned (M3) |
| Mixer | HOME > MIXER (level, pan, reverb send, mute) | from Felucca |
| Sequencers | step sequencer with chance, ratchets, locks; arpeggiator | from Felucca; Endless and Tombola planned (M5) |
| Frequency modulators | LFO with sync, modulation matrix | from Felucca |
| Playful screens | the TAPE cassette with turning reels | started (M1); per-engine screens planned (M4) |
| Album / bounce | not possible without audio memory | out of scope |

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

The tape lives in RAM. It is lost when the FM-1 is switched off. See M2.

### M2: keep the tape (next, needs a decision)

The flash map is full. Options:

1. **Give up one of the three user sample slots** (80 KiB) for the tape: it holds about 5 tapes of 16 KiB.
   Recommended.
2. Save the tape with the project (4 KiB per project today): only the first ~500 notes would fit.
3. Shrink the firmware to free flash: uncertain gain.

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
