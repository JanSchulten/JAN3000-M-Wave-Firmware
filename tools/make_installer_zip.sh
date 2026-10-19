#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# M-Vave-OP1: the web installer as a ZIP to unpack and start on your own computer (Web MIDI needs localhost):
# the site of web/make_site.py (this build's package, the official V15 of vendor/m-vave/, the editor) and two
# starters that serve it on localhost:8000 and open it. Run from the repository root after ./build.sh:
#   sh tools/make_installer_zip.sh [OUT.zip]          (default dist/M-Vave-OP1-Installer.zip)
set -e
OUT=${1:-dist/M-Vave-OP1-Installer.zip}
VER=$(git describe --always --dirty 2>/dev/null || echo dev)
TMP=$(mktemp -d)
SITE="$TMP/M-Vave-OP1-Installer"
python3 web/make_site.py build/felucca.fwsc "op1-$VER" "$SITE"
cat > "$SITE/START-Mac.command" <<'EOS'
#!/bin/sh
# M-Vave-OP1 installer: serves this folder on http://localhost:8000 and opens it (Chrome or Edge). Close this window to stop.
cd "$(dirname "$0")"
(sleep 1; open -a "Google Chrome" "http://localhost:8000/webapp/installer/" 2>/dev/null || open "http://localhost:8000/webapp/installer/") &
python3 -m http.server 8000
EOS
cat > "$SITE/START-Windows.bat" <<'EOS'
@echo off
rem M-Vave-OP1 installer: serves this folder on http://localhost:8000 and opens it (Chrome or Edge). Close this window to stop.
cd /d "%~dp0"
start "" "http://localhost:8000/webapp/installer/"
py -3 -m http.server 8000 || python -m http.server 8000
pause
EOS
chmod +x "$SITE/START-Mac.command"
mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
(cd "$TMP" && zip -qr - M-Vave-OP1-Installer) > "$OUT"
rm -rf "$TMP"
echo "installer zip: $OUT ($(wc -c < "$OUT") B, $VER)"
