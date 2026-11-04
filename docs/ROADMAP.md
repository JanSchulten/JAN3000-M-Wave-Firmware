# M-Vave-OP1 roadmap

An OP-1-style workflow on the M-VAVE FM-1, built on Felucca. This page records what the hardware allows,
how OP-1 ideas map onto it, and the order of work.

## What the hardware allows

| | FM-1 | Used by Felucca 1.5.1 |
| --- | --- | --- |
| CPU | JieLi AC791N / WL82, pi32v2 | the audio ISR renders 4 tracks, 8 voices, FX |
| RAM | ~512 KiB SRAM | 72 KiB static (55 KiB used; M-Vave-OP1 56 KiB) + 360 KiB pool (336 KiB used; M-Vave-OP1 345 KiB) |
| Flash | 1 MiB SPI NOR | ~570 KiB firmware, the rest: projects, presets, user sample slots (M-Vave-OP1: 1 of 3; the second holds the scenes, the third the tape and the FX TRACKS), OTA staging |
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
| Synth engines | Felucca's engines: FM6 (DX7-style), ANALOG, ANALOG2 (M-Vave-OP1: two oscillators, a filter and an LFO each), LOFI, WHEEL, GRAIN, PHYS, NOISE, SAMPLE (PHASE, VOICE, TRIO, SLICE retired for the Plaits engines: [PLAITS_PLAN.md](PLAITS_PLAN.md)) | from Felucca |
| Drum sampler | DRUM engine, SAMPLE with user slots (SLICE retired) | from Felucca |
| Tape: 4 tracks | the note TAPE of M1/M2 (loop, varispeed, reverse, kept in flash) | **dropped (M6)**: it did not play well; GLO > KEY took its place |
| (OP-Z) key track | **KEY**: 1..16 bars of roots, the melodic tracks moved inside their scales | **done (M6)** |
| Mixer | HOME > MIXER (level, pan, reverb send, mute) | from Felucca |
| Sequencers | step sequencer with chance, ratchets, locks; arpeggiator | from Felucca; Endless and Tombola planned (M5) |
| Frequency modulators | LFO with sync, modulation matrix | from Felucca |
| Playful screens | FX cards as effect pedals, SEND faders, the ARP keyboard, the KEY's bars | started; per-engine screens planned (M4) |
| Album / bounce | not possible without audio memory | out of scope |

And from the OP-Z (M2b):

| OP-Z | M-Vave-OP1 | Status |
| --- | --- | --- |
| FX1 / FX2 tracks | **FX TRACKS**: two lanes of the FX layer's effects, 64 steps of a 1/16, own lengths | **done (M2b)** |
| Punch-in FX, recorded with REC | REC arms a lane, FX held + keys record (quantised, held on, overdub, punch) | **done (M2b)** |
| Pattern chains, cue the next pattern | SONG: REC + A..D types a chain, LOOP, changes while it plays | **done (M2b)** |
| 16 patterns a project, switched with a key | **SCENES**: 16 a project, SEQ + a white key (OCT+ / OCT- the next / the one before; OCT+ held copies the scene into the next free one), at the pattern's end while playing | **done (M2c)** |
| Punch-ins DUCK, STEREO, PITCH, RAMP UP, RANDOM .. | **MAP B** of the FX layer (FX + ALGORITHM): DUCK, STEREO, PITCH 5TH UP / 4TH DN, RAMP UP, GLITCH, in the OP-Z's order | **done (E2)** |

And from the Telepathic Instruments Orchid, after ChoralRoot FM-1 (M2d; replaced by SCL > CSET, the Roland J-6's chord
sets, M2e):

| Orchid | M-Vave-OP1 | Status |
| --- | --- | --- |
| Chord keys (type + extensions) left, roots right | **SCL > ORCHID**: F3 .. C#4 DIM MIN MAJ SUS, 6 m7 M7 9, LOCK; roots D4 .. G5; per track | **done (M2d)** |
| Voicing up / down | KNOB 2 VOICE -12 .. +12, kept with the project | **done (M2d)** |
| Latching chord keys | KNOB 3 / B3 LOCK, ChoralRoot's Clears / Toggle behaviour | **done (M2d)** |

## Milestones

### M1: TAPE (done, dropped in M6)

- `firmware/src/tape.c`: the tape. One gap buffer of 4096 events (16 KiB) for all four tracks, so recording
  and playback cost O(1) in the audio ISR. Measured on the target build: +1.8 % static cost in the audio ISR.
