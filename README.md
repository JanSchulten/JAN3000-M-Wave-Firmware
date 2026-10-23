# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1 / OP-Z-style workflow on top: a KEY track that moves the patterns
through a chord progression, and from the OP-Z two FX tracks with recordable punch-in effects, the OP-Z's punch-ins on a second key map, 16 scenes a project
switched while they play, pattern chains, a switchable FX1 with a granular CLOUD, the Roland J-6's 100 chord
sets as SCL > CSET and patterns of up to 128 steps. What is planned next is in the [roadmap](docs/ROADMAP.md).

> **Status:** early development, running on a real FM-1 since the first flash (JAN3000 v0.8: the name on its splash).
> Installing custom firmware is at your own risk. The FM-1 has a single firmware bank and no
> recovery button; if an install fails and the unit will not start, recovery needs extra
> hardware (FM-1 Transporter with an RP2040). Develop and try changes in the host tests and the
> browser emulator first.

## KEY: a chord progression over the patterns

As the OP-Z's key track: a run of 1 to 16 bars, each with a root. While KEY is ON and the transport runs, every
melodic track's sequenced notes and its arp move to the bar's root **inside the track's own scale** (SCL ROOT and
SCALE): a pattern written in C major plays under KEY **Am F C G** as A minor, F major, C major and G major, the
scale's own chords. The keys you play live stay as they are, so you can play along; drums and other kits never move.

- **GLO** opens **KEY** (SELECT: SONG). The 16 bars show the chord each root makes in the selected track's
  scale; the bar playing is lit.
- **The keys play roots, not notes** (on this page they sound nothing, as on the OP-Z's key track; each key is its
  name, whatever the octave):
  - **stopped:** a key sets the root of the bar under the cursor and moves the cursor on (round at LEN): play
    A F C G for Am F C G;
  - **running:** a key moves the tracks **at once, live**. It holds until a bar of the run with a root of its own
    starts (KEY ON), or STOP;
  - **REC** on this page records it too (the REC LED shows it; stopped, PLAY starts it, the count-in first): into the bar playing, or
    the next one when played in a bar's last quarter (a little early for the coming bar), and KEY goes ON. REC
    again stops recording. The page shows LIVE or REC and the chord played.
- **KNOB 1 BAR** moves the cursor, **KNOB 2 ROOT** changes its root (- C .. B; - plays as written), **KNOB 3 LEN**
  1 to 16 bars, **KNOB 4** KEY ON / OFF.
- The move goes the nearer way round (at most half the scale's degrees up or down), so the parts stay near where
  they are written. A root outside the scale moves the notes chromatically; a track in CHR uses its ROOT's major
  scale. A note between the scale's degrees keeps its distance above its degree.
- The bars count from PLAY on the sequencer's grid. While a track records, its arp plays as played (the steps it
  writes move when they play). KEY is kept with the project.
- **Each scene has its own KEY.** The page edits the KEY of the scene playing; its title says whose (**KEY S3**). A
  new, empty scene starts with the KEY of the scene it is reached from; set it there and it is that scene's (even
  before it holds notes). A scene switch starts the new scene's run at bar 1, as its tracks start at step 1. Scenes
  saved before (or a project of before) take the KEY playing, so they sound as they did, and keep it from then on.
  Firmware before this one cannot read a scene bank that holds a scene's KEY (it loads the project without its
  other scenes); a project that never used KEY writes none and stays readable there.
## FX TRACKS

Two lanes of effects next to the four instrument tracks, after the OP-Z's FX tracks. Two more synth tracks do not
fit the FM-1's RAM; an effect lane costs 130 bytes. The lanes play the FX layer's effects on the master (REPEAT
1/8 1/16 1/32, REVERSE, LPF, HPF, TAPE STOP, FREEZE, OCT UP, OCT DN, FLANGER, PHASER, REV THROW, ECHO THROW, CLOUD
FREEZE, the OP-Z punch-ins below, and the mutes of tracks 1 to 4).

