// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page's part without a page (web/library/lib.js) against the .FM1 files USB DISK makes
// (tests/vdisk_test.c leaves them in build/vdisk/files):
//   ZIP     a ZIP of ours read back; a deflated one; not one, cut short: refused
//   IMPORT  .FM1 files and a ZIP of them taken in: checked, looked at by the firmware, none twice (the same bytes
//           under another name), a damaged one refused with its reason and the rest kept; the names out, none twice;
//           every file as one ZIP and back in: the same bytes
//   WHAT    what a project holds is what plays (tests/vdisk_test.c's second disk, build/vdisk/songs): its scene song;
//           else its SONG chain, with the other slots' files taken in with it (refused without them, as on the
//           FM-1); else its scenes one after another: the hits heard, bar by bar, and the length
//   OWN     the page's own order of scenes (up to 16 rows, each repeated 1..16 times): those scenes, in that order
//   LEVELS  a track's LEVEL set for the render (the page's faders): quieter by the table's dB, 0 silent, the stems too
//   PLAY    the master handed over while it is made: the same samples as web/bounce's bounce, in pieces of about
//           0.5 s, the WAV byte for byte web/bounce's; the stems' ZIP: six files
// node web/library/library_test.mjs build/bounce/felucca_bounce.wasm build/vdisk/files
import { readFileSync } from "node:fs";
import { deflateRawSync } from "node:zlib";
import { crc32, parseFM1, loadWasm, prepare, render, bounceFiles, zipParts } from "../bounce/bounce.js";
import { unzip, candidates, makeRecord, exportNames, libraryZip, inspect, defaultMode, defaultBars, play, wavOfPcm,
         stemsZip, importFiles, titleOf, siblings, chainSlots, levelTable, SCENE, SSONG, CHAIN, SCENES, OWN } from "./lib.js";

const [wasmPath = "build/bounce/felucca_bounce.wasm", dir = "build/vdisk/files", songs = `${dir}/../songs`] = process.argv.slice(2);
const wasm = readFileSync(wasmPath);
let bad = 0;
const check = (what, ok) => { console.log(`library: ${what.padEnd(100)} ${ok ? "ok" : "FAIL"}`); if (!ok) bad++; };
const join = (parts) => { const n = parts.reduce((a, p) => a + p.length, 0), o = new Uint8Array(n); let at = 0;
                          for (const p of parts) { o.set(p, at); at += p.length; } return o; };
const same = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);
const throwsLike = async (fn, re) => { try { await fn(); return false; } catch (e) { return re.test(e.code); } };   // (by the code: the page words it)
const file = (n) => new Uint8Array(readFileSync(`${dir}/${n}`));
const a = file("PROJ_A.FM1"), c = file("PROJ_C.FM1"), cur = file("CURRENT.FM1");

// ---- ZIP
const ours = join(zipParts([{ name: "A.FM1", parts: [a] }, { name: "sub/C.FM1", parts: [c.subarray(0, 100), c.subarray(100)] }]));
const back = await unzip(ours);
check("ZIP: one of ours read back: two files, their names and bytes", back.length === 2 && back[0].name === "A.FM1" &&
      same(back[0].data, a) && back[1].name === "sub/C.FM1" && same(back[1].data, c));
function deflatedZip(name, data) {                        // as a computer's ZIP tool packs: method 8
  const z = deflateRawSync(data), nm = new TextEncoder().encode(name);
  const h = new DataView(new ArrayBuffer(30)), cd = new DataView(new ArrayBuffer(46)), e = new DataView(new ArrayBuffer(22));
  h.setUint32(0, 0x04034B50, true); h.setUint16(8, 8, true); h.setUint32(14, crc32(data), true);
  h.setUint32(18, z.length, true); h.setUint32(22, data.length, true); h.setUint16(26, nm.length, true);
  cd.setUint32(0, 0x02014B50, true); cd.setUint16(10, 8, true); cd.setUint32(16, crc32(data), true);
  cd.setUint32(20, z.length, true); cd.setUint32(24, data.length, true); cd.setUint16(28, nm.length, true);
  e.setUint32(0, 0x06054B50, true); e.setUint16(8, 1, true); e.setUint16(10, 1, true);
  e.setUint32(12, 46 + nm.length, true); e.setUint32(16, 30 + nm.length + z.length, true);
  return join([new Uint8Array(h.buffer), nm, z, new Uint8Array(cd.buffer), nm, new Uint8Array(e.buffer)]);
}
const packed = deflatedZip("CURRENT.FM1", cur);
check("  a deflated one (a computer's ZIP tool): unpacked, the same bytes", same((await unzip(packed))[0].data, cur));
check("  not a ZIP: refused; cut short: refused", await throwsLike(() => unzip(a), /^nozip$/) &&
      await throwsLike(() => unzip(ours.subarray(60)), /^zip(bad|short)$/));

