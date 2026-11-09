// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M-Vave-OP1 contributors
// The library page's worker: one job each (the page starts a worker for a job and ends it when the job is done, or to
// stop it: a render has no other way out), so the page stays smooth while the firmware runs. The job, in:
//   {wasm: WebAssembly.Module, job: "inspect" | "play" | "stems", bytes, opts, base}
// out: {type: "info", info} | {type: "audio", pcm, ms, total} .. {type: "done", rate, frames} |
//      {type: "progress", ms, total} .. {type: "zip", blob} | {type: "error", message}
import { inspect, play, stemsZip } from "./lib.js";

self.onmessage = async ({ data: m }) => {
  try {
    if (m.job === "inspect") {
      self.postMessage({ type: "info", info: await inspect(m.wasm, m.bytes) });
    } else if (m.job === "play") {
      const r = await play(m.wasm, m.bytes, m.opts, (pcm, ms, total) =>
        self.postMessage({ type: "audio", pcm, ms, total }, [pcm.buffer]));
      self.postMessage({ type: "done", rate: r.rate, frames: r.frames });
    } else if (m.job === "stems") {
      let last = 0;
      const parts = await stemsZip(m.wasm, m.bytes, m.opts, m.base, (ms, total) => {
        if (ms - last < 2000) return;
        last = ms;
        self.postMessage({ type: "progress", ms, total });
      });
      self.postMessage({ type: "zip", blob: new Blob(parts, { type: "application/zip" }) });
    }
  } catch (err) {
    self.postMessage({ type: "error", message: err.message || String(err) });
  }
};
