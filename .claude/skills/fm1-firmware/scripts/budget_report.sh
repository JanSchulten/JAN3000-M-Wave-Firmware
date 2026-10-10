#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# Build the firmware and print its budgets: image, RAM, pool (build.py's checks) and the static cost of the two ISRs
# against tests/target_budget.txt (+10 % allowed). Run from the repository root.
#   sh .claude/skills/fm1-firmware/scripts/budget_report.sh
set -e
LOG="${TMPDIR:-/tmp}/fm1_build.log"
if ! ./build.sh > "$LOG" 2>&1; then
    tail -20 "$LOG"
    echo "budget: the build failed (full log: $LOG)"
    exit 1
fi
grep -E "image [0-9]+ B|app |package |pool headroom|error" "$LOG" || true
python3 tests/target_budget.py 2>&1 | grep -E "fm1_alnk0_irq|fm1_timer5_irq|over|FAIL" || true
python3 - <<'EOF'
import re
ld = open("firmware/app.ld").read()
xip = int(re.search(r"XIP\s*\(rx\)\s*:\s*ORIGIN = 0x[0-9A-Fa-f]+, LENGTH = (0x[0-9A-Fa-f]+)", ld).group(1), 16)
import os
img = os.path.getsize("build/felucca.bin")
print(f"flash for code: {img} B of {xip} B ({xip - img} B free)")
EOF
