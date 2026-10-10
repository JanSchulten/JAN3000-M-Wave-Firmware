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

M1 TAPE is done (record, overdub, loop, speed, reverse, erase). Next steps in order: mode mapping, tape transport
and tricks, tape persistence (needs the user's decision on the flash trade), lift / drop / split, recording sound
changes and knob moves onto the tape, master EQ / FX / drive, beat match, the ENDLESS, TOMBOLA and FINGER
sequencers, OP-1-style engine graphics. Check `docs/ROADMAP.md` for what has landed since.
