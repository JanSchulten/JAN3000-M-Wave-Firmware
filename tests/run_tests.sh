#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Host tests of the Felucca sources (no hardware). Run from the repo root after ./build.sh:
#   tests/run_tests.sh
#
# Regression suite (tests/regress.c, tests/target_budget.py; details at the top of regress.c):
#   golden renders  every engine x preset, the GM map on DRUM, voice modes, FX sends, a 4-track mix: one hash
#                   each in tests/golden.txt. A change of the sound fails with the list of renders.
#   health          clipping, DC, peak level, voices free after the release, silence at the end.
#   CPU             instructions / sample per preset and mix (tests/cpu_baseline.txt, +25 %), ns printed;
#                   target: loop instructions of the render functions in the pi32v2 disassembly
#                   (tests/target_budget.txt, +10 %; exact, static).
#   voices          the budget of 8, steal fades, MONO / LEGATO / UNISON keep their note, the VOICE cap,
#                   no hanging notes on any MIDI / key routing.
# ENGINE MEMORY (tests/union_test.c, 1.5): engine switches on a part (every pair and A -> B -> A of the engines that
#                   share the part's state region, with others), hashed against 1.4's (tests/union_golden.txt).
# UI renders (tests/ui_render.c): every screen in every palette from the real drawing code: the layout lint (no text
#                   off the screen, cut, hidden, overlapping or spilling out of its cell / card; only free text
#                   ellipsised), GREY gray, MONO neutral, the draw cost, the text audit (build/ui_new/text_audit.tsv);
#                   PNGs of GREY MONO GREEN PAPER NIGHT in build/ui_new (tests/ui_render.py), the findings in build/ui_new/report.txt;
#                   FM6's 32 algorithm charts as drawn (no box overlapping, no route through a box or crossing another), again
#                   on OPERATOR with each operator picked and its frequency text at its widest (1.2, #76);
#                   DIGITAL's screens (its algorithm charts, OP ENV) with FELUCCA_FM4=1 too (build/ui_fm4: lint, GREY, MONO);
#                   every frame of the rolling digits (lint, GREY, MONO), their filmstrips in build/ui_slot;
#                   the alignment: every text / icon / keycap meant to be centred, or on its neighbours' line, by its
#                   ink against its box (cells, chips, buttons, rows, knobs, the roll's strip, the keycaps' pills), every
#                   screen in FLAT and LINE and every palette, over every value it can show; 1 px off or more fails
#                   (build/ui_new/align.txt). MENU > LARGE: every screen again (FLAT, LINE, every palette) and the page / value
#                   sweep with the tall cards, the same lint and alignment; sheet_LARGE_GREY.png, sheet_LARGE_MONO.png.
# UI (tests/ui_test.c): the UI sources against stub display / buttons / knobs: sound loads keep the steps and
#                   the track's ARP / SCL / SLICER, the SAVE > PHRASES loader and its REPLACE? dialog, the
#                   one-step undo of both (SAVE held), REC on TRACKS / SEQ / ARP, STEP and ARP while recording,
#                   MIDI IN ROUT, saves refused while playing and the OVERWRITE? dialog, MUTE on TRACKS KNOB 1, FM6's operator
#                   pages (each byte alone, SLOT OWN, the sound, undo, the macros, projects / user presets),
#                   the DRUM grid (keys, knobs, LEDs, pages, live recording into it, BEAT from PATTERNS).
# Audio / persistence / editor: the real C paths against simulated DMA and NOR flash: bounded overload
#                   fades, shared-voice limits, deferred settings and retries, failed-save rollback, the autosave
#                   (1.2: when it writes, the power-on restore, damaged copies, a write cut short, wear over a session),
#                   malformed transfers, transport-stop timeouts, MIDI and UART recovery; the MENU settings over the
#                   editor (MENU_DESC / MENU_SET: every item, clamping, unknown ids, saving, USB SERIAL applied later).
#                   M-Vave-OP1: a settings save waits for quiet (no voice, no tail in the output) as for STOP, 8 s at
#                   most; the autosave waits for the tails too.
# FM6 BANK (tests/fm6bank_test.c, 1.4.1): the voice bank (src/fm6_vbank.c) and its editor commands against simulated NOR:
#                   written whole and read back, its sectors (0xE7000, staged in the autosave's older copy, nothing else
#                   touched), a power cut after any programmed byte or in any erase (the old bank or the new one, the
#                   autosave kept), a wrong CRC, an autosave between requests, SLOT B1..B32 loading the exact records,
#                   empty bank slots skipped / refused, projects and user presets keeping a bank voice, stored SLOT values
#                   (projects, user presets, motion) loading as before.
# CHORD (tests/chord_test.c): the chord keys (src/chord.c): diatonic triads / sevenths of several scales and roots,
#                   the fixed shapes and voicings (at most 4 notes), names, MONO plays the root, a release ends
#                   exactly what its key / MIDI note started, recording, the ARP, MIDI IN, kits ignore CHRD.
# FUN10 (tests/fun10_test.c, 1.2): LFO 2 (SYNC one cycle per note value at several tempi and on the external clock, OFF bit
#                   for bit as before; TRIG NOTE / FREE; POL UNI 0..+1, the matrix's AMP), NUDGE (live recording keeps a
#                   note's time, QUANTIZE ON plays as before, OFF plays the nudges late and early once a pass, never twice
#                   with live notes, RATCH on its start, nudges 0 play the same either way, a tempo change of the external
#                   clock, SEQ > AUTOMATION's QUANTIZE and NUDGE rows, STEP steps held + PRESETS), 128 motion records, FUN10 round trip, FUN9 /
#                   FUN8 / FUN7 as 1.1 / 1.0 wrote them (tests/old_pack.h) loading exactly, user presets of 99 / 103.
# KEY (tests/key_test.c, M-Vave-OP1): GLO > KEY (key.c): the move in the scale (Am F C G in C major, the nearer way, a
#                   passing note, a root outside the scale, A minor, CHR, kits never; OFF, stopped, no root: as written),
#                   a pattern under KEY bar by bar (the run round, LEN 1, each off its on), the arp's notes moved (not
#                   while recording), live keys as they are; the page (GLO's first, the knobs, a key sets a root).
# NEW PROJECT (tests/newproj_test.c, M-Vave-OP1): a new project's sounds drawn (newproj.c): F#5 NEW (START A NEW
#                   PROJECT?), OCT- on an empty slot (NEW PROJECT IN SLOT C?, the new project there), power-on with nothing
#                   restored; T1 a factory BASS, T3 a LEAD, T4 a kit (DRUM's six, VDRUM / DBOX but INIT), T2 the pad;
#                   nothing of the old music left; 900 draws: every bass, lead and kit comes up, nothing else ever.
# KIT GRID (tests/kitgrid_test.c, M-Vave-OP1): VDRUM and DBOX on the sequencer as DRUM (kitlane.c): the lanes (VDRUM
#                   its six parts, DBOX eight, BELL its cowbell B), SEQ > STEP's grid (keys, LEDs, VDRUM's 7 / 8 dark, a
#                   DRUM beat's RIM / BELL), live recording as hits, the EDIT layer's lane mutes, SEQ TOOLS' BEAT and lane
#                   row, notes as hits when a project / scene / phrase loads (the output bit for bit the same), a user
#                   preset's grid; screens in build/kitgrid_shots/.
# DRUM KEYS (tests/drumkeys_test.c, M-Vave-OP1): on a DRUM and a VDRUM track the black keys are the grid's lanes (a lane
#                   struck as designed and picked, the ones past the lanes silent), the white keys strike the lane picked
#                   a semitone a key from F4 (OCT -/+ an octave; DRUM's oscillator at 2^(st/12), past TUNE's range;
#                   VDRUM's every part), again with its black key held, other hits as designed, REC the hit (not the
#                   pitch), the message, EDIT > SOUNDS' lane, SEQ > STEP's grid as before.
# JAN (tests/jan_test.c, M-Vave-OP1 JAN3000 v0.8): another track (ALGORITHM) opens each family at its first page; REC
#                   armed takes the track's copy, REC held + OCT- undoes the recording, OCT+ redoes it; SAVE > PROJECT >
#                   NEW (asked first, not while playing): the power-on sounds, empty steps, no song / extras / slot.
# STORES (tests/opstore_test.c, M-Vave-OP1): USR3's and USR2's flash (opstore.c) on a NOR image: a slot's FX TRACKS saved
#                   and loaded with its project (a power-on, a project without extras or written by the editor: empty, a
#                   rename keeps them, a failed write), the autosave's, a song's slots; the KEY's run with a project;
#                   a slot's scenes with its project and the autosave's (copies taking turns, a
#                   write cut short, damaged, unsound, another project's or slot's, a rename); USR2 and USR3 read as empty
#                   sample slots.
# EQ (tests/eq_test.c, M-Vave-OP1): HOME > EQ, four bands a track after its INSERT (eq.c): flat at power-on (no band
#                   runs, a block untouched), each TYPE measured with sines against the RBJ cookbook in double, four bands
#                   adding up, the glide (a quarter a block), flat again stops, a silent part drops its state, the page
#                   (SELECT's place, KNOB 1..4, RESO for LP / HP, the cards' text, each track its own, a sound loaded
#                   keeps it), kept with the project (damaged: defaults; extras of before: flat; a new project: flat).
# MASTER (tests/dyn_test.c, M-Vave-OP1): HOME > MASTER, the master's compressor and limiter before the MASTER knob
#                   (dyn.c): the defaults bit for bit the master of before at the knob's top, the knob last (half the knob
#                   exactly half), USB LEVEL FIXED, COMP's reduction against the gain computer in double, GAIN, the
#                   attack and release times, LIMIT's steps, the meters, the page (SELECT's place, KNOB 1..4, the
#                   cards' text), kept with the project (damaged: defaults; extras of before and a new project: defaults).
# SCENES (tests/scene_test.c, M-Vave-OP1): 16 scenes a project (scene.c, the SEQ layer): the packed form (3000 random
#                   scenes round-trip, the densest fits, damaged bytes refused), a pick stopped at once and playing at the
#                   pattern's end (the longest track's loop, the remainder carried), taken back, the bank full (refused,
#                   or dropped at the end: nothing lost), kept with the project, SEQ + OCT+ held the copy into the next
#                   free scene (at once, the music byte for byte, a pick taken back, refusals change nothing, kept), the
#                   SEQ layer (white keys scenes, OCT+ / OCT- next / before, black keys the tools, EDIT put back, PRESETS
#                   the lane, LEDs, no song), the KO-II flow (a punch-in held while SEQ picks); screens into
#                   build/scene_shots.
# ONE SWING (tests/swingfold_test.c, M-Vave-OP1 Phase 2): the global SWG retired; a project's goes into each track's
#                   own as it loads (at most 100), into its stored scenes and, in a song, each slot's own; CLOCK no SWG.
# ARP > J-6 (tests/j6phrase_test.c, M-Vave-OP1): the Roland J-6's 108 phrases instead of the arp (j6phrase.c,
#                   j6phrase_data.h): the data (9 x 12, inside their cycles, the Phrase List's notated ones), STYLE / VAR /
#                   SNAP appended (OFF as before, kept by a sound load), every phrase over the J-6's chord for two cycles
#                   (each note in its block, its tone, length, velocity; the rest before the first kept; another tempo),
#                   the arp's settings ignored, SNAP (stopped: the free beat's one; playing: the sequencer's; the LED's
#                   beat on it), the note-offs (let go, STYLE off, silenced, HOLD), kept with the project, the page.
# SCL > CHORD's sets (tests/cset_test.c, M-Vave-OP1): the chord sets after the Roland J-6's (cset.c, cset_data.h): the
#                   hundred sets (set 1's C, the names, 1..4 notes up), the genres KNOB 3 steps; the chords (F3 .. E4 as
#                   listed, octaves away, TRN, one voice: the lowest); through the real key input (every key a chord, a
#                   black key under WHITE, OCT, a release ends its notes; off and a kit as before; the ARP); MIDI IN;
#                   the page (one CHORD page: KNOB 1 OFF .. POW, then SET 1 .. 100: POLY, back the VOICE; a sound
#                   loaded while on; KNOB 3 GENRE; SCL held KNOB 3 and its SAVE undo), STEP entry, live recording, kept with the project; screens into build/cset_shots.
# CLOUD (tests/cloud_test.c, M-Vave-OP1): FX1's TYPE (fx1.c): the DELAY untouched, a change fades, clears, starts empty;
#                   CLOUD's grains always inside what was written (30 s of random settings), DENS, PITCH +-12 / +7, the
#                   level, FDBK bounded and dying away, CLOUD FREEZE; the FX1 / CLOUD pages, F5, kept with the project;
#                   screens into build/cloud_shots, WAV demos into build/cloud_demo.
# SLICER POSITION (tests/fxchain_test.c, M-Vave-OP1): the SLICER at the start of the chain, before the DRIVE; nothing
#                   after the reverb (the retired GATE END / PUMP END play as OFF). PUMP: the drop at each step, the four
#                   curves and their lengths, no clicks, RATE 1/4, the page.
# FX CHAIN (tests/chain_test.c, M-Vave-OP1): CHORUS -> SHIMMER -> DELAY -> REVERB in series: a CHR send fills every
#                   stage after the CHORUS, a REV send the REVERB only; MIX 2's levels (SHR 0: the SHIMMER passed by;
#                   CHR 0, REV 0: silent; turned down while it rings: no jump), the CHORUS's stereo, the levels' Q16
#                   multiply bounded on four loud tracks, the levels stored (XTRA_V9), the MIX 2 page.
# PUNCH-INS (tests/punch_test.c, M-Vave-OP1 E2): the OP-Z's punch-ins on the FX layer (perform.c): DUCK's curve on the
#                   beat (also stopped), STEREO's pan a 1/16 a side with the power kept, PITCH 5TH UP / 4TH DN's pitch
#                   with no live part, RAMP UP from standing to in time, GLITCH's slices (from the next 1/16, their
#                   sources, no click); each let go: bit for bit; MAP B (FX + ALGORITHM), its key map apart from MAP A's,
#                   kept with the settings; screens into build/punch_shots, WAV demos into build/punch_demo.
# SONG CHAIN (tests/songchain_test.c, M-Vave-OP1): SONG as the OP-Z's pattern chains: REC + A..D types a chain (a new
#                   one, repeats, sections), LOOP / STOP (+ ADD's KNOB 4, kept with the project), a new chain at the end of
#                   the loop playing, sections added and cells edited while it plays, a slot not saved silent, STOP.
# FX TRACKS (tests/fxtrack_test.c, M-Vave-OP1): two lanes of punch-in effects (fxtrack.c): an effect from its step over its
#                   TIEs, LEN and two lengths, step for step with a 1/16 track, a REPEAT on its step, mutes, STOP / PLAY;
#                   recording only armed and playing, early / late presses, TIEs, overdub, a punch inside a take, FX LATCH,
#                   the black keys; FX's pages, KNOB 1..4, REC arming, FX held + a key end to end, CLEAR.
# USB DISK (tests/vdisk_test.c, M-Vave-OP1): MENU > USB DISK's mass storage (msc.c) driven as a host drives a drive:
#                   TEST UNIT READY, INQUIRY, the capacities, MODE SENSE write-protected, a write refused, an unknown
#                   command, sense, eject; the whole made-up FAT16 (vdisk.c) read and read back by tests/vdisk_check.py,
#                   a reader of its own; README.TXT, CURRENT.FM1 and the slots' PROJ_x.FM1 the flash's own bytes.
# SCENE SONG (tests/ssong_test.c, M-Vave-OP1): SEQ > SCENE SONG's rows (+ ADD, SCENE, REPS, END, EDIT deletes), the
#                   scenes one after another at each one's end (passes counted, the switch on its block), LOOP / STOP,
#                   a row on the scene playing, NEXT BAR ignored, live edits, a pick by hand or STOP ends it, kept.
# RATCH (tests/ratchet_test.c): a step's ratchet (x1..x4): its parts in the sequencer (equal, gated, chords and drum
#                   hits whole, one chance roll, swing, no slide or tie out, STOP), FUN8 round trip and older projects x1,
#                   user preset patterns, SEQ > AUTOMATION's RATCH rows and the roll / grid drawing.
# STEP 1.5 (tests/step15_test.c, #178 #173): SEQ > STEP's KNOB 3 LEN on a note (clean TIEs written after it and taken
#                   back, stopping at the next note and at LEN, chords, undo, TIME elsewhere, the DRUM grid untouched,
#                   the longer note played), a step's VEL as it plays (96 by default, ACC 127, drum hits) and
#                   SEQ > AUTOMATION's VEL rows (SHOW / HIDE, KNOB 4, EDIT, undo).
# DETAIL 1.5.1 (tests/detail_test.c, #199): SEQ > DETAIL (STEP -> DETAIL -> AUTOMATION), KNOB 1..4 the cursor step's
#                   CHANCE RATCH NUDGE VEL walked with PRESETS (NO NOTE on a REST / empty step), the same values as
#                   AUTOMATION's rows (gone at their default; EDIT there deletes one), EDIT resets, undo per gesture, the
#                   DRUM grid's keys picking steps (several held), a chord's ratchet, playback, STEP's marks.
# MOD (tests/mod_test.c): the modulation matrix: slots that do nothing are bit-identical, every source on each
#                   kind of destination, clamping, MIDI CC1 / CC11 / aftertouch routing, the cost of 4 active
#                   slots (at most +5 %), demos in build/mod_demo/.
# SOUND (tests/sound14_test.c; #132 #104 #148): the matrix's S&H / SLEW sources (a value per LFO cycle, the glide) and
#                   DEPTH destination (the LFO's gain; none = untouched), ANALOG WAVE SYNC / SUB (pitch, no DC, bounded,
#                   the band-limited restart), SPREAD (SPRD 0 / MONO / LEGATO bit for bit, a note hard left / right, the
#                   mono sum, UNISON, the SLICER's gate and the mute key on both sides, mono sends), the cost; demos in
#                   build/sound14_demo/.
# DRIVE (tests/insert_test.c, the INSERT of 1.5; #78 #177): the per-track DRIVE (src/fx.c track_insert): TYPE OFF, MIX 0,
#                   the retired types (PHASR FLANG CHOR) and DIST bit for bit (POLY, SPREAD), each TYPE at its corners on
#                   3 and 8 voices (no DC, bounded, MIX 50 % halfway to the wet), the MIX glide and TYPE changes, MIX /
#                   TYPE as automation and locks, FUN10 and user presets of 109 and 104, the DRIVE page's labels, a
#                   power-on sound's DIST as a SOFT DRIVE, the cost per type; demos in build/insert_demo/.
# SOUND 1.5 (tests/sound15_test.c; #104 #175): ANALOG's filter TYPE (LP bit for bit, every other engine ignores it, BP / HP
#                   responses around the cutoff, RES, bounded at the corners), ENV SYNC (OFF bit for bit, the note values
#                   on the knob, attack / decay / release times at 60 / 120 / 173 BPM and on the external clock), the cost;
#                   demos in build/sound15_demo/ (analog_bp, analog_hp, env_sync).
# PERFORM (tests/perform_test.c): the FX hold layer (src/perform.c): 1/16 starts, stereo buffer effects, the
#                   too-long REPEAT, the SLICER interplay, silent layer keys, idle bit-identical, cost; build/perform_demo/;
#                   OCT UP / DN (the harmonizer): pitch, stereo, clicks, the shimmer bounded, cost; FLANGER / PHASER (1.2): no DC,
#                   bounded, the sweep in time with the tempo, clean release, stacked with REPEAT / LPF / HPF, cost; the key
#                   map (any effect or none on any key, a key remapped while held, FX LATCH); build/fx_demo/; 1.5.1: the
#                   delay's dotted TIME (lengths, the 1/4D clamp, the knob order), REV / ECHO THROW (the sends as at 127,
#                   the glide, ECHO's capped feedback bounded, the tails ringing out, assignable); build/throw_demo/.
# REVERB (tests/reverb_test.c): REVERB TYPE (src/fx.c): ROOM bit for bit as before, SPRING's decay against SIZE,
#                   its chirp (group delay rising with frequency), stability at the corners, level, a model change
#                   without a click, its cost against ROOM (+30 % at most); demos in build/fx_demo/. 1.2's HALL: SPRING
#                   bit for bit as before, decay against SIZE and DAMP, stability, exact silence after the tail (27
#                   settings and levels), no offset, level, stereo, echo density / ringing / flutter against ROOM, SPRING
#                   and COMB8 (8 combs + 4 allpasses, host only, for comparison), model changes both ways, cost; the
#                   listening set in build/reverb_demo/ (drums, pluck, piano: dry and each model at SIZE 40 / 90 / 127).
#                   M-Vave-OP1: ROOM in exact silence after its tail (27 settings and levels: no hum at DAMP 0), a
#                   delay TIME / BPM change crossfading the taps (no step larger than the echo's own).
# SAMPLE (tests/sample_test.c, 1.5): PIANO is the 5-zone piano of 1.0 .. 1.1.5 again: zones, splits, pitch over the
#                   keyboard (YIN), 0.75 s at a root, the aliases, GRAIN SRC 0;
#                   1.2's PIANO HD user-slot file (gen_samples.py --user-slot: build/piano_hd/PIANO_HD), as old users may
#                   have installed it: valid in USR1 and bit for bit as SET 0.
# INPUT (tests/input_test.c): the key / button debounce of hal/fm1_input.h against the TIMER5 scan and bouncing
#                   contacts: a press within 2 scans (<= 2.3 ms), one note per bouncy press, no early or hanging
#                   release, stray samples ignored, fast repeats, the encoders' detents; the LED scan: lit LEDs every
#                   frame, dim ones a short pulse (the second line write) every frame, each only on its own column;
#                   the breath (#119): dark .. ~60 % of lit (DIM LO ~30 %), smooth, no dark run over ~10 ms near its peak;
#                   the power-on sweep (1.1, hal/fm1_led_anim.h): its length, every LED every frame at its level, the
#                   head left to right with its tail, the buttons, the end on the idle glow, the tick as before after;
#                   the idle animation (1.2, MENU > ANIM IDLE): endless, every LED every frame, the light there and back,
#                   the buttons breathing, a key down ends it before its next frame.
# CLICK (tests/click_test.c, 1.1): the metronome (src/click.c) and the count-in (src/seq.c) through audio.c: each beat at
#                   the first sample of its block, with a 1/16 track's steps 1 5 9 13 (internal and external clock, DIV 1/8
#                   and SWING too), the accent, the frequencies, length, LEVEL, MASTER, OFF / REC / ON, USB audio bit for bit
#                   without it; COUNT-IN 1 / 2 BARS (step 1 exactly N bars after PLAY, STOP cancels, PLAY again nothing,
#                   never with an external clock), notes in its last eighth onto step 1; SYNC OUT PO (MENU > AUDIO): a
#                   220-sample pulse on the left each eighth while playing, at the DAC's full scale (not the mix's -6 dB),
#                   the mono mix on the right, none stopped or OFF; around a flash erase the ISR fades the DAC out over a
#                   half, keeps it at exactly 0, fades it in after (M-Vave-OP1); build/click_demo/.
# RETRIG (tests/retrig_test.c, M-Vave-OP1): retriggers without a click (ANLG2 MONO over-lapping notes, LOFI PULSE LD in its
#                   release: the engines' .keep), the bit-crushers rounding to the nearest step (SAMPLE BITS, LOFI CRSH,
#                   NOISE CRSH: no DC against the uncrushed note; the FX layer's CRUSH keeps silence at 0).
# USB audio (tests/uac_test.c): the UAC1 descriptors as a host parses them (with and without CDC; 44100 and
#                   48000 Hz), the ring and packetiser: 44.1 frames per packet, every frame in order (bit for bit),
#                   underrun / overrun, restart; SET_CUR; the 44.1 -> 48 kHz resampler (every frame against a
#                   double-precision one, response, SNR, cost); the stream at 48 kHz (47..49 frames per packet,
#                   fast and slow I2S clocks, switching rates mid-stream).
# web (web/test_web.mjs): the editor protocol against its mock device, whose tables must equal the
#                   firmware's (tests/descdump.c -> build/host/desc.json; the MENU settings: tests/editor_test.c -> build/host/menu.json),
#                   the package builder, the updater.
# Offline bounce (web/bounce, M-Vave-OP1, when emcc is there): the emulator's firmware with taps on the mixer (fx.c
#                   FX_PART_TAP / FX_MIX_TAP); tests/vdisk_test.c's .FM1 files checked (a byte changed, cut short: refused),
#                   loaded (a slot, the autosave), bounced for 2 bars: the master and T1 sound, the empty tracks silent,
#                   the stems add up to the master below its limiter, twice the same samples; the WAVs and the ZIP.
# Library (web/library, M-Vave-OP1, with the offline bounce): .FM1 files and ZIPs of them (stored, deflated) taken in:
#                   checked, looked at by the firmware, none twice, a damaged one refused and the rest kept; the names
#                   out; the library's ZIP back into an empty one; the master handed over in pieces while it is made:
#                   the WAV byte for byte the bounce's. (The page itself: web/library/page_test.mjs, by hand.)
# Browser emulator (web/emu, when emcc is there): the firmware in WebAssembly (build/emu) boots, plays keys and MIDI,
#                   draws, lights its LEDs, keeps a save across instances, plays a song bit for bit as the same file
#                   built with cc (web/emu/native_check.c); the cost of 1 s of a heavy song against real time.
# PHYS (tests/phys_test.c): stability over the whole parameter and pitch range, the worst-case cost against
#                   the heaviest factory preset, demos in build/phys_demo/; tests/phys_ref.cpp compares the
#                   fixed-point models with DaisySP's float originals when DaisySP is there (DAISYSP=path).
# DRUM (tests/drum_test.c): the drum voices (src/drum_voice.c): pitch, decay, centroid and level against
#                   Felucca's targets, the controls' directions, no clipping, DC, retriggers, the hat choke, the
#                   kick on a small speaker; the DRUM engine (src/eng_drum.c): its key map, the 8 lanes together,
#                   one hit per lane, the choke between lanes; the cost per voice; demos in build/drum_demo/.
# VDRUM (tests/vdrum_test.c, M-Vave-OP1): the engine (src/eng_vdrum.c): every factory kit's parts under full scale,
#                   no DC, they sound and fall silent (the resonator's tail with them), a retrigger from rest the same;
#                   the GM map, the choke groups, the resonator's pitch (TUNE), DECAY, MIX 0, its sleep; KIT OWN; one
#                   voice a part; the cost; demos in build/vdrum_demo/. tests/vdrum_ui_test.c: its EDIT > LAYER .. PART 2
#                   pages (the part struck by hand, LYR, KIT OWN, undo), its kit in projects and user presets.
# PRESET FX (tests/preset_fx_test.c, M-Vave-OP1): a factory preset brings the sound, not the effects (src/ui.c
#                   preset_fx): every factory preset, the PRESETS knob, an engine switch and a Program Change keep the
#                   track's sends and INSERT; a user preset brings the ones it was saved with; the power-on sounds load
#                   with their sends as ever.
# FPU PROBE (tests/fpu_probe_test.c, M-Vave-OP1): MENU > SYSTEM > INFO's FPU probe (src/fpu_probe.c) and the Plaits
#                   waveshaper it runs (src/plaits_ws.c): the exact checks, the waveshaper's hash the unit compares
#                   against (FPB_WS_HASH), bounded over the keyboard; OCT+ refused in the boot guard's 30 s, then a run
#                   a press, the rows drawn; a screen in build/fpu_shots/.
# RETIRED (tests/retired_test.c, M-Vave-OP1): PHASE, VOICE, TRIO, SLICE and DBOX retired (src/engines.c ENG_GONE, core.h
#                   eng_successor): none offered, silent; picked, a project's track, a user preset: the successor's first
#                   preset (DBOX DRUM, PHASE WSHAPE, TRIO HRMNC, VOICE WTABLE, SLICE SWARM), the rest as stored, the motion on the E values
#                   dropped.
# WSHAPE (tests/wshape_test.c, M-Vave-OP1): the Plaits waveshaping engine (src/eng_wshape.c around src/plaits_ws.c):
#                   every preset sounding C1..C7 without DC, bounded; the corners within the filter's knee; C4 in tune
#                   from its first 10 ms; a MONO retrigger keeps the model running, UNISON's voices at different
#                   phases; SHP moving TIMBRE, clamped; a note from silence the same twice; switches with the engines
#                   that keep part memory as from boot; demos in build/wshape_demo/.
# HRMNC (tests/harm_test.c, M-Vave-OP1): the Plaits additive engine (src/eng_harm.c around src/plaits_add.c, the
#                   filter of src/plaits_voice.c): the same checks as WSHAPE's (C1..C6 sounding: Plaits fades out the
#                   partials past Nyquist), OUT bit for bit without AUX; demos in build/harm_demo/.
# PRTCL (tests/part_test.c, M-Vave-OP1): the Plaits particle engine (src/eng_part.c around src/plaits_part.c, the
#                   diffuser once a part): presets and corners bounded without DC, the band-passes ring at the note,
#                   a note fires the particles, TIMB the density, the diffuser's tail ends, a part from rest alike,
#                   pun_sqrt is sqrtf, switches; demos in build/part_demo/.
# WTABLE (tests/wtable_test.c, M-Vave-OP1): the Plaits wavetable engine (src/eng_wtable.c around src/plaits_wt.c, its
#                   8-bit tables and the part's cache of decoded waves): WSHAPE's checks, the cache's decoding, map and
#                   eviction, 8 voices sweeping the table; demos in build/wtable_demo/.
# BUDGET (tests/budget_test.c, M-Vave-OP1): the shared voice budget (src/voice.c voice_space): NVOICE at a full load
#                   (the host's), one voice past NVOICE a half while the last half's load is under VOICE_ADMIT, up to
#                   every part's NVOICE, none at the line; four chords in one half: one past NVOICE, the rest taken.
# SWARM (tests/swarm_test.c, M-Vave-OP1): the Plaits swarm engine (src/eng_swarm.c around src/plaits_swarm.c): presets
#                   and corners bounded without DC, in tune, a note's burst, Plaits' level through the limiter, a part
#                   from rest alike, switches; demos in build/swarm_demo/.
# NOISE (tests/noise_test.c): the engine (src/eng_noise.c): COLR's slope (white, pink, brown), the filter and the
#                   register clock following the key, META periodic at the key, no DC, no clipping at the
#                   corners, a note from silence the same twice, the cost per voice; demos in build/noise_demo/.
# DIGITAL -> FM6 (tests/fm4_test.c, built with FELUCCA_FM4=1): the retired four-operator engine against its conversion
#                   (src/fm4_convert.c): routes and carriers per algorithm, the presets' PTCH, and the sound (pitch,
#                   centroid, RMS envelope) of its presets and algorithms; demos in build/fm4_demo/. tests/digital_test.c
#                   (FELUCCA_FM4=1 too): DIGITAL's operator envelopes. Default builds have no DIGITAL (engine 1 reserved).
#                   tests/fm4_div0_test.c (UBSan, #61): the conversion without a divide that can be 0 gives the values the
#                   guarded form gave (every INDEX x FLT ENV, random sounds); a 0.9 project with DIGITAL ORGAN converts.
# FM6 (tests/fm6_test.c): the 6-operator FM engine (src/eng_fm6.c, src/fm6_core.c): the 32 algorithms' carriers, the
#                   operator envelopes (stages, rates, the voice ending), bit-stable notes, a click-free retrigger, no DC /
#                   clipping over the factory patches, the macros' directions, PTCH, pack / unpack and the SysEx
#                   layouts, the 6-voice cap, the cost per voice; demos in build/fm6_demo/.
# ROBUST (tests/robust_test.c): damaged or crafted stored data and editor requests: a SLICE scan bounded by the slot,
#                   a slot that fails its check leaves no zone, engine numbers past the last refused, a retained older
#                   RAM project bounded, user preset patterns inside their fields, malformed requests never stop PLAY.
# SHIMMER (tests/shimmer_send_test.c): CHO / SHIM / DLY / REV sends, shared SIZE / DAMP / PITCH / FDBK,
#                   serial SHIMMER -> DELAY -> REVERB with CHORUS parallel; independent buffers, legacy octave
#                   bit-exact, bounded feedback and exact silence, INSERT's grouped DIST control, automation,
#                   old project / scene ids and autosave. Also run with ASan / UBSan.
# Sanitizers (ASan + UBSan, when the compiler has them; SANITIZE=0 skips): the stored-data and protocol tests again
#                   (loader, M-UPGRADE entry, editor, projects, backup, ROBUST) and three short fuzz runs with fixed
#                   seeds: tests/fuzz_ed.c (editor SysEx and raw USB-MIDI packets), tests/fuzz_proj.c (mutated project
#                   stores -> import -> restore -> render), tests/fuzz_smp.c (user sample slot headers -> scan -> SAMPLE /
#                   GRAIN / SLICE). Longer runs: build/host/asan/fuzz_ed 300000 7 (iterations, seed), the same for the others.
#                   UBSan leaves out the DSP's intended wraps (signed overflow, shifts) and the XIP rebase of a user zone's
#                   offset (bounds, object-size, pointer-overflow: correct on the device's flat flash, not in C's model).
# Change baseline entries only for reviewed, intentional differences in sound or cost;
# retain every unaffected golden / CPU / target entry. VERBOSE=1: every render.
set -e
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"
# Match the target / WebAssembly float arithmetic: Apple clang otherwise contracts multiply-adds.
CC="${CC:-cc} -O1 -ffp-contract=off -Wall -Wno-unused-function"
fail=0
run() { echo "== $1"; shift; "$@" || fail=1; }

