#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# M-Vave-OP1's library page: the .FM1 files kept in the browser, played by the firmware as WebAssembly (the offline
# bounce's, web/bounce: its felucca_bounce.wasm and bounce.js) while it renders, in a folder of its own.
#   web/library/build.sh   ->  build/library/{index.html, lib.js, worker.js, bounce.js, felucca_bounce.wasm}
# Run after one ./build.sh (build/gen), with Emscripten (emcc) on the PATH.
set -e
cd "$(dirname "$0")/../.."
sh web/bounce/build.sh
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
JAN=$(sed -n 's/^#define JAN_VERSION "\([^"]*\)".*/\1/p' firmware/src/ui_draw.c)
mkdir -p build/library
sed -e "s/@VERSION@/$VERSION/g" -e "s/@JAN@/$JAN/g" web/library/index.html > build/library/index.html
# (in the source tree lib.js takes bounce.js from web/bounce; in the page's folder it lies next to it)
sed -e 's#"\.\./bounce/bounce\.js"#"./bounce.js"#' web/library/lib.js > build/library/lib.js
grep -q '"./bounce.js"' build/library/lib.js || { echo "library: lib.js does not import bounce.js"; exit 1; }
cp web/library/worker.js web/bounce/bounce.js build/bounce/felucca_bounce.wasm build/library/
echo "library: build/library ($(du -sk build/library | cut -f1) KiB, $VERSION)"