- GLO > **TAPE**: KNOB 1 POS, KNOB 2 LOOP, KNOB 3 SPEED, KNOB 4 DIR. REC arms the tape for the selected
  track (the REC LED shows the tape's arming on these pages).
- GLO > **TAPE 2**: ERASE the selected track's notes, or the whole tape (OCT+ asks first). NOTES and USED.
- PLAY runs the tape with the sequencer, from where the head stands; GLO + PLAY (RESTART) starts it from the
  loop's start, or the tape's. A count-in (MENU) also counts in a tape recording.
- `tests/tape_test.c`: 62 checks for recording, playback, overdub, loop, speed, reverse, erase, a full tape,
  the tape's end and the pages.

### M2: keep the tape (done; the tape dropped in M6, the stores stay)

Decided: notes and settings over a third sample slot. `firmware/src/opstore.c` uses the flash of the third user
sample slot (USR3, 0xC8000..0xDBFFF, 80 KiB):

- 0xD2000..0xDBFFF: **one tape**, as the OP-1's, in two copies of five sectors that take turns (its own commit
  record after the events: a write cut short leaves the copy before). Written 3 s after the tape changed, with the
  transport stopped and nothing sounding (an erase silences the audio for ~40 ms); read back at power-on, not after
  a crash. While it is written the audio ISR leaves the tape alone (`tape.hold`).
