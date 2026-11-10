// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page itself (build/library, web/library/build.sh) in a browser (Playwright's Chromium; not part of
// tests/run_tests.sh, which has no browser: run it by hand after a change to the page):
//   LANG    English first; DE: every word German, kept over a reload; EN: back
//   IN      .FM1 files picked: listed with what the firmware says of them; the same again: known; a damaged one refused
//   PLAY    a tap on a card: the worker renders, the sound runs meanwhile (the position moves, real samples leave the
//           page), pause holds, the bar seeks, the end stops; another mode's length
//   WHAT    what a project holds is what plays: its song, else its scenes in a row; a SONG chain with the other
//           slots' files of the same pick, a note and a refusal without them
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
const [root = "build/library", dir = "build/vdisk/files", shots = "", songs = `${dir}/../songs`] = process.argv.slice(2);
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
      s.cards.map((c) => c.name).sort().join() === "FIRST,THIRD" && /Slot [AC] · 120 BPM · 1 scene · /.test(s.cards[0].meta) &&
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
      /Platz [AC] · 120 BPM · 1 Szene/.test(w.meta) && /HURT\.FM1: SMP1: Prüfsumme falsch/.test(w.status) && w.more === "Mehr");
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
check(`  what plays unasked: the project's scenes in a row (it has one: 2 s and what rings on, ${s.time})`,
      secs(s.time)[1] >= 2 && secs(s.time)[1] <= 4 && await page.locator("#pmodesel").inputValue() === "3" &&
      await page.locator("#pbarsl").isHidden());
