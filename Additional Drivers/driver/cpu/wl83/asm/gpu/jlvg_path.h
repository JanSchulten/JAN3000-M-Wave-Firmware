
#ifndef __JLVG_PATH_H__
#define __JLVG_PATH_H__

#include <stdint.h>
#include "jlvg_math.h"
#include "jlvg_hw.h"

#define JLVG_PATH_DATATYPE_F32  (0)
#define JLVG_PATH_DATATYPE_S32  (1)

#define     JLVG_CAP_BUTT   0
#define     JLVG_CAP_ROUND  1
#define     JLVG_CAP_SQUARE 2

#define     JLVG_JOIN_MITER   0
#define     JLVG_JOIN_ROUND   1
#define     JLVG_JOIN_BEVEL   2


typedef struct {
    jlvg_point2_t min;
    jlvg_point2_t max;
} jlvg_boundbox_t;

typedef struct {
    uint32_t nsegments;
    uint32_t ndata;
    uint8_t *segments;
    void *data;

    uint8_t data_type;

    jlvg_boundbox_t bbox;

    uint8_t *seg;
    uint32_t seg_len;



} jlvg_path_t;

jlvg_path_t *jlvg_path_create(uint8_t data_type, uint32_t nsegments, uint8_t *segments, uint32_t ndata, void *data);

void jlvg_path_init(jlvg_path_t *path, uint8_t data_type, uint32_t nsegments, uint8_t *segments, uint32_t ndata, void *data);
void jlvg_path_update_fill(jlvg_path_t *path);
void jlvg_path_deinit(jlvg_path_t *path);
void jlvg_path_free(jlvg_path_t *path);

void jlvg_path_circle2beizer(uint8_t *segments, uint32_t *nseg, float *data, uint32_t *ndata,
                             float cx, float cy, float r, float sx, float sy, float ex, float ey, uint8_t cw, uint8_t half);

void jlvg_path_update_storke(jlvg_path_t *path, uint8_t cap_style, uint8_t join_style, float stroke_width, float miter_limit);

void jlvg_path_get_seg(jlvg_path_t *path, uint8_t **pseg, int32_t *pseg_len);

void jlvg_path_transform_boundbox(jlvg_boundbox_t *out, jlvg_boundbox_t *bbox, jlvg_matrix_t *m);

#endif

