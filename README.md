# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1 / OP-Z-style workflow on top: a KEY track that moves the patterns
through a chord progression, and from the OP-Z two FX tracks with recordable punch-in effects, the OP-Z's punch-ins on a second key map, 16 scenes a project
switched while they play, pattern chains, a switchable FX1 with a granular CLOUD and, after ChoralRoot FM-1, the
Orchid's chord keyboard as SCL > ORCHID. What is planned next is in the [roadmap](docs/ROADMAP.md).

> **Status:** early development. Nothing here has been flashed to hardware yet.
> Installing custom firmware is at your own risk. The FM-1 has a single firmware bank and no
> recovery button; if an install fails and the unit will not start, recovery needs extra
> hardware (FM-1 Transporter with an RP2040). Develop and try changes in the host tests and the
> browser emulator first.

## KEY: a chord progression over the patterns

As the OP-Z's key track: a run of 1 to 16 bars, each with a root. While KEY is ON and the transport runs, every
melodic track's sequenced notes and its arp move to the bar's root **inside the track's own scale** (SCL ROOT and
SCALE): a pattern written in C major plays under KEY **Am F C G** as A minor, F major, C major and G major, the
scale's own chords. The keys you play live stay as they are, so you can play along; drums and other kits never move.

- **GLO** opens **KEY** (tap GLO again: SONG). The 16 bars show the chord each root makes in the selected track's
  scale; the bar playing is lit.
- **A key** sets the root of the bar under the cursor and moves the cursor on (round at LEN): play A F C G for
  Am F C G.
- **KNOB 1 BAR** moves the cursor, **KNOB 2 ROOT** changes its root (- C .. B; - plays as written), **KNOB 3 LEN**
  1 to 16 bars, **KNOB 4** KEY ON / OFF.
- The move goes the nearer way round (at most half the scale's degrees up or down), so the parts stay near where
  they are written. A root outside the scale moves the notes chromatically; a track in CHR uses its ROOT's major
  scale. A note between the scale's degrees keeps its distance above its degree.
- The bars count from PLAY on the sequencer's grid. While a track records, its arp plays as played (the steps it
  writes move when they play). KEY is kept with the project.
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

## SCENES

16 scenes in a project, as the OP-Z's 16 patterns. A scene is the four tracks' sequences: their steps, LEN DIV SWING
GATE and QUANTIZE, the automation and the locks, and the two FX TRACKS. The sounds, the mixer, ARP, SCL, FX1 and the
song stay the project's.

- Hold **SEQ**: the layer says **[SEQ] SCENES**. The **white keys F3 .. G5 are scenes 1 .. 16**; **OCT+ / OCT-** pick the
  next / the one before (from the scene waiting, else the one playing).
- Stopped, a scene comes at once. **Playing, it comes at the end of the pattern**: the end of the loop of the longest
  track that plays something, on the grid, as a SONG section ends. The scene playing picked again takes the pick
  back (SCENE n STAYS). STOP with one waiting: it comes at once.
- An empty scene starts with the LEN DIV SWING GATE of the scene it is reached from. The cells: the scene playing
  filled (its key lit), one waiting ringed (its key breathing), one that holds something raised, an empty one
  dimmed.
- The **tools are on the black keys** now, F#3 .. F#5 in Felucca's order: CLEAR, REVERSE, SHIFT < >, RANDOM, COOK, on
  a DRUM track BEAT and the lane's CLEAR, REVERSE, FILL, RANDOM (**PRESETS** picks the lane). **SEQ + EDIT** puts the
  track back as the layer opened (it was OCT-). KNOB 1..4 are LEN DIV SWING GATE as before.
- **As on the KO-II:** a punch-in stays on while its key is held, also after FX is let go. Hold SEQ meanwhile and
  pick the next scene; let go of the key and the effect stops.
- The 15 scenes not playing wait packed in 7.7 KB (a scene of four 16-step parts takes about 250 to 900 bytes, the
  densest possible 3.9 KB). A pick that would not fit says **SCENE MEMORY FULL** and nothing changes.
- Kept with the project (SAVE > PROJECT, the one playing too) and with the autosave. A project of Felucca's or of an
  older M-Vave-OP1 loads as scene 1. While a song plays (GLO > SONG) scenes are not picked (STOP THE SONG FIRST); a
  song plays each slot as it was saved.

## SCL > ORCHID (the Orchid's keyboard)

