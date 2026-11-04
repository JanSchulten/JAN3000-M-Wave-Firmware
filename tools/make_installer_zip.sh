#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# M-Vave-OP1: the web installer as a ZIP to unpack and start on your own computer (Web MIDI needs localhost):
# the site of web/make_site.py (this build's package, the official V15 of vendor/m-vave/) and two starters that
# serve it on a free port of this computer (serve.py) and open it. Run from the repository root after ./build.sh:
#   sh tools/make_installer_zip.sh [OUT.zip] [PKG.fwsc]   (default dist/M-Vave-OP1-Installer.zip, build/felucca.fwsc)
# A release: ./build.sh --release 0.2-alpha, then
#   sh tools/make_installer_zip.sh dist/M-Vave-OP1-Installer.zip build/jan3000-0.2-alpha.fwsc
# (the firmware in the ZIP is then felucca-jan3000-0.2-alpha.fwsc; docs/BACKLOG.md has the release steps).
# The alpha ships the installer alone (docs/BACKLOG.md): WITH_EDITOR=1 adds the web editor, its NO DEVICE firmware
# (web/emu/build.sh, Emscripten) and the FLUTE / SAX slot files, as the ZIP held before.
set -e
OUT=${1:-dist/M-Vave-OP1-Installer.zip}
PKG=${2:-build/felucca.fwsc}
case "$PKG" in
    */jan3000-*.fwsc) VER=$(basename "$PKG" .fwsc) ;;
    *) VER="op1-$(git describe --always --dirty 2>/dev/null || echo dev)" ;;
esac
TMP=$(mktemp -d)
SITE="$TMP/M-Vave-OP1-Installer"
if [ "$WITH_EDITOR" = 1 ]; then
    # the editor's NO DEVICE: the firmware for the browser (web/emu/build.sh, Emscripten; ~/emsdk when it is there)
    # (sourced from its own folder: under sh it cannot find that itself)
    if [ -f "$HOME/emsdk/emsdk_env.sh" ] && ! command -v emcc >/dev/null 2>&1; then
        here=$(pwd); cd "$HOME/emsdk" && . ./emsdk_env.sh >/dev/null 2>&1; cd "$here"
    fi
    if command -v emcc >/dev/null 2>&1; then sh web/emu/build.sh; else echo "installer zip: no emcc: the editor without NO DEVICE"; fi
    python3 web/make_site.py "$PKG" "$VER" "$SITE"
else
    python3 web/make_site.py --no-editor "$PKG" "$VER" "$SITE"
fi
# serve.py: a free port of its own each start (an older installer's server still running on another port, or the
# browser's cache of an earlier start, can never hand out an older firmware), nothing cached, only this computer
cat > "$SITE/serve.py" <<'EOS'
# M-Vave-OP1 installer: serves this folder on a free port of this computer and opens the installer in Chrome / Edge
import http.server, os, subprocess, sys, webbrowser
os.chdir(os.path.dirname(os.path.abspath(__file__)))
class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()
srv = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
url = "http://127.0.0.1:%d/webapp/installer/" % srv.server_address[1]
print("M-Vave-OP1 installer:", url, "(close this window to stop)")
fw = [f for f in os.listdir("firmware") if f.startswith("felucca-") and f.endswith(".fwsc")]
print("firmware:", ", ".join(fw))
if sys.platform == "darwin":
    if subprocess.call(["open", "-a", "Google Chrome", url]) != 0:
        subprocess.call(["open", url])
else:
    webbrowser.open(url)
srv.serve_forever()
EOS
cat > "$SITE/START-Mac.command" <<'EOS'
#!/bin/sh
# M-Vave-OP1 installer: serves this folder and opens it (Chrome or Edge). Close this window to stop.
cd "$(dirname "$0")"
python3 serve.py
EOS
cat > "$SITE/START-Windows.bat" <<'EOS'
@echo off
rem M-Vave-OP1 installer: serves this folder and opens it (Chrome or Edge). Close this window to stop.
cd /d "%~dp0"
py -3 serve.py || python serve.py
pause
EOS
chmod +x "$SITE/START-Mac.command"
# FLUTE and SAX, once built in: slot files for the editor's Samples tab (Slot file -> USR1)
if [ "$WITH_EDITOR" = 1 ] && ls dist/samples/*.slot >/dev/null 2>&1; then
    mkdir -p "$SITE/samples"
    cp dist/samples/*.slot "$SITE/samples/"
fi
mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
(cd "$TMP" && zip -qr - M-Vave-OP1-Installer) > "$OUT"
rm -rf "$TMP"
echo "installer zip: $OUT ($(wc -c < "$OUT") B, $VER)"
