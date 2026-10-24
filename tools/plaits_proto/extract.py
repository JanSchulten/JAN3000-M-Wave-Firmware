# SPDX-License-Identifier: GPL-3.0-only
"""Pull Plaits tables out of resources.cc into a C header: extract.py RESOURCES_CC OUT_H name..."""
import re, sys
src = open(sys.argv[1]).read()
out = ["/* SPDX-License-Identifier: MIT\n * Tables from Mutable Instruments Plaits (plaits/resources.cc), Copyright 2016 Emilie Gillet, MIT licence (the notice:\n * plaits_ws.c). Extracted by tools/plaits_proto/extract.py. */", "#pragma once"]
for name in sys.argv[3:]:
    m = re.search(r"const\s+(float|int16_t|uint16_t|int8_t|uint8_t|uint32_t)\s+" + name + r"\[\]\s*=\s*\{(.*?)\};", src, re.S)
    if not m:
        sys.exit("missing " + name)
    t, body = m.group(1), m.group(2)
    vals = [v.strip() for v in body.replace("\n", " ").split(",") if v.strip()]
    out.append(f"static const {t} {name}[{len(vals)}] = {{")
    for i in range(0, len(vals), 8):
        out.append("  " + ", ".join(vals[i:i + 8]) + ",")
    out.append("};")
    print(name, t, len(vals), file=sys.stderr)
open(sys.argv[2], "w").write("\n".join(out) + "\n")
