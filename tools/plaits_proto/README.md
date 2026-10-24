# Plaits prototype (Phase 0, docs/PLAITS_PLAN.md)

Not built into the firmware. `ws_proto.c` is Plaits' waveshaping engine as C in float for the FPU (MIT, Emilie
Gillet: the notice is in the file); `ref.cc` compares it sample by sample with the original; `scost.py` counts the
static loop cost of `clang -S` output the way `tests/target_budget.py` counts the objdump listing.

```sh
git clone --depth 1 https://github.com/pichenettes/eurorack.git /tmp/eurorack
git clone --depth 1 https://github.com/pichenettes/stmlib.git /tmp/eurorack/stmlib
python3 -I tools/plaits_proto/extract.py /tmp/eurorack/plaits/resources.cc tools/plaits_proto/ws_tables.h \
    lut_sine lut_fold lut_fold_2 lut_ws_inverse_tan lut_ws_inverse_sin lut_ws_linear lut_ws_bump lut_ws_double_bump
g++ -O2 -w -I/tmp/eurorack -Itools/plaits_proto -o /tmp/ws_ref tools/plaits_proto/ref.cc \
    /tmp/eurorack/plaits/dsp/engine/waveshaping_engine.cc /tmp/eurorack/plaits/resources.cc /tmp/eurorack/stmlib/dsp/units.cc
/tmp/ws_ref                       # add -DWS_FAST for the variant without per-sample divides
T=~/.jieli/toolchain/pi32v2/bin
$T/clang -target pi32v2 -Os -fno-builtin '-DWS_FPU=__attribute__((target("fprev1")))' -Itools/plaits_proto \
    -S tools/plaits_proto/ws_proto.c -o /tmp/ws.s
python3 -I tools/plaits_proto/scost.py /tmp/ws.s=ws_render,ws_slope
```