$CC -o "$OUT/storage_test" tests/storage_test.c
run "flash storage (A/B, torn writes)" "$OUT/storage_test"

$CC -o "$OUT/upreset_test" tests/upreset_test.c
run "user presets (UP_PUT parser, bank round trip, versions, PHYS DRUM and SAMPLE PERC -> DRUM, grid records, DIGITAL kept)" "$OUT/upreset_test"

$CC -o "$OUT/input_test" tests/input_test.c
run "keys and buttons: fast press, long release, bouncy contacts (one note each), glitches, encoders" "$OUT/input_test"

$CC -o "$OUT/midi_uart_test" tests/midi_uart_test.c
run "TRS MIDI parser" "$OUT/midi_uart_test"

HALF=$(sed -n 's/^#define HALF_FRAMES \([0-9]*\).*/\1/p' firmware/src/core.h)
$CC -O2 -DT_CDC=1 -DHALF_FRAMES=$HALF -o "$OUT/uac_test" tests/uac_test.c -lm
run "USB audio input: descriptors (with CDC), ring and packets, 44.1 -> 48 kHz resampler, 48 kHz stream" "$OUT/uac_test"
$CC -O2 -DT_CDC=0 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_nocdc" tests/uac_test.c -lm
run "USB audio input: descriptors (without CDC), ring and packets, resampler, 48 kHz stream" "$OUT/uac_test_nocdc"
# USB descriptor layouts (#67): CDC UAC LAYOUT CDC-presented 48K; layout 0 and the console left out = 1.0's bytes
# (+ the 48 kHz rate with 48K = 1, the default; 48K = 0: byte for byte; + the AC collection's audio streaming
# interface first, FELUCCA_UAC_AS_FIRST = 1, the default since 1.6)
for v in 1.1.0.1.1 1.1.1.1.1 1.1.2.1.1 1.1.3.1.1 1.1.0.0.1 1.1.2.0.1 0.1.0.1.1 1.1.0.1.0 1.1.0.0.0 1.1.2.1.0 0.1.0.1.0 \
         1.0.0.1.1 1.0.1.1.1 1.0.0.0.1 0.0.0.1.1; do
    IFS=. read -r t_cdc t_uac t_lay t_on t_48 <<EOF
