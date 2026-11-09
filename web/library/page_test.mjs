// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page itself (build/library, web/library/build.sh) in a browser (Playwright's Chromium; not part of
// tests/run_tests.sh, which has no browser: run it by hand after a change to the page):
//   LANG    English first; DE: every word German, kept over a reload; EN: back
//   IN      .FM1 files picked: listed with what the firmware says of them; the same again: known; a damaged one refused
//   PLAY    a tap on a card: the worker renders, the sound runs meanwhile (the position moves, real samples leave the
//           page), pause holds, the bar seeks, the end stops; another mode's length
//   OUT     the WAV byte for byte web/bounce's; the .FM1 as it came; the library's ZIP, and that ZIP into an empty
//           library; a reload keeps the list (IndexedDB), a title and a note; delete
// NODE_PATH=$(npm root -g) node web/library/page_test.mjs [build/library] [build/vdisk/files] [screenshot directory]
import { createServer } from "node:http";
import { readFileSync, existsSync, mkdirSync } from "node:fs";
import { extname, join as pjoin } from "node:path";
import { createRequire } from "node:module";
import { parseFM1, loadWasm, prepare, render, bounceFiles } from "../bounce/bounce.js";
import { unzip } from "./lib.js";

const { chromium } = createRequire(import.meta.url)("playwright");   // (found by NODE_PATH too: a global install)
const [root = "build/library", dir = "build/vdisk/files", shots = ""] = process.argv.slice(2);
let bad = 0;
const check = (what, ok) => { console.log(`page: ${what.padEnd(100)} ${ok ? "ok" : "FAIL"}`); if (!ok) bad++; };
const same = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);
const join = (parts) => { const n = parts.reduce((a, p) => a + p.length, 0), o = new Uint8Array(n); let at = 0;
                          for (const p of parts) { o.set(p, at); at += p.length; } return o; };
const TYPES = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm" };
const server = createServer((q, s) => {
  const f = pjoin(root, q.url === "/" ? "index.html" : q.url.split("?")[0]);
  if (!existsSync(f)) { s.writeHead(404); s.end(); return; }
  s.writeHead(200, { "Content-Type": TYPES[extname(f)] || "application/octet-stream" });
  s.end(readFileSync(f));
});
await new Promise((ok) => server.listen(0, "127.0.0.1", ok));
const url = `http://127.0.0.1:${server.address().port}/`;
const browser = await chromium.launch({ args: ["--autoplay-policy=no-user-gesture-required"] });
const ctx = await browser.newContext({ viewport: { width: 390, height: 844 }, deviceScaleFactor: 2, acceptDownloads: true });
const page = await ctx.newPage();
const errors = [];
page.on("pageerror", (e) => errors.push(e.message));
page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });
// what leaves the page: the peak of every buffer handed to the sound output
await page.addInitScript(() => {
  window.__out = { n: 0, peak: 0 };
  const start = AudioBufferSourceNode.prototype.start;
  AudioBufferSourceNode.prototype.start = function (...a) {
    const d = this.buffer.getChannelData(0);
    let p = 0;
    for (let i = 0; i < d.length; i++) p = Math.max(p, Math.abs(d[i]));
    window.__out.n++;
    window.__out.peak = Math.max(window.__out.peak, p);
    return start.apply(this, a);
  };
});
await page.goto(url);
const shot = async (name) => { if (shots) { mkdirSync(shots, { recursive: true }); await page.screenshot({ path: pjoin(shots, name) }); } };
const state = () => page.evaluate(() => ({ cards: [...document.querySelectorAll("#list article")].map((a) => ({
    name: a.querySelector(".name").textContent, meta: a.querySelector(".meta").textContent, on: a.classList.contains("on") })),
  status: document.getElementById("status").textContent, err: document.getElementById("status").classList.contains("err"),
  player: !document.getElementById("player").hidden, time: document.getElementById("ptime").textContent,
  pp: document.getElementById("pp").getAttribute("aria-label") }));
const secs = (t) => { const m = /^(\d+):(\d+) \/ (\d+):(\d+)/.exec(t); return m ? [60 * +m[1] + +m[2], 60 * +m[3] + +m[4]] : [-1, -1]; };
const dl = async (click) => { const [d] = await Promise.all([page.waitForEvent("download"), click()]);
                              return { name: d.suggestedFilename(), data: new Uint8Array(readFileSync(await d.path())) }; };

// ---- IN
let s = await state();
check("an empty library: said so, no player, no backup section", s.cards.length === 0 && !s.player &&
      await page.locator(".empty").count() === 1 && await page.locator("#backup").isHidden());
