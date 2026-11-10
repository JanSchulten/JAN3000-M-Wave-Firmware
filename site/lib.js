// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page's part without a page (web/library/index.html and its worker use it, web/library/library_test.mjs
// tests it): .FM1 files (MENU > USB DISK) and ZIPs of them read into records, the records kept in the browser
// (IndexedDB: on this device only, nothing is sent anywhere), a record looked at and played by the FM-1's own firmware
// (web/bounce/bounce.js on felucca_bounce.wasm) with the master handed over slice by slice while it is made, and the
// way back out: the .FM1 as it came, the master as a WAV, the stems as a ZIP, every file as one ZIP.

import { crc32, parseFM1, loadWasm, prepare, render, wavHeader, bounceFiles, zipParts, levelDb } from "./bounce.js";

// an error with a code and its values: the page says it in its language (bounce.js's errors carry the same; their
// messages are German, as web/bounce's page, ours English)
const fail = (code, message, args = []) => Object.assign(new Error(message), { code, args });
const SONG_MAX_MS = 600000;                               // a song ends itself; this is the stop of one that does not
export const TAIL_MS = 3000;
const BATCH = 8;                                          // slices of 64 ms handed over together (about 0.5 s)

// ---- ZIP in (ours are stored; one packed elsewhere may be deflated)

async function inflateRaw(data) {
  const ds = new DecompressionStream("deflate-raw"), w = ds.writable.getWriter();
  w.write(data).catch(() => {});
  w.close().catch(() => {});
  return new Uint8Array(await new Response(ds.readable).arrayBuffer());
}

// the files of a ZIP: [{name, data}]. Throws with a message for the page when it is not one
export async function unzip(bytes) {
  const u8 = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
  const dv = new DataView(u8.buffer, u8.byteOffset, u8.byteLength);
  let e = u8.length - 22;
  while (e >= 0 && dv.getUint32(e, true) !== 0x06054B50) e--;
  if (e < 0) throw fail("nozip", "Not a ZIP file.");
  const n = dv.getUint16(e + 10, true), dec = new TextDecoder();
  let c = dv.getUint32(e + 16, true);
  const out = [];
  for (let i = 0; i < n; i++) {
    if (c + 46 > u8.length || dv.getUint32(c, true) !== 0x02014B50) throw fail("zipbad", "The ZIP file is damaged.");
    const method = dv.getUint16(c + 10, true), csize = dv.getUint32(c + 20, true), size = dv.getUint32(c + 24, true);
    const nl = dv.getUint16(c + 28, true), xl = dv.getUint16(c + 30, true), kl = dv.getUint16(c + 32, true);
    const at = dv.getUint32(c + 42, true), name = dec.decode(u8.subarray(c + 46, c + 46 + nl));
    c += 46 + nl + xl + kl;
    if (name.endsWith("/")) continue;
    if (at + 30 > u8.length || dv.getUint32(at, true) !== 0x04034B50) throw fail("zipbad", "The ZIP file is damaged.");
    const from = at + 30 + dv.getUint16(at + 26, true) + dv.getUint16(at + 28, true);
    if (from + csize > u8.length) throw fail("zipshort", "The ZIP file is incomplete.");
    const raw = u8.subarray(from, from + csize);
    let data;
    if (method === 0) data = raw.slice();
    else if (method === 8) data = await inflateRaw(raw).catch(() => { throw fail("zipinflate", `${name}: cannot be unpacked.`, [name]); });
    else throw fail("zipmethod", `${name}: unknown ZIP compression.`, [name]);
    if (data.length !== size) throw fail("ziplen", `${name}: wrong length in the ZIP file.`, [name]);
    out.push({ name, data });
  }
  return out;
}

// ---- records

const isZip = (u8) => u8.length > 3 && u8[0] === 0x50 && u8[1] === 0x4B && (u8[2] === 3 || u8[2] === 5);
const baseName = (path) => path.replace(/^.*[\\/]/, "");
export const safeName = (s) => (s || "").replace(/[^A-Za-z0-9_-]+/g, "_").replace(/^_+|_+$/g, "") || "FM1";

// what was picked or dropped, as .FM1 candidates: ZIPs opened, their .FM1 files taken (other files in them left out)
export async function candidates(files) {
  const out = [], bad = [];
  for (const f of files) {
    if (!isZip(f.data)) { out.push({ name: baseName(f.name), data: f.data }); continue; }
    try {
      const inner = (await unzip(f.data)).filter((x) => /\.fm1$/i.test(x.name) && !/(^|\/)\._/.test(x.name));
      if (!inner.length) bad.push({ name: f.name, why: "The ZIP file holds no .FM1 file.", code: "zipnofm1", args: [] });
      for (const x of inner) out.push({ name: baseName(x.name), data: x.data });
    } catch (err) {
      bad.push({ name: f.name, why: err.message, code: err.code, args: err.args });
    }
  }
  return { out, bad };
}

