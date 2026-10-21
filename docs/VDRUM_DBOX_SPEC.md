# VDRUM and DBOX: two drum engines (spec)

Status: **VDRUM built** (`firmware/src/eng_vdrum.c`, `ui_vdrum.c`; tests `tests/vdrum_test.c`, `tests/vdrum_ui_test.c`),
**DBOX not yet.** The decisions come from the interviews of 2026-10-21; section 7 lists what the build changed against
the first draft.

Two new engines, each selectable on any of the four tracks like every other engine:

| Engine | Model | In one sentence |
| --- | --- | --- |
| **VDRUM** | Korg volca drum | 6 drum parts with 2 synthesis layers each, played on the GM drum notes, all into a waveguide resonator (TUBE / STRING) whose TYPE DECAY BODY TUNE are KNOB 1..4 on HOME |
| **DBOX** | OP-1 DBOX | 12 keys, each its own two-oscillator drum sound (pitch, wave, envelope, cross modulation, filter), no resonator |

Both are Felucca-style designs of our own after the two instruments' published concepts: no code, samples or tables
are taken from Korg or Teenage Engineering.

## 1. Decisions from the interview

| Question | Decision |
| --- | --- |
| Model | volca drum (6 parts, 2 layers, waveguide resonator) |
| Scope | two engines: VDRUM, and DBOX as on the OP-1 (without resonator) |
| Keys of VDRUM's parts | the GM drum notes (kick on C, as DRUM): existing patterns and MIDI drum parts play |
| HOME KNOB 1..4 (VDRUM) | TYPE, DECAY, BODY, TUNE of the resonator |
| Further engine values (VDRUM) | KIT, MIX, ACC, DRV |
| Part being edited | the last key played by hand (not by the sequencer), as on the OP-1 |
| Layers per part | 2, as on the volca drum |
| Layer editing | a LYR selector L1 / L2 / L1+L2 on every layer page (L1+L2 moves both) |
| Choke | per part: a choke group 0..3 (0 = none) |
| Storage of own kits | in the track's 128-byte FM6 patch field of projects and user presets (switching the engine to FM6 and back loses the other one's data) |
| DBOX size | 12 keys at full resolution; the second octave plays the same 12 sounds an octave higher |
| Factory kits (VDRUM) | 6..8 kits of our own design |
| Names | VDRUM, DBOX |
| Way of working | this spec first, then code with host tests and the emulator |

## 2. VDRUM

### 2.1 Signal flow

```
            layer 1 ─┐
 part k:             ├─ + ─ BIT ─ FOLD ─ DRIVE ─┬─ GAIN ──────────────► dry sum ──┐
            layer 2 ─┘                          └─ SEND ─► resonator in            │
                                                                                   ├─► out = dry + MIX · reso
                            resonator (TUBE | STRING: TUNE, DECAY, BODY) ─────────┘
 (6 parts; out is the track's mono signal: then the track's SLICER, INSERT, PAN, SPREAD, sends as for every engine)
```

### 2.2 Keys and parts

The keys play the GM drum map with the kick on the first C (as DRUM, `eng_drum.c` `drum_keys`). Each GM note goes to
one part:

| Part | Name | GM notes | Note |
| --- | --- | --- | --- |
| 1 | KICK | 35, 36 | |
| 2 | SNARE | 38, 40 | |
| 3 | CLAP | 37, 39 | rim and clap |
| 4 | HAT CL | 42, 44 | choke group 1 in the factory kits |
| 5 | HAT OP | 46, 49, 51..59 | choke group 1; cymbals land here |
| 6 | TOM | 41, 43, 45, 47, 48, 50, every other note | pitched: the note moves part 6 by its distance from 45 (low / mid / high toms from one part) |