- 0xC8000..0xD1FFF: each project slot's **extras** and the autosave's (the FX TRACKS, see M2b), A/B, tagged with
  the CRC of the project they belong to: a project written another way (the editor's backups) gets empty lanes.
- The web editor uploads samples into USR1 and USR2 only; USR3 reads as an empty slot (since M2c USR1 only: USR2
  holds the scenes).

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

### M2c: OP-Z patterns: SCENES (done)

16 scenes in a project, picked while they play, as the OP-Z's patterns (`firmware/src/scene.c`). A scene is what a
SONG section plays: the four tracks' steps, timing and QUANTIZE, the automation and the locks, the FX TRACKS.

- The scene playing stays where the music always was (the tracks, `motion`, `fxt_lane`): every page, tool, undo,
  recording and save works on it as before. The other 15 wait packed in a bank of 7880 bytes in the pool (a step
  only where it differs from an empty one, and of it only the bytes that do; an empty scene takes no byte).
- A pick: the main loop unpacks the scene into the SONG chain's first source (free while no song plays) and swaps
  it in at once when stopped; playing, the audio ISR swaps it at the end of the loop of the longest track that
  plays something, the remainder carried so the grid does not drift (`scene_due`, O(1) a block while one waits).
  The swap first measures the scene that leaves; one that grew past what the bank has for it is dropped there
  (SCENE MEMORY FULL), nothing lost.
- The SEQ layer: white keys the scenes, OCT+ / OCT- the next / the one before, the SEQ TOOLS on the black keys, SEQ +
  EDIT puts the track back, PRESETS picks a DRUM lane. A punch-in held after FX is let go stays on while SEQ picks
  (the KO-II's flow).
- SEQ + OCT+ held 0.7 s: the scene playing copied into the next free scene, which plays from then
  on (`scene_copy`). Nothing is swapped: the scene's bytes go into the bank in its own place, the tracks are the copy,
  so it is at once and silent, playing too. The tracks are copied into the buffer with the interrupts off (live
  recording writes them in the ISR) and packed from there; a pick waiting is taken back.
- Kept in USR2's flash (`opstore.c`): each slot's bank and the autosave's, two copies of 8 KiB that take turns, tagged
  with the project's CRC as the extras are. The autosave writes the bank when its bytes or the scene playing changed.
- Tests: `tests/scene_test.c` (the packed form, switching, memory, kept, the copy, the layer, KO-II), `tests/opstore_test.c`
  (flash: copies, a write cut short, damage, another project's, rename, autosave); `ui_test.c` follows the tools'
  new keys.

### M2f: ARP > ARP 3, the J-6's phrases (done)

The arp timed by the Roland J-6's 108 phrases (`firmware/src/j6phrase.c`; `j6phrase_data.h` from
`assets/j6-phrases.json` by `tools/gen_j6phrase.py`): the Phrase List's 9 STYLE x 12 VARIATION, played into the J-6
Phrase Log from a J-6 holding G3 A#3 D4 F4, its first note's latency undone, one cycle each, the rest before the first
note put back (from the Phrase List for STYLE 4 and 5, none where the cycle has no room, a guess by the metre for 23
of STYLE 6 .. 9). A third ARP page, ARP 3: STYLE (OFF, ARP 8 .. STR R) and VAR (1..12), two track parameters before the
engine values (P_COUNT 114, P_E0 106; FUN10 as it was, an older project OFF). With a STYLE and the arp on, the phrase
sets the notes' starts, lengths (x GATE / 64) and accents (velocity 127); MODE J-6 (appended) plays its own chord
tones over the held notes, low to high, any other MODE its rhythm. Up to eight notes at once, each its own note-off.
The panel draws the phrase over its cycle. Tests: `tests/j6phrase_test.c` (every phrase's timing and notes over two
cycles, the modes, PROB, the note-offs, kept, the page).

### M2e: SCL > CSET, the J-6's chord sets (done)

SCL > ORCHID gave way to the Roland J-6's 100 chord sets (`firmware/src/cset.c`, `cset_data.h` from the J-6 owner's
manual's Chord Set List by `tools/gen_cset.py`), on SCL's third page CSET, per track: ON (the track POLY, its VOICE
back with OFF), SET 1 .. 100, GENRE. Every key and MIDI note plays the chord of its key, F3 .. E4 as the list voices
them, each octave away an octave up or down; live recording, STEP entry and the ARP take the chords. ON, SET and the
VOICE before ON are kept in the project extras (XTRA_V6). Tests: `tests/cset_test.c`.

### M2d: SCL > ORCHID, the Orchid's keyboard (done; replaced by M2e)

The Orchid's keyboard as ChoralRoot FM-1 by Quixotic7 (GPL-3.0) does it (`firmware/src/orchid.c`), on SCL's third
page ORCHID, switched on per track (it was the ARP page's MODE CHORD for one version; the ARP is Felucca's again).

- The key input (`keyboard_block`) gives F3 .. C#4 to the chord block on a track with ORCHID on (not a kit, not on
  the DRUM grid or a layer): they set a type / extension / LOCK state and sound nothing. A root key builds its chord
  when it goes down (ChoralRoot's Simple style) through Felucca's own `key_on` / `key_off`, so the chord's notes go
  the same way as a CHRD chord's: the voices, the ARP, MIDI OUT, live recording, the TAPE. A one-voice part plays the
  root.
- LOCK and the latch are the ISR's state (`orc`), set by the keys in the ISR and by KNOB 3 through a request
  (`orc_lock_req`) taken at the start of a block; reset with a project load.
- ON (`orc_en`) and VOICE (-12 .. +12) per track are kept in the project extras (`proj_xtra_t`, appended; Felucca's
  own project format unchanged).
- The ORCHID page: ON, VOICE, LOCK on the knobs; the graph shows the chord the Orchid's way (the name large, a
  keyboard with its notes, the notes by name). STEP entry writes the chord.
- Tests: `tests/orchid_test.c` (types, extensions, VOICE, names, the key input, the ARP over a chord, LOCK step by
  step as ChoralRoot's manual, the page, LEDs, STEP entry, kept).

### E1..E5: effects (E1, E2 done)

- **E1 (done):** FX1 (the delay bus) gets a TYPE: DELAY or **CLOUD**, a granular cloud in the delay's own line
  (`firmware/src/fx1.c`): up to 8 grains, SIZE DENS PITCH SPREAD, RAND FDBK MIX, CLOUD FREEZE on the FX layer's F5,
  kept in the project extras. `tests/cloud_test.c` with WAV demos.
- **E2 (done):** the OP-Z's punch-ins on the FX layer (`firmware/src/perform.c`): DUCK and STEREO as stages of their
  own after PHASER, PITCH 5TH UP / 4TH DN on the harmonizer's rotating delay (the shifted sound alone), RAMP UP (TAPE
  STOP backwards, ending in time) and GLITCH (a random slice each 1/16) on the loop's buffer. A second key map, MAP B
  (FX + ALGORITHM), holds them in the OP-Z's order, kept in 10 spare bytes of the settings. `tests/punch_test.c` with
  WAV demos.
- Next: new track INSERTs (RING, SHIFT, COMB, VOWEL, WOW), more FX1 types (SHIMMER, REVERSE, TAPE ECHO), SCATTER,
  SHORT / LONG. All on buffers that exist already. The plan, in German: [EFFEKTE_PLAN.md](EFFEKTE_PLAN.md).

### M3: tape editing (dropped with the TAPE)

### M6: KEY instead of the TAPE (done)

The note tape did not play well and is gone: `tape.c`, GLO > TAPE / TAPE 2, its store (0xD2000..0xDBFFF is free
again) and its tests. In its place GLO > **KEY** (`firmware/src/key.c`), as the OP-Z's key track: a run of 1..16
bars, each a root; while ON and running, the melodic tracks' sequenced notes and arps move to the bar's root inside
their own scale (Am F C G over a pattern in C major: A minor, F, C, G), the nearer way round; live keys and kits stay
as they are. A key sets the cursor bar's root. Kept in the project extras (`XTRA_V4`). `tests/key_test.c`.

### M7: 128 steps a track (done, then rolled back)

LEN 1..128 (64 before) on every track and on the FX TRACKS, everything a step has on steps 65..128 too (chords,
ratchets, chance, nudges, LEGATO, automation, locks). Felucca's project format holds steps 1..64: the rest goes into
the project's extras (`XTRA_V7`, the tail after `proj_xtra_t`, read from the flash when a project or a song needs it);
scenes past step 64 pack wide (`SCN_F_WIDE`); the song chain's four sources moved to the pool (RAM 56 KB, pool 376 KB).
Firmware of 64 steps loads such a project as its first 64 steps.
Rolled back to 64 (core.h `NSTEP`) when the SHIMMER bus and SAVE's grouped undo joined: together they did not fit the
SRAM (15 KB over). The formats' room for steps 65..128 stays and stays empty; `tests/steps128_test.c` went with it.

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