- **FX** opens FX; tap again for **FX TRACKS**, again for **FX TRACKS 2**.
- **KNOB 1** picks the lane (FX1, FX2), **KNOB 2** the step, **KNOB 3** the step's effect (OFF, TIE to hold the effect
  before it on, the effects, the mutes), **KNOB 4** the lane's length (1 to 128 steps of a 1/16). Two lengths drift
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

## SCENES

16 scenes in a project, as the OP-Z's 16 patterns. A scene is the four tracks' sequences: their steps, LEN DIV SWING
GATE and QUANTIZE, the automation and the locks, the two FX TRACKS, and its **KEY** (GLO > KEY, above). The sounds,
the mixer, ARP, SCL, FX1 and the song stay the project's.

- Hold **SEQ**: the layer says **[SEQ] SCENES**. The **white keys F3 .. G5 are scenes 1 .. 16**; **OCT+ / OCT-** pick the
  next / the one before (from the scene waiting, else the one playing).
- **SEQ + OCT+ held (0.7 s) copies the scene playing into the next free scene** (the first empty one after it, round
  past 16 to 1) and goes on with the copy: COPIED TO SCENE n, while OCT+ is still held; letting go then steps no scene.
  It is at once, playing too, and nothing is heard: the music plays on where it was; from then on every edit changes
  the copy, the original waits in its scene, and OCT- goes back to it. A pick waiting is taken back. Else it says
  NOTHING TO COPY (the scene is empty), NO FREE SCENE or SCENE MEMORY FULL. The footer: [OCT-/+] PREV NEXT,
  [OCT+] HOLD: COPY.
- Stopped, a scene comes at once. **Playing, it comes at the end of the pattern**: the end of the loop of the longest
  track that plays something, on the grid, as a SONG section ends. **MENU > CONTROL > SCENE CHANGE**: AT END (that,
  the default) or **NEXT BAR**: the scene comes at the next bar line (4/4, the metronome's), whatever the tracks'
  lengths, and starts from its step 1 there. The scene playing picked again takes the pick back (SCENE n STAYS). STOP
  with one waiting: it comes at once.
- An empty scene starts with the LEN DIV SWING GATE of the scene it is reached from. The cells: the scene playing
  filled (its key blinking), one waiting ringed (its key breathing), one that holds something raised (its key lit),
  an empty one dimmed (its key dark).
- The **tools are on the black keys** now, F#3 .. F#5 in Felucca's order: CLEAR, REVERSE, SHIFT < >, RANDOM, COOK, on
  a DRUM track BEAT and the lane's CLEAR, REVERSE, FILL, RANDOM (**PRESETS** picks the lane). **SEQ + EDIT** puts the
  track back as the layer opened (it was OCT-). KNOB 1..4 are LEN DIV SWING GATE as before.
- **As on the KO-II:** a punch-in stays on while its key is held, also after FX is let go. Hold SEQ meanwhile and
  pick the next scene; let go of the key and the effect stops.
- The 15 scenes not playing wait packed in 7.7 KB (a scene of four 16-step parts takes about 250 to 900 bytes, the
  densest possible 3.9 KB). A pick that would not fit says **SCENE MEMORY FULL** and nothing changes.
- **The scene song: SEQ held + SELECT** turns the layer to its second level, **[SEQ] SONG** (SELECT back: SCENES; the
  next hold opens where it was left). The song is a chain of cells, 4 a row and on into the next: each cell a scene and
  its passes (x1..x16; a pass is the scene's loop, its longest track), the one playing filled, the one selected ringed,
  a cell on an empty scene dimmed. The **white keys add their scene** to the chain's end (the last cell's scene again:
  one pass more), **KNOB 1** the cell, **KNOB 2** its scene, **KNOB 3** its REPS, **KNOB 4** END (LOOP: the first cell
  again, STOP); **OCT- / OCT+** the cell before / after, **EDIT** deletes the cell, **F#3** CLEAR (after OCT+). **PLAY**
  plays the song from the cell selected (PLAY again: STOP). It always changes at a scene's end (MENU > SCENE CHANGE is
  for picks by hand); a cell on the scene playing just plays it on. While it plays, every page edits and records into
  the scene playing, the cells too (a REPS turned on the one playing counts at once). A pick by hand (SEQ + a key on
  SCENES, OCT+ / OCT-, the copy) or STOP ends the song where it is. Kept with the project (its extras: firmware before
  this reads the project without its song). The SEQ pages stay the step pages: STEP, DETAIL, AUTOMATION.