SCL's third page (SCL, CHORD, **ORCHID**) turns the keyboard into the Telepathic Instruments Orchid's, as
[ChoralRoot FM-1](https://github.com/Quixotic7/ChoralRootFM1) by Quixotic7 does (GPL-3.0; its chord block, tables,
voicing and LOCK are rewritten in `firmware/src/orchid.c`). **KNOB 1** turns it on or off for the selected track
(each track its own, before the track's CHRD); the ARP page is Felucca's and arpeggiates the Orchid's chords too.

- **F3 .. C#4 are the chord block** and play nothing themselves. Black keys: **F#3 DIM, G#3 MIN, A#3 MAJ, C#4 SUS**.
  White keys: **F3 6, G3 m7, A3 M7, C4 9** (added to the type), **B3 LOCK**.
- **D4 .. G5 are the roots.** A root plays its chord with the block's keys held now (the type pressed last counts);
  with no type the root alone. The chord is fixed when the root goes down; letting go of the root ends it.
- **KNOB 2 VOICE** (-12 .. +12): each step up moves the lowest note an octave up, each step down the highest an octave
  down. **KNOB 3 LOCK** (or B3): chord keys let go stay latched (lit). With LOCK on, a type pressed alone resets the
  latch to that type; the only latched type pressed again alone turns it off (the extensions wait for the next
  type); an extension pressed alone toggles. LOCK off clears the latch.
- The page shows the chord as the Orchid does (its Geek Out view): the name large (DM, Dm, D 7 9, Dsus m7; three
  extensions JAZZ, four WTF), a keyboard with the chord's notes lit and the notes by name. The block's LEDs show the
  keys held or latched.
- **One voice:** a track on VOICE MONO, LEGATO or UNISON plays the root alone (the page says ROOT ONLY; track 1 is
  LEGATO by default). A DRUM or other kit track has no chord block.
- STEP entry writes the chord (up to four notes a step, as the keys' chords); live recording takes the
  notes as played. ON and VOICE are kept with the project; LOCK and the latch start off after a load.

## Other changes to Felucca's workflow

- **EDIT as rows:** EDIT's pages without a picture of their own (EDIT 1 / 2, FILTER, VOICE ..) show four rows under
  the cards in place of the scope: every page's values, the row the knobs edit raised. A tap of EDIT goes to the next
  row. WHEEL, SAMPLE and FM6 keep their pictures.
- **FX in seven cards, six drawn as effect pedals** (each in the colours of a classic of its kind: an orange drive,
  a light blue chorus, a magenta flanger, a teal vibrato, a green delay or a purple CLOUD box, a blue room, a brown
  spring, a silver hall, a pearl shimmer, a yellow slicer): the knobs where KNOB 1..4 sit (turned: the value under
  it), a window with the effect moving, the name, the LED and the footswitch. A tap of FX the next: **1 SEND** (not a
  pedal: four mixer faders, the track's DST CHO DLY REV), **2 INSERT** (TYPE A B MIX), **3 CHORUS** (TYPE CHORUS /
  FLANGER / VIBRATO, RATE, DEPTH, FDBK), **4 DELAY** (TYPE DELAY: TIME FDBK MIX; CLOUD, the granular: SIZE PITCH
  DENS), **5 REVERB** (TYPE ROOM / SPRING / HALL / SHIMMER, SIZE, DAMP), **6 SLICER**, **7 FX TRACKS**.
- **CHORUS TYPE:** CHORUS is Felucca's (FDBK 0: bit for bit); FLANGER sweeps a 0.5 .. 6 ms comb, FDBK up to 0.87 for
  the jet; VIBRATO a 1 .. 10 ms line, the copy's pitch swinging (the bus is all wet: CHO up and the track's LEVEL down
  for the vibrato alone). TYPE and FDBK are kept with the project.
- **REVERB SHIMMER:** the HALL with its tail shifted an octave up and fed back into it, so the tail climbs an octave
  a pass (each one darker and quieter: it dies away at any SIZE).
- **ARP on one screen:** ARP and ARP 2 show both their rows (MODE RATE OCT GATE, SWING PROB HOLD ORDER; the row the
  knobs edit raised, an ARP tap goes to the other) and under them a keyboard: the notes the arp runs through lit (the
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
- **SYNC OUT PO** (MENU > AUDIO > SYNC OUT: OFF, PO): Pocket Operator sync on the headphone jack while playing. The
  left channel carries a 5 ms pulse each eighth note (2 a quarter, as the POs' SY modes and the Volcas expect), the
  right one the mono mix. A stereo to two mono cable: left into the PO's or Volca's sync input, right as audio.
  USB audio stays stereo. The FM-1's TRS MIDI jack is an input only, so this is the way out for a clock.

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
