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

- **FX** opens the FX family; **SELECT** browses to **FX TRACKS**.
- **KNOB 1** picks the lane (FX1, FX2), **KNOB 2** the step, **KNOB 3** the step's effect (OFF, TIE to hold the effect
  before it on, the effects, the mutes), **KNOB 4** the lane's length (1 to 128 steps of a 1/16). Two lengths drift
  against each other, as the OP-Z's tracks do.
- **Punch-ins, recorded:** REC on FX TRACKS arms the selected lane (stopped: PLAY starts). Hold **FX** and press the
  white keys (effects) or the first four black keys (mutes): what you play goes onto the lane, quantised to the
  1/16, held on as TIEs. Nothing held writes nothing (overdub); a punch inside an older take gives the rest of it
  back. FX LATCH records too.
- **FX TRACKS + EDIT** asks before clearing the selected lane; OCT+ confirms, OCT- cancels.
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
  filled (its key lit), one waiting ringed (its key breathing), one that holds something raised, an empty one
  dimmed.
- The **tools are on the black keys** from G#3: REVERSE, SHIFT < >, RANDOM, COOK, on a DRUM track BEAT and the
  lane's CLEAR, REVERSE, FILL, RANDOM (**PRESETS** picks the lane). F#3 is unassigned. Hold **SAVE** to undo all
  edits in that opening, hold it again to redo. KNOB 1..4 are LEN DIV SWING GATE. **SELECT** switches to SONG, the
  scene song (see below).
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

- **Steps written as notes before the grid** become hits when the project loads or the scene is picked: the same
  notes at the same velocities, nothing sounds different; other notes stay notes, shown on their lane.

## WSHAPE: Plaits' waveshaper, a filter, the ADSR

The first engine after Mutable Instruments' Plaits (engine 17, shown **WSHAPE**, after FM6 in PRESETS; MIT, Emilie
Gillet): Plaits' waveshaping model, sample for sample the module's own, computed on the chip's FPU, then ANALOG's
filter and the ADSR, as Behringer's Grind plays it.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| SHAPE | HARM | TIMB | MORPH | AUX |
| FLT | CUT | RES | DRV | KTR |

- **HARM:** the waveshaper's curve, from the inverse tangent through sine, linear and bump to the double bump (Plaits'
  HARMONICS).
- **TIMB:** the wavefolder's gain: more folds, brighter (Plaits' TIMBRE). The SHP routings (ENV / LFO DEST SHP, the
  matrix's SHP) move it, as they move ANALOG's pulse width.
- **MORPH:** the slope's asymmetry: a triangle at 64, towards a saw either way (Plaits' MORPH).
- **AUX:** the blend into Plaits' second output (the triangle's sine into a second folder): rounder, more hollow.
- **CUT RES DRV KTR:** ANALOG's low-pass, its drive before it and the key tracking (64 an octave an octave).
- Eight presets: SOFT FOLD, FOLD BASS, GLASS LEAD, WARM PAD, BUZZ SAW, SOFT KEYS, METAL PLUCK, SWEEP. HOME's knobs:
  HARM, TIMB, MORPH, CUT. A sound stored with PHASE loads as SOFT FOLD.
- A voice costs about 6 % of the audio's time on the unit (measured), less with AUX at 0: then Plaits' second output
  is not computed.

## HRMNC: Plaits' additive oscillator, a filter, the ADSR

The second engine after Plaits (engine 18, shown **HRMNC**, after WSHAPE in PRESETS): Plaits' additive model, sample
for sample the module's own on the FPU, then the same filter and ADSR as WSHAPE.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| SPECT | HARM | TIMB | MORPH | AUX |
| FLT | CUT | RES | DRV | KTR |

- 24 sine partials. **TIMB** moves the spectrum's peak from the fundamental to partial 24 (the SHP routings move it);
  **MORPH** is its width, from a wide spread (0, bright) to a single partial (127, a sine); **HARM** adds bumps beside
  the peak, from one lobe to a comb.
