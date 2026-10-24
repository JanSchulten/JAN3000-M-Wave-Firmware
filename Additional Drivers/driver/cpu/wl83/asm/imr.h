#ifndef __IMR_H__
#define __IMR_H__

#include "typedef.h"
#include "device/device.h"
#include "ioctl.h"
#include "video/fb.h"

#define IMR_SET_CONFIG 			_IOW('R', 0, sizeof(int))
#define IMR_SET_KSTART 			_IOW('R', 1, sizeof(int))


enum IMR_FORMAT {
    IMR_FMT_YUV420,
    IMR_FMT_YUV422,
    IMR_FMT_YUV444_OR_RGB888,
    // IMR_FMT_RGB888,
};

#define IMR_HORI_MIRROR_EN		0x01	/* 水平镜像 */
#define IMR_VERT_MIRROR_EN		0x02    /* 垂直镜像 */

enum IMR_MODE {
    IMR_CAPTURE,
    IMR_DISPLAY,
};


enum IMR_SOURCE {
    IMR,
    IMR_MAX_NUM,
};

enum IMR_STA {
    IMR_IDLE,
    IMR_INUSE,
};

struct imr_reg {
    volatile u32 con;
    volatile u32 haw;
    volatile u32 vaw;
    volatile u32 b0_adr0;
    volatile u32 b0_adr1;
    volatile u32 b0_adr2;
    volatile u32 b1_adr0;
    volatile u32 b1_adr1;
    volatile u32 b1_adr2;
    volatile u32 wr_adr0;
    volatile u32 wr_adr1;
    volatile u32 wr_adr2;
};

struct imr_rotate_info {
    u8 *sour;
    u8 *dest;
    u16 width;
    u16 height;
};

struct imr_info {
    void *sour0;
    void *sour0a;
    void *sour0b;
    void *sour1;
    void *sour1a;
    void *sour1b;
    void *dest;
    u16 width;
    u16 height;
    enum IMR_FORMAT format;
    u8 rotate;
    u8 mode;
    u8 index;
    struct fb_map_user *map;
};

struct imr {
    void *sour0;
    void *sour0a;
    void *sour0b;
    void *sour1;
    void *sour1a;
    void *sour1b;
    void *dest;
    u16 width;
    u16 height;
    enum IMR_FORMAT format;
    enum IMR_STA status;
    u8 index;
    u8 rotate;
    u8 mode;
    u32 dma_cnt;
    struct imr_reg *reg;
    struct imr_info *info;
    struct fb_info fb;
    struct fb_map_user map;
};

#define imr_for_fb_info(fb_info) \
	container_of(fb_info, struct imr, fb)

extern const struct device_operations imr_dev_ops;

int imr_busy();

#endif