// ---- IMPORT (the store: a Map standing in for IndexedDB)
const map = new Map();
const store = { get: async (k) => map.get(k), put: async (r) => void map.set(r.key, r) };
const look = (bytes) => inspect(wasm, bytes);
const hurt = a.slice(); hurt[hurt.length - 10] ^= 1;
const r1 = await importFiles(store, [{ name: "PROJ_A.FM1", data: a }, { name: "COPY.FM1", data: a },
                                     { name: "HURT.FM1", data: hurt }, { name: "notes.txt", data: new Uint8Array(80) },
                                     { name: "/x/two.zip", data: join([ours]) }, { name: "p.zip", data: packed }], look, 1000);
check("IMPORT: A, the ZIP's C and the packed CURRENT taken; A again (a copy, and in the ZIP) known, not stored twice",
      r1.added.length === 3 && map.size === 3 && r1.known.join() === "COPY.FM1,A.FM1" &&
      r1.added.map((r) => r.name).join() === "FIRST,THIRD,LIVE");
check("  the damaged one and the text file refused, each with its reason (a code, its part); the rest kept",
      r1.bad.length === 2 && r1.bad[0].code === "crc" && r1.bad[0].args[0] === "SMP1" && r1.bad[0].name === "HURT.FM1" &&
      r1.bad[1].code === "notfm1");
const ra = r1.added[0];
check("  a record: the bytes as they came, slot A, its parts, what the firmware plays (120 BPM, 1 bar, no songs)",
      same(ra.bytes, a) && ra.slotName === "A" && ra.parts[0].tag === "PROJ" && ra.info.bpm === 120 && ra.info.bars === 1 && ra.info.rate === 44100 &&
      !ra.info.has[0] && !ra.info.has[1] && ra.info.scenes === 1 && defaultMode(ra.info) === SCENES && defaultBars(ra.info) === 4);
check("  what plays unasked: the scene song, else the SONG chain when its projects are there, else the scenes in a row",
      defaultMode({ has: [true, true], miss: 0 }) === SSONG && defaultMode({ has: [false, true], miss: 0 }) === CHAIN &&
      defaultMode({ has: [false, true], miss: 5 }) === SCENES && defaultMode({ has: [false, true], miss: 5 }, 0) === CHAIN &&
      defaultMode({ has: [false, false], miss: 0 }) === SCENES && defaultBars({ bars: 8 }) === 16);
const r2 = await importFiles(store, [{ name: "again.FM1", data: c }], look, 2000);
check("  a later import of a file already there: known, nothing added", !r2.added.length && r2.known.length === 1 && map.size === 3);
const zc = await candidates([{ name: "empty.zip", data: join(zipParts([{ name: "a.txt", parts: [new Uint8Array(3)] }])) }]);
check("  a ZIP without a .FM1 file: said so", zc.out.length === 0 && zc.bad[0].code === "zipnofm1");
const recs = [...map.values()];
recs[1].title = "First";                                  // a title given in the page, the same as another's name
check("  names out: FIRST.FM1, FIRST_2.FM1, LIVE.FM1 (none twice); the title before the project's name",
      exportNames(recs).join() === "FIRST.FM1,FIRST_2.FM1,LIVE.FM1" && titleOf(recs[1]) === "First" && titleOf(recs[0]) === "FIRST");