// a checked .FM1 as a record (without what only the firmware knows: inspect() adds that). batches: the times it was
// taken in; files taken in together (one read of the FM-1's drive) share one: their slots belong together, which a
// SONG chain needs
export function makeRecord(name, data, now = Date.now()) {
  const fm1 = parseFM1(data);
  return { key: crc32(data).toString(16).padStart(8, "0") + "-" + data.length, file: name, bytes: data.slice(),
           name: fm1.name || name.replace(/\.fm1$/i, ""), title: "", note: "", slot: fm1.slot, slotName: fm1.slotName,
           fw: fm1.fw, parts: fm1.entries.map((x) => ({ tag: x.tag, len: x.len })), added: now, batches: [now], mode: -1, bars: 0 };
}
export const titleOf = (r) => r.title || r.name || r.file;
// the files of the other slots a record's SONG chain plays, from the records taken in with it (a batch of its): {sib: their
// bytes, miss: the slots (0..3) not there, or there more than once (which one, nobody can say)}
export function siblings(rec, recs) {
  const sib = [], miss = [];
  for (const k of chainSlots(rec.info, rec.slot)) {
    const c = recs.filter((x) => x !== rec && x.slot === k && (x.batches || []).some((b) => (rec.batches || []).includes(b)));
    if (c.length === 1) sib.push(c[0].bytes); else miss.push(k);
  }
  return { sib, miss };
}

// file names for a set of records, none twice: NAME.FM1, NAME_2.FM1 ..
export function exportNames(recs) {
  const seen = new Map();
  return recs.map((r) => {
    const b = safeName(titleOf(r)).toUpperCase(), k = (seen.get(b) || 0) + 1;
    seen.set(b, k);
    return k > 1 ? `${b}_${k}.FM1` : `${b}.FM1`;
  });
}
export function libraryZip(recs) {
  const names = exportNames(recs);
  return zipParts(recs.map((r, i) => ({ name: names[i], parts: [r.bytes] })));
}

// ---- the firmware at work (in the page: inside web/library/worker.js)

// the modes (bounce.js render): what the FM-1 would play of a project
export const SCENE = 0, SSONG = 1, CHAIN = 2, SCENES = 3, OWN = 4;
export const ORDER_MAX = 16;                              // rows of an own order (the scene song's, scene.c SSG_ROWS)
const others = (sib) => sib.map((b) => parseFM1(b));

// boot an FM-1 with the record's flash: what it holds. bars: one pass of the scene's patterns; has: [a scene song, a
// SONG chain]; scenes: how many hold something; need, miss: the slots (bit s: A + s) the SONG chain plays, and those of
// them with no project (sib: the .FM1 files of the other slots, their bytes, when the library has them)
export async function inspect(wasm, bytes, sib = []) {
  const p = prepare(await loadWasm(wasm), parseFM1(bytes), others(sib));
  return { rate: p.rate, bpm: p.bpm, bars: Math.max(1, Math.ceil(p.loopMs / (240000 / p.bpm) - 0.01)), has: p.has,
           scenes: p.scenes, sceneMask: p.sceneMask, need: p.chain.need, miss: p.chain.miss, levels: p.levels };
}
// the four tracks' LEVEL as the project has them is info.levels (0..127, the FM-1's LVL); a record may carry its own
// (rec.levels, the page's faders: null entries keep the project's). What each value is in dB:
export const levelTable = async (wasm) => levelDb(await loadWasm(wasm));
// what a record plays when nothing was chosen, as it is on the FM-1: its song when it has one (the scene song, which
// the file holds whole, before the SONG chain; the chain only when every project it plays is there: miss, as inspect
// with the siblings says), else its scenes one after another
export const defaultMode = (info, miss = info.miss) => (info.has[0] ? SSONG : info.has[1] && !miss ? CHAIN : SCENES);
export const defaultBars = (info) => Math.max(4, 2 * info.bars);
// the slots a SONG chain needs from other files: need without the record's own (slot 4, CURRENT: all of them)
export const chainSlots = (info, slot) => [0, 1, 2, 3].filter((k) => info.has[1] && info.need >> k & 1 && k !== slot);

export const toPcm16 = (f32) => {
  const p = new Int16Array(f32.length);
  for (let i = 0; i < f32.length; i++) p[i] = Math.max(-32768, Math.min(32767, Math.round(f32[i] * 32768)));
  return p;
};