$v
EOF
    $CC -DT_CDC="$t_cdc" -DT_UAC="$t_uac" -DT_LAYOUT="$t_lay" -DT_ON="$t_on" -DT_48K="$t_48" -o "$OUT/usb_desc_test" \
        tests/usb_desc_test.c
    run "USB descriptors: CDC $t_cdc (presented $t_on), UAC $t_uac (48 kHz $t_48), layout $t_lay" "$OUT/usb_desc_test"
done
# M-Vave-OP1, MENU > USB DISK (FELUCCA_DISK, the firmware's default): its mass storage device besides, the MIDI + audio
# device's descriptors unchanged with it built in
for v in 1.1.0.1.1 1.1.0.0.1; do
    IFS=. read -r t_cdc t_uac t_lay t_on t_48 <<EOF
$v
EOF
    $CC -DT_CDC="$t_cdc" -DT_UAC="$t_uac" -DT_LAYOUT="$t_lay" -DT_ON="$t_on" -DT_48K="$t_48" -DT_DISK=1 \
        -o "$OUT/usb_desc_test" tests/usb_desc_test.c
    run "USB descriptors with USB DISK: CDC $t_cdc (presented $t_on), UAC $t_uac (48 kHz $t_48), the disk's own" \
        "$OUT/usb_desc_test"
