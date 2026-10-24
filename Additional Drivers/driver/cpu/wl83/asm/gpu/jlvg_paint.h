
#ifndef __JLVG_PAINT_H__
#define __JLVG_PAINT_H__

#include <stdint.h>
#include "jlvg_hw.h"

#include "jlvg_math.h"

typedef struct {
    union {
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
        uint32_t full;
    };
} jlvg_color_t;

typedef struct {
    jlvg_hw_painttype_t	paint_type;
} jlvg_paint_t;

typedef struct {
    jlvg_hw_painttype_t	paint_type;
    jlvg_color_t			paint_color;
} jlvg_paint_color_t;

typedef struct {
    jlvg_hw_painttype_t	paint_type;

    jlvg_hw_spreadmode_t spread_mode;

    //[0,1,2,3,4,5,6]->[2,3,5,9,17,33,64]
    uint32_t log2size;
    jlvg_color_t *color_ramp_stops;

    jlvg_point2_t p0;
    jlvg_point2_t p1;

} jlvg_paint_linear_t;

typedef struct {
    jlvg_hw_painttype_t	paint_type;

    jlvg_hw_spreadmode_t spread_mode;

    //[0,1,2,3,4,5,6]->[2,3,5,9,17,33,64]
    uint32_t log2size;
    jlvg_color_t *color_ramp_stops;

    jlvg_point2_t center;
    float radius;
    float inner_radius;

} jlvg_paint_radial_t;

jlvg_paint_color_t *jlvg_paint_color_create(jlvg_color_t paint_color);
void jlvg_paint_color_init(jlvg_paint_color_t *p, jlvg_color_t paint_color);


jlvg_paint_linear_t *jlvg_paint_linear_create(jlvg_point2_t *p0, jlvg_point2_t *p1);
void jlvg_paint_linear_init(jlvg_paint_linear_t *p, jlvg_point2_t *p0, jlvg_point2_t *p1);


jlvg_paint_radial_t *jlvg_paint_radial_create(jlvg_point2_t *center,
        float radius,
        float inner_radius);

void jlvg_paint_radial_init(jlvg_paint_radial_t *p, jlvg_point2_t *center,
                            float radius,
                            float inner_radius);

void jlvg_paint_set_rampstops(jlvg_paint_t *p,
                              jlvg_hw_spreadmode_t spread_mode,
                              uint32_t log2size,
                              jlvg_color_t *rampstops);

void jlvg_paint_deinit(jlvg_paint_t *paint);
void jlvg_paint_free(jlvg_paint_t *p);


int jlvg_paint_to_frag(jlvg_paint_t *paint, jlvg_matrix_t *m, jlvg_hw_fragpaint_t *frag_paint);

int jlvg_color_255to256(int color);
uint8_t jlvg_color_mul(const int c0, const int c1);
void jlvg_color_premultiply(jlvg_color_t *c);
#endif