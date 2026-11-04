# JAN3000 v0.2 alpha (M-Vave-OP1)

The second public test version of M-Vave-OP1, the OP-1 / OP-Z-inspired custom firmware for the **M-VAVE FM-1**. On
the device it calls itself **JAN3000 v0.2-alpha**, based on Felucca v1.5.1 by Leo Kuroshita (Hügelton Instruments).
GPL-3.0-only.

**What the alpha is:** the firmware, its installer and the user guide (English and German), nothing else. The web
editor, the browser emulator, the offline bounce, USB DISK and the FLUTE / SAX slot files wait in the
[backlog](BACKLOG.md). Expect rough edges, and please report what you find (GitHub issues).

## What's new in v0.2

- **ARP > J-6:** the Roland J-6's 108 phrases (its Phrase List's 9 STYLE x 12 VARIATION), recorded from a real J-6,
  played over the keys you hold instead of the arpeggiator. A third ARP page (ARP, ARP 2, J-6) with **STYLE** (OFF,
  ARP 8, ARP 16, BEAT, PHR A, PHR B, CHD S, CHD R, STR S, STR R), **VAR** (1..12), **SNAP** and **HOLD** (ARP 2's).
  A STYLE switches it on; the arpeggiator's settings rest meanwhile and come back with STYLE OFF. The phrase plays
  its own notes over the held keys, the way the J-6 voices them over a chord: chords, strums and runs, with the J-6's
  lengths and accents. The screen draws the phrase over its cycle.
- **SNAP:** ON starts each new chord on the next bar's one, silent until then: the sequencer's bar while it plays,
  otherwise the beat the ARP LED flashes on.
- **The ARP LED** now flashes exactly on the sequencer's beat while it plays.
- **The guide's download link** now points at this release's installer (v0.1's led nowhere).

**Projects:** a project saved with v0.2 keeps the J-6 settings and shows as **EMPTY** in v0.1. Projects and user
presets of v0.1 load in v0.2 as before (the J-6 OFF). If you might go back to v0.1, keep a copy of your projects
first.

## Install

1. Unzip `JAN3000-v0.2-alpha-Installer.zip` (attached to this release; also `dist/M-Vave-OP1-Installer.zip` in the
   repository).
2. Start `START-Mac.command` or `START-Windows.bat`. They serve the installer on this computer and open it in Chrome
   or Edge (Web MIDI needs Chrome or Edge).
3. Connect the FM-1 over USB and install. MENU > SYSTEM > INFO and the splash then show JAN3000 v0.2-alpha.

**Back to the official firmware:** the same installer has **Return to official V15** (M-VAVE's FM-1 V15 is in the ZIP,
checked by its size and SHA-256 before use).

**Please read before installing:** the FM-1 has one firmware bank and no recovery button. Custom firmware is
installed at your own risk. Install while the unit starts and runs normally, from a computer that stays on, with a
good cable. If an install fails and the unit no longer starts, recovery needs extra hardware (FM-1 Transporter with
an RP2040). Felucca projects and user presets load as before.

The package identity is `FM-1_900` (as every M-Vave-OP1 build; Felucca's releases are `FM-1_9XY`).

## What JAN3000 adds to Felucca

Every item is described in the user guide (online, and `web/manual/` in the repository; English and German).

- **KEY:** a chord progression over the patterns, as the OP-Z's key track, played and recorded live; one per scene.
- **SCENES:** 16 patterns a project, switched while they play; copy (SEQ + OCT+ held), clear (SEQ + OCT- held), a
  SCENE SONG.
- **SONG:** pattern chains, typed on the keys.
- **FX TRACKS:** two lanes of recordable punch-in effects; the OP-Z's punch-ins on a second key map (FX MAP B).
- **FX1** with a granular **CLOUD**, **SHIMMER** (SIZE DAMP PITCH FDBK), a serial FX chain, DIST in INSERT, the
  **SLICER** and **PUMPER** at the start or the end of the chain, CHORUS types, FX as seven cards drawn as pedals.
- **HOME > EQ:** four bands a track (low shelf, two bells, high shelf; LP / HP), kept with the project.
- **SCL > CHORD:** chord shapes and the Roland J-6's 100 chord sets on one knob.
- **Engines:** ANALOG2 (two oscillators, a filter and an LFO each), VDRUM (six drum parts into a resonator), the
  Plaits engines WSHAPE, HRMNC, PRTCL, WTABLE and SWARM, ACID (a TB-303 line); five older engines retired.
- **Sequencer:** 64 steps a track, a note turned OFF with KNOB 2, transpose a track (SEQ + A#4 / C#5), REC arms and
  PLAY starts, legato recorded as played, SAVE held is undo and redo.
- **Projects:** SAVE opens PROJECT, played on the keys; a new project draws its sounds.
- **SYNC OUT PO:** Pocket Operator sync on the headphone jack.
- **An installer ZIP** that also returns the FM-1 to M-VAVE's official V15.

## Known limits

- **Alpha:** a public test version; earlier internal builds were called v0.8 and v1.0, the public numbering starts
  with v0.1.
- **J-6 phrases:** chords and strums need a POLY sound; a mono sound plays one tone of them. For 23 phrases of STYLE
  6..9 the rest before the first note could not be recorded and is a best guess by the metre (they may start a 16th
  or two early or late against the J-6).
- **Project format:** the J-6 took the last room for track parameters in the project format; v0.1 shows a v0.2
  project as EMPTY.
- **Flash:** the extras (FX TRACKS, KEY, the scene song, the EQ) and the scenes use the flash of the second and third
  user sample slots; USR1 is the one slot left for a sample of your own.
- **No editor yet:** own samples (USR1), the FM6 voice bank (B1–B32) and FLUTE / SAX need the web editor, which is in
  the backlog. SAMPLE plays PIANO; a sound saved with SET FLUTE or SAX plays PIANO.
- **Backups:** the installer's Return to official V15 saves a backup archive first; restoring it needs the web editor,
  so keep the archive.
- **Memory:** the large buffers are nearly full (the pool at 97 %); new features will have to make room.
- **HALL headroom:** very loud, dense reverb sends from several tracks can clip the reverb's lines sooner than in
  v0.8 (the price of its lower noise).

## Thanks

Felucca by Leo Kuroshita (Hügelton Instruments), which this firmware is built on; Plaits and Rings by Emilie Gillet,
DaisySP by Electrosmith, msfa (Dexed), the Inter Tight and DotGothic16 fonts, the VSCO-2 and VCSL samples. Licences:
[LICENSING.md](../LICENSING.md).
