

#ifndef __JLVG_UTILS_H__
#define __JLVG_UTILS_H__

#include <stdint.h>

#include "jlvg_hw.h"
#include "jlvg_path.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


typedef struct {
    int x;
    int y;
    int w;
    int h;
} jlvg_rect_t;

void jlvg_rect_intersect(jlvg_rect_t *dst, jlvg_rect_t *src0, jlvg_rect_t *src1);
void jlvg_rect_union(jlvg_rect_t *dst, jlvg_rect_t *src0, jlvg_rect_t *src1);
int jlvg_rect_equal(jlvg_rect_t *src0, jlvg_rect_t *src1);
int jlvg_get_image_format_bpp(jlvg_hw_imageformat_t format);
void jlvg_bbox2rect(jlvg_rect_t *rect, jlvg_boundbox_t *bbox);
int jlvg_get_bgcolor_is_color_key(void *fb_buf, uint16_t color_key);
#endif