- **AUX** blends into Plaits' second output: the same spectrum on an organ's partials (1 2 3 4 6 8 10 12, drawbars).
- High up a bright spectrum gets quieter: Plaits fades out the partials that pass half the sample rate.
- Eight presets: DRAWBARS, AIR PAD, BRIGHT LEAD, SUB BASS, GLASS BELL, VOWEL, COMB KEYS, SPECTRAL. HOME's knobs: HARM,
  TIMB, MORPH, CUT. A sound stored with TRIO loads as DRAWBARS.

## PRTCL: Plaits' particles, a filter, the ADSR, a diffuser

The third engine after Plaits (engine 19, shown **PRTCL**, after HRMNC in PRESETS): Plaits' particle model, sample
for sample the module's own on the FPU, then the same filter and ADSR as WSHAPE; Plaits' diffuser once for the part,
after its voices.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| DUST | HARM | TIMB | MORPH | AUX |
| FLT | CUT | RES | DRV | KTR |

- Six particles a voice: random impulses, each through a band-pass at the note. **TIMB** is the density, from a sparse
  crackle to a dense hiss (the SHP routings move it); **HARM** detunes each band-pass at random, up to 4 octaves.
- **MORPH:** below 64 the diffuser (a smear of short echoes, the most at 0: the part rings on after its notes), above
  64 the band-passes' resonance (ringing, pitched pings at 127).
- **AUX** blends into the raw impulses, unfiltered (dust).
- A note fires every particle at once (0.7 ms in), so even a sparse setting starts with a tap.
- Eight presets: DUST CLOUD, RAIN, RESO PING, CRACKLE, GLITTER PAD, WOOD TAPS, GEIGER, SHIMMER.
- The diffuser's delays are 7/10 of Plaits' (it fits the part's memory): a slightly shorter smear.

## WTABLE: Plaits' wavetables, a filter, the ADSR

The fourth engine after Plaits (engine 20, shown **WTABLE**, after PRTCL in PRESETS): Plaits' wavetable model, then the
same filter and ADSR. TIMB / MORPH / HARM pick the row, column and bank of 4 x 64 waves (the fourth bank a shuffle of
the others); from HARM's middle up the banks come back in reverse and the moves between waves turn to steps. AUX blends
into a 5-bit copy. The tables are stored in 8 bits (25 KB instead of 50): playing, they stay within 49 dB of Plaits'
own. A sound stored with VOICE loads as WAVE SCAN. Presets: WAVE SCAN, DIGI BASS, GLASS KEYS, SCAN PAD, BIT LEAD, STEP
TABLE, HOLLOW, CHAOS ROW.

## SWARM: Plaits' swarm, a filter, the ADSR

The fifth engine after Plaits (engine 21, shown **SWARM**, after WTABLE): 8 saws a voice (AUX: 8 sines) under grain
envelopes. HARM spreads them around the note, TIMB is the grains' rate, MORPH their size: short grains low, held and
gliding oscillators high. Each note starts a burst (as Plaits with its trigger patched): the swarm restarts fast, then
settles. A sound stored with SLICE loads as SAW CLOUD. Presets: SAW CLOUD, SUPERSAW, GRAIN BUZZ, DRONE, SINE SWARM,
BURST PLUCK, BEE HIVE, CHORUS BASS.

All five Plaits engines play at Plaits' own relative levels (its `voice.cc`: WSHAPE 0.7, HRMNC 0.8, WTABLE 0.6, PRTCL
and SWARM into Plaits' limiter).

## ACID: a TB-303 bass line

Engine 22, shown **ACID**, last in the list (EDIT held + D#5): a bass line after Roland's TB-303 as Behringer's TD-3
plays it. One oscillator (SAW or SQR) into a four-pole diode-ladder low-pass, its own envelope on the cutoff, the
accent and the slide as the 303 makes them. The controls are the TD-3's: WAVE TUNE CUT RES ENV DEC ACC DIST (HOME
KNOB 1..4: CUT RES ENV DEC). An accented note (a step's ACCENT, or velocity 110 and up) is louder, its envelope deeper
and shorter, and it charges the accent sweep: accents in a row open the filter further. A step's SLIDE glides into the
next note without restarting the envelopes. The voice's level is the engine's own (the track's ADSR does nothing).
Presets (all MONO, each with a pattern): ACID LINE, SQUELCH, SQR ACID, RUBBER, DIST 303, DEEP SUB, SCREAM, SOFT BASS.

- **Patterns:** ACID 2 .. ACID 5 (patterns 14..17) are 303 lines with accents and slides; the presets load them.
- **SEQ + RANDOM** on an ACID track rolls a new acid line (RANDOM ACID): 16ths in the track's scale (CHR plays minor)
  from the root in octave 2, rests and ties, about 30 % accents and 22 % slides, step 1 the root, accented. **COOK**
  also turns slides on and off.
- **The web editor's TD-3 grid** (Sequencer tab, an ACID track): 16 columns of note, octave up / down, time (note,
  tie, rest), ACC and SLD, written to the track's steps as you click; "Roll acid" rolls a line as RANDOM does.

The filter follows Open303's structure of the 303's diode ladder (four coupled one-pole stages, the feedback from the
last; no code taken), with its own coefficients: the resonant peak at the cutoff, RES up to 97 % of self-oscillation.

## Retired: PHASE, VOICE, TRIO, SLICE and DBOX

Five engines made room for engines after Mutable Instruments' Plaits (the module Behringer's Grind builds on; the plan:
[docs/PLAITS_PLAN.md](docs/PLAITS_PLAN.md)): about 46 KB of flash and 5 KB of RAM. Their numbers stay reserved, so
nothing that names one breaks: a project, the autosave, a user preset or the editor that asks for one loads its
successor's first preset instead (PHASE becomes WSHAPE, TRIO HRMNC, VOICE WTABLE, SLICE SWARM, DBOX DRUM; the rest of the
sound and the steps as stored). PRTCL is the fifth Plaits engine, beside them.

