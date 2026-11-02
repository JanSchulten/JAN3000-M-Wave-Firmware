#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""The user guide's screens (web/manual/manual.html) from the firmware's own UI renders, so the guide shows what the
FM-1 shows. The guide names a screen by its render's name (tests/ui_render.c: build/ui_new/<PALETTE>/<name>.png) in
data-shot="name" (a panel drawing: the real screen in place of the drawn one) or data-shots="name name" (a row of
screens on their own). This writes every screen it names, as lossless WebP in data: URIs, into the guide's
<script id="jan-shots"> block, so the guide stays one file (a claude.ai Artifact takes it as it is, and
tools/make_vercel_site.sh copies it as it is). A screen the guide names that has no render is an error.
Run tests/run_tests.sh (or tests/ui_render.c and tests/ui_render.py on their own) first, then:
  python3 tools/make_manual_shots.py [RENDERS] [MANUAL]     (default build/ui_new/PAPER web/manual/manual.html)"""
import base64
import io
import re
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
renders = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "ui_new" / "PAPER"
manual = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "web" / "manual" / "manual.html"

html = manual.read_text(encoding="utf-8")
names = []
for m in re.finditer(r'data-shots?="([^"]*)"', html):
    for n in m.group(1).split():
        if n not in names:
            names.append(n)
missing = [n for n in names if not (renders / f"{n}.png").exists()]
if missing:
    raise SystemExit(f"make_manual_shots: no render for {', '.join(missing)} in {renders}")

rows, total = [], 0
for n in sorted(names):
    buf = io.BytesIO()
    Image.open(renders / f"{n}.png").convert("RGB").save(buf, "WEBP", lossless=True, method=6)
    total += buf.tell()
    rows.append(f'  "{n}": "data:image/webp;base64,{base64.b64encode(buf.getvalue()).decode()}"')
block = ('<script id="jan-shots">\n'
         f"/* the firmware's own screens ({renders.name}), written by tools/make_manual_shots.py from the renders of\n"
         " * tests/ui_render.c: do not edit by hand */\n"
         "var JAN_SHOTS = {\n" + ",\n".join(rows) + "\n};\n</script>")
pat = re.compile(r'<script id="jan-shots">.*?</script>', re.S)
if not pat.search(html):
    raise SystemExit('make_manual_shots: the guide has no <script id="jan-shots"> block')
manual.write_text(pat.sub(lambda _: block, html, count=1), encoding="utf-8")
print(f"make_manual_shots: {len(names)} screens, {total // 1024} KiB WebP -> {manual.relative_to(ROOT)}")