const all = await unzip(join(libraryZip(recs)));
const map2 = new Map(), store2 = { get: async (k) => map2.get(k), put: async (r) => void map2.set(r.key, r) };
const r3 = await importFiles(store2, [{ name: "JAN3000-Bibliothek.zip", data: join(libraryZip(recs)) }], look, 3000);
check("  every file as one ZIP, and that ZIP into an empty library: the same three, byte for byte",
      all.length === 3 && r3.added.length === 3 && recs.every((r) => map2.has(r.key) && same(map2.get(r.key).bytes, r.bytes)));
check("  a record's key: the same bytes the same key, other bytes another", makeRecord("x", a).key === makeRecord("y", a).key &&
      makeRecord("x", a).key !== makeRecord("x", c).key);

// ---- PLAY
const ref = await render(prepare(await loadWasm(wasm), parseFM1(a)), { mode: 0, maxMs: 4000, tailMs: 3000, stems: false });
const refWav = join(bounceFiles(ref, "X")[0].parts);
const got = [];
let lastMs = 0, order = true, total = 0;
const pr = await play(wasm, a, { mode: 0, bars: 2 }, (pcm, ms, tot) => { got.push(pcm); order &&= ms > lastMs; lastMs = ms; total = tot; });
const wav = join(wavOfPcm(got, pr.frames, pr.rate));
check("PLAY: 2 bars handed over while made: web/bounce's frames, the WAV byte for byte web/bounce's master",
      pr.frames === ref.frames && pr.rate === 44100 && same(wav, refWav));
const longest = Math.max(...got.map((p) => p.length / 2 / 44100));
check(`  in ${got.length} pieces of at most ${longest.toFixed(2)} s, in order, the planned length said (7.0 s)`,
      got.length >= 8 && longest <= 0.6 && order && total === 7000);
const sz = await unzip(join(await stemsZip(wasm, a, { mode: 0, bars: 2 }, "FIRST")));
check("  the stems' ZIP: FIRST_MIX.wav and T1 T2 T3 T4 FX, the master the same file",
      sz.map((f) => f.name).join() === "FIRST_MIX.wav,FIRST_T1.wav,FIRST_T2.wav,FIRST_T3.wav,FIRST_T4.wav,FIRST_FX.wav" &&
      same(sz[0].data, refWav));
check("  a file that is not a project: the reason, not a crash", await throwsLike(() => play(wasm, new Uint8Array(64), {}, () => {}), /^notfm1$/));

// ---- WHAT PLAYS (tests/vdisk_test.c's second disk, its projects told by T1's hits in a bar: B three scenes of 4, 8
// and 2; D the same with a scene song 2 2 1 4; CURRENT a SONG chain of project A once (4) and C twice (8), the
// projects in their own files)
const song = (n) => new Uint8Array(readFileSync(`${songs}/${n}`));
const sa = song("PROJ_A.FM1"), sb = song("PROJ_B.FM1"), sc = song("PROJ_C.FM1"), sd = song("PROJ_D.FM1"), scur = song("CURRENT.FM1");
// the hits in the bar (2 s at 120 BPM) that starts at t seconds: the level in 10 ms pieces, a hit where it jumps
function hitsAt(pcm, t) {
  const lv = [];
  for (let k = -3; k < 200; k++) {
    const from = Math.max(0, Math.round((t + k * 0.01) * 44100));
    let e = 0;
    for (let i = 0; i < 441; i++) e += (pcm[2 * (from + i)] || 0) ** 2;
    lv.push(Math.sqrt(e / 441));
  }
  let n = 0, last = -99;
  for (let k = 3; k < lv.length; k++)
    if (lv[k] > 150 && lv[k] > 3 * Math.max(lv[k - 3], 1) && k - last > 10) { n++; last = k; }
  return n;
}
async function heard(bytes, opts, starts) {
  const got = [];
  const r = await play(wasm, bytes, opts, (p) => got.push(p));
  const all = new Int16Array(2 * r.frames);
  let at = 0;
  for (const p of got) { all.set(p, at); at += p.length; }
  return { secs: r.frames / r.rate, notes: starts.map((t) => hitsAt(all, t)).join() };
}
const ib = await inspect(wasm, sb), id = await inspect(wasm, sd), ic = await inspect(wasm, scur);
check("WHAT PLAYS: B holds 3 scenes and no song: its scenes in a row; D a scene song: that; CURRENT a SONG chain of A and C",
      ib.scenes === 3 && !ib.has[0] && !ib.has[1] && defaultMode(ib) === SCENES && id.has[0] && defaultMode(id) === SSONG &&
      ic.has[1] && ic.need === 5 && ic.miss === 5 && chainSlots(ic, 4).join() === "0,2");
