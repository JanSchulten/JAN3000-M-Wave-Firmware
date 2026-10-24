// SPDX-License-Identifier: MIT (the engine compared is Plaits', Emilie Gillet)
// Compare the C port (ws_proto.c) with Plaits' own WaveshapingEngine, sample by sample.
#include <cstdio>
#include <cmath>
#include "plaits/dsp/engine/waveshaping_engine.h"
extern "C" {
#include "ws_proto.c"
}
int main() {
  plaits::WaveshapingEngine ref;
  ref.Init(NULL);
  ws_t port;
  ws_init(&port);
  const int N = 12, B = 20000;
  float ro[N], ra[N], po[N], pa[N];
  double maxd = 0, maxa = 0, se = 0, ss = 0;
  for (int b = 0; b < B; b++) {
    float t = (float)b / B;
    plaits::EngineParameters p;
    p.trigger = plaits::TRIGGER_LOW; p.accent = 0.5f;
    p.note = 36.0f + 48.0f * (0.5f + 0.5f * sinf(t * 7.0f));
    p.harmonics = 0.5f + 0.5f * sinf(t * 13.0f);
    p.timbre = 0.5f + 0.5f * sinf(t * 5.0f + 1.0f);
    p.morph = 0.5f + 0.5f * sinf(t * 3.0f + 2.0f);
    bool env = false;
    ref.Render(p, ro, ra, N, &env);
    ws_render(&port, plaits::NoteToFrequency(p.note), p.harmonics, p.timbre, p.morph, po, pa, N);
    for (int i = 0; i < N; i++) {
      double d = fabs(ro[i] - po[i]), da = fabs(ra[i] - pa[i]);
      if (d > maxd) maxd = d;
      if (da > maxa) maxa = da;
      se += d * d; ss += (double)ro[i] * ro[i];
    }
  }
  printf("samples %d  max |out diff| %.3g  max |aux diff| %.3g  error/signal %.1f dB\n", N * B, maxd, maxa,
         10 * log10(se / ss + 1e-30));
  return 0;
}
