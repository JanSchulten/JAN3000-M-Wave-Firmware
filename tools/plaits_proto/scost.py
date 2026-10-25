# SPDX-License-Identifier: GPL-3.0-only
"""Per-function loop cost from clang -S assembly (pi32v2), weighted as tests/target_budget.py weighs the
objdump listing: an instruction in a loop of depth d weighs 4 ** (d - 1), a divide (integer or float) 1 + 8.
Each block's depth is clang's own annotation ('Loop Header: Depth=d' / 'in Loop: ... Depth=d').
  scost.py FILE.s=fn,fn ...   -> instructions, in loops, float ops in loops, divides, calls in loops, cost"""
import re, sys
def funcs(path):
    out, cur = {}, None
    for line in open(path):
        m = re.match(r"^([A-Za-z_][\w.]*):", line)
        if m and not m.group(1).startswith(".L"):
            cur = out.setdefault(m.group(1), [])
            continue
        if cur is not None:
            if line.startswith(".Lfunc_end"):
                cur = None
                continue
            cur.append(line.rstrip("\n"))
    return out
def cost(lines):
    d = n = loop = fl = div = calls = w = 0
    for l in lines:
        if re.match(r"^(\.LBB\w+|; BB#\d+):", l):
            m = re.search(r"Depth=(\d+)", l)
            d = int(m.group(1)) if m else 0
            continue
        s = l.split(";")[0].strip()
        if not s or s.startswith((".", "#")) or s in ("{", "}") or s.startswith("} else"):
            continue
        n += 1
        if not d:
            continue
        k = 4 ** (d - 1)
        loop += 1
        isf = "(f)" in s or re.search(r"\b(ftoi|itof|utof)\(", s)
        fl += bool(isf)
        if re.search(r"= r\d+ / r\d+", s):
            div += 1
            k *= 9
        calls += s.startswith("call")
        w += k
    return n, loop, fl, div, calls, w
for arg in sys.argv[1:]:
    path, names = arg.split("=")
    fs = funcs(path)
    for nm in names.split(","):
        if nm not in fs:
            print(f"{nm:18s} (not found)")
            continue
        n, loop, fl, div, calls, w = cost(fs[nm])
        print(f"{nm:18s} {n:5d} insns, {loop:4d} in loops ({fl:3d} float), {div} div, {calls:2d} calls: cost {w}")
