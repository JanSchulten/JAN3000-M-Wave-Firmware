#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# The symbol lab (an Artifact page): every card shape (ui_cards.c) at every value of a real parameter, drawn by the
# firmware's own draw_column through the UI test harness, as sprites the page scrubs with a slider.
#   sh tools/symbol_lab/build.sh OUT [PREV_REV] [VERSION]
# OUT/site holds the page (symbol-labor.html), data.json and sprites/*.png; with PREV_REV (a git revision) the shapes
# that differ from that revision get a "vorher" sprite (prev/*.png) and a change line from tools/symbol_lab/changes.json
# (id -> text). Needs build/gen (./build.sh or tools/build.py's generate) and Pillow.
set -e
cd "$(dirname "$0")/../.."
OUT=$1; PREV=$2; VER=${3:-1}
[ -n "$OUT" ] || { echo "usage: build.sh OUT [PREV_REV] [VERSION]"; exit 1; }
mkdir -p "$OUT/cur" "$OUT/site"
cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/sym_lab" tools/symbol_lab/sym_lab.c -lm
"$OUT/sym_lab" "$OUT/cur"
if [ -n "$PREV" ]; then
    rm -rf "$OUT/prevsrc"; mkdir -p "$OUT/prevsrc" "$OUT/prev"
    git archive "$PREV" firmware tests | tar -x -C "$OUT/prevsrc"
    cc -O1 -w -Ibuild/gen -I"$OUT/prevsrc/firmware/src" -I"$OUT/prevsrc/tests" -o "$OUT/sym_lab_prev" tools/symbol_lab/sym_lab.c -lm
    "$OUT/sym_lab_prev" "$OUT/prev"
fi
python3 tools/symbol_lab/site.py "$OUT" "$VER"
cp tools/symbol_lab/symbol-labor.html "$OUT/site/"
echo "symbol lab: $OUT/site (publish symbol-labor.html with data.json, sprites/, prev/)"
