
#ifndef __DMA2D_DRIVER_H__
#define __DMA2D_DRIVER_H__


#define DMA2D_PLATFORM_PC	0

#include <stdint.h>
#include "jldma2d.h"


#if DMA2D_PLATFORM_PC
extern JLDMA2D_TypeDef *JL_DMA2D;

#else

#include "asm/WL83.h"
#include "system/includes.h"
#include "asm/dma2d_rcf.h"

#endif

#define JLDMA2D_R2M     0
#define JLDMA2D_M2M     1
#define JLDMA2D_M2M_PFC 2
#define JLDMA2D_M2M_BLEND   3

int dma2d_init();
int dma2d_free();
int dma2d_run();
int dma2d_wait_done();

struct dma2d_rcf *dma2d_create_task();
uint32_t dma2d_get_format_bpp(uint32_t format);
void dma2d_reset_all_regs();
void dma2d_set_mode(uint32_t mode);
void dma2d_set_blend_mode(uint32_t blend_mode);
void dma2d_fill(uint8_t *dest_buf, uint32_t dest_stride, uint32_t format, uint32_t endian, uint32_t color, uint32_t w, uint32_t h);
void dma2d_copy(uint8_t *dest_buf, uint32_t dest_stride, uint8_t *src_buf, uint32_t src_stride, uint32_t format, uint32_t w, uint32_t h);


typedef struct {
    uint8_t *data;
    uint16_t w;
    uint16_t h;
    uint32_t stride;
    uint8_t format;
    uint8_t alpha_inv;
    uint8_t rbs;
    uint8_t lr;
    uint8_t dither_en;
    uint8_t endian;
} dma2d_out_layer_params_t;

void dma2d_set_out_layer(dma2d_out_layer_params_t *param);


typedef struct {
    uint8_t *data;
    uint16_t x;
    uint16_t y;
    uint32_t stride;
    uint32_t color;
    uint32_t *clut;
    uint32_t clut_size;
    uint32_t image_size;
    uint8_t format;
    uint8_t rle_en;
    uint8_t endian;
    uint8_t alpha;
    uint8_t alpha_mode;
    uint8_t alpha_inv;
    uint8_t rbs;
    uint8_t lr;
    uint8_t colorkey;
    uint8_t paintmode;
    uint8_t clut_format;
} dma2d_input_layer_params_t;

void dma2d_set_fg_layer(dma2d_input_layer_params_t *param);

void dma2d_set_bg_layer(dma2d_input_layer_params_t *param);

void dma2d_set_ypnd(uint8_t en, uint32_t y);

uint32_t dma2d_pack_pixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b, uint32_t format);

struct dma2d_rcf *dma2d_get_current_task(void);

typedef struct {
    int x, y, w, h;
} dma2d_rectangle_t;

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

void dma2d_rectangle_intersect(dma2d_rectangle_t *dst, dma2d_rectangle_t *src0, dma2d_rectangle_t *src1);


#endif
