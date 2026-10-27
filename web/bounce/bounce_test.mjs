// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The offline bounce (web/bounce/bounce.js on build/bounce/felucca_bounce.wasm) against the .FM1 files USB DISK makes
// (tests/vdisk_test.c leaves them in build/vdisk/files):
//   FILE    PROJ_A.FM1 and CURRENT.FM1 read and checked; a byte changed, one cut short, not one: refused, saying why
//   PLAY    PROJ_A (its notes on T1) bounced for 2 bars: the length at its tempo plus the tail; the master sounds, T1
//           sounds, the empty tracks are silent; the master within full scale, the stems add up to it where the
//           limiter leaves it alone; twice the same bytes (each bounce a fresh boot); CURRENT (slot 4: the autosave)
//           its own music; faster than real time (the cost printed)
//   FILES   the WAVs (16-bit master, 32-bit float stems) and the ZIP: headers, sizes, CRCs as a reader checks them
// node web/bounce/bounce_test.mjs build/bounce/felucca_bounce.wasm build/vdisk/files
import { readFileSync } from "node:fs";
import { crc32, parseFM1, loadWasm, prepare, render, bounceFiles, zipParts, STEMS } from "./bounce.js";

const [wasmPath = "build/bounce/felucca_bounce.wasm", dir = "build/vdisk/files"] = process.argv.slice(2);
const wasm = readFileSync(wasmPath);
let bad = 0;
const check = (what, ok) => { console.log(`bounce: ${what.padEnd(100)} ${ok ? "ok" : "FAIL"}`); if (!ok) bad++; };
const refused = (bytes, re) => { try { parseFM1(bytes); return false; } catch (e) { return re.test(e.message); } };
const join = (parts) => { const n = parts.reduce((a, p) => a + p.length, 0), o = new Uint8Array(n); let at = 0;
                          for (const p of parts) { o.set(p, at); at += p.length; } return o; };
const peak = (chunks, ch) => { let p = 0; for (const c of chunks) for (let i = ch; i < c.length; i += 2) p = Math.max(p, Math.abs(c[i])); return p; };

const a = new Uint8Array(readFileSync(`${dir}/PROJ_A.FM1`)), cur = new Uint8Array(readFileSync(`${dir}/CURRENT.FM1`));
const fa = parseFM1(a), fc = parseFM1(cur);
check("FILE: PROJ_A.FM1 slot A, FIRST, its PROJ first; CURRENT.FM1 slot 4 (CURRENT), LIVE",
      fa.slot === 0 && fa.name === "FIRST" && fa.entries[0].tag === "PROJ" && fc.slot === 4 && fc.slotName === "CURRENT" &&
      fc.name === "LIVE");
const hurt = a.slice(); hurt[hurt.length - 10] ^= 1;
check("  a byte changed in an entry: refused (its checksum)", refused(hurt, /Prüfsumme/));
check("  cut short: refused (incomplete); not FM1X: refused", refused(a.subarray(0, a.length - 4), /unvollständig/) &&
      refused(new Uint8Array(100), /Keine/));

async function bounce(fm1, opts) {
  const prep = prepare(await loadWasm(wasm), fm1);
  const t0 = performance.now(), r = await render(prep, opts);
  return { prep, r, cost: performance.now() - t0 };
}
const bar = (bpm) => 4 * 60000 / bpm;
const { prep, r, cost } = await bounce(fa, { mode: 0, maxMs: 2 * bar(120), tailMs: 1500 });
const want = 2 * bar(prep.bpm);
check(`PLAY: 2 bars at ${prep.bpm} BPM (${want} ms) and at most 1.5 s of tail: ${(r.frames / r.rate * 1000) | 0} ms`,
      r.frames / r.rate * 1000 >= want && r.frames / r.rate * 1000 <= want + 1500 + 200 && r.chunks.length === STEMS.length);
