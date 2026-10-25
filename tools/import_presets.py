#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""Sounds of the web editor's library -> factory presets (M-Vave-OP1).

  python3 tools/import_presets.py FILE.json [FILE.json ...] [--dry-run]

FILE.json: a file of the editor's Library tab ("Export presets", "Export library" or a single sound's "Export": the
felucca-library format, web/editor.html libraryFile). Every sound in it becomes a factory preset of its engine: one
named like a factory preset of that engine replaces it (its place, number and favourites stay), any other is
appended to the engine's list. Written:
  firmware/src/eng_*.c         the engine's preset table (core.h preset_t; ANLG2 also A2_OSC2_PRESET)
  firmware/src/category.c      the preset's category letter (PRESET_CAT)
  web/editor.html              the editor's offline tables (its mock: test_web.mjs checks them against the firmware)
Then: ./build.sh, GOLDEN_UPDATE=1 tests/run_tests.sh (a changed or new preset is a new golden render).

What a factory preset holds (core.h preset_t, ui.c preset_load_to): the name (12 characters), E0..E7, ATK DEC SUS
REL, ENV -> FILTER (FLT), MONO (VCE other than POLY; a mono preset loads LEGATO), the sends (for the power-on sounds
only) and a pattern hint (PATTERNS: the sound's 16 steps when they are one of them). Everything else a sound has
(the LFO, the modulation matrix, the filter TYPE, ENV SYNC, SPREAD, GLIDE, ..) a preset load sets to its default:
the script lists what is lost that way. Not possible: SAMPLE (its presets are the sample sets, tools/gen_samples.py),
DRUM (one kit), FM6 with its own patch (SLOT OWN or B1..B32: a factory preset can only pick F1..F8).
Category: a tag of the sound named BASS LEAD PAD PLUCK KEYS DRUM FX OTHER (any case), else the replaced preset's,
else OTHER.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "firmware" / "src"
WEB = ROOT / "web" / "editor.html"
DESC = ROOT / "build" / "host" / "desc.json"          # tests/descdump.c (run_tests.sh): defaults and ranges

NAME_MAX = 12
NAME_OK = re.compile(r"[^A-Z0-9 .+\-/&#']")
CAT_LETTER = {"BASS": "B", "LEAD": "L", "PAD": "P", "PLUCK": "U", "KEYS": "K", "DRUM": "D", "FX": "F", "OTHER": "O"}
NO_PRESETS = {"SAMPLE": "its presets are the sample sets (tools/gen_samples.py)", "DRUM": "one kit, no presets to add"}
# keys (web/editor.html paramKeys) a preset load leaves alone (ui.c param_kept) or that are the sends / INSERT
# (preset_fx): not lost
KEPT = {"LVL", "PAN", "MUTE", "MODE", "RATE#2", "OCT", "GATE", "SWG", "PROB", "HOLD", "ORD", "ROOT", "SCL", "QNT",
        "TRN", "LEN", "DIV", "SWG#2", "GATE#2", "SLCR", "PAT", "RATE#3", "DEPTH", "CHRD", "VOIC", "QNTZ",
        "DST", "CHO", "DLY", "REV", "INSRT", "INS A", "INS B", "INS C", "MIX"}
SET = {"ATK", "DEC", "SUS", "REL", "FLT", "VCE"} | {f"E{k}" for k in range(8)}
A2_KEYS = ["ATK#2", "DEC#2", "SUS#2", "REL#2"]          # ANLG2's WAVE 2 CUT2 RES2 LFO2 RATE2 (eng_analog2.c P_A2_*)
SENDS = ["DST", "CHO", "DLY", "REV"]

ENTRY = re.compile(r'\{"([^"]*)", \{([^}]*)\}, \{([^}]*)\}, (-?\d+), (\d+)(?:, FX\(([^)]*)\))?(?:, PAT\((\d+)\))?\},')


def ints(s):
    """a C list's values: numbers, a name (an enum, a macro) as it is"""
    return [int(x) if re.fullmatch(r"-?\d+", x.strip()) else x.strip() for x in s.split(",")] if s.strip() else []


class Table:
    """an engine's preset table in its eng_*.c"""

    def __init__(self, path, var, name):
        self.path, self.var, self.name = path, var, name
        self.text = path.read_text(encoding="utf-8")
        m = re.search(r"static const preset_t " + var + r"\[\] = \{\n(.*?)\n\};", self.text, re.S)
        if not m:
            raise SystemExit(f"{path.name}: no table {var}")
        self.span = m.span(1)
        self.lines = m.group(1).split("\n")
        self.rows = []                                # [line index, entry dict]
        self.bad = None
        self.changed = set()                          # rows put here (the others: as desc.json has them)
        for i, ln in enumerate(self.lines):
            e = ENTRY.search(ln)
            if e:
                self.rows.append([i, dict(name=e[1], e=ints(e[2]), env=ints(e[3]), fenv=int(e[4]), mono=int(e[5]),
                                          fx=ints(e[6]) if e[6] else None, pat=int(e[7]) if e[7] else None)])
            elif ln.strip().startswith("{\""):
                self.bad = f"{path.name}: a line of {var} this script cannot read ({ln.strip()[:40]!r}..)"
        self.osc2 = None
        if var == "ANALOG2_PRESETS":
            m2 = re.search(r"A2_OSC2_PRESET\[NELEM\(ANALOG2_PRESETS\)\]\[4\] = \{\n(.*?)\n\};", self.text, re.S)
            self.osc2_span = m2.span(1)
            self.osc2 = [ints(x) for x in re.findall(r"\{([-\d, ]+)\}", m2.group(1))]
            if len(self.osc2) != len(self.rows):
                raise SystemExit("eng_analog2.c: A2_OSC2_PRESET has not a row per preset")

    def find(self, name):
        return next((k for k, (_, r) in enumerate(self.rows) if r["name"] == name), None)

    @staticmethod
    def line(r):
        s = f'    {{"{r["name"]}", {{{", ".join(map(str, r["e"]))}}}, {{{", ".join(map(str, r["env"]))}}}, {r["fenv"]}, {r["mono"]}'
        if r["fx"] is not None:
            s += f', FX({", ".join(map(str, r["fx"]))})'
        if r["pat"] is not None:
            s += f', PAT({r["pat"]})'
        return s + "},"

    def put(self, k, r):
        if k is None:
            self.lines.append(self.line(r))
            self.rows.append([len(self.lines) - 1, r])
            k = len(self.rows) - 1
        else:
            self.lines[self.rows[k][0]] = self.line(r)
            self.rows[k][1] = r
        self.changed.add(k)

    def write(self):
        t = self.text
        if self.osc2 is not None:                     # (after the table in the file: replaced first)
            body = wrap("    ", ["{" + ", ".join(map(str, x)) + "}" for x in self.osc2], 120)
            t = t[:self.osc2_span[0]] + body + t[self.osc2_span[1]:]
        t = t[:self.span[0]] + "\n".join(self.lines) + t[self.span[1]:]
        self.path.write_text(t, encoding="utf-8")


def wrap(indent, items, width):
    out, cur = [], indent
    for it in items:
        add = it + ","
        if cur.strip() and len(cur) + 1 + len(add) > width:
            out.append(cur.rstrip())
            cur = indent
        cur += ("" if not cur.strip() else " ") + add
    out.append(cur.rstrip())
    return "\n".join(out)


def tables():
    """engine name -> Table, from the eng_*.c files (.name, .presets)"""
    out = {}
    for f in sorted(SRC.glob("eng_*.c")):
        txt = f.read_text(encoding="utf-8")
        for m in re.finditer(r'static const engine_t \w+ = \{\s*\.name = "([^"]+)".*?\.presets = (\w+),', txt, re.S):
            if re.search(r"static const preset_t " + m[2] + r"\[\]", txt):
                out[m[1]] = Table(f, m[2], m[1])
    return out


def load_desc():
    try:
        return json.loads(DESC.read_text())
    except (OSError, ValueError):
        return None


def sound_of(patch, keys, desc, eng, table, notes):
    """a library sound -> a preset_t entry (+ ANLG2's WAVE 2 row); notes: what it cannot keep"""
    p = patch.get("params") or patch.get("p") or []
    at = {k: i for i, k in enumerate(keys)}
    tp = desc["TP"] if desc else None

    def val(key, default=0):
        i = at.get(key)
        v = p[i] if i is not None and i < len(p) else None
        if v is None and tp and i is not None and i < len(tp):
            v = tp[i]["def"]
        return default if v is None else int(v)

    ed = next((x for x in desc["ENG"] if x["name"] == eng), None) if desc else None
    e = []
    for k in range(8):
        v = val(f"E{k}")
        if ed:
            d = ed["edit"][k]
            v = max(d["min"], min(d["max"], v))
        e.append(max(-128, min(127, v)))
    env = [max(0, min(127, val(k))) for k in ("ATK", "DEC", "SUS", "REL")]
    fenv = max(-64, min(63, val("FLT")))
    mono = 1 if val("VCE") else 0
    fx = [max(0, min(127, val(k))) for k in SENDS]
    pat = 0
    pt = patch.get("pattern")
    if desc and isinstance(pt, list) and len(pt) == 16:
        notes_ = [int(x[0]) if isinstance(x, list) else 0 for x in pt]
        for n, (pn, pf) in enumerate(desc["PATTERNS"]):
            if pn == notes_:
                pat = n + 1
                break
    osc2 = [max(0, min(127, val(k))) for k in A2_KEYS] if eng == "ANLG2" else None
    if tp:                                            # what a preset load sets to its default: lost
        lost = []
        for i, k in enumerate(keys):
            if k in KEPT or k in SET or (osc2 is not None and k in A2_KEYS) or i >= len(tp) or i >= len(p):
                continue
            if p[i] is not None and int(p[i]) != tp[i]["def"]:
                lost.append(f"{k} {p[i]}")
        if lost:
            notes.append("not kept (a preset load sets them to their defaults): " + ", ".join(lost))
    if eng == "FM6" and e[7] > 7:
        raise ValueError("FM6 with its own patch (SLOT OWN or a bank voice): a factory preset picks F1..F8 only")
    if pt and not pat:
        notes.append("its steps are no factory pattern: the pattern hint stays as it was (new: none)")
    return dict(e=e, env=env, fenv=fenv, mono=mono, fx=fx, pat=pat), osc2


def preset_name(s):
    n = NAME_OK.sub("", str(s).upper()).strip()[:NAME_MAX].strip()
    return n or "PRESET"


def category_letter(patch):
    for t in patch.get("tags") or []:
        c = CAT_LETTER.get(str(t).strip().upper())
        if c:
            return c
    return None


CAT_LINE = re.compile(r'^(    "([A-Z?]*)",)( *)(?=/\*|$)', re.M)


def update_categories(changes):
    """changes: engine index -> {preset index: letter}; the comments stay in their column"""
    path = SRC / "category.c"
    txt = path.read_text(encoding="utf-8")
    m = re.search(r"static const char \*const PRESET_CAT\[\d+\] = \{\n(.*?)\n\};", txt, re.S)
    body = m.group(1)
    for ei, ch in sorted(changes.items()):
        ln = list(CAT_LINE.finditer(body))[ei]
        s = list(ln[2])
        for k, c in sorted(ch.items()):
            s += ["O"] * (k + 1 - len(s))
            s[k] = c
        head = '    "' + "".join(s) + '",'
        col = len(ln[1]) + len(ln[3])
        new = head + " " * max(1 if ln[3] else 0, col - len(head))
        body = body[:ln.start()] + new + body[ln.end():]
    path.write_text(txt[:m.start(1)] + body + txt[m.end(1):], encoding="utf-8")


def cat_of(ei, k):
    txt = (SRC / "category.c").read_text(encoding="utf-8")
    m = re.search(r"static const char \*const PRESET_CAT\[\d+\] = \{\n(.*?)\n\};", txt, re.S)
    s = [x[2] for x in CAT_LINE.finditer(m.group(1))][ei]
    return s[k] if k < len(s) else None


def mock_entry(r):
    args = [json.dumps(r["name"]), "[" + ", ".join(map(str, r["e"])) + "]", "[" + ", ".join(map(str, r["env"])) + "]"]
    pat = r["pat"] or 0
    if r["mono"] or pat:
        args.append(str(r["mono"]))
    if pat:
        args.append(str(pat))
    return "P(" + ", ".join(args) + ")"


def update_mock(table):
    """the editor's offline table of this engine (its mock, web/editor.html): the presets imported, in place (a
    replaced one) or at the end of the list (an added one)"""
    html = WEB.read_text(encoding="utf-8")
    m = re.search(r'\{ name: "' + re.escape(table.name) + r'", titles:', html)
    if not m:
        raise SystemExit(f"web/editor.html: no offline table of {table.name}")
    i = html.index("presets: [", m.end())
    j, depth = i + len("presets: ["), 1
    while depth:
        depth += (html[j] == "[") - (html[j] == "]")
        j += 1
    body = html[i:j - 1]                              # (j - 1: the list's "]")
    ent = list(re.finditer(r'P\("((?:[^"\\]|\\.)*)", \[[^\]]*\], \[[^\]]*\](?:, \d+)?(?:, \d+)?\)', body))
    if len(ent) > len(table.rows) or any(json.loads('"' + e[1] + '"') != table.rows[k][1]["name"] for k, e in enumerate(ent)):
        raise SystemExit(f"web/editor.html: the offline presets of {table.name} differ from the firmware's")
    for k in sorted(table.changed, reverse=True):
        if k < len(ent):
            body = body[:ent[k].start()] + mock_entry(table.rows[k][1]) + body[ent[k].end():]
    add = [mock_entry(table.rows[k][1]) for k in sorted(table.changed) if k >= len(ent)]
    if add:
        body += "".join(",\n        " + a for a in add)
    WEB.write_text(html[:i] + body + html[j - 1:], encoding="utf-8")


def main(argv):
    dry = "--dry-run" in argv
    files = [a for a in argv if a != "--dry-run"]
    if not files:
        raise SystemExit(__doc__)
    desc = load_desc()
    if not desc:
        print("note: build/host/desc.json missing (tests/run_tests.sh writes it): no range check, no pattern match,"
              " no list of what is lost")
    tabs = tables()
    order = [x["name"] for x in desc["ENG"]] if desc else None
    cats, touched, report = {}, set(), []
    for f in files:
        obj = json.loads(Path(f).read_text(encoding="utf-8"))
        if obj.get("format") == "felucca-patch":
            obj = {"paramLabels": obj.get("paramLabels"), "patches": [dict(obj, params=obj.get("p"))]}
        elif obj.get("format") != "felucca-library":
            raise SystemExit(f"{f}: not a file of the editor's library (felucca-library)")
        keys = obj.get("paramLabels") or []
        if not keys:
            raise SystemExit(f"{f}: no paramLabels (export it again from the editor)")
        for patch in obj.get("patches", []):
            eng = patch.get("engineName") or ""
            name = preset_name(patch.get("name"))
            notes = []
            if eng in NO_PRESETS or eng not in tabs:
                report.append(f"  skipped {eng or '?'} {name}: " + NO_PRESETS.get(eng, "no factory presets for this engine"))
                continue
            tb = tabs[eng]
            if tb.bad:
                report.append(f"  skipped {eng} {name}: {tb.bad}")
                continue
            try:
                r, osc2 = sound_of(patch, keys, desc, eng, tb, notes)
            except ValueError as e:
                report.append(f"  skipped {eng} {name}: {e}")
                continue
            k = tb.find(name)
            old = tb.rows[k][1] if k is not None else None
            r["name"] = name
            if old:                                   # replaced: its pattern hint (unless the steps are one) stays,
                r["pat"] = r["pat"] or old["pat"]     # and a table without sends stays without
                if old["fx"] is None:
                    r["fx"] = None
            tb.put(k, r)
            idx = k if k is not None else len(tb.rows) - 1
            if osc2 is not None:
                if k is None:
                    tb.osc2.append(osc2)
                else:
                    tb.osc2[k] = osc2
            ei = order.index(eng) if order else None
            if ei is not None:
                c = category_letter(patch) or (cat_of(ei, idx) if k is not None else None) or "O"
                cats.setdefault(ei, {})[idx] = c
            touched.add(eng)
            report.append(f"  {'replaced' if old else 'added   '} {eng} {idx + 1:2d} {name}")
            report += ["      " + n for n in notes]
    print("\n".join(report) if report else "nothing to import")
    if dry or not touched:
        print("(dry run: nothing written)" if dry else "")
        return
    for eng in sorted(touched):
        tabs[eng].write()
        update_mock(tabs[eng])
    if cats:
        update_categories(cats)
    print("written: " + ", ".join(sorted({tabs[e].path.name for e in touched})) + ", category.c" +
          ", web/editor.html")
    print("next: ./build.sh && GOLDEN_UPDATE=1 tests/run_tests.sh")


if __name__ == "__main__":
    main(sys.argv[1:])