let h = await heard(sb, { mode: SCENES }, [0, 2, 4]);
check(`  B, the scenes in a row: scene 1, 2, 4 (4 8 2 hits: ${h.notes}), each one pass, 6 s and what rings on (${h.secs.toFixed(2)} s)`,
      h.notes === "4,8,2" && h.secs > 6 && h.secs < 9.1);
h = await heard(sd, { mode: SSONG }, [0, 2, 4, 6]);
check(`  D, its scene song: scene 2 twice, 1, 4 (8 8 4 2 hits: ${h.notes}), 8 s and what rings on (${h.secs.toFixed(2)} s), then STOP`,
      h.notes === "8,8,4,2" && h.secs > 8 && h.secs < 11.1);
h = await heard(sa, { mode: SCENES }, [0]);
check(`  A, one scene: that scene once (4 hits: ${h.notes}; ${h.secs.toFixed(2)} s)`, h.notes === "4" && h.secs > 2 && h.secs < 5.1);
h = await heard(sd, { mode: SCENE, bars: 3 }, [0, 2, 4]);
check(`  D, the scene alone for 3 bars: the one the project was saved on, looping (4 4 4 hits: ${h.notes})`, h.notes === "4,4,4");
check("  CURRENT's SONG chain without the projects it plays: refused as on the FM-1, naming them (A, C)",
      await (async () => { try { await play(wasm, scur, { mode: CHAIN }, () => {}); return false; }
                           catch (e) { return e.code === "chainmiss" && e.args[0] === "A, C"; } })());
h = await heard(scur, { mode: CHAIN, sib: [sa, sc] }, [0, 2, 4]);
check(`  .. with A's and C's files: A once, C twice (4 8 8 hits: ${h.notes}), 6 s and what rings on (${h.secs.toFixed(2)} s), then STOP`,
      h.notes === "4,8,8" && h.secs > 6 && h.secs < 9.1 && (await inspect(wasm, scur, [sa, sc])).miss === 0);
// the library finds those files: the ones taken in with the record (one read of the drive)
const map3 = new Map(), store3 = { get: async (k) => map3.get(k), put: async (r) => void map3.set(r.key, r) };
const look3 = (bytes) => inspect(wasm, bytes);
await importFiles(store3, [{ name: "CURRENT.FM1", data: scur }, { name: "PROJ_A.FM1", data: sa }], look3, 5000);
let lib = [...map3.values()], rc = lib.find((r) => r.name === "CHAIN"), sbs = siblings(rc, lib);
check("  the library: CURRENT taken in with A only: A found, C missing; a project without a chain needs none",
      sbs.sib.length === 1 && same(sbs.sib[0], sa) && sbs.miss.join() === "2" &&
      siblings(lib.find((r) => r.name === "FIRST"), lib).miss.length === 0);
await importFiles(store3, [{ name: "PROJ_C.FM1", data: sc }], look3, 6000);
lib = [...map3.values()];
check("  C taken in later, by itself: not the same read of the drive, still missing", siblings(rc, lib).miss.join() === "2");
await importFiles(store3, [{ name: "CURRENT.FM1", data: scur }, { name: "PROJ_C.FM1", data: sc }], look3, 6500);
lib = [...map3.values()];
check("  both picked again together (nothing new: both known): now one read of the drive, C found too",
      map3.size === 3 && siblings(rc, lib).miss.length === 0 && siblings(rc, lib).sib.length === 2);
