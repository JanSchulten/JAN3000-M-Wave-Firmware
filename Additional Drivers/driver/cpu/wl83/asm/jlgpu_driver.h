

#ifndef JL_GPU2P5D_DRIVER_H__
#define JL_GPU2P5D_DRIVER_H__

#include "hwApi.h"


#define JLGPU_RUN_ON_PC     0
#define JLGPU_DUMP_PARAMETERS   0


int jlgpu_init();
int jlgpu_free();

int jlgpu_add_layer(VGHW_CMD_TypeDef *layer);

int jlgpu_run();
int jlgpu_wait_done();
void jlgpu_reset_all_regs();

void jlgpu_set_out_callback(void *cb_func);
unsigned int jlgpu_get_gradlut_ptr();
void jlgpu_set_gradlut_ptr(unsigned int ptr);
void jlgpu_set_gradlut_dat(unsigned int dat);

void jlgpu_set_out_window(void *win_adr, int minx, int maxx, int miny, int maxy, int stride);
void jlgpu_set_out_format(int format, int adr_mode, int dither_en, int rbs, int premult);
void jlgpu_set_max_bez_div(int div);
void jlgpu_set_max_msaa_lvl(int lvl);
void jlgpu_set_bg_mode(int bg_mode, unsigned int pad_color);

#endif
