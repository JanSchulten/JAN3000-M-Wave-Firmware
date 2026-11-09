// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page's part without a page (web/library/lib.js) against the .FM1 files USB DISK makes
// (tests/vdisk_test.c leaves them in build/vdisk/files):
//   ZIP     a ZIP of ours read back; a deflated one; not one, cut short: refused
//   IMPORT  .FM1 files and a ZIP of them taken in: checked, looked at by the firmware, none twice (the same bytes
//           under another name), a damaged one refused with its reason and the rest kept; the names out, none twice;
//           every file as one ZIP and back in: the same bytes
//   PLAY    the master handed over while it is made: the same samples as web/bounce's bounce, in pieces of about
//           0.5 s, the WAV byte for byte web/bounce's; the stems' ZIP: six files
// node web/library/library_test.mjs build/bounce/felucca_bounce.wasm build/vdisk/files
import { readFileSync } from "node:fs";
import { deflateRawSync } from "node:zlib";
import { crc32, parseFM1, loadWasm, prepare, render, bounceFiles, zipParts } from "../bounce/bounce.js";
import { unzip, candidates, makeRecord, exportNames, libraryZip, inspect, defaultMode, defaultBars, play, wavOfPcm,
         stemsZip, importFiles, titleOf } from "./lib.js";

const [wasmPath = "build/bounce/felucca_bounce.wasm", dir = "build/vdisk/files"] = process.argv.slice(2);
const wasm = readFileSync(wasmPath);
let bad = 0;
const check = (what, ok) => { console.log(`library: ${what.padEnd(100)} ${ok ? "ok" : "FAIL"}`); if (!ok) bad++; };
const join = (parts) => { const n = parts.reduce((a, p) => a + p.length, 0), o = new Uint8Array(n); let at = 0;
                          for (const p of parts) { o.set(p, at); at += p.length; } return o; };
const same = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);
const throwsLike = async (fn, re) => { try { await fn(); return false; } catch (e) { return re.test(e.message); } };
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
check("  not a ZIP: refused; cut short: refused", await throwsLike(() => unzip(a), /Keine ZIP/) &&
      await throwsLike(() => unzip(ours.subarray(60)), /beschädigt|unvollständig/));

// ---- IMPORT (the store: a Map standing in for IndexedDB)
const map = new Map();
const store = { has: async (k) => map.has(k), put: async (r) => void map.set(r.key, r) };
const look = (bytes) => inspect(wasm, bytes);
const hurt = a.slice(); hurt[hurt.length - 10] ^= 1;
const r1 = await importFiles(store, [{ name: "PROJ_A.FM1", data: a }, { name: "COPY.FM1", data: a },
                                     { name: "HURT.FM1", data: hurt }, { name: "notes.txt", data: new Uint8Array(80) },
                                     { name: "/x/two.zip", data: join([ours]) }, { name: "p.zip", data: packed }], look, 1000);
check("IMPORT: A, the ZIP's C and the packed CURRENT taken; A again (a copy, and in the ZIP) known, not stored twice",
      r1.added.length === 3 && map.size === 3 && r1.known.join() === "COPY.FM1,A.FM1" &&
      r1.added.map((r) => r.name).join() === "FIRST,THIRD,LIVE");
check("  the damaged one and the text file refused, each with its reason; the rest kept",
      r1.bad.length === 2 && /Prüfsumme/.test(r1.bad[0].why) && r1.bad[0].name === "HURT.FM1" && /FM1X/.test(r1.bad[1].why));
const ra = r1.added[0];
check("  a record: the bytes as they came, slot A, its parts, what the firmware plays (120 BPM, 1 bar, no songs)",
      same(ra.bytes, a) && ra.slotName === "A" && ra.parts[0].tag === "PROJ" && ra.info.bpm === 120 && ra.info.bars === 1 &&
      ra.info.rate === 44100 && !ra.info.has[0] && !ra.info.has[1] && defaultMode(ra.info) === 0 && defaultBars(ra.info) === 4);
check("  defaults: the SONG chain before the scene song before the scene; a long scene twice",
      defaultMode({ has: [true, true] }) === 2 && defaultMode({ has: [true, false] }) === 1 && defaultBars({ bars: 8 }) === 16);
const r2 = await importFiles(store, [{ name: "again.FM1", data: c }], look, 2000);
check("  a later import of a file already there: known, nothing added", !r2.added.length && r2.known.length === 1 && map.size === 3);
const zc = await candidates([{ name: "empty.zip", data: join(zipParts([{ name: "a.txt", parts: [new Uint8Array(3)] }])) }]);
check("  a ZIP without a .FM1 file: said so", zc.out.length === 0 && /keine \.FM1/.test(zc.bad[0].why));
const recs = [...map.values()];
recs[1].title = "First";                                  // a title given in the page, the same as another's name
check("  names out: FIRST.FM1, FIRST_2.FM1, LIVE.FM1 (none twice); the title before the project's name",
      exportNames(recs).join() === "FIRST.FM1,FIRST_2.FM1,LIVE.FM1" && titleOf(recs[1]) === "First" && titleOf(recs[0]) === "FIRST");
const all = await unzip(join(libraryZip(recs)));
const map2 = new Map(), store2 = { has: async (k) => map2.has(k), put: async (r) => void map2.set(r.key, r) };
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
check("  a file that is not a project: the reason, not a crash", await throwsLike(() => play(wasm, new Uint8Array(64), {}, () => {}), /FM1X/));

console.log(bad ? `library: ${bad} FAILED` : "library: all ok");
process.exit(bad ? 1 : 0);
