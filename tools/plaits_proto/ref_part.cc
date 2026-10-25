// SPDX-License-Identifier: GPL-3.0-only
// firmware/src/plaits_part.c (with Plaits' delay lengths: PDF_FULL) against Plaits' ParticleEngine, sample by sample
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "plaits/dsp/engine/particle_engine.h"
#include "stmlib/utils/buffer_allocator.h"
#include "stmlib/utils/random.h"
#define FM1_FPU
#define PDF_FULL
namespace fw {
#include "plaits_ws.c"
#include "plaits_part.c"
}
static char ram[32768];
static fw::pdf_t dif;
int main() {
  stmlib::BufferAllocator al(ram, sizeof ram);
  plaits::ParticleEngine ref; ref.Init(&al); ref.Reset();
  fw::ppa_t port; fw::ppa_init(&port); fw::pdf_init(&dif, 48000.0f);
  uint32_t rng = 0x21; stmlib::Random::Seed(0x21);
  const int N = 12, B = 40000; float ro[N], ra[N], po[N], pa[N]; double maxd = 0, maxa = 0, rms = 0;
  for (int b = 0; b < B; b++) {
    float t = (float)b / B; plaits::EngineParameters p;
    p.trigger = (b % 300) == 0 ? plaits::TRIGGER_RISING_EDGE : plaits::TRIGGER_LOW; p.accent = 0.5f;
    p.note = 36.0f + 48.0f * (0.5f + 0.5f * sinf(t * 7.0f));
    p.harmonics = 0.5f + 0.5f * sinf(t * 13.0f); p.timbre = 0.5f + 0.5f * sinf(t * 5.0f + 1.0f); p.morph = 0.5f + 0.5f * sinf(t * 3.0f + 2.0f);
    bool env = false; ref.Render(p, ro, ra, N, &env);
    fw::ppa_par_t q; fw::ppa_params(&q, p.timbre, p.morph, p.harmonics, 1.0f);
    fw::ppa_render(&port, &rng, p.trigger == plaits::TRIGGER_RISING_EDGE, &q, plaits::NoteToFrequency(p.note), po, pa, N, 3u);
    fw::pdf_process(&dif, 0.8f * q.diffusion * q.diffusion, 0.5f * q.diffusion + 0.25f, po, N);
    for (int i = 0; i < N; i++) { maxd = fmax(maxd, fabs(ro[i] - po[i])); maxa = fmax(maxa, fabs(ra[i] - pa[i])); rms += ro[i] * ro[i]; }
  }
  printf("firmware plaits_part.c vs Plaits: %d samples (rms %.3f), max |out diff| %g, max |aux diff| %g\n", N * B,
         sqrt(rms / (N * B)), maxd, maxa);
  return maxd != 0 || maxa != 0;
}
