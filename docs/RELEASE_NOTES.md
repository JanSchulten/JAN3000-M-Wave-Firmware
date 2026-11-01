# JAN3000 v0.1 alpha (M-Vave-OP1)

The first public test version of M-Vave-OP1, the OP-1 / OP-Z-inspired custom firmware for the **M-VAVE FM-1**. On the
device it calls itself **JAN3000 v0.1-alpha**, based on Felucca v1.5.1 by Leo Kuroshita (Hügelton Instruments).
GPL-3.0-only.

**What the alpha is:** the firmware, its installer and the user guide (English and German), nothing else. The web
editor, the browser emulator, the offline bounce, USB DISK and the FLUTE / SAX slot files wait in the
[backlog](BACKLOG.md). Expect rough edges, and please report what you find (GitHub issues).

## Install

1. Unzip `M-Vave-OP1-Installer.zip` (attached to this release; also `dist/` in the repository).
2. Start `START-Mac.command` or `START-Windows.bat`. They serve the installer on this computer and open it in Chrome
   or Edge (Web MIDI needs Chrome or Edge).
3. Connect the FM-1 over USB and install. MENU > SYSTEM > INFO and the splash then show JAN3000 v0.1-alpha.

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

- **Alpha:** the first public test version; earlier internal builds were called v0.8 and v1.0, the public numbering
  starts here.
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