await page.setInputFiles("#file", [`${dir}/PROJ_A.FM1`, `${dir}/PROJ_C.FM1`]);
await page.waitForFunction(() => document.querySelectorAll("#list article").length === 2);
s = await state();
check("IN: two files picked: two cards, each its name, slot, tempo and length from the firmware",
      s.cards.map((c) => c.name).sort().join() === "FIRST,THIRD" && /Slot [AC] · 120 BPM · 1 bar · /.test(s.cards[0].meta) &&
      /2 projects added/.test(s.status) && !s.err);
await page.setInputFiles("#file", [`${dir}/PROJ_A.FM1`]);
await page.waitForFunction(() => /already in the library/.test(document.getElementById("status").textContent));
const hurt = readFileSync(`${dir}/CURRENT.FM1`); hurt[hurt.length - 10] ^= 1;
await page.setInputFiles("#file", [{ name: "HURT.FM1", mimeType: "application/octet-stream", buffer: hurt }]);
await page.waitForFunction(() => /checksum/.test(document.getElementById("status").textContent));
s = await state();
check("  the same file again: known; a damaged one: refused with its reason, in the warning colour; still two cards",
      s.cards.length === 2 && s.err && /HURT\.FM1/.test(s.status));
await shot("1-list.png");

// ---- the language
const words = () => page.evaluate(() => ({ lang: document.documentElement.lang, title: document.title,
  h1: [...document.querySelectorAll("h1 span")].find((e) => getComputedStyle(e).display !== "none").textContent,
  pick: [...document.querySelectorAll("#pick span")].find((e) => getComputedStyle(e).display !== "none").textContent,
  meta: document.querySelector("#list .meta").textContent, status: document.getElementById("status").textContent,
  more: document.querySelector("#list summary").textContent }));
let w = await words();
check("LANGUAGE: English first: the title, the heading, the button, a card, the refusal",
      w.lang === "en" && w.title === "JAN3000 Library" && w.h1 === "Library" && w.pick === "Choose .FM1 files" &&
      /Slot/.test(w.meta) && /HURT\.FM1: SMP1: wrong checksum/.test(w.status) && w.more === "More");
await page.locator('.lang-sw button[data-lang="de"]').click();
w = await words();
check("  DE: all of it German, the refusal already on the page too",
      w.lang === "de" && w.title === "JAN3000 Bibliothek" && w.h1 === "Bibliothek" && w.pick === ".FM1-Dateien wählen" &&
      /Platz [AC] · 120 BPM · 1 Takt/.test(w.meta) && /HURT\.FM1: SMP1: Prüfsumme falsch/.test(w.status) && w.more === "Mehr");
await page.reload();
await page.waitForFunction(() => document.querySelectorAll("#list article").length === 2);
w = await words();
check("  a reload: still German (the user guide's choice, kept); EN: English again",
      w.lang === "de" && w.h1 === "Bibliothek" && /Platz/.test(w.meta));
await shot("1-list-de.png");
await page.locator('.lang-sw button[data-lang="en"]').click();
check("  (and back)", (await words()).h1 === "Library");

