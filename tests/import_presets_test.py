#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""tools/import_presets.py on a copy of the tree (run_tests.sh, after tests/descdump.c wrote build/host/desc.json):
a preset file as the editor's "Export presets" writes it -> the preset tables, the categories and the editor's
offline tables; the copy is compiled with the host build of the presets (tests/descdump.c) and its tables compared.
  python3 tests/import_presets_test.py [CC]"""
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
bad = 0


def check(name, ok):
    global bad
    print(f"import_presets: {name:<90} {'ok' if ok else 'FAIL'}")
    bad += not ok


def keys_of(desc):                                    # web/editor.html paramKeys
    seen, out = {}, []
    for x in desc["TP"]:
        seen[x["label"]] = seen.get(x["label"], 0) + 1
        out.append(x["label"] if seen[x["label"]] == 1 else f'{x["label"]}#{seen[x["label"]]}')
    return out + [f"E{k}" for k in range(8)]


def main():
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    desc = json.loads((ROOT / "build/host/desc.json").read_text())
    keys = keys_of(desc)
    at = {k: i for i, k in enumerate(keys)}

    def sound(**kv):
        p = [x["def"] for x in desc["TP"]] + [0] * 8
        for k, v in kv.items():
            p[at[k.replace("_", "#")]] = v
        return p

    with tempfile.TemporaryDirectory() as tmp:
        t = Path(tmp)
        for d in ("firmware/src", "firmware/hal", "firmware/include", "tools", "tests", "build/gen"):
            if (ROOT / d).is_dir():
                shutil.copytree(ROOT / d, t / d, ignore=shutil.ignore_patterns("__pycache__"))
        (t / "web").mkdir()
        shutil.copy(ROOT / "web/editor.html", t / "web")
        (t / "build/host").mkdir(parents=True)
        shutil.copy(ROOT / "build/host/desc.json", t / "build/host")
        wsh = sound(E0=60, E1=20, E2=70, E4=100, E5=10, ATK=5, DEC=60, SUS=100, REL=40, VCE=2, RATE=99)
        a2 = sound(E0=1, E2=2, E3=3, E4=90, E5=40, E6=30, E7=60, ATK_2=11, DEC_2=22, SUS_2=33, REL_2=44, FLT=-20)
        f = {"format": "felucca-library", "version": 1, "kind": "presets", "paramLabels": keys, "patches": [
            {"name": "Soft Fold", "engineName": "WSHAPE", "params": wsh, "pattern": None, "tags": []},
            {"name": "my duo!", "engineName": "ANLG2", "params": a2,
             "pattern": [[n, 0] for n in desc["PATTERNS"][1][0]], "tags": ["Bass"]},
            {"name": "OWN FM", "engineName": "FM6", "params": sound(E7=9), "pattern": None, "tags": []},
            {"name": "PNO", "engineName": "SAMPLE", "params": sound(), "pattern": None, "tags": []}]}
        (t / "in.json").write_text(json.dumps(f))
        r = subprocess.run([sys.executable, "tools/import_presets.py", "in.json"], cwd=t, capture_output=True, text=True)
        out = r.stdout
        check("runs; SOFT FOLD replaced, MY DUO added, FM6 OWN and SAMPLE skipped, LFO RATE named as lost",
              r.returncode == 0 and "replaced WSHAPE  1 SOFT FOLD" in out and "added    ANLG2  9 MY DUO" in out and
              "skipped FM6 OWN FM" in out and "skipped SAMPLE PNO" in out and "RATE 99" in out)
        first = {p: (t / p).read_text() for p in ("firmware/src/eng_wshape.c", "firmware/src/eng_analog2.c",
                                                  "firmware/src/category.c", "web/editor.html")}
        r2 = subprocess.run([sys.executable, "tools/import_presets.py", "in.json"], cwd=t, capture_output=True, text=True)
        check("the same file again: MY DUO replaced, nothing else changes",
              r2.returncode == 0 and "replaced ANLG2  9 MY DUO" in r2.stdout and
              all((t / p).read_text() == s for p, s in first.items()))
        exe = t / "descdump"
        r = subprocess.run([cc, "-O1", "-w", "-Ibuild/gen", "-Ifirmware/src", "-o", str(exe), "tests/descdump.c", "-lm"],
                           cwd=t, capture_output=True, text=True)
        check("the tables compile (tests/descdump.c)", r.returncode == 0)
        if r.returncode:
            print(r.stderr[-2000:])
            return
        d2 = json.loads(subprocess.run([str(exe)], cwd=t, capture_output=True, text=True).stdout)
        eng = {x["name"]: x for x in d2["ENG"]}
        w, a = eng["WSHAPE"]["presets"], eng["ANLG2"]["presets"]
        old = {x["name"]: x for x in desc["ENG"]}
        check("WSHAPE 1: the sound's E0..E7, envelope, MONO (VCE LEGATO); its pattern hint kept; the rest as before",
              w[0]["e"] == [60, 20, 70, 0, 100, 10, 0, 0] and w[0]["env"] == [5, 60, 100, 40] and w[0]["mono"] == 1 and
              w[0]["pat"] == old["WSHAPE"]["presets"][0]["pat"] and w[1:] == old["WSHAPE"]["presets"][1:])
        check("ANLG2 9: appended, its steps are factory pattern 2 (PAT(2)), the other eight as before",
              len(a) == 9 and a[8]["name"] == "MY DUO" and a[8]["e"] == [1, 0, 2, 3, 90, 40, 30, 60] and
              a[8]["pat"] == 2 and a[:8] == old["ANLG2"]["presets"])
        c = (t / "firmware/src/eng_analog2.c").read_text()
        check("ANLG2 9: WAVE 2's CUT2 RES2 LFO2 RATE2 (A2_OSC2_PRESET) and ENV->FLT -20", "{11, 22, 33, 44}," in c and
              '{"MY DUO", {1, 0, 2, 3, 90, 40, 30, 60}, {10, 70, 90, 60}, -20, 0,' in c)
        cat = (t / "firmware/src/category.c").read_text()
        check("category: ANLG2 9 BASS (its tag), the comments in their column",
              '    "LBPBUDLFB",                 /* 14 ANALOG2' in cat)
        r = subprocess.run(["node", "-e", """
const fs = require("fs"), html = fs.readFileSync(process.argv[1], "utf8"), d = JSON.parse(fs.readFileSync(process.argv[2]));
const m = /\\{ name: "ANLG2", titles:[\\s\\S]*?presets: \\[([\\s\\S]*?)\\] \\}/.exec(html);
const P = (name, e, env = [10, 70, 90, 60], mono = 0, pat = 0) => ({ name, e, env, mono, pat });
const got = eval("[" + m[1] + "]"), want = d.ENG.find((x) => x.name === "ANLG2").presets;
process.exit(JSON.stringify(got) === JSON.stringify(want) ? 0 : 1);""", str(t / "web/editor.html"), "/dev/stdin"],
                           input=json.dumps(d2), capture_output=True, text=True)
        check("the editor's offline ANLG2 presets == the firmware's", r.returncode == 0)


main()
print("import_presets: " + ("all ok" if not bad else f"{bad} FAILED"))
sys.exit(1 if bad else 0)