done
# FELUCCA_UAC_AS_FIRST=0 (#67): the AC collection of 1.0 .. 1.5 (MIDI first), 1.0's bytes; the parser model of the
# kernel AppleUSBAudio (macOS <= 15) stops before the audio streaming interface there
for v in 1.1.0.1.1 1.1.0.0.1 1.1.0.1.0 1.1.0.0.0 0.1.0.1.1; do
    IFS=. read -r t_cdc t_uac t_lay t_on t_48 <<EOF
$v
EOF
    $CC -DT_CDC="$t_cdc" -DT_UAC="$t_uac" -DT_LAYOUT="$t_lay" -DT_ON="$t_on" -DT_48K="$t_48" -DT_ASF=0 \
        -o "$OUT/usb_desc_test" tests/usb_desc_test.c
    run "USB descriptors (AC collection MIDI first): CDC $t_cdc (presented $t_on), UAC $t_uac (48 kHz $t_48)" \
        "$OUT/usb_desc_test"
done

[ -f build/felucca.fwsc ] || { echo "run ./build.sh first"; exit 1; }

$CC -DOWN_PKG=1 -o "$OUT/ota_test" tests/ota_test.c
run "M-UPGRADE entry (own loader)" "$OUT/ota_test" build/felucca.fwsc

head -c 200000 build/felucca.bin > "$OUT/old_app.bin"
python3 tools/fm1pkg_make.py "$OUT/old_app.bin" build/loader/ota.bin "$OUT/old.fwsc" >/dev/null
$CC -o "$OUT/ldr_test" tests/ldr_test.c
run "update loader: other app -> this build" "$OUT/ldr_test" "$OUT/old.fwsc" build/felucca.fwsc

