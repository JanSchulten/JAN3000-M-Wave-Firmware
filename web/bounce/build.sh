#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# M-Vave-OP1's offline bounce page: the firmware as WebAssembly with the mixer's taps (web/bounce/bounce_web.c, on the
# browser emulator's web/emu/felucca_web.c), the page and its script.
#   web/bounce/build.sh   ->  build/bounce/{index.html, bounce.js, felucca_bounce.wasm}
# Run after one ./build.sh (build/gen), with Emscripten (emcc) on the PATH.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "bounce: run ./build.sh once first (build/gen)"; exit 1; }
command -v emcc >/dev/null 2>&1 || { echo "bounce: no emcc (Emscripten)"; exit 1; }
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
JAN=$(sed -n 's/^#define JAN_VERSION "\([^"]*\)".*/\1/p' firmware/src/ui_draw.c)
mkdir -p build/bounce
emcc -O2 -ffp-contract=off -std=gnu99 -w "-DFELUCCA_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/bounce/felucca_bounce.wasm web/bounce/bounce_web.c
sed -e "s/@VERSION@/$VERSION/g" -e "s/@JAN@/$JAN/g" web/bounce/index.html > build/bounce/index.html
cp web/bounce/bounce.js build/bounce/
echo "bounce: build/bounce ($(wc -c < build/bounce/felucca_bounce.wasm | tr -d ' ') B wasm, $VERSION)"
