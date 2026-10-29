# JAN3000 v1.0 (M-Vave-OP1)

The first release of M-Vave-OP1, the OP-1 / OP-Z-inspired custom firmware for the **M-VAVE FM-1**. On the device it
calls itself **JAN3000 v1.0**, based on Felucca v1.5.1 by Leo Kuroshita (Hügelton Instruments). GPL-3.0-only.

## Install

1. Unzip `M-Vave-OP1-Installer.zip` (attached to this release; also `dist/` in the repository).
2. Start `START-Mac.command` or `START-Windows.bat`. They serve the installer on this computer and open it in Chrome
   or Edge (Web MIDI needs Chrome or Edge).
3. Connect the FM-1 over USB and install. MENU > SYSTEM > INFO and the splash then show JAN3000 v1.0.

**Back to the official firmware:** the same installer has **Return to official V15** (M-VAVE's FM-1 V15 is in the ZIP,
checked by its size and SHA-256 before use).

**Please read before installing:** the FM-1 has one firmware bank and no recovery button. Custom firmware is
installed at your own risk. Install while the unit starts and runs normally, from a computer that stays on, with a
good cable. If an install fails and the unit no longer starts, recovery needs extra hardware (FM-1 Transporter with
an RP2040). Felucca projects and user presets load as before.

The package identity is `FM-1_900` (as every M-Vave-OP1 build; Felucca's releases are `FM-1_9XY`).

## What JAN3000 adds to Felucca

Every item is described in the [README](../README.md) and the user guide (`web/manual/`, English and German).

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
- **Projects:** SAVE opens PROJECT, played on the keys; a new project draws its sounds; USB DISK shows the projects
  as files.
- **SYNC OUT PO:** Pocket Operator sync on the headphone jack.
- **The web editor** without a device (the firmware in the page), presets as files, an installer ZIP.

## Since JAN3000 v0.8

- HOME > EQ; clear a scene; transpose a track.
- **The user guide** (`web/manual/`, English and German) now explains every feature: ACID, the SLICER's PUMP and its
  END modes, the scene song, USB DISK and the web editor (without a device, presets as files, FLUTE and SAX
  as slot files).
- **HALL reverb, cleaner highs:** a half-band filter for its internal rate change (no mirror tones in the tail), and
  its lines 6 dB louder inside (6 to 12 dB less noise as a tail fades). The tail rings about 20 % longer at the same
  SIZE than in v0.8 (SIZE 90: 2.9 s instead of 2.4 s).
- A fix: clearing a scene while another was about to start, with a damaged scene bank, could switch to a half-filled
  scene; it now keeps the scene playing.
- Inside, invisible on the device: each page's own code in one place (`firmware/src/ui_pages.c`), dead code of
  retired pages removed, and tools that prove a change leaves every screen the same (`tools/ui_compare.sh`).

## Known limits

- **Flash:** the extras (FX TRACKS, KEY, the scene song, the EQ) and the scenes use the flash of the second and third
  user sample slots; the web editor uploads samples into USR1 only.
- **FLUTE and SAX** are no longer built in: they come as slot files (`dist/samples/`), written into USR1 by the editor.
- **Memory:** the large buffers are nearly full (the pool at 97 %); new features will have to make room.
- **USB DISK** is tested in the host tests (a FAT reader of its own reads the whole drive); few phones and computers
  have seen it yet. Please report one that does not show the FM-1 drive.
- **HALL headroom:** very loud, dense reverb sends from several tracks can clip the reverb's lines sooner than in
  v0.8 (the price of its lower noise).

## Thanks

Felucca by Leo Kuroshita (Hügelton Instruments), which this firmware is built on; Plaits and Rings by Emilie Gillet,
DaisySP by Electrosmith, msfa (Dexed), the Inter Tight and DotGothic16 fonts, the VSCO-2 and VCSL samples. Licences:
[LICENSING.md](../LICENSING.md).
