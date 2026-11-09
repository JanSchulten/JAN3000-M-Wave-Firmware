// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// M-Vave-OP1's offline bounce, the part the page (web/bounce/index.html) and the test (web/bounce/bounce_test.mjs)
// share: a .FM1 file (MENU > USB DISK, firmware/src/usbdisk.c: FM1X) read and checked, its flash extents written into
// the firmware's NOR image (felucca_bounce.wasm: the FM-1's own sources, web/bounce/bounce_web.c), the project loaded
// and played as fast as the CPU goes, the master and the stems gathered, then written as WAV files (the master 16-bit
// PCM, as USB audio sends it; the stems 32-bit float: before the master's limiter they may pass full scale) and,
// for one download, a ZIP (stored, no compression).

const CRC_T = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();
export function crc32(b, c = 0xFFFFFFFF) {                // zlib's CRC-32 (storage.c st_crc32); running: pass ~crc
  for (let i = 0; i < b.length; i++) c = CRC_T[(c ^ b[i]) & 255] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}
function crc32Parts(parts) {
  let c = 0xFFFFFFFF;
  for (const p of parts) for (let i = 0; i < p.length; i++) c = CRC_T[(c ^ p[i]) & 255] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

// an error with its message (German, as web/bounce's page) and a code and its values, for a page in another language
// (web/library: its own words by the code)
const fail = (code, message, args = []) => Object.assign(new Error(message), { code, args });

export const SLOT_NAMES = ["A", "B", "C", "D", "CURRENT"];
const cstr = (b) => { const z = b.indexOf(0); return new TextDecoder("latin1").decode(z < 0 ? b : b.subarray(0, z)); };

// FM1X (usbdisk.c): a 64-byte header, then n entries of 16 bytes (tag, flash offset, length, CRC-32), then each entry's
// bytes padded to 4. Throws with a message for the page when the file is not one, or is damaged
export function parseFM1(bytes) {
  const u8 = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
  const dv = new DataView(u8.buffer, u8.byteOffset, u8.byteLength);
  if (u8.length < 64 || cstr(u8.subarray(0, 4)) !== "FM1X") throw fail("notfm1", "Keine .FM1-Datei (FM1X fehlt).");
  const ver = dv.getUint16(4, true), n = dv.getUint16(6, true), size = dv.getUint32(8, true), slot = u8[12];
  if (ver !== 1) throw fail("version", `FM1X Version ${ver}: diese Seite kennt nur Version 1.`, [ver]);
  if (size !== u8.length) throw fail("short", `Die Datei ist unvollständig (${u8.length} von ${size} Bytes).`, [u8.length, size]);
  if (n < 1 || n > 16 || slot > 4) throw fail("head", "Der Kopf der Datei ist beschädigt.");
  const table = u8.subarray(64, 64 + 16 * n);
  if (crc32(table) !== dv.getUint32(48, true)) throw fail("table", "Das Inhaltsverzeichnis der Datei ist beschädigt.");
  const entries = [];
  let at = 64 + 16 * n;
  for (let i = 0; i < n; i++) {
    const tag = cstr(table.subarray(16 * i, 16 * i + 4)), off = dv.getUint32(64 + 16 * i + 4, true);
    const len = dv.getUint32(64 + 16 * i + 8, true), crc = dv.getUint32(64 + 16 * i + 12, true);
    if (off + len > 0x100000 || at + len > u8.length) throw fail("outside", `${tag}: liegt außerhalb.`, [tag]);
    const data = u8.subarray(at, at + len);
    if (crc32(data) !== crc) throw fail("crc", `${tag}: Prüfsumme falsch, die Datei ist beschädigt.`, [tag]);
    entries.push({ tag, off, len, data });
    at += (len + 3) & ~3;
  }
  if (!entries.length || entries[0].tag !== "PROJ") throw fail("noproj", "Die Datei enthält kein Projekt.");
  return { slot, slotName: SLOT_NAMES[slot], fw: cstr(u8.subarray(16, 32)), name: cstr(u8.subarray(32, 48)), entries };
}

// a fresh instance (each bounce boots its own FM-1): from the bytes, or from a module compiled once (the page's)
export async function loadWasm(src) {
  if (src instanceof WebAssembly.Module) return (await WebAssembly.instantiate(src, {})).exports;
  const { instance } = await WebAssembly.instantiate(src, {});
  return instance.exports;
}

const BOOT_MS = 1500;                                     // the splash and the first frames (main.c: ~1.03 s)
const SLICE_MS = 64;                                      // a slice (bounce_run): 64 ms of the device, < BN_CAP frames
export const STEMS = ["MIX", "T1", "T2", "T3", "T4", "FX"];

// boot a fresh instance with fm1's flash, load its project: {ex, rate, bpm, loopMs, has: [pattern, scene song, chain]}
export function prepare(ex, fm1) {
  ex._initialize();
  ex.web_nor_erase();
  const nor = new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size());
  for (const e of fm1.entries) nor.set(e.data, e.off);
  ex.web_seed(1);
  ex.web_master(1023);
  ex.web_boot();
  ex.bounce_gather(0);
  for (let t = 0; t < BOOT_MS; t += SLICE_MS) ex.bounce_run(SLICE_MS);
  if (!ex.bounce_load(fm1.slot)) throw fail("load", "Das Projekt ließ sich nicht laden.");
  for (let t = 0; t < 200; t += SLICE_MS) ex.bounce_run(SLICE_MS);
  return { ex, rate: ex.web_sample_rate(), bpm: ex.bounce_bpm10() / 10, loopMs: ex.bounce_loop_ms(),
           has: [1, 2].map((m) => !!ex.bounce_has(m)) };
}

