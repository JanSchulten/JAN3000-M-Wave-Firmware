# Plaits tools (docs/PLAITS_PLAN.md)

Not built into the firmware. `extract.py` writes Plaits' tables as a C header (`firmware/src/plaits_tables.h` was made
with it); `ref.cc` compares `firmware/src/plaits_ws.c` sample by sample with Plaits' own waveshaping engine, `ref_add.cc`
`firmware/src/plaits_add.c` with its additive engine, `ref_part.cc` `firmware/src/plaits_part.c` (Plaits' delay lengths:
`PDF_FULL`) with its particle engine (each must print a difference of 0; `-DTEST` keeps stmlib off its ARM assembly); `scost.py` counts the static loop cost of `clang -S` output the way `tests/target_budget.py`
counts the objdump listing, FPU instructions included.

```sh
git clone --depth 1 https://github.com/pichenettes/eurorack.git /tmp/eurorack
git clone --depth 1 https://github.com/pichenettes/stmlib.git /tmp/eurorack/stmlib
python3 -I tools/plaits_proto/extract.py /tmp/eurorack/plaits/resources.cc firmware/src/plaits_tables.h \
    lut_sine lut_fold lut_fold_2 lut_ws_inverse_tan lut_ws_inverse_sin lut_ws_linear lut_ws_bump lut_ws_double_bump
g++ -O2 -w -I/tmp/eurorack -Ifirmware/src -o /tmp/ws_ref tools/plaits_proto/ref.cc \
    /tmp/eurorack/plaits/dsp/engine/waveshaping_engine.cc /tmp/eurorack/plaits/resources.cc /tmp/eurorack/stmlib/dsp/units.cc
/tmp/ws_ref
g++ -O2 -w -I/tmp/eurorack -Ifirmware/src -o /tmp/add_ref tools/plaits_proto/ref_add.cc \
    /tmp/eurorack/plaits/dsp/engine/additive_engine.cc /tmp/eurorack/plaits/resources.cc /tmp/eurorack/stmlib/dsp/units.cc
/tmp/add_ref
g++ -DTEST -O2 -w -I/tmp/eurorack -Ifirmware/src -o /tmp/part_ref tools/plaits_proto/ref_part.cc \
    /tmp/eurorack/plaits/dsp/engine/particle_engine.cc /tmp/eurorack/plaits/resources.cc \
    /tmp/eurorack/stmlib/dsp/units.cc /tmp/eurorack/stmlib/utils/random.cc
/tmp/part_ref
T=~/.jieli/toolchain/pi32v2/bin   # the loop cost on the target: compile with the FPU on, as core.h FM1_FPU does
printf 'typedef short int16_t; typedef int int32_t; typedef unsigned uint32_t;\n#define FM1_FPU __attribute__((target("fprev1")))\n#include "plaits_ws.c"\nFM1_FPU void r(pws_t *s, float *o, float *a) { pws_render(s, 0.01f, 0.5f, 0.5f, 0.5f, o, a, 32, 3u); }\n' > /tmp/ws_cost.c
$T/clang -target pi32v2 -Os -fno-builtin -ffreestanding -Ifirmware/src -S /tmp/ws_cost.c -o /tmp/ws.s
python3 -I tools/plaits_proto/scost.py /tmp/ws.s=r,pws_slope   # r: pws_render inlined; pws_slope runs twice a block
```
