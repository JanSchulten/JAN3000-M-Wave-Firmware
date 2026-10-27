#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""The web editor as a claude.ai Artifact page (M-Vave-OP1), into OUT_DIR (default build/artifact/editor):

  index.html          web/editor.html without its doctype, html, head and body tags (the Artifact host wraps the page)
  fukiai.ttf          the icon font, fm1backup.js the backup module (as webapp/editor has them)
  emu/felucca.wasm    the firmware for NO DEVICE (web/emu/build.sh) and emu/felucca-js.mjs, its JavaScript build for a
                      frame that does not run WebAssembly (web/lab/build.sh: wasm2js)

  python3 web/make_editor_artifact.py [OUT_DIR]

In the Artifact the editor works without a device (NO DEVICE: Web MIDI is not there), saves files through the page's
downloads capability and asks through its own dialogs (editor.html saveBlob, askDialog)."""
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
TITLE = "M-Vave-OP1 Editor"


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "artifact" / "editor"
    html = (HERE / "editor.html").read_text(encoding="utf-8")
    head = html[html.index("<head>") + 6:html.index("</head>")]
    body = html[html.index("<body>") + 6:html.rindex("</body>")]
    head = re.sub(r'<meta charset="utf-8">\s*|<meta name="viewport"[^>]*>\s*|<title>[^<]*</title>\s*', "", head)
    page = f"<title>{TITLE}</title>\n" + head.strip() + "\n" + body.strip() + "\n"
    (out / "emu").mkdir(parents=True, exist_ok=True)
    (out / "index.html").write_text(page, encoding="utf-8")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt", "fm1backup.js"):
        shutil.copy(HERE / f, out / f)
    for src in (ROOT / "build/emu/felucca.wasm", ROOT / "build/lab/felucca-js.mjs"):
        if not src.exists():
            raise SystemExit(f"{src}: build it first (web/lab/build.sh builds both)")
        shutil.copy(src, out / "emu" / src.name)
    print(f"editor artifact: {out} ({len(page)} B page)")


if __name__ == "__main__":
    main()
