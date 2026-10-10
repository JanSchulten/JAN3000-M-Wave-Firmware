# OP-1 on the FM-1: the short version

Full comparison (in German): `docs/OP1_COMPARISON.md`. The OP-1 manual as Markdown: `docs/reference/op1-manual.md`.

## The one big difference

The OP-1 records **audio** (tape, album, samplers). The FM-1 has ~1/100 of the memory, so M-Vave-OP1 records
**notes and parameters** and plays them through the engines. Tape track *k* plays through synth track *k*.
Anything that depends on stored sound (album, bouncing, sampling the tape, mic / radio input) is out of reach;
anything about time and pitch (loops, varispeed, reverse, cut and paste) works on events.

## Controls

| OP-1 | FM-1 |
| --- | --- |
| Blue, green, white, orange encoders | KNOB 1..4 (keep the colours in graphics) |
| T1..T4 in synth mode: engine, envelope, effect, LFO | EDIT, ENV, FX, LFO |
| Main modes SYNTH, DRUM, TAPE, MIXER | HOME (track, sound), GLO > TAPE, HOME HOME > MIXER |
| Sound keys 1..8 | PRESETS, SAVE > USER |
| Tape tricks 1..8 | proposed: FX held on the TAPE page |
| ◀ ▶ (octave / REW FF) | OCT− OCT+ (proposed: REW / FF on the TAPE page) |
| SHIFT | held layers (FX, GLO, SCL, EDIT, REC, SEQ) |
| REC, PLAY, STOP | REC, PLAY (PLAY again stops) |

## Status and plan

Done: M1 TAPE (record, overdub, loop, speed, reverse, erase), M2 the tape kept in flash (USR3 given up), M2b from the
OP-Z: FX TRACKS (two effect lanes, punch-ins recorded with REC) and SONG as pattern chains (REC + A..D, LOOP, live).
The user asked for 6 equal tracks; RAM allows 4 instrument tracks + 2 FX tracks (a synth track costs ~23 KB).
Next: M2c OP-Z patterns (16 a project, switched while playing) and more punch-ins (DUCK, RAMP, RANDOMIZE, STEREO),
then mode mapping, tape transport and tricks, lift / drop / split, sound changes on the tape, master EQ / FX /
drive, beat match, ENDLESS / TOMBOLA / FINGER, OP-1-style engine graphics. Check `docs/ROADMAP.md`.