if [ -f build/gen/felucca_tables.h ]; then
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/hostsim" tests/hostsim.c -lm
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/scale_test" tests/scale_test.c -lm
    run "scales: white-key mapping and note lifecycle" "$OUT/scale_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/chord_test" tests/chord_test.c -lm
    run "chord keys: diatonic and fixed chords, voicings, MONO root, releases, recording, ARP, MIDI IN, kits" "$OUT/chord_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/speaker_test" tests/speaker_test.c -lm
    run "SPEAKER EQ: FLAT / LOWCUT / BASS+ responses, BASS+ harmonics of the bass, the sub cut, no offset after, headroom on a full-scale kick (#180)" "$OUT/speaker_test"
    run "DSP render (ANALOG preset 0)" "$OUT/hostsim" 0 0 1 "$OUT/render.wav"
    mkdir -p build/tracks_demo
    run "TRACKS: 4-track pattern, live recording (lengths, swing), voice budget, engine switch, cost" env TRACKS=build/tracks_demo "$OUT/hostsim" 0 0 1 "$OUT/tracks.wav"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/project_test" tests/project_test.c -lm
    run "project formats (FUN1..FUN5 -> FUN6, the grid and song chain; DIGITAL tracks -> FM6, SAMPLE PERC -> DRUM)" "$OUT/project_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/motion_test" tests/motion_test.c -lm
    run "motion, whole-step chance, FUN7 migration, song restore, ARP repeat and the 1.2 ARP modes" "$OUT/motion_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/ratchet_test" tests/ratchet_test.c -lm
    run "RATCH: x1..x4 in a step (notes, chords, drum hits), gates, chance, swing, projects, user presets, DETAIL RATCH editor" "$OUT/ratchet_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/key_test" tests/key_test.c -lm
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/jan_test" tests/jan_test.c -lm
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/workflow_test" tests/workflow_test.c -lm
    run "JAN3000 single-path controls: navigation, storage, clearing, step values, recording and layer undo" "$OUT/workflow_test"
    run "JAN3000: family navigation, SAVE undo / redo, PROJECT slots and NEW" "$OUT/jan_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/newproj_test" tests/newproj_test.c -lm
    run "NEW PROJECT's sounds drawn: F#5 only, empty slot load refused, power-on; every bass, lead and kit comes up, nothing else" "$OUT/newproj_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/kitgrid_test" tests/kitgrid_test.c -lm
    mkdir -p build/kitgrid_shots
    run "KIT GRID: VDRUM (6 parts) and DBOX (8 lanes) on the grid as DRUM, REC, mutes, BEAT, notes as hits on load" "$OUT/kitgrid_test" build/kitgrid_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/drumkeys_test" tests/drumkeys_test.c -lm
    run "DRUM KEYS: black keys the lanes, white keys the lane picked a semitone a key from F4 (DRUM, VDRUM), REC the hit" "$OUT/drumkeys_test"
    run "KEY: the move in the scale, a pattern under KEY bar by bar, the arp, live keys, the page" "$OUT/key_test"
    mkdir -p build/cloud_shots build/cloud_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/cloud_test" tests/cloud_test.c -lm
    run "FX1 CLOUD: granular in the delay's line (TYPE change, grains inside the line, DENS, PITCH, level, FDBK, FREEZE), pages, F5, kept" "$OUT/cloud_test" build/cloud_shots build/cloud_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/fxchain_test" tests/fxchain_test.c -lm
    mkdir -p build/fxchain_shots
    run "SLICER at the start of the chain (before the DRIVE, nothing after the reverb), PUMP" "$OUT/fxchain_test" build/fxchain_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/chain_test" tests/chain_test.c -lm
    mkdir -p build/chain_shots
    run "FX chain CHORUS -> SHIMMER -> DELAY -> REVERB: flow, MIX 2 levels, stereo chorus, bounds, stored, page" "$OUT/chain_test" build/chain_shots
    mkdir -p build/punch_shots build/punch_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/punch_test" tests/punch_test.c -lm
    run "E2 punch-ins: DUCK, STEREO, PITCH 5TH UP / 4TH DN, RAMP UP, GLITCH (each bit for bit when let go), MAP B" "$OUT/punch_test" build/punch_shots build/punch_demo
    mkdir -p build/scene_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/scene_test" tests/scene_test.c -lm
    run "SCENES: 16 a project, packed (round trip, damage), picked at the pattern's end, memory, kept, SEQ + OCT+ held copy, SEQ + OCT- held clear, the SEQ layer, KO-II" "$OUT/scene_test" build/scene_shots
    mkdir -p build/eq_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/eq_test" tests/eq_test.c -lm
    run "HOME > EQ: four bands a track (each TYPE's response against the cookbook), the glide, the page, kept with the project" "$OUT/eq_test" build/eq_shots
    mkdir -p build/dyn_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/dyn_test" tests/dyn_test.c -lm
    run "HOME > MASTER: compressor + limiter before the knob (as before at the defaults, the curve, times, LIMIT, meters), the page, kept" "$OUT/dyn_test" build/dyn_shots
    mkdir -p build/j6phrase_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/j6phrase_test" tests/j6phrase_test.c -lm
    run "ARP > J-6 (the J-6's phrases): the data, every phrase's timing and notes, the arp ignored, SNAP, the note-offs, kept, the page" "$OUT/j6phrase_test" build/j6phrase_shots
    mkdir -p build/cset_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/cset_test" tests/cset_test.c -lm
    run "SCL > CHORD's sets (after the J-6's): the data, the chords, the key input, MIDI IN, the page, STEP entry, recording, kept" "$OUT/cset_test" build/cset_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/songchain_test" tests/songchain_test.c -lm
    run "SONG as OP-Z pattern chains: typed with REC + A..D, LOOP / STOP, new chains and edits while it plays" "$OUT/songchain_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fxtrack_test" tests/fxtrack_test.c -lm
    run "FX TRACKS: lanes of punch-in effects (play, LEN, no drift, REPEAT on its step), recording (early / late, TIEs, overdub, punch, LATCH, mutes), pages" "$OUT/fxtrack_test"
    mkdir -p build/ssong_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/ssong_test" tests/ssong_test.c -lm
    run "SCENE SONG: the scenes one after another (passes, LOOP / STOP), the page, live edits, picks by hand, kept" "$OUT/ssong_test" build/ssong_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/step15_test" tests/step15_test.c -lm
    run "1.5 STEP: a note's LEN (TIEs on KNOB 3), a step's VEL (playback, AUTOMATION's VEL rows)" "$OUT/step15_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/detail_test" tests/detail_test.c -lm
    run "1.5.1 SEQ > DETAIL: a step's CHANCE RATCH NUDGE VEL on KNOB 1..4 (walk, entries, undo, DRUM, chords, playback, marks)" "$OUT/detail_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fun10_test" tests/fun10_test.c -lm
    run "1.2 (FUN10): LFO 2 SYNC / TRIG / POL, NUDGE (record, QUANTIZE, play, controls), 128 motion records, FUN10 / FUN9 / FUN8 / FUN7, user presets" "$OUT/fun10_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/midi_control_test" tests/midi_control_test.c -lm
    run "USB/TRS clock, bend, sustain, ownership and panic recovery" "$OUT/midi_control_test"
    $CC -O1 -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/digital_test" tests/digital_test.c -lm
    run "DIGITAL (retired, built here with FELUCCA_FM4=1): operator envelopes/levels" "$OUT/digital_test"
    $CC -O2 -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/fm4_test" tests/fm4_test.c -lm
    mkdir -p build/fm4_demo
    run "DIGITAL -> FM6: the conversion against DIGITAL (FELUCCA_FM4=1): pitch, centroid, RMS envelope; demos" \
        "$OUT/fm4_test" build/fm4_demo
    $CC -O2 -w -fsanitize=integer-divide-by-zero -fno-sanitize-recover=all -Ibuild/gen -Ifirmware/src \
        -o "$OUT/fm4_div0_test" tests/fm4_div0_test.c -lm
    run "DIGITAL -> FM6 without a divide by zero (#61): the same values as before, a 0.9 ORGAN project (UBSan)" \
        "$OUT/fm4_div0_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/theme_test" tests/theme_test.c -lm
    run "themes: contrast, text blending and font metrics" "$OUT/theme_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/text_ref_test" tests/text_ref_test.c -lm
    run "text: pens, kerning and pixels equal the reference renderer" "$OUT/text_ref_test"
    if python3 -c "import PIL" 2>/dev/null; then
        run "text spacing: glyph gaps against the font's own (16x supersampled), S M <= 0.25 px, L <= 0.5 px" \
            python3 tests/text_spacing_test.py build/gen/ui_fonts.h
    fi
    $CC -O1 -w -Ibuild/gen -o "$OUT/settings_test" tests/settings_test.c
    run "settings: PER1..PER4 migration, palette ids and preference preservation" "$OUT/settings_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/ui_test" tests/ui_test.c -lm
    run "UI: sounds keep steps, undo, recording, MIDI overflow, pending saves, panel recovery, drum grid, song chain, GREY gray, MONO neutral" "$OUT/ui_test"
    FV=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)   # (the splash, ABOUT: the real version)
    $CC -O1 -w "-DFELUCCA_VERSION=\"$FV\"" -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/ui_render" tests/ui_render.c -lm
    mkdir -p build/ui_new/ppm build/ui_slot
    run "UI renders: layout lint (every screen and palette, every page, engine and column value), GREY gray, MONO neutral, alignment by ink (1 px fails), draw cost" \
        "$OUT/ui_render" build/ui_new build/ui_slot
    if python3 -c "import PIL" 2>/dev/null; then python3 tests/ui_render.py build/ui_new build/ui_slot; fi
    $CC -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/ui_test_fm4" tests/ui_test.c -lm
    run "UI built with FELUCCA_FM4=1 (DIGITAL, kept in the tree): its OP pages, EDIT cycle, algorithm charts" "$OUT/ui_test_fm4"
    $CC -O1 -w -DFELUCCA_FM4=1 "-DFELUCCA_VERSION=\"$FV\"" -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/ui_render_fm4" tests/ui_render.c -lm
    mkdir -p build/ui_fm4/ppm build/ui_fm4_slot
    run "UI renders with FELUCCA_FM4=1: DIGITAL's screens (EDIT, OP ENV, the 8 algorithm charts), lint, GREY gray, MONO neutral" \
        "$OUT/ui_render_fm4" build/ui_fm4 build/ui_fm4_slot
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/audio_test" tests/audio_test.c -lm
    run "audio: overload protection, bounded fades and DMA diagnostics" "$OUT/audio_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/click_test" tests/click_test.c -lm
    run "metronome and count-in: beats sample for sample (internal and external clock), accent, level, MASTER, not in USB, count-in timing, notes onto step 1" "$OUT/click_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/retrig_test" tests/retrig_test.c -lm
    run "retriggers without a click (ANLG2, LOFI), bit-crushers without DC (SAMPLE, LOFI, NOISE, FX CRUSH)" "$OUT/retrig_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/persistence_test" tests/persistence_test.c -lm
    run "persistence: deferred settings (until STOP and quiet), retry and failed-save rollback" "$OUT/persistence_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/vdisk_test" tests/vdisk_test.c -lm
    run "USB DISK: SCSI over bulk-only, the made-up FAT16 read whole and read back (vdisk_check.py), the .FM1 files" \
        "$OUT/vdisk_test" build/vdisk
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/vdisk_screen_test" tests/vdisk_screen_test.c -lm
    run "USB DISK's screen: title, files and names, status, hint; fits, both palettes" "$OUT/vdisk_screen_test" build/vdisk
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/opstore_test" tests/opstore_test.c -lm
    run "M-Vave-OP1 stores in USR3 / USR2: FX TRACKS, the KEY and scenes with projects and the autosave, USR2 / USR3 empty" "$OUT/opstore_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/backup_test" tests/backup_test.c -lm
    run "full backup: CRC before writes, stale runtime, USB reset / timeout, malformed objects, older projects" "$OUT/backup_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/fm6bank_test" tests/fm6bank_test.c -lm
    run "FM6 voice bank (1.4.1): write / read, power cuts, CRC, SLOT B1..B32, stored SLOT values as before" "$OUT/fm6bank_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/editor_test" tests/editor_test.c -lm
    run "editor: real C protocol, malformed transfers, queue recovery, MENU settings (writes build/host/menu.json)" \
        env MENU_JSON="$OUT/menu.json" "$OUT/editor_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/robust_test" tests/robust_test.c -lm
    run "robustness: crafted sample slots, engine numbers, retained old projects, preset patterns, malformed requests" \
        "$OUT/robust_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/mod_test" tests/mod_test.c -lm
    mkdir -p build/mod_demo
    run "modulation matrix: off = bit-identical, the math, MIDI CC1 / CC11 / aftertouch, cost, demos" "$OUT/mod_test" build/mod_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/sound14_test" tests/sound14_test.c -lm
    mkdir -p build/sound14_demo
    run "sound items: matrix S&H / SLEW / DEPTH, ANALOG SYNC / SUB, SPREAD (bit for bit at 0, L / R, mono sum), cost, demos" \
        "$OUT/sound14_test" build/sound14_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/insert_test" tests/insert_test.c -lm
    mkdir -p build/insert_demo
    run "DRIVE: OFF / MIX 0 / retired types and DIST bit for bit, each TYPE (no DC, bounded, dry / wet), MIX glide, automation, stored, cost, demos" \
        "$OUT/insert_test" build/insert_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/sound15_test" tests/sound15_test.c -lm
    mkdir -p build/sound15_demo
    run "1.5 sound items: ANALOG filter TYPE LP / BP / HP (responses, LP and other engines bit for bit), ENV SYNC (times at several tempi), cost, demos" \
        "$OUT/sound15_test" build/sound15_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/slicer_test" tests/slicer_test.c -lm
    mkdir -p build/slicer_demo
    run "SLICER: no clicks, timing, sync with the sequencer, STUT, cost, demos" "$OUT/slicer_test" build/slicer_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/swing_test" tests/swing_test.c -lm
    run "SWING: track + stored global at most 100, sequencer and SLICER step lengths, the SWG display" "$OUT/swing_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/swingfold_test" tests/swingfold_test.c -lm
    run "ONE SWING: a project's global SWG into its tracks, stored scenes and song slots; CLOCK without SWG" "$OUT/swingfold_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/perform_test" tests/perform_test.c -lm
    mkdir -p build/perform_demo build/fx_demo build/throw_demo
    run "FX layer effects: on the 1/16, stereo, too-long REPEAT, SLICER, silent keys, idle bit-identical, clicks, OCT UP / DN, FLANGER, PHASER, the key map, dotted TIME, REV / ECHO THROW, cost, demos" "$OUT/perform_test" build/perform_demo build/fx_demo build/throw_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/shimmer_send_test" tests/shimmer_send_test.c -lm
    mkdir -p build/shimmer_shots
    run "SHIMMER: independent send, stereo DSP, UI, automation, old project and scene ids" "$OUT/shimmer_send_test" build/shimmer_shots
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/reverb_test" tests/reverb_test.c -lm
    mkdir -p build/reverb_demo
    run "REVERB TYPE: ROOM / SPRING bit-identical, ROOM silence, the delay TIME crossfade, SPRING and HALL decay / stability / level, HALL silence / stereo / ringing, model changes, cost, demos" "$OUT/reverb_test" build/fx_demo build/reverb_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/regress" tests/regress.c -lm
    run "regression: golden renders, health, voices, CPU budget" "$OUT/regress" tests/golden.txt tests/cpu_baseline.txt
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/union_test" tests/union_test.c -lm
    run "engine memory (1.5): every switch between the engines with per-part state sounds as in 1.4 (720 sequences)" \
        "$OUT/union_test" tests/union_golden.txt
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/descdump" tests/descdump.c -lm
    echo "== parameter and engine tables as JSON (for the editor mock test)"
    "$OUT/descdump" > "$OUT/desc.json" || fail=1
    if command -v node >/dev/null 2>&1; then
        run "M-Vave-OP1: the editor's preset file -> factory presets (tools/import_presets.py on a copy of the tree)" \
            python3 tests/import_presets_test.py "${CC%% *}"
    fi
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/phys_test" tests/phys_test.c -lm
    mkdir -p build/phys_demo
    run "PHYS: stability C-1..G9 over the parameter corners, worst-case cost against PHASE WIRE, demos" "$OUT/phys_test" build/phys_demo
    D=${DAISYSP:-vendor/DaisySP}/Source
    if [ -d "$D/PhysicalModeling" ] && command -v c++ >/dev/null 2>&1; then
        $CC -O2 -w -Ibuild/gen -Ifirmware/src -Itests -c -o "$OUT/phys_fixed.o" tests/phys_fixed.c
        c++ -O2 -std=c++14 -w -I"$D" -I"$D/Utility" -o "$OUT/phys_ref" tests/phys_ref.cpp \
            "$D/PhysicalModeling/modalvoice.cpp" "$D/PhysicalModeling/resonator.cpp" "$D/PhysicalModeling/stringvoice.cpp" \
            "$D/PhysicalModeling/KarplusString.cpp" "$D/Filters/svf.cpp" "$D/Utility/dcblock.cpp" \
            "$D/Dynamics/crossfade.cpp" "$OUT/phys_fixed.o"
        run "PHYS: the fixed-point models against DaisySP's float originals (mode frequencies, decays, Svf)" "$OUT/phys_ref"
    else
        echo "== skip PHYS reference test (no DaisySP: set DAISYSP to a checkout)"
    fi
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/drum_test" tests/drum_test.c -lm
    mkdir -p build/drum_demo
    run "DRUM: voice targets, controls, no clipping, retrigger, hat choke, the kick on a small speaker, keys, 8 lanes, cost, demos" "$OUT/drum_test" build/drum_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/vdrum_test" tests/vdrum_test.c -lm
    mkdir -p build/vdrum_demo
    run "VDRUM: kits under full scale, DC, tails end, retrigger, GM map, choke, resonator pitch / DECAY / sleep, OWN, voices, cost, demos" "$OUT/vdrum_test" build/vdrum_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/vdrum_ui_test" tests/vdrum_ui_test.c -lm
    mkdir -p build/vdrum_shots
    run "VDRUM: the part pages (part struck by hand, LYR, KIT OWN, undo), the kit in projects and user presets, screens" "$OUT/vdrum_ui_test" build/vdrum_shots
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/preset_fx_test" tests/preset_fx_test.c -lm
    run "PRESET FX: factory presets keep the track's sends and INSERT, user presets bring theirs, power-on as ever" "$OUT/preset_fx_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/retired_test" tests/retired_test.c -lm
    run "RETIRED: PHASE VOICE TRIO SLICE DBOX never offered; picked, projects, user presets load their successors" "$OUT/retired_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/wshape_test" tests/wshape_test.c -lm
    mkdir -p build/wshape_demo
    run "WSHAPE: presets sound without DC, corners bounded, in tune, MONO / UNISON phases, SHP, determinism, switches, demos" "$OUT/wshape_test" build/wshape_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/harm_test" tests/harm_test.c -lm
    mkdir -p build/harm_demo
    run "HRMNC: presets sound without DC, corners bounded, in tune, MONO / UNISON phases, SHP, determinism, switches, demos" "$OUT/harm_test" build/harm_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/part_test" tests/part_test.c -lm
    mkdir -p build/part_demo
    run "PRTCL: presets and corners bounded, no DC, rings at the note, note-on burst, density, diffuser tail, switches, demos" "$OUT/part_test" build/part_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/wtable_test" tests/wtable_test.c -lm
    mkdir -p build/wtable_demo
    run "WTABLE: presets sound without DC, bounded, in tune, MONO / UNISON, SHP, the wave cache, switches, demos" "$OUT/wtable_test" build/wtable_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/budget_test" tests/budget_test.c -lm
    run "BUDGET: NVOICE at a full load; past it one voice a half while the CPU has room; chords at once not past it" "$OUT/budget_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/swarm_test" tests/swarm_test.c -lm
    mkdir -p build/swarm_demo
    run "SWARM: presets and corners bounded, no DC, in tune, a note's burst, Plaits' level, switches, demos" "$OUT/swarm_test" build/swarm_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/acid_test" tests/acid_test.c -lm
    mkdir -p build/acid_demo
    run "ACID: presets bounded, no DC, in tune, corners silent after release, accent, sweep, slide, ENV and RES, demos" \
        "$OUT/acid_test" build/acid_demo
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/fpu_probe_test" tests/fpu_probe_test.c -lm
    mkdir -p build/fpu_shots
    run "FPU PROBE: exact checks, the waveshaper's hash, bounded; INFO's OCT+ after the boot guard, its rows, a screen" "$OUT/fpu_probe_test" build/fpu_shots
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/noise_test" tests/noise_test.c -lm
    mkdir -p build/noise_demo
    run "NOISE: colour slopes, key-tracked filter and clock, META period, DC, clipping, retrigger, cost, demos" "$OUT/noise_test" build/noise_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fm6_test" tests/fm6_test.c -lm
    mkdir -p build/fm6_demo
    run "FM6: algorithms, envelopes, retrigger, DC, clipping, macros, patch formats, voices, cost, demos" "$OUT/fm6_test" build/fm6_demo
    # SAMPLE's PIANO (1.5: as 1.1.5), 1.2's PIANO HD in a user slot
    if grep -q '^    {"PIANO", 0, 5},' build/gen/felucca_samples.h; then
        mkdir -p build/sample_demo build/piano_hd
        python3 tools/gen_samples.py --user-slot PIANO "PIANO HD" build/piano_hd/PIANO_HD >/dev/null
        $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/sample_test" tests/sample_test.c -lm
        run "SAMPLE: PIANO (5 zones as 1.1.5: splits, pitch, length, GRAIN), a PIANO HD slot" "$OUT/sample_test" \
            build/piano_hd/PIANO_HD build/sample_demo
    else
        echo "== skip SAMPLE's PIANO (no CC0 library: assets/samples-cc0, tools/fetch_cc0.py)"
    fi