// play and gather: mode 0 the scene (maxMs long), 1 the scene song to its end, 2 the SONG chain (to its end, or maxMs);
// then tailMs of the tails (ended early once the master has been silent for 0.5 s). onProgress(ms done, ms planned)
// may return a promise (the page yields there). Returns {rate, frames, chunks: [stem][Float32Array interleaved]}.
// With sink(stem, Float32Array) each slice is handed over as it is made and not kept (web/library plays it meanwhile):
// chunks comes back empty
export async function render(prep, { mode = 0, maxMs = 8000, tailMs = 3000, stems = true, onProgress, sink } = {}) {
  const { ex, rate } = prep;
  const chunks = STEMS.map(() => []);
  let frames = 0, ms = 0, stopped = false, quiet = 0, tail = 0, last = null;
  if (!ex.bounce_play(mode)) throw mode === 1 ? fail("nosong", "Das Projekt hat keinen Szenen-Song.") :
                              mode === 2 ? fail("nochain", "Das Projekt hat keine Song-Kette.") : fail("noplay", "PLAY ging nicht.");
  ex.bounce_gather(1);
  const take = () => {
    const n = ex.bounce_take();
    for (let s = 0; s < (stems ? STEMS.length : 1); s++) {
      const c = new Float32Array(ex.memory.buffer, ex.bounce_buf(s), 2 * n).slice();
      if (!s) last = c;
      if (sink) sink(s, c); else chunks[s].push(c);
    }
    frames += n;
    return n;
  };
  for (;;) {
    ex.bounce_run(SLICE_MS);
    ms += SLICE_MS;
    const n = take();
    if (!stopped && (ms >= maxMs || (mode !== 0 && !ex.bounce_busy()))) {
      stopped = true;
      ex.bounce_stop();
    } else if (stopped) {
      let peak = 0;
      for (let i = 0; i < last.length; i++) peak = Math.max(peak, Math.abs(last[i]));
      quiet = peak < 1 / 32768 ? quiet + SLICE_MS : 0;
      tail += SLICE_MS;
      if (tail >= tailMs || quiet >= 500) break;
    }
    if (onProgress) await onProgress(ms, stopped ? ms : Math.max(maxMs, ms) + tailMs);
    if (!n && ms > maxMs + 600000) throw fail("silent", "Das Gerät gibt keinen Ton mehr aus.");
  }
  ex.bounce_gather(0);
  return { rate, frames, chunks: stems ? chunks : [chunks[0]] };
}