A hit gets one voice per part (like DRUM's lanes): a new hit of the same part restarts that voice, others ring on.
A hit chokes every other sounding part of the same choke group (non-zero) with a ~1.5 ms release (as `dv_choke`).
Velocity scales the level; ACC is the extra punch at full velocity (as DRUM's ACC).

### 2.3 A layer (9 values)

| Value | Range | What it does |
| --- | --- | --- |
| SRC | SIN, SAW, HPN, LPN, BPN | the source: sine, saw (polyBLEP), white noise through a high-, low- or band-pass at PITCH |
| MOD | EXP, TRI, RND | the pitch modulator: EXP a decaying sweep (kicks, toms), TRI a triangle LFO, RND sample-and-hold |
| EG | AD, EXP, MULT | the amp envelope: AD linear attack + exponential decay; EXP an exponential attack and decay; MULT re-struck 3..4 times (claps) |
| PITCH | 0..127 | 20 Hz .. ~12 kHz, exponential (for noise: the filter's frequency) |
| MAMT | 0..127 | the modulation depth (EXP: up to +4 octaves at the start) |
| MRAT | 0..127 | the modulation rate (EXP: its decay time, TRI / RND: its frequency) |
| ATK | 0..127 | the attack (MULT: the interval between the strikes) |
| REL | 0..127 | the decay / release |
| LVL | 0..127 | the layer's level in the part |

### 2.4 A part (7 values, besides its two layers)

| Value | Range | What it does |
| --- | --- | --- |
| BIT | 0..127 | bit reduction after the layer sum (0 = none .. 2 bits) |
| FOLD | 0..127 | wavefolder depth (more overtones) |
| DRV | 0..127 | soft overdrive |
| GAIN | 0..127 | the dry level into the track's output |
| SEND | 0..127 | how much goes into the resonator |
| CHOK | 0..3 | choke group, 0 = none |
| PAN | –64..63 | **open point 1**: see section 6 |

### 2.5 The resonator

One per track, mono, after the six parts.

| Value | Range | What it does |
| --- | --- | --- |
| TYPE | TUBE, STRING | TUBE: a delay line with inverted feedback (closed–open tube: odd harmonics, round, woody, drum-shell-like); STRING: an extended Karplus-Strong loop (all harmonics, metallic, plucked) |
| DECAY | 0..127 | the loop gain: ~20 ms .. ~4 s ring time |
| BODY | 0..127 | the loop filter: dark and muffled .. bright and metallic (STRING), narrow .. wide bore (TUBE) |
| TUNE | 0..127 | the resonator's pitch, in semitones over C1..C7, fixed (it does not follow the keys, as on the volca) |

Design: an `int16_t` delay line of 2048 samples (lowest pitch ~21.5 Hz for STRING, the inverted loop of TUBE sounds
an octave below its line, so 1024 samples cover it), linear-interpolated read, a one-pole in the loop (BODY), its
delay compensated in the tuning (as `phys_dsp.c` does for its string), a DC blocker and a soft limit in the loop.
It keeps ringing after the last voice: the engine reports "still ringing" (section 4.1) until its peak has been below
–80 dBFS for 50 ms, then clears the line.

### 2.6 Engine values (track parameters P_E0..P_E7)

These are ordinary track parameters, so they can be automated, locked per step and set by the MOD matrix.

| | P_E0 | P_E1 | P_E2 | P_E3 | P_E4 | P_E5 | P_E6 | P_E7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Name | TYPE | DECAY | BODY | TUNE | KIT | MIX | ACC | DRV |
| Range | TUBE / STRING | 0..127 | 0..127 | 0..127 | F1..F8, OWN | 0..127 | 0..127 | 0..127 |

- **HOME:** `.knob = {P_E0, P_E1, P_E2, P_E3}`: TYPE, DECAY, BODY, TUNE.
- **EDIT 1 (RESO):** TYPE DECAY BODY TUNE. **EDIT 2 (KIT):** KIT MIX ACC DRV.
- **KIT** behaves like FM6's SLOT: F1..F8 load a factory kit; an edit of any part makes it OWN (the track's own kit);
  a load shows F n when the kit is that factory one unchanged.
- **DRV** is a drive on the sum (after the resonator). SPREAD is already a track parameter (`P_SPRD`), so DRV takes the
  slot.
- The track's ENV DEST / LFO DEST: FLT moves BODY, SHP moves FOLD of every part, PIT the parts' pitch.

### 2.7 Pages (EDIT family, a VDRUM track only, after EDIT 2; in the manner of `ui_fm6op.c`)

The page title names the part being edited: the last key played by hand, e.g. `PART 3 CLAP`. KNOB 1 is LYR on the
layer pages (L1, L2, L1+L2; one choice for all three, kept while one moves between them). With L1+L2 a detent moves
both layers by the same step.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| LAYER | LYR | SRC | MOD | EG |
| LAYER PITCH | LYR | PITCH | MAMT | MRAT |
| LAYER ENV | LYR | ATK | REL | LVL |
| PART | BIT | FOLD | DRV | GAIN |
| PART 2 | SEND | CHOK | (PAN) | – |

Panel graphics (step 9 of the OP-1 plan, may come later): LAYER PITCH draws the pitch curve, LAYER ENV the envelope,
the resonator pages a tube or a string that vibrates with DECAY / BODY. Every value shown goes into
`graph_signature()`.

An edit is one undo per knob gesture (as on FM6's operator pages) and makes the kit OWN.

### 2.8 Factory kits (proposal, names at most 8 characters)

| | Name | Character |
| --- | --- | --- |
| F1 | INIT | plain sine kick, noise snare, clean hats; resonator off (MIX 0) |
| F2 | ELECTRO | sweeping sine kick, saw-and-noise snare, short folded hats |
| F3 | TUBES | everything through TUBE: woody, hand-drum-like |
| F4 | STRINGS | STRING tuned to C2: plucked, tonal percussion |
| F5 | METAL | folded saws and band noise, bright STRING: bells and clang |
| F6 | DUB | long kick, rim and clap into a long dark TUBE |
| F7 | CRUSH | BIT and DRV high: lo-fi kit |
| F8 | WOOD | short TUBE, MULT claps, block-like toms |

Each kit is checked by ear in the emulator and with a host test (level, no clipping, no DC, tails end).

### 2.9 Kit format (128 bytes, every byte 0..127)

The FM6 patch field is stored as 128 bytes of 7-bit values (user presets pack them 8 to 7 bytes, `up_fm6.c`), so every
byte stays below 128.

| Bytes | Content |
| --- | --- |
| 0 | tag `0x56` ('V') |
| 1 | format version (1) |
| 2 + 21·k .. 22 + 21·k | part k (k = 0..5): layer 1 (7 bytes), layer 2 (7 bytes), part (7 bytes) |

A layer's 7 bytes: `SRC | MOD << 3 | EG << 5` (SRC 0..4: 3 bits, MOD 0..2 and EG 0..2: 2 bits each, 7 bits in all),
then PITCH, MAMT, MRAT, ATK, REL, LVL. A part's 7 bytes: BIT, FOLD, DRV, GAIN, SEND, CHOK, PAN (spare while PAN is
left out, open point 1).

6 × 21 + 2 = 128 bytes: full. A later format version must replace something, not add.

## 3. DBOX

### 3.1 Sound of a key

```
 osc 1 (PIT1, WAV1, ENV1) ──┬──────────────────────────────┐
                            │ cross modulation (XMOD, MODE)├─ + ─ filter (CUT) ─ LVL ─► out
 osc 2 (PIT2, WAV2, ENV2) ──┴──────────────────────────────┘
```

- **WAV:** a morph sine → triangle → saw → square → noise (0..127, blended between neighbours).
- **ENV:** one value per oscillator, as on the OP-1: 0..63 shortens the decay with an instant attack, 64..127
  lengthens it and adds a soft attack (short – long, attack).
- **PIT:** 20 Hz .. ~8 kHz exponential, plus a fixed short pitch drop (the drum "thump"), deeper at low ENV.
- **MODE 1..7 (cross modulation):** 1 mix (none), 2 FM (osc 2 → osc 1 phase), 3 ring, 4 hard sync, 5 AM,
  6 FM + noise, 7 osc 2 into the filter cutoff. The OP-1 manual shows a MODE dial 1..7 without saying what it
  does; this list is our own.
- **CUT:** a state-variable low-pass (`dsp.c` SVF), its resonance fixed low.

### 3.2 Keys

12 sounds: C .. B of one octave, each key its own sound. The keys of the next octave play the same 12 sounds an octave
higher (both PIT up 12 semitones). OCT− / OCT+ move the keyboard as always. The sequencer and MIDI play note n as sound
n mod 12 at octave (n div 12 – 5).

### 3.3 Per key (10 values)

PIT1, WAV1, ENV1, PIT2, WAV2, ENV2, XMOD, CUT, MODE, LVL.

### 3.4 Engine values (P_E0..P_E7)

| | P_E0 | P_E1 | P_E2 | P_E3 | P_E4 | P_E5 | P_E6 | P_E7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Name | KIT | TUNE | DECY | TONE | ACC | DRV | – | – |

TUNE, DECY and TONE move every key from its own value (64 = as stored), as on DRUM. **Open point 2**: the HOME knobs
(proposal: TUNE, DECY, TONE, DRV).

### 3.5 Pages (EDIT family, a DBOX track only)

Title: the key last played by hand (`KEY D#`). As on the OP-1, oscillator 1 without SHIFT and oscillator 2 with SHIFT;
the FM-1 has no SHIFT, so they are two pages.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| OSC 1 | PIT1 | WAV1 | ENV1 | XMOD |
| OSC 2 | PIT2 | WAV2 | ENV2 | CUT |
| KEY | MODE | LVL | – | – |

### 3.6 Kit format (128 bytes)

Byte 0 tag `0x44` ('D'), byte 1 version 1, bytes 2..121 the 12 keys of 10 bytes, 122..127 spare (0).

### 3.7 Factory kits

Four (proposal): INIT, 808-ISH, CHIP, METAL. **Open point 3.**

## 4. Firmware changes

### 4.1 Engine framework

- **Engine numbers:** VDRUM = 14, DBOX = 15, appended after SLICE (13). `NENGINES` currently is `13 + FELUCCA_SLICE`;
  with SLICE switched off 13 must stay reserved (as DIGITAL's 1 is) so the stored numbers keep their meaning.
  `ENGINE_ORDER`: both after SLICE, before DRUM (drum engines together).
- **A new optional hook `post`** in `engine_t`: `int (*post)(track_t *t, int32_t *out, uint32_t n)`, called by
  `voice.c` `track_render` after the voice loop (after `spread_sum`), returns 1 while it still produces sound.
  `track_render` returns non-zero when it did, so `fx.c` `mix_part` keeps mixing the resonator's tail after the last
  voice ended. During an engine switch's fade it is scaled like the voices. It is `noinline` and only VDRUM sets it,
  so the other engines' ISR cost does not change.
- **Voices:** both engines are `oneshot` (always POLY, no glide); VDRUM `poly = 6` (one voice per part),
  DBOX `poly = 6`.
- **Engine memory** (`engines.c` `eng_mem`, 12 888 bytes per part, zeroed on a switch): VDRUM's 6 part states and the
  resonator line (4 KB) ≈ 5 KB; DBOX's voice states < 1 KB. Both fit; `ENG_MEM_FITS` checks it.
- **The kit in RAM:** per track a 128-byte kit (`vk_kit[NTRK][128]`, ~512 B of pool), the main loop writes it and bumps
  a generation counter that the ISR takes at the next block (as `fm6_patch` / `fm6_pgen`).

### 4.2 Storage (no new flash, no new format id)

- **Projects:** the 128 bytes per track at `PROJ_FM6_OFF` hold what the track's engine needs: the packed FM6 patch for
  an FM6 track (as now), the kit for a VDRUM or DBOX track. On load, the engine number decides how they are read; a kit
  with a wrong tag or version loads the KIT's factory kit.
- **User presets:** `up_fm6.c` ties an entry to an FM6 record by its hash; this is extended to VDRUM and DBOX records
  (the entry then holds the kit). The 7-bit rule (section 2.9) keeps the 8-into-7 packing working.
- **Switching engines:** a track's FM6 patch and its kit share one field in the stored record, so the engine the
  track has when it is saved decides what is kept; the other one comes back as the init voice or the INIT kit. While the
  unit is on both stay in RAM (section 7).
- **Older firmware** refuses such a project (an unknown engine number: section 7).
- Formats stay append-only: no `P_*` id is added (the engines use P_E0..P_E7), no enum value is reused.

### 4.3 Editor protocol and web editor

Not part of the first step. The engines appear with their engine numbers and their eight values; the per-part / per-key
data is device-only until the editor learns a `KIT` message (later).

### 4.4 Files

| File | Content |
| --- | --- |
| `firmware/src/eng_vdrum.c` | the engine: parts, layers, choke, resonator, `post`, factory kits |
| `firmware/src/eng_dbox.c` | the engine: 12 key sounds, cross modulation, factory kits |
| `firmware/src/vk_kit.c` | the kit field: RAM copy, pack / unpack, tags, project and preset hooks (shared by both) |
| `firmware/src/ui_vdrum.c` | the EDIT-family pages of both engines (as `ui_fm6op.c`) |
| `firmware/src/engines.c`, `core.h`, `voice.c`, `project.c`, `up_fm6.c`, `ui_graph.c` | registration, the `post` hook, storage, signatures |
| `tests/vdrum_test.c`, `tests/dbox_test.c` | host tests (section 5) |

## 5. Budgets and tests

**Budgets** (current numbers in `.claude/skills/fm1-firmware/references/hardware.md`):

| | Limit | Plan |
| --- | --- | --- |
| Host CPU per sample (`tests/regress.c`) | heaviest engines today ~1 500 (PHASE WIRE 1 558) | VDRUM worst case (6 parts, 12 layers, resonator) ≤ 1 600; DBOX ≤ 1 200; layers at LVL 0 and silent parts are skipped |
| Target static cost (`tests/target_budget.py`) | new lines for `vdrum_render`, `vdrum_post`, `dbox_render` | `fm1_alnk0_irq` must not grow (the hook call is outside its loops) |
| RAM `.data` + `.bss` | 72 KB, ~17 KB free | < 200 B (choices, generation counters) |
| Pool | ≥ 8 KB must stay free | ~512 B (kits); the engine memory is already allocated |
| Flash (code) | ~71 KB free | estimate 12..16 KB for both engines and the pages |

**Host tests:**

- every factory kit: peak below 0 dBFS at velocity 127 with ACC 127, no DC, every part's tail ends, the resonator's tail
  ends within its DECAY + 0.5 s and the track then renders nothing;
- the GM map: each note reaches its part; part 6's pitch follows the toms;
- choke: a group's hit ends the other part within 2 ms; group 0 never chokes;
- LYR L1+L2 moves both layers; an edit makes KIT OWN; undo restores the kit;
- kit pack / unpack round trip; every byte ≤ 127; a wrong tag loads the factory kit;
- project and user preset round trip for VDRUM, DBOX and FM6 tracks side by side; switching FM6 → VDRUM → FM6;
- DBOX: each MODE renders, the second octave is an octave up;
- the UI harness: the pages render, `graph_signature()` changes with every value shown, the alignment lint passes;
- the full suite `tests/run_tests.sh` green; the CPU baseline gets the new lines (`BUDGET_UPDATE=1` only for them).

**Emulator:** a `web/lab` workflow per engine (pick VDRUM, play the kit, sweep the four HOME knobs; DBOX, edit a key),
screenshots in the colour palette. Real hardware is only installed with the user's go-ahead once everything above is
green.

## 6. Decided in the second interview

1. **PAN per part:** left out for now (the track's PAN and SPREAD apply); the kit's PAN byte is reserved (64).
2. **DBOX's HOME knobs:** TUNE, DECY, TONE, DRV.
3. **DBOX factory kits:** INIT, 808-ISH, CHIP, METAL.
4. **Resonator TUNE:** fixed, as on the volca (the keys do not move it).
5. **Order of work:** VDRUM first, then DBOX on the same base.

## 7. What the build changed against the first draft

- **Favourites:** the settings keep favourites per engine in `favorites.factory[16][32]`, but rows 14 (MIDI LEARN's
  map) and 15 (MENU and FX-layer settings) are taken. Engines from 14 on keep 32 favourites each at the end of ANALOG's
  row (bytes 24..27 for VDRUM, 28..31 for DBOX: `favorites.c favorite_cell`). DBOX (15) has room there, a third new
  engine would not.
- **The kit in RAM is separate from the FM6 patch.** Every track has its own kit (`vd_own`, 128 B of pool a track)
  next to its FM6 patch, so switching a track between FM6 and VDRUM loses nothing while the unit is on. Only the
  stored record holds one of them: the one for the track's engine when the project or user preset is saved.
- **Older firmware** refuses a project with an engine number it does not know (`project.c proj_engines_ok`): a project
  with a VDRUM track does not load on Felucca or an older M-Vave-OP1 (it shows EMPTY there), rather than playing
  ANALOG.
- **`FELUCCA_SLICE=0`** builds keep engine 13 as a silent, never selectable stub (`ENG_SLICE_GONE`), so VDRUM stays 14.
- **Undo** takes one copy per knob gesture (as FM6's operator pages): SAVE held undoes the last gesture, again redoes it.
- **The `post` hook** (`engine_t.post`) runs after `spread_sum` and gets the engine switch's fade as an argument.
- **Resonator arithmetic:** every product in its loop rounds towards zero, and its low-pass is written as
  `(1 - a) lp + a r`; a floor, or `lp += a (r - lp)`, held a small offset against the loop gain for ever (a limit
  cycle of a few units: the resonator never fell asleep).
- **Budgets after VDRUM:** image 531 500 -> 540 164 B (41 KB of code flash left), RAM unchanged (56 136 B), pool
  338 904 -> 339 544 B; static ISR cost 37 541 -> 37 899 (budget 41 252); new budget lines `vd_render` 165,
  `vd_layer_run` 115, `vd_post` 124. Host cost: six parts with both layers and the resonator ~90 ns a sample (an x86
  container; the instruction count of `tests/regress.c` needs the Mac).
