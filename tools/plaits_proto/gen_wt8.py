# SPDX-License-Identifier: GPL-3.0-only
"""Plaits' wavetables (plaits/resources.cc wav_integrated_waves: 192 integrated waves of 132 int16) in 8 bits for the
FM-1 (docs/PLAITS_PLAN.md): each wave's integral on a grid of PWT_GRID (9), stored as its first value and 131 int8
steps. The grid is re-quantized at every sample (the rounded integral's differences), so its error never adds up: every
value within 4 of Plaits' (of +-32768), a wave's period closes as Plaits' does, the waveform (the steps) about 42 dB
clean. gen_wt8.py RESOURCES_CC OUT_H"""
import re
import sys

GRID = 9
src = open(sys.argv[1]).read()
m = re.search(r"const int16_t wav_integrated_waves\[\] = \{(.*?)\};", src, re.S)
v = [int(x) for x in m.group(1).replace("\n", " ").split(",") if x.strip()]
assert len(v) == 192 * 132, len(v)
first, steps, worst = [], [], 0
for w in range(192):
    t = v[w * 132:(w + 1) * 132]
    c = [round((x - t[0]) / GRID) for x in t]
    d = [c[k] - c[k - 1] for k in range(1, 132)]
    assert max(abs(x) for x in d) <= 127
    first.append(t[0])
    steps.append(d)
    acc = t[0]
    for k in range(132):                       # (the decoder's values against Plaits')
        if k:
            acc += GRID * d[k - 1]
        worst = max(worst, abs(acc - t[k]))
out = ["/* SPDX-License-Identifier: MIT",
       " * Plaits' wavetables (Mutable Instruments Plaits, plaits/resources.cc wav_integrated_waves), Copyright 2016 Emilie",
       " * Gillet, MIT licence (the notice: plaits_ws.c), in 8 bits: a wave's first value and its integral's steps on a grid",
       f" * of PWT_GRID. Written by tools/plaits_proto/gen_wt8.py (every value within {worst} of Plaits'). */",
       "#pragma once", f"#define PWT_GRID {GRID}",
       "static const int16_t pwt_first[192] = {"]
for i in range(0, 192, 12):
    out.append("    " + ", ".join(str(x) for x in first[i:i + 12]) + ",")
out.append("};")
out.append("static const int8_t pwt_steps[192][131] = {")
for d in steps:
    out.append("    {" + ",".join(str(x) for x in d) + "},")
out.append("};")
open(sys.argv[2], "w").write("\n".join(out) + "\n")
print(f"gen_wt8: 192 waves, {192 * 2 + 192 * 131} bytes, every value within {worst} of Plaits'", file=sys.stderr)
