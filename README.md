# JAN3000 (M-Vave-OP1)

OP-1 / OP-Z-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

JAN3000 is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments). It keeps
Felucca's hardware layer, USB, update loader, storage, engines and tests, and builds an OP-1 / OP-Z-style workflow on
top of it.

> **Status:** JAN3000 v0.1 alpha, the first public test version
> ([release notes](docs/RELEASE_NOTES.md)). Installing custom firmware is at your own risk: the FM-1 has a single
> firmware bank and no recovery button. If an install fails and the unit no longer starts, recovery needs extra
> hardware (FM-1 Transporter with an RP2040).

## Manual

📖 **[User manual (English and German)](https://m-vave-op1-web.vercel.app/manual/)**: every feature and control
explained. The same manual is in this repository at [`web/manual/manual.html`](web/manual/manual.html).

## Features

**Sequencing and song structure**

- **KEY track:** a chord progression of 1 to 16 bars over the patterns, as on the OP-Z. Melodic tracks follow each
  bar's root inside their own scale; set, played live or recorded. One KEY per scene.
- **16 scenes per project:** switched while playing (at the end of the pattern or at the next bar), copied and
  cleared on the fly, chained into a **scene song**.
- **SONG:** pattern chains of project slots A to D, typed on the keys, looping or stopping.
- **Up to 64 steps per track**, a step turned off with a knob, track transpose, legato recorded as played.
- **REC arms, PLAY starts**, with an optional count-in.
- **Undo / redo** with SAVE held, for recordings and layer changes.

**Effects**

- **Two FX tracks:** lanes of recordable punch-in effects (repeat, reverse, filters, tape stop, freeze, pitch,
  flanger, phaser, throws, track mutes), each 1 to 64 steps long.
- **FX MAP B:** the OP-Z punch-ins on a second key map, including DUCK, STEREO, PITCH, RAMP UP and GLITCH.
- **FX1 with a granular CLOUD** after Mutable Instruments' Clouds, besides the classic delay.
- **Serial FX chain:** CHORUS (chorus / flanger / vibrato) → SHIMMER → DELAY → REVERB (room / spring / hall), plus
  per-track SLICER and DRIVE (soft, hard, fold, fuzz, crush). FX pages are drawn as pedals.
- **Four-band EQ per track** (shelves, bells, low- and high-pass).
- **Master compressor and limiter.**

**Chords**

- **SCL > CHORD:** chord shapes (diatonic triads and sevenths, major, minor, sus, power …) and the **100 chord sets
  of the Roland J-6** on one knob; every key plays a chord of the set, live, in steps and through the arpeggiator.

**Sound engines**

- **ANALOG2:** two oscillators, each with its own filter and LFO.
- **VDRUM:** six drum parts into a tube or string resonator, eight kits, editable per part.
- **Five Plaits engines** after Mutable Instruments' Plaits: **WSHAPE** (waveshaper), **HRMNC** (additive),
  **PRTCL** (particles), **WTABLE** (wavetables), **SWARM** (swarm of saws).
- **ACID:** a TB-303-style bass line with diode-ladder filter, accent and slide, plus a random acid-line generator.
- **DRUM:** per-sound tune, decay, tone and kit piece; drum keys play lanes and pitches.

**Projects and connectivity**

- **Project storage on the keys:** four project slots, 32 user sound slots, rename, delete, new project.
- **New projects draw their sounds** (bass, pad, lead, drum kit).
- **SYNC OUT PO:** Pocket Operator / Volca sync pulses on the headphone jack.
- **Installer that can also return the FM-1 to M-VAVE's official firmware V15.**
- Felucca projects and user presets load as before.

## Installation

**You need:** an M-VAVE FM-1 that starts and runs normally, a Mac or Windows computer with **Python 3**, **Google
Chrome or Microsoft Edge** (Web MIDI), and a good USB cable.

1. Download **`M-Vave-OP1-Installer.zip`** from the
   [releases page](https://github.com/JanSchulten/JAN3000-M-Wave-Firmware/releases) (it is also in this repository
   at [`dist/M-Vave-OP1-Installer.zip`](dist/M-Vave-OP1-Installer.zip)).
2. Unzip it into a new folder.
3. Start the installer:
   - **Mac:** double-click `START-Mac.command` (the first time: right-click → Open, if macOS warns).
   - **Windows:** double-click `START-Windows.bat`.

   It serves the installer on your computer and opens it in Chrome or Edge. If the browser does not open, copy the
   address shown in the starter window into Chrome or Edge. Keep the starter window open while you install.
4. Connect the FM-1 via USB, click **Install** and allow MIDI (SysEx) access.
5. Wait until the progress bar reports **done**. The FM-1 restarts; the splash and MENU > SYSTEM > INFO show
   **JAN3000 v0.1-alpha**.

**While installing, never** unplug the cable, switch off the FM-1, close or reload the browser tab, or let the
computer go to sleep.

**If the install is interrupted:** the FM-1 usually waits in update mode. Open the same installer again and click
**Install** once more; it resumes the interrupted write.

**Back to the official firmware:** in the installer, open **Return to official V15** and use the V15 included in the
ZIP. It backs up your music to a file on your computer first; keep that file.

## Development

- Building the firmware: [BUILDING.md](BUILDING.md)
- Contributing: [CONTRIBUTING.md](CONTRIBUTING.md)
- Roadmap and backlog: [docs/ROADMAP.md](docs/ROADMAP.md), [docs/BACKLOG.md](docs/BACKLOG.md)
- Felucca's history is merged into this repository; upstream changes can be pulled in with:

  ```sh
  git remote add upstream https://github.com/hugelton/Felucca.git
  git fetch upstream
  git merge upstream/main
  ```

## Licence

GPL-3.0-only, like Felucca. Bundled and ported parts keep their own licences; see [LICENSING.md](LICENSING.md) and
`LICENSES/`. If you distribute this firmware, you must also offer its complete corresponding source under the same
licence.
