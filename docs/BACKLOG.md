# Backlog: what the alpha leaves out

JAN3000 v0.1 alpha ships three things: **the firmware**, **its installer** (the installer ZIP) and **the user guide**
(online and in `web/manual/`). Everything else was taken out of the guide, the ZIP and the site for now. Nothing was
deleted: the code stays in this repository, its host and web tests still run, and each item below says how to switch
it back on and what it needs first.

The guide's sections on these items are in the history: `git show 6e616e2:web/manual/manual.html` (the last version
before the alpha; English ids `st-browser`, `sv-disk`, `sv-editor`, German `de-start-browser`, `de-sp-disk`,
`de-sp-editor`, and the editor notes under FM6 SLOT, ACID and SAMPLE).

| Item | Out of | Code | Back on with | Needs before it comes back |
| --- | --- | --- | --- | --- |
| **Web editor** (sounds, patterns, settings over USB; library, user presets as files, full backup, FM6 voice bank as .syx, TD-3 grid for ACID) | ZIP, guide, README | `web/editor.html`, `web/EDITOR_PROTOCOL.md`, `web/fm1backup.js` | `WITH_EDITOR=1 sh tools/make_installer_zip.sh` | A test round on a real FM-1 with this firmware; the guide's section back (EN + DE). The installer's backup text (`stockBackup` in `web/index_pkg.html`) says restoring needs the editor and that the alpha has none: reword it when the editor is back. |
| **Editor without a device** (NO DEVICE: the firmware as WebAssembly inside the editor) | ZIP, guide, README | `web/emu/` | `WITH_EDITOR=1` with Emscripten on the PATH | Comes with the editor. |
| **Browser emulator** (the panel on screen, played with mouse, touch and keyboard) | site, guide, README | `web/emu/`, `web/lab/` | `WITH_EXTRAS=1 sh tools/make_vercel_site.sh` (after `./build.sh`, with emsdk) and a card on `web/site/index.html` | A decision which page is the public one (`site/emu` or `web/lab` with its guided workflows, German only). |
| **Bounce** (a `.FM1` file to WAV, offline) | site, README | `web/bounce/` | `WITH_EXTRAS=1 sh tools/make_vercel_site.sh` and a card on `web/site/index.html` | USB DISK (it reads its files). |
| **USB DISK** (the projects as files, read-only) | guide, README | `firmware/src/usbdisk.c`, `tests/vdisk_test.c` | Already in the firmware (MENU > SYSTEM > USB DISK, `FELUCCA_DISK`); only the description is out | A phone and a computer have not seen the drive yet: test both, then the guide's section back. |
| **FLUTE and SAX slot files** | ZIP, guide, README | `dist/samples/*.slot`, `tools/gen_samples.py --user-slot` | `WITH_EDITOR=1` (the ZIP's `samples/`) | The editor (its Samples tab writes them into USR1). |
| **Own samples into USR1, FM6 voice bank B1–B32** | guide | the editor; `tools/fm1_sample_upload.py` | with the editor | The guide now says these come with a later release. |

## Release steps of the alpha (for the next one)

1. `./build.sh --release 0.1-alpha` (JAN3000 `v0.1-alpha` on the splash and in INFO; `build/jan3000-0.1-alpha.fwsc`,
   `build/release-0.1-alpha/`).
2. `sh tools/make_installer_zip.sh dist/M-Vave-OP1-Installer.zip build/jan3000-0.1-alpha.fwsc`.
3. `sh tools/make_vercel_site.sh`, commit `site/`.
4. Tag `v0.1-alpha`, a GitHub release (pre-release) with `M-Vave-OP1-Installer.zip` attached under exactly that name:
   the guide and the site link `releases/download/v0.1-alpha/M-Vave-OP1-Installer.zip`.

## The README's sections on these items

Moved here as they were in the README before the alpha.

### USB DISK: the projects as files

**MENU > SYSTEM > USB DISK** (OCT+) turns the FM-1 into a USB drive for as long as its screen is up: plugged into a
phone (the Files app) or a computer it shows up as **FM-1**, read-only, with

- `README.TXT`: what is on it, the slots and their projects' names, the firmware,
- `CURRENT.FM1`: the music as it is now (saved into the autosave first, as a power-on with RESTORE LAST would bring
  it back),
- `PROJ_A.FM1` .. `PROJ_D.FM1`: the project slots that hold a project.

Copy them wherever you like. Any button or key ends it, and USB is MIDI + audio again (the host sees the FM-1 unplugged
and plugged in again). A `.FM1` file holds the project exactly as the FM-1 keeps it in its flash: the project, its
extras (FX TRACKS, KEY, the scene song ..), its scenes, the settings, the user sample USR1 and FM6's voice
bank, each with its flash offset and CRC (format FM1X, described in `firmware/src/usbdisk.c`). Nothing is converted, so
a reader can put the bytes back where they were: the browser emulator runs the same firmware, which is the way to an
offline bounce without the FM-1. Copying files onto the FM-1 is not there yet (the drive is write-protected).

The drive is made up as it is read (a FAT16 of 8 MiB, nothing of it stored). The transport stops and the screen stays
on USB DISK while it is up; MIDI, USB audio, the editor and the console are off for that time. Built with
`FELUCCA_DISK` (on in the firmware). Tested on the host (`tests/vdisk_test.c` reads the whole drive the way a host
does and checks it with a FAT reader of its own, `tests/vdisk_check.py`); a phone or a computer has not seen it yet.

### Bounce: the projects as WAV, without the FM-1

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

### Try it in the browser

`web/lab` is an emulator page with guided workflows (in German): the whole firmware as WebAssembly, with a
JavaScript fallback. Build it with `sh web/lab/build.sh` (after `./build.sh`, with the emsdk), then
`cd build/lab && python3 -m http.server 8000` and open `http://localhost:8000/local.html`.

### FLUTE and SAX: sample files, no longer built in

SAMPLE's FLUTE and SAX left the firmware (62 KB of flash, room for what comes next): SAMPLE's factory sound is PIANO,
GRAIN's CLOUD PAD and GLITCH grain PIANO now. Both come as slot files, `dist/samples/FLUTE.slot` and `SAX.slot`
(`python3 tools/gen_samples.py --user-slot FLUTE FLUTE dist/samples/FLUTE` builds them: the same zones, roots and
sustain loops as when they were built in). The web editor's Samples tab writes one into USR1 (**Slot file**); SAMPLE's
SET USR1 (or GRAIN's SRC USR1) plays it. A sound saved with SET FLUTE or SAX plays PIANO.

### The editor without a device

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

### Presets as files

- **Projects > Presets on the device** (web editor): the user preset slots (U01 .. U32) as files beside the projects:
  download one, replace one with a file (its button, or drop the file on its row), upload several (a sound named
  like a slot replaces it, the others go into empty slots), download all. They show in PRESETS beside the factory
  presets.
- **Library > Export presets**: the sounds shown as a preset file. `python3 tools/import_presets.py FILE.json` makes
  them factory presets (a name of the engine's replaces that preset, any other is added; a tag BASS LEAD PAD PLUCK
  KEYS DRUM FX OTHER is its category). A factory preset keeps E0..E7, ATK DEC SUS REL, ENV > FILTER, MONO, the
  sends and a pattern hint; the script lists what else a sound had (the LFO, the matrix, ..), which a preset load
  sets to its defaults.


### The web editor's TD-3 grid (ACID)

- **The web editor's TD-3 grid** (Sequencer tab, an ACID track): 16 columns of note, octave up / down, time (note,
  tie, rest), ACC and SLD, written to the track's steps as you click; "Roll acid" rolls a line as RANDOM does.
