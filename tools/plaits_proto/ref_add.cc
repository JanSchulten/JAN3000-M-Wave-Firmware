// SPDX-License-Identifier: GPL-3.0-only
// firmware/src/plaits_add.c against Plaits' AdditiveEngine, sample by sample
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "plaits/dsp/engine/additive_engine.h"
#include "stmlib/utils/buffer_allocator.h"
#define FM1_FPU
namespace fw {
#include "plaits_ws.c"
#include "plaits_add.c"
}
int main() {
  static char ram[4096]; stmlib::BufferAllocator al(ram, sizeof ram);
  plaits::AdditiveEngine ref; ref.Init(&al); ref.Reset();
  fw::pad_t port; fw::pad_init(&port);
  const int N = 12, B = 20000; float ro[N], ra[N], po[N], pa[N]; double maxd = 0, maxa = 0;
  for (int b = 0; b < B; b++) {
    float t = (float)b / B; plaits::EngineParameters p;
    p.trigger = plaits::TRIGGER_LOW; p.accent = 0.5f;
    p.note = 36.0f + 48.0f * (0.5f + 0.5f * sinf(t * 7.0f));
    p.harmonics = 0.5f + 0.5f * sinf(t * 13.0f); p.timbre = 0.5f + 0.5f * sinf(t * 5.0f + 1.0f); p.morph = 0.5f + 0.5f * sinf(t * 3.0f + 2.0f);
    bool env = false; ref.Render(p, ro, ra, N, &env);
    fw::pad_update(&port, p.harmonics, p.timbre, p.morph, 0.001f);
    fw::pad_render(&port, plaits::NoteToFrequency(p.note), po, pa, N, 3u);
    for (int i = 0; i < N; i++) { maxd = fmax(maxd, fabs(ro[i] - po[i])); maxa = fmax(maxa, fabs(ra[i] - pa[i])); }
  }
  printf("firmware plaits_add.c vs Plaits: %d samples, max |out diff| %g, max |aux diff| %g\n", N * B, maxd, maxa);
  return maxd != 0 || maxa != 0;
}