await page.locator("#pmodesel").selectOption("0");
await page.waitForFunction(() => /\/ 0:(0[89]|1[01])$/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
await page.waitForFunction(() => /^0:0[1-9]/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
s = await state();
check("  one scene, looped, chosen: 4 bars and what rings on, the bars to set, the card says 4 bars; all there before the sound",
      !/rendering/.test(s.time) && await page.locator("#pbarsl").isVisible() && await page.locator("#pbars").inputValue() === "4" &&
      /· 4 bars ·/.test(s.cards.find((c) => c.name === "FIRST").meta));
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
check("  the WAV of a project the player does not hold: rendered for it (its scene once)", wav3.name === "THIRD_MIX.wav" &&
      wav3.data.length > 44 + 4 * 2 * 44100);
// ---- the faders (THIRD's card is open)
const fader = (card, t) => card.locator(".fader").nth(t);
const setFader = (card, t, v) => fader(card, t).locator("input").evaluate((e, val) => {
  e.value = String(val); e.dispatchEvent(new Event("input", { bubbles: true })); e.dispatchEvent(new Event("change", { bubbles: true })); }, v);
check("FADERS: four in the open card, T1..T4, each at the project's level (−4.0 dB); no way back offered yet",
      await third.locator(".fader").count() === 4 && await fader(third, 0).locator("b").textContent() === "T1" &&
      await fader(third, 3).locator("output").textContent() === "−4.0 dB" && await third.locator("button.small").isHidden());
await setFader(third, 0, 0);
await page.waitForFunction(() => !!document.querySelector("#list .fader.own"));
const mute = await dl(() => third.getByRole("button", { name: "WAV (mix)" }).click());
check("  T1 pulled down to nothing: −∞ dB, marked as changed; the WAV of it: silence",
      await fader(third, 0).locator("output").textContent() === "−∞ dB" && mute.data.subarray(44).every((x) => x === 0) &&
      !wav3.data.subarray(44).every((x) => x === 0));
await third.locator(".play").click();
await page.waitForFunction(() => /^0:0\d \/ 0:0[1-9]$/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
await page.evaluate(() => { window.__out = { n: 0, peak: 0 }; });   // (all that left the page so far was silence)
await setFader(third, 0, 90);
await page.waitForTimeout(1500);
const out3 = await page.evaluate(() => window.__out);
check("  played (silent), the fader moved up while it plays: rendered again, the sound is there",
      out3.n > 0 && out3.peak > 0.005 && (await state()).cards.find((c) => c.name === "THIRD").on);
await page.locator("#pp").click();
await page.reload();
await page.waitForFunction(() => document.querySelectorAll("#list article").length === 2);
await page.locator("#list article", { hasText: "THIRD" }).locator("summary").click();
await page.locator("#list article", { hasText: "FIRST" }).locator("summary").click();
check("  a reload: THIRD's T1 still at 90, the way back offered; FIRST's faders untouched",
      await fader(third, 0).locator("input").inputValue() === "90" && await third.locator("button.small").isVisible() &&
      await fader(first, 0).locator("input").inputValue() === "104");
await third.locator("button.small").click();
await page.waitForFunction(() => !document.querySelector("#list .fader.own"));
check("  back to the project's levels: 104 again, nothing marked", await fader(third, 0).locator("input").inputValue() === "104");
await shot("3b-faders.png");
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

// ---- WHAT PLAYS (another visitor: an empty library; tests/vdisk_test.c's second disk)
const ctx2 = await browser.newContext({ viewport: { width: 390, height: 844 }, deviceScaleFactor: 2 });
const p2 = await ctx2.newPage();
p2.on("pageerror", (e) => errors.push(e.message));
await p2.goto(url);
const card2 = (name) => p2.locator("#list article", { hasText: name });
// the length of what the player holds once it is all rendered (the bar's text follows a frame after a tap: wait a moment)
const total = async () => {
  await p2.waitForTimeout(300);
  await p2.waitForFunction(() => /^\d+:\d\d \/ \d+:\d\d$/.test(document.getElementById("ptime").textContent) &&
                                 !/\/ 0:00$/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
  return secs(await p2.locator("#ptime").textContent())[1];
};
await p2.setInputFiles("#file", [`${songs}/CURRENT.FM1`]);
await p2.waitForFunction(() => document.querySelectorAll("#list article").length === 1);
await card2("CHAIN").locator("summary").click();
check("WHAT PLAYS: a project with a SONG chain, taken in alone: its 3 scenes play; a note names the projects missing (A, C)",
      /CURRENT · 120 BPM · 3 scenes ·/.test(await card2("CHAIN").locator(".meta").textContent()) &&
      /project A, C.*PROJ_A\.FM1, PROJ_C\.FM1/.test(await card2("CHAIN").locator(".hint.chain").textContent()));
await card2("CHAIN").locator(".play").click();
let len2 = await total();
await p2.locator("#pmodesel").selectOption("2");
await p2.waitForFunction(() => /song chain plays project A, C/.test(document.getElementById("ptime").textContent), null, { timeout: 20000 });
check(`  played: the scenes in a row (6 s and what rings on: ${len2} s); the chain chosen all the same: refused, saying why`,
      len2 >= 6 && len2 <= 9);
await p2.setInputFiles("#file", ["CURRENT", "PROJ_A", "PROJ_B", "PROJ_C", "PROJ_D"].map((n) => `${songs}/${n}.FM1`));
await p2.waitForFunction(() => document.querySelectorAll("#list article").length === 5);
const metas = Object.fromEntries(await p2.evaluate(() => [...document.querySelectorAll("#list article")].map((a) =>
  [a.querySelector(".name").textContent, a.querySelector(".meta").textContent])));
check("  the whole drive in one pick: CHAIN's card now says Song chain, no note; SSONG Scene song; SCENES 3 scenes",
      /· Song chain ·/.test(metas.CHAIN) && await card2("CHAIN").locator(".hint.chain").count() === 0 &&
      /· Scene song ·/.test(metas.SSONG) && /· 3 scenes ·/.test(metas.SCENES) && /already in the library/.test(await p2.locator("#status").textContent()));
await card2("CHAIN").locator(".play").click();
len2 = await total();
check(`  CHAIN played: the chain, A once and C twice (6 s and what rings on: ${len2} s)`, len2 >= 6 && len2 <= 9 &&
      await p2.locator("#pmodesel").inputValue() === "2");
await card2("SSONG").locator(".play").click();
len2 = await total();
check(`  SSONG played: its scene song, four passes (8 s and what rings on: ${len2} s)`, len2 >= 8 && len2 <= 11 &&
      await p2.locator("#pmodesel").inputValue() === "1");
await card2("SCENES").locator(".play").click();
len2 = await total();
check(`  SCENES played: its three scenes in a row (6 s and what rings on: ${len2} s); no song to choose`, len2 >= 6 && len2 <= 9 &&
      await p2.locator("#pmodesel").inputValue() === "3" && await p2.locator('#pmodesel option[value="1"]').isDisabled() &&
      await p2.locator('#pmodesel option[value="2"]').isDisabled());
if (shots) await p2.screenshot({ path: pjoin(shots, "4-songs.png") });
check(`no error on the page${errors.length ? ": " + errors.join(" | ") : ""}`, errors.length === 0);

await browser.close();
server.close();
console.log(bad ? `page: ${bad} FAILED` : "page: all ok");
process.exit(bad ? 1 : 0);