const map4 = new Map(), store4 = { get: async (k) => map4.get(k), put: async (r) => void map4.set(r.key, r) };
await importFiles(store4, ["CURRENT", "PROJ_A", "PROJ_B", "PROJ_C", "PROJ_D"].map((n) => ({ name: `${n}.FM1`, data: song(`${n}.FM1`) })), look3, 7000);
lib = [...map4.values()]; rc = lib.find((r) => r.name === "CHAIN"); sbs = siblings(rc, lib);
check("  the whole drive taken in at once: A and C found, nothing missing",
      sbs.miss.length === 0 && sbs.sib.length === 2 && same(sbs.sib[0], sa) && same(sbs.sib[1], sc));

// ---- OWN ORDER (the page's own row of scenes: B's scenes 1, 2, 4 hold 4, 8, 2 hits)
check("OWN ORDER: which scenes B holds: 1, 2 and 4 (bits 0, 1, 3)", ib.sceneMask === 0b1011);
h = await heard(sb, { mode: OWN, order: [{ scene: 3, repeat: 1 }, { scene: 0, repeat: 2 }, { scene: 1, repeat: 1 }] }, [0, 2, 4, 6]);
check(`  scene 4, scene 1 twice, scene 2 (2 4 4 8 hits: ${h.notes}), 8 s and what rings on (${h.secs.toFixed(2)} s), then STOP`,
      h.notes === "2,4,4,8" && h.secs > 8 && h.secs < 11.1);
h = await heard(sb, { mode: OWN, order: [{ scene: 1, repeat: 1 }, { scene: 1, repeat: 1 }, { scene: 3, repeat: 2 }] }, [0, 2, 4, 6]);
check(`  the same scene in two rows one after another, then one twice (8 8 2 2 hits: ${h.notes})`, h.notes === "8,8,2,2");
check("  no rows, or more than 16: refused, saying why",
      await throwsLike(() => play(wasm, sb, { mode: OWN, order: [] }, () => {}), /^noorder$/) &&
      await throwsLike(() => play(wasm, sb, { mode: OWN, order: Array(17).fill({ scene: 0, repeat: 1 }) }, () => {}), /^noorder$/));
const so = await unzip(join(await stemsZip(wasm, sb, { mode: OWN, order: [{ scene: 3, repeat: 1 }] }, "X")));
check("  the stems follow the order too (one pass of scene 4: 2 s and what rings on)", so.length === 6 &&
      (so[0].data.length - 44) / 4 / 44100 > 2 && (so[0].data.length - 44) / 4 / 44100 < 5.1);

// ---- LEVELS (the page's faders: a track's LEVEL for the render, the project's own left in the file)
const db = await levelTable(wasm);
const loud = async (levels) => { let e = 0, n = 0; await play(wasm, sb, { mode: SCENES, levels }, (p) => { for (const v of p) { e += v * v; n++; } });
                                 return 10 * Math.log10(e / n); };
const l0 = await loud(null), l1 = await loud([104, null, null, null]), l2 = await loud([80, null, null, null]);
check("LEVELS: the project's own are 104 on every track (-4 dB); the table: 0 silent, 104 -4 dB, 127 the loudest",
      ib.levels.join() === "104,104,104,104" && db[0] === -Infinity && Math.abs(db[104] + 4) < 0.05 && db[127] > db[126] && db.length === 128);
check(`  T1 at its own value: the same level; at 80 (${(db[80] - db[104]).toFixed(1)} dB): that much quieter (${(l2 - l0).toFixed(1)} dB)`,
      l1 === l0 && Math.abs((l2 - l0) - (db[80] - db[104])) < 0.5);
check("  T1 at 0: silence (the project plays T1 only); the other tracks' faders change nothing there",
      await loud([0, null, null, null]) === -Infinity && await loud([null, 0, 0, 0]) === l0);
const st = await unzip(join(await stemsZip(wasm, sb, { mode: SCENES, levels: [0, null, null, null] }, "X")));
check("  the stems with T1 at 0: T1's file silent too", st[1].name === "X_T1.wav" && st[1].data.subarray(44).every((x) => x === 0));

console.log(bad ? `library: ${bad} FAILED` : "library: all ok");
process.exit(bad ? 1 : 0);
