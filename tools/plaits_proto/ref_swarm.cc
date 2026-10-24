// SPDX-License-Identifier: GPL-3.0-only
// firmware/src/plaits_swarm.c against Plaits' SwarmEngine, sample by sample (trigger patched: bursts; and unpatched)
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "plaits/dsp/engine/swarm_engine.h"
#include "stmlib/utils/buffer_allocator.h"
#include "stmlib/utils/random.h"
#define FM1_FPU
namespace fw {
#include "plaits_ws.c"
#include "plaits_part.c"
#include "plaits_swarm.c"
}
static char ram[8192];
int main() {
  int fail = 0;
  for (int mode = 0; mode < 2; mode++) {
    stmlib::BufferAllocator al(ram, sizeof ram);
    plaits::SwarmEngine ref; ref.Init(&al); ref.Reset();
    fw::psw_t port; fw::psw_init(&port);
    uint32_t rng = 0x21; stmlib::Random::Seed(0x21);
    const int N = 12, B = 40000; float ro[N], ra[N], po[N], pa[N]; double maxd = 0, maxa = 0, rms = 0;
    for (int b = 0; b < B; b++) {
      float t = (float)b / B; plaits::EngineParameters p;
      const bool start = (b % 300) == 0;
      p.trigger = mode ? plaits::TRIGGER_UNPATCHED : (start ? plaits::TRIGGER_RISING_EDGE : plaits::TRIGGER_LOW);
      p.accent = 0.5f;
      p.note = 36.0f + 48.0f * (0.5f + 0.5f * sinf(t * 7.0f));
      p.harmonics = 0.5f + 0.5f * sinf(t * 13.0f); p.timbre = 0.5f + 0.5f * sinf(t * 5.0f + 1.0f); p.morph = 0.5f + 0.5f * sinf(t * 3.0f + 2.0f);
      bool env = false; ref.Render(p, ro, ra, N, &env);
      float density, spread, size_ratio;
      fw::psw_params(p.timbre, p.morph, p.harmonics, N, 1.0f, &density, &spread, &size_ratio);
      fw::psw_render(&port, &rng, plaits::NoteToFrequency(p.note), density, spread, size_ratio, !mode, !mode && start, po, pa, N, 3u);
      for (int i = 0; i < N; i++) { maxd = fmax(maxd, fabs(ro[i] - po[i])); maxa = fmax(maxa, fabs(ra[i] - pa[i])); rms += ro[i] * ro[i]; }
    }
    printf("firmware plaits_swarm.c vs Plaits (%s): %d samples (rms %.3f), max |out diff| %g, max |aux diff| %g\n",
           mode ? "unpatched" : "bursts", N * B, sqrt(rms / (N * B)), maxd, maxa);
    fail |= maxd != 0 || maxa != 0;
  }
  return fail;
}
