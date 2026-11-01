#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# The static site Vercel serves (vercel.json: outputDirectory "site", no build there: Vercel has no Emscripten), made
# here and committed:
#   site/index.html   the front page (web/site/index.html): the guide and the installer ZIP of the GitHub release
#   site/manual/      the user guide on its own (web/manual/manual.html as index.html: no link to the site's other
#                     pages; written as a page body, as a claude.ai Artifact takes it: here it gets the document
#                     around it)
# and with WITH_EXTRAS=1 (in the backlog for the alpha, docs/BACKLOG.md; the front page does not link them):
#   site/bounce/      the offline bounce (web/bounce/build.sh)
#   site/emu/         the browser emulator (web/emu/build.sh)
# Run from anywhere (WITH_EXTRAS=1: after ./build.sh, with Emscripten (emcc) on the PATH); then commit site/.
set -e
cd "$(dirname "$0")/.."
if [ "$WITH_EXTRAS" = 1 ]; then
    sh web/bounce/build.sh
    sh web/emu/build.sh
fi
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
JAN=$(sed -n 's/^#define JAN_VERSION "\([^"]*\)".*/\1/p' firmware/src/ui_draw.c)
rm -rf site
mkdir -p site
if [ "$WITH_EXTRAS" = 1 ]; then
    cp -R build/bounce site/bounce
    cp -R build/emu site/emu
fi
sed -e "s/@VERSION@/$VERSION/g" -e "s/@JAN@/$JAN/g" web/site/index.html > site/index.html
mkdir -p site/manual
{   # (the reset a claude.ai Artifact gives its pages: no body margin, images inside their column, [hidden] hidden)
    printf '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
    printf '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
    printf '<style>body{margin:0}img{max-width:100%%}[hidden]{display:none!important}</style>\n'
    cat web/manual/manual.html
    printf '\n</html>\n'
} > site/manual/index.html
echo "site: $(find site -type f | wc -l | tr -d ' ') files, $(du -sk site | cut -f1) KiB (JAN3000 $JAN, Felucca $VERSION)"