export function wavHeader(frames, rate, float) {
  const bps = float ? 4 : 2, data = frames * 2 * bps, h = new DataView(new ArrayBuffer(44));
  const s = (o, t) => { for (let i = 0; i < 4; i++) h.setUint8(o + i, t.charCodeAt(i)); };
  s(0, "RIFF"); h.setUint32(4, 36 + data, true); s(8, "WAVE"); s(12, "fmt ");
  h.setUint32(16, 16, true); h.setUint16(20, float ? 3 : 1, true); h.setUint16(22, 2, true);
  h.setUint32(24, rate, true); h.setUint32(28, rate * 2 * bps, true); h.setUint16(32, 2 * bps, true);
  h.setUint16(34, 8 * bps, true); s(36, "data"); h.setUint32(40, data, true);
  return new Uint8Array(h.buffer);
}
// a stereo WAV as a list of byte arrays (no copy into one buffer: a long song's stems stay chunks)
export function wavParts(chunks, frames, rate, float) {
  const parts = [wavHeader(frames, rate, float)];
  for (const c of chunks) {
    if (float) { parts.push(new Uint8Array(c.buffer, c.byteOffset, c.byteLength)); continue; }
    const p = new Int16Array(c.length);
    for (let i = 0; i < c.length; i++) p[i] = Math.max(-32768, Math.min(32767, Math.round(c[i] * 32768)));
    parts.push(new Uint8Array(p.buffer));
  }
  return parts;
}
export function bounceFiles(result, base) {
  return result.chunks.map((c, s) => ({ name: `${base}_${STEMS[s]}.wav`,
                                        parts: wavParts(c, result.frames, result.rate, s > 0) }));
}

// a ZIP of the files, stored (method 0), as a list of byte arrays
export function zipParts(files) {
  const out = [], central = [], enc = new TextEncoder();
  let off = 0;
  for (const f of files) {
    const name = enc.encode(f.name), size = f.parts.reduce((a, p) => a + p.length, 0), crc = crc32Parts(f.parts);
    const h = new DataView(new ArrayBuffer(30));
    h.setUint32(0, 0x04034B50, true); h.setUint16(4, 20, true); h.setUint16(8, 0, true);
    h.setUint16(10, 0x6000, true); h.setUint16(12, 0x5C21, true);                // (12:00, 2026-01-01: as the disk)
    h.setUint32(14, crc, true); h.setUint32(18, size, true); h.setUint32(22, size, true);
    h.setUint16(26, name.length, true);
    out.push(new Uint8Array(h.buffer), name, ...f.parts);
    const c = new DataView(new ArrayBuffer(46));
    c.setUint32(0, 0x02014B50, true); c.setUint16(4, 20, true); c.setUint16(6, 20, true);
    c.setUint16(12, 0x6000, true); c.setUint16(14, 0x5C21, true);
    c.setUint32(16, crc, true); c.setUint32(20, size, true); c.setUint32(24, size, true);
    c.setUint16(28, name.length, true); c.setUint32(42, off, true);
    central.push(new Uint8Array(c.buffer), name);
    off += 30 + name.length + size;
  }
  const csize = central.reduce((a, p) => a + p.length, 0), e = new DataView(new ArrayBuffer(22));
  e.setUint32(0, 0x06054B50, true); e.setUint16(8, files.length, true); e.setUint16(10, files.length, true);
  e.setUint32(12, csize, true); e.setUint32(16, off, true);
  return [...out, ...central, new Uint8Array(e.buffer)];
}