- Kept with the project (SAVE > PROJECT, the one playing too) and with the autosave. A project of Felucca's or of an
  older M-Vave-OP1 loads as scene 1. While a song plays (GLO > SONG) scenes are not picked (STOP THE SONG FIRST); a
  song plays each slot as it was saved.

## SCL > CSET: chord sets after the Roland J-6

SCL's third page (SCL, CHORD, **CSET**) plays the 100 chord sets of the Roland J-6 (the owner's manual's Chord Set
List, version 1.02: POP, JAZZ, BLUES, the UTILITY sets, SYNTHWAVE, HOUSE, EDM, NEO SOUL, BOSSA NOVA, CLASSICAL and
more; the note data in `firmware/src/cset_data.h`, made by `tools/gen_cset.py`). It replaces SCL > ORCHID.

- **KNOB 1 ON / OFF** for the selected track (each track its own, before the track's CHRD and QNT). ON makes the
  track POLY (a chord needs the voices), OFF gives the VOICE it had back. A sound loaded while it is on plays POLY too
  (OFF then gives that sound's VOICE). A track set to MONO / LEG / UNI by hand plays the chord's lowest note.
- **Every key is a chord:** the key's note C .. B plays that chord of the set, voiced as the list voices it. The keys
  F3 .. E4 play it where the list has it (its basses F2 .. E3, so the twelve chords lie together as on the J-6);
  each octave above or below plays it an octave higher or lower, the octave buttons too. TRN moves the chords. A MIDI
  note plays the chord of its key the same way.
- **KNOB 2 SET** 1 .. 100, **KNOB 3 GENRE**: the first set of the next or previous genre (the list's order).
- The page shows the set's number and genre, its twelve chords by name in the keys' order (C C# D / D# E F / ...;
  the black keys' framed), the chord played last lit and its notes by name.
- Live recording and STEP entry write the chord (up to four notes a step); the ARP arpeggiates it. ON, each track's
  SET and the VOICE before ON are kept with the project. A project saved with ORCHID on loads with CSET off.

## ANALOG2: two oscillators, a filter and an LFO each

A synth engine of its own (engine 14, shown **ANLG2**, after ANALOG in PRESETS): two oscillators, each through its own low-pass with
its own sine LFO on the cutoff, summed. EDIT shows it on one screen of four rows:

| Row | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| OSC | WAVE | TUNE | WAVE2 | TUNE2 |
| WAVE 1 | CUT | RES | LFO | RATE |
| WAVE 2 | CUT2 | RES2 | LFO2 | RATE2 |
| VOICE | VCE | GLD | GLMOD | PRIO |

- **WAVE / WAVE2:** SAW SQR TRI SIN PWM NOISE (band-limited as ANALOG's; PWM's width from the SHP routings).
- **TUNE / TUNE2:** the oscillator's detune in Hz, -50 .. +50: two oscillators a few Hz apart beat at that rate on
  every note.
- **CUT RES:** each oscillator's own filter (ANALOG's state-variable low-pass). ENV, LFO and the matrix to FLT move
  both, and both follow the keyboard a little (ANALOG's KTR 64).
- **LFO / RATE:** the depth (up to half of CUT's range either way) and speed (0.05 .. 30 Hz, shown in Hz) of the
  oscillator's own sine LFO on its CUT; it starts with each note.
- Eight presets: DUAL SAW, WOBBLE, SWEEP PAD, DUO BASS, BELL SIN, NOISE HAT, STEREO LD, WIND. HOME's knobs: CUT, CUT2,
  ATK, REL.
- WAVE 2's four values sit in track parameters the retired DIGITAL engine left free, so projects, user presets,
  automation and the editor keep them without a new format.
## VDRUM: six drum parts into a resonator

A drum engine after the concept of Korg's volca drum (our own design and sounds; spec: [docs/VDRUM_DBOX_SPEC.md](docs/VDRUM_DBOX_SPEC.md)).
Pick it like any engine (PRESETS, EDIT held); it plays the General MIDI drum notes with the kick on the first C, so the
BEAT pattern and MIDI drum parts play on it.

- **Six parts** (KICK, SNARE, CLAP, HAT CL, HAT OP, TOM; the toms' notes pitch part 6), each of **two layers**: a source
  (SIN, SAW, high-, low- or band-passed noise), a pitch modulator (EXP sweep, TRI, RND) and an envelope (AD, EXP, MULT for
  claps). Per part BIT, FOLD, DRV, GAIN, the SEND into the resonator and a choke group (the hats choke each other).
- **The resonator** after the parts: TUBE (odd harmonics, a shell) or STRING (a plucked string). HOME's KNOB 1..4 are
  its **TYPE, DECAY, BODY, TUNE**; EDIT 2: KIT (F1..F8, OWN), MIX, ACC, DRV.
- **Editing a part:** strike its key, then EDIT > LAYER, LAYER PITCH, LAYER ENV (KNOB 1: L1, L2 or L1+L2), PART,
  PART 2. The first edit makes the kit the track's own (KIT OWN); SAVE held undoes. Projects and user presets keep the
  own kit (in the place an FM6 track keeps its patch).
- Eight factory kits: INIT, ELECTRO, TUBES, STRINGS, METAL, DUB, CRUSH, WOOD.
- **On the sequencer as DRUM:** SEQ > STEP is the drum grid with a lane per part (BD SD CP CH OH TM: black keys 1..6;
  7 and 8 stay dark). Live recording writes hits, EDIT held + a black key mutes a part (its resonator send too), SEQ
  TOOLS has BEAT and the lane tools. A DRUM beat moved onto VDRUM keeps its lanes (its RIM and BELL hits play and show
  on CLAP and HAT OP).
- **Clean tails:** a layer's level ramp now lands exactly on the next block's level. It used to fall a little short
  and jump back every 32 samples, a sawtooth at 1378 Hz under the decaying sound (around -70 dBFS whatever its level),
  heard as a hiss on the kicks. Above 4 kHz the kicks' tails are now about 10 to 20 dB lower (INIT: from -74..-79 to
  -89..-96 dBFS), as clean as DRUM's 808; the attack and body (the first 400 ms) are unchanged.

## DBOX: twelve keys of two oscillators

After the OP-1's DBOX (our own design and sounds). Each of the twelve keys C .. B is its own drum sound; the next octave
plays the same twelve an octave up. The factory kits (INIT, 808-ISH, CHIP, METAL) put their sounds on the GM notes, so
the BEAT pattern plays on them.

- **A sound:** two oscillators, each PITCH, WAVE (a morph sine, triangle, saw, square, noise) and ENV (one value: short
  decays, then longer ones with an attack), a cross modulation MODE 1..7 (MIX, FM, RING, SYNC, AM, NOISE, FILT) at XMOD,
  a low-pass CUT and LVL.
- **HOME's KNOB 1..4:** TUNE, DECY, TONE, DRV on every sound at once; EDIT 1: KIT TUNE DECY TONE, EDIT 2: ACC DRV.
- **Editing a key:** strike it, then EDIT > OSC 1 (PITCH WAVE ENV XMOD), OSC 2 (PITCH WAVE ENV CUT), SOUND (MODE LVL);
  KIT OWN, undo and storage as on VDRUM.
- **On the sequencer as DRUM:** SEQ > STEP is the drum grid with DRUM's eight lanes on the keys the kits give those
  drums: C kick, D snare, D# clap, F# closed hat, A# open hat, A tom, C# rim, and B the cowbell (CB). A note of the other
  keys shows on the lane of its drum (E on SD, F G on TM, G# on CH). Live recording, the lane mutes (EDIT held + a black
  key) and SEQ TOOLS' BEAT and lane tools as on DRUM.
- **Clean tails:** the same level-ramp fix as VDRUM's: the block-rate lines under a decaying sound (k x 1378 Hz, around
  -72 dB) are now around -110 dB on the sine kicks (INIT, 808-ISH, METAL).
- **Steps written as notes before the grid** (on VDRUM or DBOX) become hits when the project loads or the scene is
  picked: the same notes at the same velocities, nothing sounds different; other notes stay notes, shown on their lane.

## Other changes to Felucca's workflow

- **SELECT turns the pages:** on any page SELECT goes to its family's next (or previous) page, round and round; the
  page's button tapped again goes HOME. The tempo is HOME's: SELECT sets it on HOME and on HOME's MIXER and CLOCK (HOME
  taps go round those as before). GLO + SELECT no longer sets it. A double tap still locks a layer over the page shown.

- **JAN3000 v0.8 splash, cyber style:** the name in a segment face of its own (an LCD's bars, a slashed zero) boots
  like a terminal: each glyph flickers through random bars before it locks, over scan lines, with colour copies in
  the accent and theme colour beside it and a band torn sideways now and then. Then the version with a blinking block
  cursor, a sine settling under it, "based on Felucca" below.
- **SAVE opens PROJECT, played on the keys:** F3 G3 A3 B3 pick project A..D (the keys are silent there and lit),
  OCT+ saves into it (the NAME screen; a used one asks OVERWRITE? first), OCT- loads it (LOAD PROJECT? first). F#5,
  the highest black key, is **NEW**: OCT+ starts a new project (empty steps, the default tempo, no song, FX TRACKS,
  KEY or scenes, no slot), stopped only (STOP FIRST). OCT- on an empty project asks NEW PROJECT IN SLOT C? and starts
  the new one there (SAVE goes to it). SELECT goes on to USER, PRESETS, PHRASES, TOOLS.
- **A new project draws its sounds:** T1 a bass, T3 a lead (any factory preset of those categories, any engine), T4 a
  kit (one of DRUM's six, STD 808 10 66 55 77, or a VDRUM or DBOX kit); T2 keeps its pad. So does power-on when
  nothing is restored (RESTORE LAST OFF, the first start). A DRUM track is named after its kit ("66 KIT") in the
  footer and on HOME.
- **STEP: a note turned OFF with KNOB 2.** Turning NOTE past the lowest note (or the highest) sets the step OFF (a
  rest; a fast turn stops at the end first, a chord keeps its shape); from OFF a turn brings in the note played last.
  SAVE held undoes it. EDIT no longer clears the step on STEP (nor on the drum grid): a tap opens EDIT as on every
  page, with a step held it still clears that step's locks.
- **128 steps a track.** LEN goes to 128 (64 before): eight pages of 16 on STEP (the grid's page keys, PRESETS and
  KNOB 1 walk them; the footer says PAGE 8/8), HOME's track rows show up to eight page marks. Everything a step has
  works on steps 65 to 128 too: chords, ties, ratchets, chance, nudges, LEGATO, automation and parameter locks
  (AUTOMATION's rows say STEP 100 .. 128), live recording, the tools; the FX TRACKS go to 128 steps as well (their
  overview two steps a cell past 64). A project keeps steps 1 to 64 where Felucca's format has them and the rest, with
  a LEN past 64, in its extras: firmware of 64 steps loads such a project as its first 64 steps (LEN 64), the rest
  stays in the slot until it is saved over there. A scene that holds something past step 64 is packed in a wider form
  that such firmware does not read (its other scenes: as before); one dense on all 128 steps of all four tracks does
  not fit the scene memory (SCENE MEMORY FULL). The editor's INFO says 64 (as before) and 128 in a new block, so the
  web editor lists the steps 65 to 128 and older editors keep working on the first 64.
- **REC arms, PLAY starts.** A REC tap only arms the recording (the pattern's, GLO > KEY's, an FX TRACK's): REC ARMED:
  PRESS PLAY. PLAY then starts it, with the count-in first when MENU / REC layer COUNT-IN is set (Felucca's REC
  started the transport at once).
- **Legato is recorded as played.** On a LEG or UNI track (VOICE; the mono presets are LEG) a note pressed while the
  one before is still held plays legato: no new envelope, a glide only if GLIDE is set. Live recording now keeps
  that: the step is LEGATO, the step before holds its note into it, and playback sounds as you played it (before, it
  started a new envelope there). The roll draws the join as a dim line (a SLIDE's is bright, and still glides).
  REVERSE keeps the legato between the same two notes. A note pressed after letting go, or any note on a MONO / POLY
  track, starts its own envelope as before. LEGATO is saved with the project (its extras): an older firmware loads
  the project with every note starting its envelope; a scene bank with LEGATO steps it cannot read (it loads the
  project's current scene only).
- **REC held: OCT- undo, OCT+ redo** of what was recorded since REC was armed (once each in a row). A clear or CLICK
  change in the same hold is put back by OCT- as before.
- **REC held + F#5 (the highest black key): BACK**, one step back from the last change, whatever it was (a recording,
  a step turned OFF, a sound or pattern load, a SEQ tool), as SAVE held. Pressed again: ALREADY UNDONE (OCT+ redoes
  it). Its key breathes while there is a step to go back; the layer's map shows it as its last row.
- **Another track opens each family at its first page:** turn ALGORITHM, then FX (SCL, ENV ..) starts at its first
  card again; the page shown when the track changes stays.
- **DRUM, every sound its own: EDIT > SOUNDS** (after LANES 2). A key picks the sound (it plays as everywhere), KNOB
  1..4 set that sound's TUNE (-12..+12 semitones), DECAY and TONE (-6..+6 steps around the kit) and SOUND (KIT, or the
  piece of STD / 808 / 10 / 66 / 55 / 77: an 808 kick under a 10 snare). The screen lists all eight. Projects and user
  presets keep them (in track parameters the retired DIGITAL engine left free); a preset load sets them back.
- **DRUM, SEQ grid:** a lane's black key lights for a moment whenever that lane plays (a hi-hat's key flashes with
  each hit). VDRUM and DBOX have the grid too (above).

- **EDIT as rows:** EDIT's pages without a picture of their own (EDIT 1 / 2, FILTER, VOICE ..) show four rows under
  the cards in place of the scope: every page's values, the row the knobs edit raised. SELECT goes to the next
  row. WHEEL, SAMPLE and FM6 keep their pictures.
- **FX in seven cards, six drawn as effect pedals** (each in the colours of a classic of its kind: an orange drive,
  a light blue chorus, a magenta flanger, a teal vibrato, a green delay or a purple CLOUD box, a blue room, a brown
  spring, a silver hall, a pearl shimmer, a yellow slicer): the knobs where KNOB 1..4 sit (turned: the value under
  it), a window with the effect moving, the name, the LED and the footswitch. SELECT the next: **1 SEND** (not a
  pedal: four mixer faders, the track's DST CHO DLY REV), **2 INSERT** (TYPE A B MIX), **3 CHORUS** (TYPE CHORUS /
  FLANGER / VIBRATO, RATE, DEPTH, FDBK), **4 DELAY** (TYPE DELAY: TIME FDBK MIX; CLOUD, the granular: SIZE PITCH
  DENS), **5 REVERB** (TYPE ROOM / SPRING / HALL / SHIMMER, SIZE, DAMP), **6 SLICER**, **7 FX TRACKS**.
- **CHORUS TYPE:** CHORUS is Felucca's (FDBK 0: bit for bit); FLANGER sweeps a 0.5 .. 6 ms comb, FDBK up to 0.87 for
  the jet; VIBRATO a 1 .. 10 ms line, the copy's pitch swinging (the bus is all wet: CHO up and the track's LEVEL down
  for the vibrato alone). TYPE and FDBK are kept with the project.
- **REVERB SHIMMER:** the HALL with its tail shifted an octave up and fed back into it, so the tail climbs an octave
  a pass (each one darker and quieter: it dies away at any SIZE).
- **ARP on one screen:** ARP and ARP 2 show both their rows (MODE RATE OCT GATE, SWING PROB HOLD ORDER; the row the
  knobs edit raised, SELECT goes to the other) and under them a keyboard: the notes the arp runs through lit (the
  keys held brighter, the octaves OCT adds quieter), the note sounding in the accent.
- **REC held:** F#3 clears both FX TRACKS, G#3 the selected track's automation (its recorded knob moves), each after
  OCT+; the notes stay (F3 CLEAR clears the notes and the automation, as before).
- **HOME shows the four tracks** by default. The oscilloscope is MENU > DISPLAY > HOME **SCOPE** (TRACKS, LEVELS,
  SCOPE).
- **STEP, stopped: OCT+ adds a note, OCT- removes one.** OCT+ puts the note played last into the cursor step (KNOB 1
  the cursor, KNOB 2 moves the step's notes), as another chord note up to four; OCT- takes the step's last note out.
  While playing, and on every other page, OCT shifts the keyboard's octave as before.
- **DRUM starts with the 808 kit** (KIT 808, the 808 model kit Felucca calls 80): power-on, a track switched to DRUM,
  its preset. Projects and sounds saved before keep their kit.
- **SYNC OUT PO** (MENU > AUDIO > SYNC OUT: OFF, PO, PO INV): Pocket Operator sync on the headphone jack. The left
  channel carries a pulse each eighth note while playing (2 a quarter, as the POs' SY modes and the Volcas expect):
  20 ms at the DAC's +full scale, whatever MASTER; between the pulses (and while stopped) it sits at -1 % of full
  scale, never at 0. That is the shape of [PocketSync](https://github.com/chr15m/PocketSync), a phone app the POs
  follow: a DAC channel held at digital 0 can be auto-muted, and a pulse out of a muted channel comes out ramped, a
  tick in the headphones but no edge for the PO. PO INV is the same pulse upside down, for an output stage that
  inverts it. The right channel is the mono mix. A stereo cable into the PO's line in, or a stereo to two mono cable:
  left into the PO's or Volca's sync input, right as audio. On the PO a sync-in mode: SY2 (sync only) or SY4 (left
  sync, right audio through); **press PLAY on the PO first** (it waits for the clock), then PLAY on the FM-1. If the
  PO does not follow on PO, try PO INV. USB audio stays stereo. The FM-1's TRS MIDI jack is an input only, so this is
  the way out for a clock.

## Try it in the browser

`web/lab` is an emulator page with guided workflows (in German): the whole firmware as WebAssembly, with a
JavaScript fallback. Build it with `sh web/lab/build.sh` (after `./build.sh`, with the emsdk), then
`cd build/lab && python3 -m http.server 8000` and open `http://localhost:8000/local.html`.

## Flash: two sample slots less

The FX TRACKS, the KEY and the other extras and the scenes need flash that Felucca's project format does not have.
The extras live in the flash of the third user sample slot (USR3, 80 KiB), the scenes in the second's (USR2, 80 KiB):
the web editor uploads samples into USR1 only (a sample uploaded into USR2 by an older version is not played any
more). Felucca projects load as before (with empty FX TRACKS, as scene 1).

## Documents

- **First flash on a real FM-1** (German checklists): [docs/ERSTER_FLASH.md](docs/ERSTER_FLASH.md), then
  [docs/TESTLISTE.md](docs/TESTLISTE.md). The installer as a ZIP to start on your computer:
  `dist/M-Vave-OP1-Installer.zip` (`sh tools/make_installer_zip.sh`); it can return the FM-1 to M-VAVE's official
  V15 without a separate download (`vendor/m-vave/`, Apache-2.0)
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
