#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# Set up a Linux x86-64 machine to build and test FM-1 firmware (Felucca / M-Vave-OP1). Run from the repository root.
#   sh .claude/skills/fm1-firmware/scripts/setup_env.sh           toolchain, SDK files, Python packages
#   sh .claude/skills/fm1-firmware/scripts/setup_env.sh --emsdk   + Emscripten (browser emulator, wasm2js)
# Idempotent: what is there already is kept.
set -e
SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
TC="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
SDK_TAG=AC79NN_SDK_V1.2.1_2023-12-13

retry() {                                  # retry "$@" up to 5 times, 2 4 8 16 s apart (Gitee resets connections)
    n=0
    until "$@"; do
        n=$((n + 1))
        [ "$n" -ge 5 ] && return 1
        sleep $((1 << n))
    done
}

if [ -x "$TC/pi32v2/bin/clang" ]; then
    echo "toolchain: $TC (kept)"
else
    retry sh tools/get_toolchain.sh
fi

if [ -f "$SDK/cpu/wl82/tools/uboot.boot" ]; then
    echo "sdk: $SDK (kept)"
else
    rm -rf "$SDK"
    retry git clone -q --depth 1 --filter=blob:none --sparse --branch "$SDK_TAG" \
        https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$SDK"
    (cd "$SDK" && git sparse-checkout set cpu/wl82/tools)
    ls "$SDK/cpu/wl82/tools/uboot.boot" "$SDK/cpu/wl82/tools/cfg_tool.bin" "$SDK/cpu/wl82/tools/cfg/eq_cfg_hw.bin"
fi

python3 -c "import PIL, fontTools" 2>/dev/null || pip3 install -q Pillow fonttools
python3 -c "import mido, rtmidi" 2>/dev/null || pip3 install -q mido python-rtmidi > /dev/null 2>&1 || \
    echo "note: mido / python-rtmidi missing (only tools/fm1_install.py needs them; python-rtmidi needs ALSA headers)"

if [ "$1" = "--emsdk" ]; then
    if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
        echo "emsdk: $HOME/emsdk (kept)"
    else
        retry git clone -q --depth 1 https://github.com/emscripten-core/emsdk.git "$HOME/emsdk"
        (cd "$HOME/emsdk" && ./emsdk install latest && ./emsdk activate latest)
    fi
    echo "emsdk: run '. $HOME/emsdk/emsdk_env.sh' in the shell that builds the emulator"
fi
echo "ready: ./build.sh, then tests/run_tests.sh"
