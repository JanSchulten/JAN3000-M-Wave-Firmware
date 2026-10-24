// SPDX-License-Identifier: GPL-3.0-only
// firmware/src/plaits_wt.c (with Plaits' 16-bit tables: PWT_FULL) against Plaits' WavetableEngine, sample by sample
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include "plaits/dsp/engine/wavetable_engine.h"
#include "plaits/resources.h"
#include "stmlib/utils/buffer_allocator.h"
#define FM1_FPU
#define PWT_FULL
#define PWT_TABLE plaits::wav_integrated_waves
namespace fw {
#include "plaits_wt.c"
}
static char ram[4096];
int main() {
  stmlib::BufferAllocator al(ram, sizeof ram);
  plaits::WavetableEngine ref; ref.Init(&al); ref.LoadUserData(NULL); ref.Reset();
  fw::pwt_t port; fw::pwt_init(&port);
  const int N = 12, B = 40000; float ro[N], ra[N], po[N], pa[N]; double maxd = 0, maxa = 0, rms = 0;
  for (int b = 0; b < B; b++) {
    float t = (float)b / B; plaits::EngineParameters p;
    p.trigger = plaits::TRIGGER_LOW; p.accent = 0.5f;
    p.note = 36.0f + 48.0f * (0.5f + 0.5f * sinf(t * 7.0f));
    p.harmonics = 0.5f + 0.5f * sinf(t * 13.0f); p.timbre = 0.5f + 0.5f * sinf(t * 5.0f + 1.0f); p.morph = 0.5f + 0.5f * sinf(t * 3.0f + 2.0f);
    bool env = false; ref.Render(p, ro, ra, N, &env);
    fw::pwt_par_t q = {p.timbre, p.morph, p.harmonics, plaits::NoteToFrequency(p.note), 0.2f, 0.05f};
    fw::pwt_render(&port, 0, &q, po, pa, N, 3u);
    for (int i = 0; i < N; i++) { maxd = fmax(maxd, fabs(ro[i] - po[i])); maxa = fmax(maxa, fabs(ra[i] - pa[i])); rms += ro[i] * ro[i]; }
  }
  printf("firmware plaits_wt.c vs Plaits: %d samples (rms %.3f), max |out diff| %g, max |aux diff| %g\n", N * B,
         sqrt(rms / (N * B)), maxd, maxa);
  return maxd != 0 || maxa != 0;
}