else
    echo "== skip hostsim (run ./build.sh once)"
fi

# ASan + UBSan: the stored-data and protocol paths, and short fuzz runs (fixed seeds: the same inputs every run)
SAN="-O1 -g -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow"
SAN="$SAN -fno-sanitize-recover=undefined -w"
san_ok() {
    printf 'int main(void){return 0;}\n' > "$OUT/san_probe.c" &&
        ${CC%% *} $SAN -o "$OUT/san_probe" "$OUT/san_probe.c" 2>/dev/null && "$OUT/san_probe"
}
if [ "${SANITIZE:-1}" = 0 ]; then
    echo "== skip the sanitizer runs (SANITIZE=0)"
elif [ ! -f build/gen/felucca_tables.h ]; then
    echo "== skip the sanitizer runs (run ./build.sh once)"
elif ! san_ok; then
    echo "== skip the sanitizer runs (the compiler has no ASan / UBSan)"
else
    mkdir -p "$OUT/asan"
    A="$OUT/asan"
    SCC="${CC%% *} $SAN -ffp-contract=off -Ibuild/gen -Ifirmware/src"
    export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
    $SCC -o "$A/ldr_test" tests/ldr_test.c
    run "ASan/UBSan: update loader (other app -> this build)" "$A/ldr_test" "$OUT/old.fwsc" build/felucca.fwsc
    $SCC -DOWN_PKG=1 -o "$A/ota_test" tests/ota_test.c
    run "ASan/UBSan: M-UPGRADE entry (own loader)" "$A/ota_test" build/felucca.fwsc
    for t in editor_test project_test backup_test robust_test fun10_test fm6bank_test shimmer_send_test; do
        $SCC -o "$A/$t" tests/$t.c -lm
        run "ASan/UBSan: $t" "$A/$t"
    done
    $SCC -o "$A/fuzz_ed" tests/fuzz_ed.c -lm
    run "ASan/UBSan fuzz: editor SysEx and raw USB-MIDI packets (20000, seed 7)" "$A/fuzz_ed" 20000 7
    $SCC -o "$A/fuzz_proj" tests/fuzz_proj.c -lm
    run "ASan/UBSan fuzz: project stores -> import -> restore -> render (5000, seed 13)" "$A/fuzz_proj" 5000 13
    $SCC -o "$A/fuzz_smp" tests/fuzz_smp.c -lm
    run "ASan/UBSan fuzz: user sample slot headers -> scan -> SAMPLE / GRAIN / SLICE (1000, seed 17)" "$A/fuzz_smp" 1000 17
