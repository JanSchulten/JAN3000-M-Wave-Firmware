#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# M-Vave-OP1: a refactor of the UI changes no pixel. Renders every screen of tests/ui_render.c (every palette, LINE,
# LARGE) from the working tree and from BASE (a git revision, default origin/main, checked out in a temporary
# worktree) and compares them byte for byte. Prints the screens that differ; exit 1 if any does. Run from the
# repository root after ./build.sh (build/gen):
#   sh tools/ui_compare.sh [BASE]
set -e
BASE=${1:-origin/main}
TMP=$(mktemp -d)
trap 'git worktree remove --force "$TMP/base" >/dev/null 2>&1; rm -rf "$TMP"' EXIT
git worktree add -q --detach "$TMP/base" "$BASE"
for side in base work; do
    src=$([ "$side" = base ] && echo "$TMP/base" || pwd)
    mkdir -p "$TMP/$side/out/ppm" "$TMP/$side/slot"
    ${CC:-cc} -O1 -w "-DFELUCCA_VERSION=\"TEST\"" -Ibuild/gen -I"$src/firmware/src" -I"$src/tests" \
        -o "$TMP/$side/ui_render" "$src/tests/ui_render.c" -lm
    (cd "$TMP/$side" && ./ui_render out slot > /dev/null 2>&1) || true
done
n=$(ls "$TMP/work/out/ppm" | wc -l)
if diff -rq "$TMP/base/out/ppm" "$TMP/work/out/ppm" > "$TMP/diff.txt" && diff -rq "$TMP/base/slot" "$TMP/work/slot" >> "$TMP/diff.txt"; then
    echo "ui_compare: $n screens, all identical to $BASE"
else
    sed 's|.*/||' "$TMP/diff.txt"
    echo "ui_compare: $(wc -l < "$TMP/diff.txt") of $n screens differ from $BASE"
    exit 1
fi