## Other changes to Felucca's workflow

- **SELECT turns every family's pages**, including HOME → MIXER → CLOCK → HOME. A page button opens its family;
  the same button again returns HOME. HOME always returns HOME. Hold a button for its live layer. Tempo is CLOCK's
  BPM knob or GLO + F4 TAP. SELECT never sets tempo; the old BPM LOCK bit remains reserved.

- **JAN3000 v0.8 splash, cyber style:** the name in a segment face of its own (an LCD's bars, a slashed zero) boots
  like a terminal: each glyph flickers through random bars before it locks, over scan lines, with colour copies in
  the accent and theme colour beside it and a band torn sideways now and then. Then the version with a blinking block
  cursor, a sine settling under it, "based on Felucca" below.
- **SAVE opens PROJECT, played on the keys:** F3 G3 A3 B3 pick project A..D (the keys are silent there and lit),
  OCT+ saves into it (the NAME screen; a used one asks OVERWRITE? first), OCT- loads it (LOAD PROJECT? first). F#5,
  the highest black key, is **NEW**: OCT+ starts a new project (empty steps, the default tempo, no song, FX TRACKS,
  KEY or scenes, no slot), stopped only (STOP FIRST). OCT- on an empty slot says EMPTY SLOT. SELECT goes on to USER,
  PRESETS and PHRASES. USER uses the same key / OCT workflow, with two banks on KNOB 1. F#3 renames and G#3 deletes
  a selected project or user sound after confirmation.
- **A new project draws its sounds:** T1 a bass, T3 a lead (any factory preset of those categories, any engine), T4 a
  kit (one of DRUM's six, STD 808 10 66 55 77, or a VDRUM kit); T2 keeps its pad. So does power-on when
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
  PRESS PLAY. PLAY then starts it, with the count-in first when REC layer COUNT-IN is set (Felucca's REC
  started the transport at once).
- **Legato is recorded as played.** On a LEG or UNI track (VOICE; the mono presets are LEG) a note pressed while the
  one before is still held plays legato: no new envelope, a glide only if GLIDE is set. Live recording now keeps
  that: the step is LEGATO, the step before holds its note into it, and playback sounds as you played it (before, it
  started a new envelope there). The roll draws the join as a dim line (a SLIDE's is bright, and still glides).
  REVERSE keeps the legato between the same two notes. A note pressed after letting go, or any note on a MONO / POLY
  track, starts its own envelope as before. LEGATO is saved with the project (its extras): an older firmware loads
  the project with every note starting its envelope; a scene bank with LEGATO steps it cannot read (it loads the
  project's current scene only).
- **SAVE held is the only undo; held again it redoes.** A recording is one step across all its armed domains;
  a layer's persistent changes are one step. It works with the layer still open. REC has no BACK or OCT undo;
  OCT shifts octave in non-SEQ live layers.
- **Another track opens each family at its first page:** turn ALGORITHM, then FX (SCL, ENV ..) starts at its first
  card again; the page shown when the track changes stays.
- **DRUM, every sound its own: EDIT > SOUNDS** (after LANES 2). A key picks the sound (it plays as everywhere), KNOB
  1..4 set that sound's TUNE (-12..+12 semitones), DECAY and TONE (-6..+6 steps around the kit) and SOUND (KIT, or the
  piece of STD / 808 / 10 / 66 / 55 / 77: an 808 kick under a 10 snare). The screen lists all eight. Projects and user
  presets keep them (in track parameters the retired DIGITAL engine left free); a preset load sets them back.
- **DRUM, SEQ grid:** a lane's black key lights for a moment whenever that lane plays (a hi-hat's key flashes with
  each hit). VDRUM has the grid too (above).
- **A factory preset brings the sound, not the effects:** the track's sends (DIST, CHORUS, DELAY, REVERB) and its
  INSERT stay as they are when a factory preset loads (PRESETS, the EDIT layer, the editor, a Program Change, an engine
  switch, a new project's draw). A user preset brings the effects it was saved with. The power-on sounds keep their
  sends, so the power-on project sounds as it always did.
- **MENU > SYSTEM > INFO: the FPU probe.** OCT+ checks the chip's floating-point unit (which Felucca never used) and
  times it: FPU OK (results bit for bit as on a computer, a Plaits waveshaper rendered exactly as Plaits renders it),
  the ns of a float and an integer multiply-add and a float divide, and one waveshaper voice in % of the audio's
  time. It runs only on request and only after the first 30 s, so it can never count as a failed boot.

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
- **Clearing has one home per object:** REC + F3 clears the selected pattern's notes and automation. AUTOMATION's
  PLAY / CLEAR row clears its motion only. FX TRACKS + EDIT clears the selected lane after confirmation; SONG's
  F#3 deletes a section and G#3 clears the song, both with confirmation. SEQ + F#3 has no action.
- **HOME shows the four tracks** by default. MENU > DISPLAY > HOME selects TRACKS or SCOPE. Stored LEVELS maps to
  TRACKS. MIXER edits LEVEL, PAN and MUTE; REV stays on FX > SEND. VOICE 2 contains ALLOC DTUNE SPRD.
- **OCT always shifts the keyboard octave on STEP**, stopped and playing. Notes are entered with the keyboard;
  MENU > CONTROL > CHORD ENTRY ADD adds individually pressed notes to a chord.
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

## USB DISK: the projects as files

**MENU > SYSTEM > USB DISK** (OCT+) turns the FM-1 into a USB drive for as long as its screen is up: plugged into a
phone (the Files app) or a computer it shows up as **FM-1**, read-only, with

- `README.TXT`: what is on it, the slots and their projects' names, the firmware,
- `CURRENT.FM1`: the music as it is now (saved into the autosave first, as a power-on with RESTORE LAST would bring
  it back),
- `PROJ_A.FM1` .. `PROJ_D.FM1`: the project slots that hold a project.

Copy them wherever you like. Any button or key ends it, and USB is MIDI + audio again (the host sees the FM-1 unplugged
and plugged in again). A `.FM1` file holds the project exactly as the FM-1 keeps it in its flash: the project, its
extras (FX TRACKS, KEY, steps 65..128, the scene song ..), its scenes, the settings, the user sample USR1 and FM6's voice
bank, each with its flash offset and CRC (format FM1X, described in `firmware/src/usbdisk.c`). Nothing is converted, so
a reader can put the bytes back where they were: the browser emulator runs the same firmware, which is the way to an
offline bounce without the FM-1. Copying files onto the FM-1 is not there yet (the drive is write-protected).

The drive is made up as it is read (a FAT16 of 8 MiB, nothing of it stored). The transport stops and the screen stays
on USB DISK while it is up; MIDI, USB audio, the editor and the console are off for that time. Built with
`FELUCCA_DISK` (on in the firmware). Tested on the host (`tests/vdisk_test.c` reads the whole drive the way a host
does and checks it with a FAT reader of its own, `tests/vdisk_check.py`); a phone or a computer has not seen it yet.

## Bounce: the projects as WAV, without the FM-1

`web/bounce` is a page that takes a `.FM1` file (USB DISK, above) and renders it with the FM-1's own firmware as
WebAssembly, offline: no FM-1 needed, about 30 to 100 times faster than real time. Out come `NAME_MIX.wav` (16-bit,
the master exactly as USB audio sends it: USB LEVEL FIXED, SPEAKER EQ FLAT, no click) and, if wanted, the stems
`NAME_T1.wav` .. `NAME_T4.wav` and `NAME_FX.wav` (32-bit float, before the master's limiter: each track's share of the
mix, and the FX buses' returns with the FX layer's punch-ins; together they make the master up to its limiter), one by
one or as a ZIP, or shared into the Files app on a phone. What it plays: the scene (so many bars), the scene song to
its end, or the SONG chain. Each bounce boots a fresh FM-1: the same file gives the same samples every time (chance and
RANDOM fall the same way each time, not as a particular run on the device did).

The site (front page, bounce, emulator) is static: `sh tools/make_vercel_site.sh` (after `./build.sh`, with the
emsdk) builds it into `site/`, which is committed; `vercel.json` serves `site/` as it is (no build on Vercel, which has
no Emscripten). On Vercel: import the repository, keep the defaults (vercel.json sets them), deploy.

## Try it in the browser

`web/lab` is an emulator page with guided workflows (in German): the whole firmware as WebAssembly, with a
JavaScript fallback. Build it with `sh web/lab/build.sh` (after `./build.sh`, with the emsdk), then
`cd build/lab && python3 -m http.server 8000` and open `http://localhost:8000/local.html`.

## FLUTE and SAX: sample files, no longer built in

SAMPLE's FLUTE and SAX left the firmware (62 KB of flash, room for what comes next): SAMPLE's factory sound is PIANO,
GRAIN's CLOUD PAD and GLITCH grain PIANO now. Both come as slot files, `dist/samples/FLUTE.slot` and `SAX.slot`
(`python3 tools/gen_samples.py --user-slot FLUTE FLUTE dist/samples/FLUTE` builds them: the same zones, roots and
sustain loops as when they were built in). The web editor's Samples tab writes one into USR1 (**Slot file**); SAMPLE's
SET USR1 (or GRAIN's SRC USR1) plays it. A sound saved with SET FLUTE or SAX plays PIANO.

## The editor without a device

The web editor's **No device** button (beside Connect) starts the FM-1's firmware in the page (`web/emu`, the same
sources as the device's): the editor talks to it as to a device, so the Sound tab's parameter list, the engines and
their presets, the 6-OP FM tab, the library and the user presets all work, with the firmware's own sound. Saves stay
in that browser. The computer keyboard plays the selected track as Ableton's does (A W S E D F T G Y H U J K O L P ; '
from the octave's C, Z / X octave, C / V velocity), on the device too; a MIDI keyboard on the computer plays the
page's FM-1. Connected to the FM-1, the editor keeps a copy of its user presets in the browser (again after every
change); No device starts with them, so they play with the firmware's own sound and can be edited without the FM-1,
and a button in Projects > Presets on the device brings them back to it. The editor speaks English, German and Japanese. `python3 web/make_editor_artifact.py` builds it as a
claude.ai Artifact page (there without Web MIDI: No device only; files are saved through the page's downloads
capability).

## Presets as files

- **Projects > Presets on the device** (web editor): the user preset slots (U01 .. U32) as files beside the projects:
  download one, replace one with a file (its button, or drop the file on its row), upload several (a sound named
  like a slot replaces it, the others go into empty slots), download all. They show in PRESETS beside the factory
  presets.
- **Library > Export presets**: the sounds shown as a preset file. `python3 tools/import_presets.py FILE.json` makes
  them factory presets (a name of the engine's replaces that preset, any other is added; a tag BASS LEAD PAD PLUCK
  KEYS DRUM FX OTHER is its category). A factory preset keeps E0..E7, ATK DEC SUS REL, ENV > FILTER, MONO, the
  sends and a pattern hint; the script lists what else a sound had (the LFO, the matrix, ..), which a preset load
  sets to its defaults.

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

## JAN3000 controls

OCT+ and OCT- move the keyboard octave on STEP, stopped and playing. To add chord notes one at a time, set
MENU > CONTROL > CHORD ENTRY to ADD, arm STEP with REC, and press the note keys; press a note again to remove it.
The octave keys confirm or cancel dialogs and actions; held SEQ uses them for scenes.

The MIXER edits LEVEL, PAN and MUTE (KNOB 1, 2 and 4). REV is on FX > SEND only. EDIT > VOICE 2 holds
ALLOC, DTUNE and SPRD. HOME offers TRACKS and SCOPE; a saved LEVELS view loads as TRACKS.

CLICK, COUNT-IN and CLICK LEVEL are edited in the REC layer only (hold REC, KNOB 1..3). Their saved values
and their effect on recording and playback are unchanged. MENU > AUDIO no longer repeats these controls.

SEQ > DETAIL is the only editor for a step's CHANCE, RATCH, NUDGE and VEL. AUTOMATION lists parameter locks and
recorded knob moves, with PLAY / CLEAR and QUANTIZE as its controls. PRESETS always moves the STEP cursor,
even while a step is held; it never edits NUDGE. Stored step values and their playback are unchanged.

Pattern CLEAR lives on REC + F3 and clears notes and automation together. SEQ + F#3 is unassigned. AUTOMATION's
PLAY / CLEAR row clears automation only; FX TRACKS + EDIT asks before clearing the selected FX lane. On SONG,
F#3 deletes the selected section and G#3 clears the song, both with confirmation. SAVE no longer has TOOLS;
initialize a sound with EDIT held + F#5.

SELECT browses the current family everywhere, including HOME → MIXER → CLOCK → HOME (and backwards).
HOME always returns HOME. Pressing any other page button opens its family; pressing it again returns HOME,
including two quick taps. Hold the button for its live layer. Tempo is CLOCK's BPM knob or GLO held + F4 TAP;
SELECT never changes tempo, and the old BPM LOCK bit remains reserved in stored settings.

PROJECT and USER share one storage workflow: white keys select the slot, OCT+ saves and OCT- asks to load.
USER has 32 slots in two banks of 16, selected with KNOB 1; PROJECT uses its first four white keys for A–D.
F#3 renames the selected slot, G#3 asks to delete it; OCT+ confirms and OCT- cancels. Knobs no longer pick
PROJECT slots or storage actions. PROJECT's F#5 starts a new project after confirmation; an empty slot's
OCT- says EMPTY SLOT. Deleting a stored project leaves the music currently in RAM intact.

SAVE held is the sole undo; held again it redoes. A layer's persistent changes form one step, including mix,
scale, sound, favorites, drum mutes, REC settings and FX key maps. Undo also works while the layer stays open.
A recording is one step across its armed instrument tracks, automation, KEY, FX lanes or SONG chain entry;
SAVE ends its recording arms before swapping the take. REC + F#5 BACK, REC + OCT undo/redo and SEQ + EDIT
put-back are retired. OCT keys shift octave in every non-SEQ live layer, including FX LATCH.
