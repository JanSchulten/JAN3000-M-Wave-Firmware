// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// Drive the browser emulator (web/emu or web/lab/local.html, served from localhost) with Playwright and save
// screenshots. Edit the STEPS list for the workflow to check.
//   NODE_PATH=$(npm root -g) node emu_drive.js http://127.0.0.1:8765/index.html /tmp/shots
// Steps: ["tap", code] a button or key by keyboard code; ["hold", code, ms]; ["knob", "KNOB 2", n] n detents;
// ["play", [codes], ms] keys one after another; ["wait", ms]; ["shot", name].
const { chromium } = require('playwright');
const fs = require('fs');
const [url, out = '/tmp/shots'] = process.argv.slice(2);
const STEPS = [
  ['tap', 'Digit6'],                         // GLO: TAPE
  ['shot', 'tape'],
  ['knob', 'KNOB 2', 2],                     // LOOP 2 BARS
  ['tap', 'Equal'],                          // REC: arm the tape, PLAY starts
  ['play', ['KeyZ', 'KeyC', 'KeyB', 'KeyM', 'KeyB', 'KeyC', 'KeyX', 'KeyZ'], 220],
  ['shot', 'recording'],
  ['tap', 'Equal'],                          // disarm, the loop plays on
  ['wait', 1500],
  ['shot', 'loop'],
];
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const CHROME = ['/opt/pw-browsers/chromium-1194/chrome-linux/chrome'].find((p) => fs.existsSync(p));

(async () => {
  fs.mkdirSync(out, { recursive: true });
  const browser = await chromium.launch({ executablePath: CHROME, args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1360, height: 900 } });
  const logs = [];
  page.on('console', (m) => { if (m.type() === 'error') logs.push(m.text()); });
  page.on('pageerror', (e) => logs.push('pageerror: ' + e.message));
  await page.goto(url);
  if (await page.$('#go')) {                 // web/emu: "Switch on", then the splash
    await page.click('#go');
    await page.waitForSelector('#start', { state: 'hidden', timeout: 20000 });
  } else {                                   // web/lab: boots by itself
    await page.waitForFunction(() => document.getElementById('core')?.textContent !== 'lädt …', null, { timeout: 30000 });
  }
  await sleep(3500);
  const tap = async (code, ms = 140) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(250); };
  for (const [op, a, b] of STEPS) {
    if (op === 'tap') await tap(a);
    else if (op === 'hold') await tap(a, b);
    else if (op === 'wait') await sleep(a);
    else if (op === 'play') for (const k of a) { await page.keyboard.down(k); await sleep(b || 200); await page.keyboard.up(k); await sleep(70); }
    else if (op === 'knob') {
      await page.focus(`.hit.knob[aria-label="${a}"], .knob[aria-label="${a}"]`);
      for (let i = 0; i < Math.abs(b); i++) { await page.keyboard.press(b > 0 ? 'ArrowUp' : 'ArrowDown'); await sleep(70); }
      await page.evaluate(() => document.activeElement && document.activeElement.blur());   // or keys are ignored
      await sleep(150);
    } else if (op === 'shot') {
      await page.locator('#screen').screenshot({ path: `${out}/${a}-screen.png` });
      await page.screenshot({ path: `${out}/${a}-page.png` });
    }
  }
  console.log(logs.length ? 'console errors:\n' + logs.join('\n') : 'no console errors');
  await browser.close();
})().catch((e) => { console.error(e); process.exit(1); });