// play the record (mode, bars for the scene; sib: the other slots' files for a SONG chain; levels: the tracks' LEVEL
// in place of the project's) and hand the master out as it is made: onAudio(Int16Array interleaved,
// ms done, ms planned or 0 when the song says where it ends), about twice a second of music. The master is 16-bit as
// USB audio sends it, so nothing is lost. Returns {rate, frames}
export async function play(wasm, bytes, { mode = 0, bars = 4, tailMs = TAIL_MS, sib = [], levels = null, order = [] } = {}, onAudio) {
  const prep = prepare(await loadWasm(wasm), parseFM1(bytes), others(sib), levels);
  const maxMs = mode === 0 ? Math.max(1, bars) * 240000 / prep.bpm : SONG_MAX_MS;
  let held = [], n = 0;
  const flush = (ms) => {
    if (!n) return;
    const all = new Int16Array(n);
    let at = 0;
    for (const c of held) { all.set(c, at); at += c.length; }
    held = []; n = 0;
    onAudio(all, ms, mode === 0 ? maxMs + tailMs : 0);
  };
  const r = await render(prep, { mode, maxMs, tailMs, stems: false, order,
    sink: (s, c) => { const p = toPcm16(c); held.push(p); n += p.length; },
    onProgress: (ms) => { if (held.length >= BATCH) flush(ms); } });
  flush(r.frames / r.rate * 1000);
  return { rate: r.rate, frames: r.frames };
}

// the master as a WAV (16-bit PCM), from what play() handed out: a list of byte arrays, the same file web/bounce makes
export function wavOfPcm(chunks, frames, rate) {
  return [wavHeader(frames, rate, false), ...chunks.map((c) => new Uint8Array(c.buffer, c.byteOffset, c.byteLength))];
}

// the master and the stems (T1..T4, FX) as a ZIP of WAVs, as web/bounce: a list of byte arrays
export async function stemsZip(wasm, bytes, { mode = 0, bars = 4, tailMs = TAIL_MS, sib = [], levels = null, order = [] } = {}, base = "FM1", onProgress) {
  const prep = prepare(await loadWasm(wasm), parseFM1(bytes), others(sib), levels);
  const maxMs = mode === 0 ? Math.max(1, bars) * 240000 / prep.bpm : SONG_MAX_MS;
  return zipParts(bounceFiles(await render(prep, { mode, maxMs, tailMs, stems: true, onProgress, order }), base));
}

// ---- the store (IndexedDB "jan3000-library", one object store of records by key). idb: the browser's, or a stand-in

export function openStore(idb = globalThis.indexedDB) {
  const req = (r) => new Promise((ok, no) => { r.onsuccess = () => ok(r.result); r.onerror = () => no(r.error); });
  const ready = new Promise((ok, no) => {
    if (!idb) return no(fail("noidb", "This browser has no storage for the library (IndexedDB)."));
    const r = idb.open("jan3000-library", 1);
    r.onupgradeneeded = () => r.result.createObjectStore("saves", { keyPath: "key" });
    r.onsuccess = () => ok(r.result);
    r.onerror = () => no(fail("openfail", "The library could not be opened."));
  });
  const tx = async (mode, fn) => req(fn((await ready).transaction("saves", mode).objectStore("saves")));
  return {
    all: async () => (await tx("readonly", (s) => s.getAll())).sort((a, b) => b.added - a.added),
    get: (key) => tx("readonly", (s) => s.get(key)),
    put: (rec) => tx("readwrite", (s) => s.put(rec)),
    remove: (key) => tx("readwrite", (s) => s.delete(key)),
  };
}

// take the candidates in: checked, looked at by the firmware (look(bytes) -> inspect's), stored unless already there
// (one already there joins this batch).
// Returns {added: [records], known: [names], bad: [{name, why, code, args}]} (why: the message; code, args: as fail())
export async function importFiles(store, files, look, now = Date.now()) {
  const { out, bad } = await candidates(files), added = [], known = [], seen = new Set();
  for (const f of out) {
    try {
      const rec = makeRecord(f.name, f.data, now + added.length);
      rec.batches = [now];
      const old = seen.has(rec.key) ? null : await store.get(rec.key);
      if (old) {                                          // already there: it belongs to this read of the drive too
        old.batches = [...(old.batches || []), now];
        await store.put(old);
      }
      if (old || seen.has(rec.key)) { known.push(f.name); seen.add(rec.key); continue; }
      seen.add(rec.key);
      rec.info = await look(rec.bytes);
      await store.put(rec);
      added.push(rec);
    } catch (err) {
      bad.push({ name: f.name, why: err.message, code: err.code, args: err.args });
    }
  }
  return { added, known, bad };
}