check("  the master sounds, T1 sounds, T2 T3 T4 silent (no steps), within full scale",
      peak(r.chunks[0], 0) > 0.01 && peak(r.chunks[1], 0) > 0.01 && peak(r.chunks[2], 0) === 0 &&
      peak(r.chunks[3], 0) === 0 && peak(r.chunks[4], 0) === 0 && peak(r.chunks[0], 0) <= 1);
{
  let err = 0, n = 0;
  for (let k = 0; k < r.chunks[0].length; k++)
    for (let i = 0; i < r.chunks[0][k].length; i++) {
      const m = r.chunks[0][k][i];
      if (Math.abs(m) > 0.25) continue;                  // (where the limiter may work: not compared)
      let s = 0;
      for (let t = 1; t < STEMS.length; t++) s += r.chunks[t][k][i];
      err = Math.max(err, Math.abs(s - m));
      n++;
    }
  check(`  T1..T4 + FX add up to the master below the limiter (${n} samples, worst ${(err * 32768).toFixed(1)} LSB)`,
        n > 1000 && err * 32768 <= 8);   // (each stem rounds its own DC blocker)
}
const again = await bounce(fa, { mode: 0, maxMs: 2 * bar(120), tailMs: 1500 });
const same = again.r.frames === r.frames && r.chunks.every((c, s) => c.every((x, k) => x.every((v, i) => v === again.r.chunks[s][k][i])));
check("  bounced again: the same samples, every stem (a fresh boot each time)", same);
const c = await bounce(fc, { mode: 0, maxMs: bar(120), tailMs: 500 });
let differs = false;
for (let k = 0; k < Math.min(c.r.chunks[1].length, r.chunks[1].length) && !differs; k++)
  differs = c.r.chunks[1][k].some((v, i) => v !== r.chunks[1][k][i]);
check("  CURRENT.FM1 (the autosave) loads and plays its own music (T1 not PROJ_A's)", peak(c.r.chunks[1], 0) > 0.01 && differs);
const secs = r.frames / r.rate;
console.log(`bounce: ${secs.toFixed(2)} s of music in ${(cost / 1000).toFixed(2)} s: ${(secs * 1000 / cost).toFixed(1)}x real time`);
check("  faster than real time", cost < secs * 1000);

const files = bounceFiles(r, "FIRST");
const mix = join(files[0].parts), t1 = join(files[1].parts), dv = new DataView(mix.buffer), dv1 = new DataView(t1.buffer);
check("FILES: FIRST_MIX.wav 16-bit PCM stereo, FIRST_T1.wav 32-bit float; sizes as the frames",
      files.map((f) => f.name).join(" ") === "FIRST_MIX.wav FIRST_T1.wav FIRST_T2.wav FIRST_T3.wav FIRST_T4.wav FIRST_FX.wav" &&
      dv.getUint16(20, true) === 1 && dv.getUint16(34, true) === 16 && dv.getUint32(24, true) === r.rate &&
      dv.getUint32(40, true) === r.frames * 4 && mix.length === 44 + r.frames * 4 &&
      dv1.getUint16(20, true) === 3 && dv1.getUint16(34, true) === 32 && t1.length === 44 + r.frames * 8);
const zip = join(zipParts(files)), zv = new DataView(zip.buffer), eocd = zip.length - 22;
let ok = zv.getUint32(eocd, true) === 0x06054B50 && zv.getUint16(eocd + 10, true) === 6;
for (let at = zv.getUint32(eocd + 16, true), i = 0; ok && i < 6; i++) {
  const nl = zv.getUint16(at + 28, true), lo = zv.getUint32(at + 42, true), size = zv.getUint32(at + 24, true);
  const ln = zv.getUint16(lo + 26, true), body = zip.subarray(lo + 30 + ln, lo + 30 + ln + size);
  ok = zv.getUint32(lo, true) === 0x04034B50 && crc32(body) === zv.getUint32(at + 16, true) &&
       new TextDecoder().decode(zip.subarray(at + 46, at + 46 + nl)) === files[i].name && body.length === join(files[i].parts).length;
  at += 46 + nl;
}
check("  the ZIP: six files stored, each its name, size and CRC-32 as its central directory says", ok);
console.log(bad ? `bounce: ${bad} FAILED` : "bounce: all ok");
process.exit(bad ? 1 : 0);