// ---- PLAY
const first = page.locator("#list article", { hasText: "FIRST" });
await first.locator(".play").click();
await page.waitForFunction(() => /^0:0[1-9]/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
s = await state();
const out1 = await page.evaluate(() => window.__out);
check("PLAY: a tap: the player is up, the card marked, the position moves, samples leave the page",
      s.player && s.cards.find((c) => c.name === "FIRST").on && s.pp === "Pause" && out1.n > 0 && out1.peak > 0.01);
check("  the whole render is there long before the sound gets to it (4 bars and what rings on: 0:08 to 0:11)",
      !/rechnet/.test(s.time) && secs(s.time)[1] >= 8 && secs(s.time)[1] <= 11);
await shot("2-playing.png");
await page.locator("#pp").click();
await page.waitForFunction(() => document.getElementById("pp").getAttribute("aria-label") === "Play");
const t1 = (await state()).time;
await page.waitForTimeout(700);
s = await state();
check("  pause: the position holds, the button offers play", s.time === t1 && s.pp === "Play");
const box = await page.locator("#wave").boundingBox();
await page.mouse.click(box.x + box.width * 0.75, box.y + box.height / 2);
await page.waitForTimeout(100);                           // (the bar's text follows with the next frame)
s = await state();
const [at, len] = secs(s.time);
check(`  a tap at three quarters of the bar: the position there (${s.time})`, Math.abs(at - 0.75 * len) <= 1.5);
await page.locator("#pp").click();
await page.waitForFunction(() => document.getElementById("pp").getAttribute("aria-label") === "Pause");
await page.waitForFunction(() => document.getElementById("pp").getAttribute("aria-label") === "Play", null, { timeout: 15000 });
s = await state();
check("  played on from there to the end: stopped at the end", secs(s.time)[0] >= secs(s.time)[1] - 1);
await page.locator("#pbars").fill("1");
await page.locator("#pbars").dispatchEvent("change");
await page.waitForFunction(() => /\/ 0:0[3-5]$/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
check("  1 bar chosen: rendered again, 2 s and the tail", true);

// ---- OUT
await first.locator("summary").click();
const a = new Uint8Array(readFileSync(`${dir}/PROJ_A.FM1`));
const ref = await render(prepare(await loadWasm(readFileSync(pjoin(root, "felucca_bounce.wasm"))), parseFM1(a)),
                         { mode: 0, maxMs: 2000, tailMs: 3000, stems: false });
const wav = await dl(() => first.getByRole("button", { name: "WAV (mix)" }).click());
check("OUT: WAV (Mix): FIRST_MIX.wav, byte for byte web/bounce's master of 1 bar",
      wav.name === "FIRST_MIX.wav" && same(wav.data, join(bounceFiles(ref, "X")[0].parts)));
const third = page.locator("#list article", { hasText: "THIRD" });
await third.locator("summary").click();
const wav3 = await dl(() => third.getByRole("button", { name: "WAV (mix)" }).click());
check("  the WAV of a project the player does not hold: rendered for it (4 bars: longer)", wav3.name === "THIRD_MIX.wav" &&
      wav3.data.length > wav.data.length);
const fm1 = await dl(() => first.getByRole("button", { name: "Save .FM1 file" }).click());
check("  the .FM1 file: FIRST.FM1, the bytes as they came", fm1.name === "FIRST.FM1" && same(fm1.data, a));
const stems = await dl(() => first.getByRole("button", { name: "Stems (ZIP)" }).click());
check("  the stems: FIRST.zip with the master and T1..T4, FX", (await unzip(stems.data)).length === 6 && stems.name === "FIRST.zip");
await first.locator("input[type=text]").fill("Erster Versuch");
await first.locator("input[type=text]").dispatchEvent("change");
const renamed = page.locator("#list article", { hasText: "Erster Versuch" });
await renamed.locator("textarea").fill("mit dem neuen Bass");
await renamed.locator("textarea").dispatchEvent("change");
await page.waitForFunction(() => !!document.querySelector("#list .note"));
await shot("3-details.png");
const zip = await dl(() => page.locator("#zipall").click());
const inZip = await unzip(zip.data);
check("  the library's ZIP: both projects, the renamed one under its title",
      /^JAN3000-Library-\d{4}-\d\d-\d\d\.zip$/.test(zip.name) && inZip.map((f) => f.name).sort().join() === "ERSTER_VERSUCH.FM1,THIRD.FM1");
await page.reload();
await page.waitForFunction(() => document.querySelectorAll("#list article").length === 2);
s = await state();
check("  a reload: the list, the title and the note are still there; the player is not up",
      s.cards.map((c) => c.name).sort().join() === "Erster Versuch,THIRD" && !s.player &&
      await page.locator("#list .note").textContent() === "mit dem neuen Bass");
page.on("dialog", (d) => d.accept());
for (const name of ["Erster Versuch", "THIRD"]) {
  const c = page.locator("#list article", { hasText: name });
  await c.locator("summary").click();
  await c.getByRole("button", { name: "Delete" }).click();
  await page.waitForFunction((n) => ![...document.querySelectorAll("#list .name")].some((e) => e.textContent === n), name);
}
check("  both deleted (asked first): the library is empty again", (await state()).cards.length === 0);
await page.setInputFiles("#file", [{ name: zip.name, mimeType: "application/zip", buffer: Buffer.from(zip.data) }]);
await page.waitForFunction(() => document.querySelectorAll("#list article").length === 2);
check("  the ZIP back in: both projects again", (await state()).cards.map((c) => c.name).sort().join() === "FIRST,THIRD");
check(`no error on the page${errors.length ? ": " + errors.join(" | ") : ""}`, errors.length === 0);

await browser.close();
server.close();
console.log(bad ? `page: ${bad} FAILED` : "page: all ok");
process.exit(bad ? 1 : 0);