fi

run "regression: target cost of the render loops (pi32v2 disassembly)" python3 tests/target_budget.py \
    build/felucca.dis tests/target_budget.txt

run "installer CLI (fm1_install.py) against a simulated FM-1" python3 tests/install_test.py

if command -v node >/dev/null 2>&1; then
    run "web pages: editor protocol + samples, package builder, update protocol" node web/test_web.mjs
    run "web backup: capture, validation before writes, restore order" node web/test_backup.mjs
else
    echo "== skip web tests (no node)"
fi

if ! command -v emcc >/dev/null 2>&1; then
    echo "== skip the browser emulator (no emcc: Emscripten builds web/emu)"
elif ! command -v node >/dev/null 2>&1 || [ ! -f build/gen/felucca_tables.h ]; then
    echo "== skip the browser emulator (needs node and build/gen)"
else
    run "browser emulator: the firmware to WebAssembly (web/emu/build.sh -> build/emu)" sh web/emu/build.sh
    cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -o "$OUT/emu_native" web/emu/native_check.c -lm
    run "browser emulator: the same song from the native build" "$OUT/emu_native" "$OUT/emu_native.f32"
    run "browser emulator: boot, keys, MIDI, screen, LEDs, PLAY, a save kept across instances, = native, cost" \
        node web/emu/emu_test.mjs build/emu/felucca.wasm "$OUT/emu_native.f32"
    run "offline bounce: the firmware with the mixer's taps to WebAssembly (web/bounce/build.sh -> build/bounce)" \
        sh web/bounce/build.sh
    run "offline bounce: USB DISK's .FM1 files read, loaded, bounced (master + stems, = twice), WAV and ZIP" \
        node web/bounce/bounce_test.mjs build/bounce/felucca_bounce.wasm build/vdisk/files
    run "library: .FM1 files and ZIPs taken in (none twice, damaged ones refused), the master handed over while made = the bounce's" \
        node web/library/library_test.mjs build/bounce/felucca_bounce.wasm build/vdisk/files
fi

[ $fail -eq 0 ] && echo "ALL HOST TESTS PASSED" || { echo "HOST TESTS FAILED"; exit 1; }
