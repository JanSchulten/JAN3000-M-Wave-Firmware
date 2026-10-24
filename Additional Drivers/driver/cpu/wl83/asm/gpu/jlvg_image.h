

#ifndef __JLVG_IMAGE_H__
#define __JLVG_IMAGE_H__

#include <stdint.h>
#include "jlvg_hw.h"
#include "jlvg_utils.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    jlvg_hw_imageformat_t format;

    uint8_t *data;
    uint8_t *clut;
    uint32_t data_size;

    jlvg_hw_imagequality_t quality;
    jlvg_hw_clutformat_t clut_format;

    jlvg_hw_imagemode_t image_mode;

    uint8_t premult;
    uint8_t rb_swap;

    uint8_t rle_en;
    uint8_t adr_mode;

    uint8_t ckey_en;
    uint8_t ckey_red;
    uint8_t ckey_green;
    uint8_t ckey_blue;

    uint8_t a_color_alpha;
    uint8_t a_color_red;
    uint8_t a_color_green;
    uint8_t a_color_blue;

} jlvg_image_t;


typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    jlvg_hw_imageformat_t format;

    uint8_t *data;
    uint32_t data_size;

    uint8_t premult;
    uint8_t rb_swap;
    uint8_t adr_mode;
    uint8_t dither_en;

    int bpp;

} jlvg_surface_t;

jlvg_image_t *jlvg_image_create(uint8_t *data, uint32_t data_size,
                                uint32_t w, uint32_t h, uint32_t stride, jlvg_hw_imageformat_t format,
                                uint8_t rle_en, uint8_t adr_mode, uint8_t premult, uint8_t rb_swap);

int jlvg_image_init(jlvg_image_t *img, uint8_t *data, uint32_t data_size,
                    uint32_t w, uint32_t h, uint32_t stride, jlvg_hw_imageformat_t format,
                    uint8_t rle_en, uint8_t adr_mode, uint8_t premult, uint8_t rb_swap);

int jlvg_image_deinit(jlvg_image_t *img);
int jlvg_image_free(jlvg_image_t *img);

int jlvg_image_set_clut(jlvg_image_t *img, uint8_t *clut, jlvg_hw_clutformat_t clut_format);

int jlvg_image_set_colorkey(jlvg_image_t *img, uint8_t ckey_en, uint8_t ckey_red, uint8_t ckey_green, uint8_t ckey_blue);

int jlvg_image_set_a_color(jlvg_image_t *img, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);

int jlvg_image_set_quality(jlvg_image_t *img, jlvg_hw_imagequality_t quality);

int jlvg_image_set_image_mode(jlvg_image_t *img, jlvg_hw_imagemode_t image_mode);


int jlvg_image_to_frag(jlvg_image_t *img, jlvg_matrix_t *m, jlvg_hw_fragimage_t *frag_image);

jlvg_surface_t *jlvg_surface_create(uint8_t *data, uint32_t data_size,
                                    uint32_t w, uint32_t h, uint32_t stride, jlvg_hw_imageformat_t format,
                                    uint8_t dither_en, uint8_t adr_mode, uint8_t premult, uint8_t rb_swap);

int jlvg_surface_init(jlvg_surface_t *surface, uint8_t *data, uint32_t data_size,
                      uint32_t w, uint32_t h, uint32_t stride, jlvg_hw_imageformat_t format,
                      uint8_t dither_en, uint8_t adr_mode, uint8_t premult, uint8_t rb_swap);

int jlvg_surface_deinit(jlvg_surface_t *surface);
int jlvg_surface_free(jlvg_surface_t *surface);

int jlvg_surface_get_rect(jlvg_surface_t *surface, jlvg_rect_t *rect);

#endif