#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# The emulator page (web/lab/index.html) with the firmware, into build/lab:
#   index.html       the page as published (no doctype: the Artifact host wraps it)
#   local.html       the same with a doctype, for a local web server
#   felucca.wasm     the firmware (web/emu/build.sh)
#   felucca-js.mjs   the same firmware as JavaScript (wasm2js), for browsers that block WebAssembly on the page
# Needs one ./build.sh first (build/gen), Emscripten (emcc) and Binaryen's wasm2js (both in the emsdk).
#   web/lab/build.sh
#   cd build/lab && python3 -m http.server 8000    # then open http://localhost:8000/local.html
set -e
cd "$(dirname "$0")/../.."
sh web/emu/build.sh
W2J="${WASM2JS:-$(command -v wasm2js || echo "$HOME/emsdk/upstream/bin/wasm2js")}"
[ -x "$W2J" ] || { echo "lab: no wasm2js (Binaryen, part of the emsdk)"; exit 1; }
mkdir -p build/lab
cp build/emu/felucca.wasm build/lab/
"$W2J" --enable-bulk-memory --enable-sign-ext --enable-nontrapping-float-to-int --enable-mutable-globals -O2 \
    build/emu/felucca.wasm -o build/lab/felucca-js.mjs
cp web/lab/index.html build/lab/index.html
{ printf '<!doctype html>\n<html lang="de">\n<head>\n<meta charset="utf-8">\n'
  printf '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n</head>\n<body>\n'
  cat web/lab/index.html
  printf '</body>\n</html>\n'; } > build/lab/local.html
echo "lab: build/lab ($(wc -c < build/lab/felucca.wasm | tr -d ' ') B wasm, $(wc -c < build/lab/felucca-js.mjs | tr -d ' ') B js)"
