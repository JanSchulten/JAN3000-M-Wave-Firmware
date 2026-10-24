#ifndef __JLVG_HW_H__
#define __JLVG_HW_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JLVG_HW_COORD_FRAC_BITS		(8)


typedef enum { //EXTEND_COLOR_FORMAT
    VGHW_FORMAT_ARGB8888 =	    0x00,
    VGHW_FORMAT_ARGB8565 =	    0x01,
    VGHW_FORMAT_ARGB1555 =	    0x02,  //newly added
    VGHW_FORMAT_ARGB4444 =	    0x03, //newly added
    VGHW_FORMAT_RGB888 =        0x04,
    VGHW_FORMAT_RGB565 =	    0x05,
    VGHW_FORMAT_YUV422_BT601 =	0x06, //newly added
    VGHW_FORMAT_YUV422_BT709 =  0x07, //newly added
    VGHW_FORMAT_AL88 =		    0x08,
    VGHW_FORMAT_AL44 =	      	0x10,
    VGHW_FORMAT_AL22 =		    0x11,
    VGHW_FORMAT_L8 =		    0x12,
    VGHW_FORMAT_L4 =		    0x13,
    VGHW_FORMAT_L2 =		    0x14,
    VGHW_FORMAT_L1 =		    0x15, //newly added
    VGHW_FORMAT_A8 =		    0x16,
    VGHW_FORMAT_A4 =		    0x17,
    VGHW_FORMAT_A2 =		    0x18,
    VGHW_FORMAT_A1 =		    0x19,

} jlvg_hw_imageformat_t;


typedef enum {
    VGHW_CLUT_FORMAT_ARGB8888  = 0x00,
    VGHW_CLUT_FORMAT_ARGB8565  = 0x01,
    VGHW_CLUT_FORMAT_RGB888    = 0x02,
    VGHW_CLUT_FORMAT_RGB565    = 0x03,
} jlvg_hw_clutformat_t;

typedef enum {
    VGHW_COLOR_RAMP_SPREAD_PAD = 0,
    VGHW_COLOR_RAMP_SPREAD_REFLECT = 1,
    VGHW_COLOR_RAMP_SPREAD_REPEAT = 2,
} jlvg_hw_spreadmode_t;

typedef enum {
    VGHW_PAINT_TYPE_COLOR = 0,
    VGHW_PAINT_TYPE_LINEAR_GRADIENT = 1,
    VGHW_PAINT_TYPE_RADIAL_GRADIENT = 2,
} jlvg_hw_painttype_t;

typedef enum {
    VGHW_RENDERING_QUALITY_NONANTIALIASED = 0,
    VGHW_RENDERING_QUALITY_MSAA_4X = 1,
    VGHW_RENDERING_QUALITY_MSAA_8X = 2,
} jlvg_hw_rendingquality_t;

typedef enum {
    VGHW_IMAGE_QUALITY_BILINEAR = 0,
    VGHW_IMAGE_QUALITY_NONANTIALIASED = 1,
} jlvg_hw_imagequality_t;

typedef enum {
    VGHW_DRAW_IMAGE_NORMAL = 0,
    VGHW_DRAW_IMAGE_MULTIPLY = 1,
    VGHW_DRAW_IMAGE_STENCIL = 2,
} jlvg_hw_imagemode_t;

typedef enum {
    VGHW_BLEND_SRC = 0,
    VGHW_BLEND_SRC_OVER = 1,
    VGHW_BLEND_DST_OVER = 2,
    VGHW_BLEND_SRC_IN = 3,
    VGHW_BLEND_DST_IN = 4,
    VGHW_BLEND_MULTIPLY = 5,
    VGHW_BLEND_SCREEN = 6,
    VGHW_BLEND_ADDITIVE = 7,

} jlvg_hw_blendmode_t;

typedef enum {
    VGHW_NONE          = 0,
    VGHW_PADDING_MODE  = 1,
    VGHW_BLENDING_MODE = 2,
} jlvg_hw_bgmode_t;

typedef enum {
    VGHW_FRAG_IMAGE = 0,
    VGHW_FRAG_PAINT_MULTIPLY_IMAGE = 1,
    VGHW_FRAG_PAINT_STENCIL_IMAGE = 2,
    VGHW_FRAG_PAINT = 3,
} jlvg_hw_paintmode_t;

typedef enum {
    VGHW_NON_ZERO = 0,
    VGHW_EVEN_ODD = 1,
} jlvg_hw_fillrule_t;

typedef enum {
    VGHW_TILE_FILL = 0,
    VGHW_TILE_PAD = 1,
    VGHW_TILE_REPEAT = 2,
    VGHW_TILE_REFLECT = 3,

} jlvg_hw_tilingmode_t;

typedef enum {
    VGHW_CLOSE_PATH = (0),
    VGHW_MOVE_TO = (1),
    VGHW_LINE_TO = (2),
    VGHW_HLINE_TO = (3),
    VGHW_VLINE_TO = (4),
    VGHW_QUAD_TO = (5),
    VGHW_SQUAD_TO = (6),
    VGHW_CUBIC_TO = (7),
    VGHW_SCUBIC_TO = (8),
    VGHW_SCCWARC_TO = (9),
    VGHW_SCWARC_TO = (10),
    VGHW_LCCWARC_TO = (11),
    VGHW_LCWARC_TO = (12),
} jlvg_hw_pathtype_t;



typedef struct {
    //0
    struct {
        uint32_t		blend_mode : 4;
        uint32_t		paint_mode : 2;
        uint32_t		msaa_lvl : 2;

        uint32_t		gb_alpha: 8;
        uint32_t		seg_cmd_len: 16;
    };

    //1
    uint32_t next_inst_adr;

    //2
    uint32_t frag_base_adr;

    //3
    uint32_t seg_base_adr;

    //4
    struct {
        uint32_t		act_x_min : 12;
        uint32_t        reserved0 : 4;
        uint32_t		act_x_max : 12;
    };

    //5
    struct {
        uint32_t		act_y_min : 12;
        uint32_t        reserved1 : 4;
        uint32_t		act_y_max : 12;
    };

} jlvg_hw_cmd_t;

typedef struct {
    uint32_t		trans_m00;
    uint32_t		trans_m01;
    uint32_t		trans_m02;
    uint32_t		trans_m10;
    uint32_t		trans_m11;
    uint32_t		trans_m12;
    struct {
        uint32_t		fill_rule : 1;
        uint32_t		reserved : 7;
        uint32_t		seg_byte0 : 8;
        uint32_t		seg_byte1 : 8;
        uint32_t		seg_byte2 : 8;
    };
} jlvg_hw_seg_t;

typedef struct {
    //0
    struct {
        uint32_t	paint_type : 2;
        uint32_t	spread_mode : 2;
        uint32_t	grad_lut_lvl : 3;

        uint32_t	reserved0 : 9;
        //TODO
        uint32_t	grad_lut_base : 11;
    };

    //1
    union {
        struct {
            uint32_t		inside_radius : 24;
        };

        struct {
            uint8_t	pad_blue;
            uint8_t	pad_green;
            uint8_t	pad_red;
            uint8_t	pad_alpha;
        } pad_color;
    };

    uint32_t		paint_im00;
    uint32_t		paint_im01;
    uint32_t		paint_im02;
    uint32_t		paint_im10;
    uint32_t		paint_im11;
    uint32_t		paint_im12;
    uint32_t		paint_im20;
    uint32_t		paint_im21;

} jlvg_hw_fragpaint_t;


typedef struct {
    struct {
        uint32_t   img_color_fmt : 5;
        uint32_t   rle_mode : 1; //TODO
        uint32_t   img_adr_mode : 1; //TODO 0:tile-base 1:line-base
        uint32_t   quality : 1;
        uint32_t   premult : 1;
        uint32_t   rb_swap : 1;
        uint32_t   clut_format : 2;

        uint32_t   color_ext : 1; //newly added
        uint32_t   ckey_en : 1;
        uint32_t   margin_mode : 1; //newly added
        uint32_t   reserved0 : 1;
        uint32_t   ckey_red : 8;
        uint32_t   ckey_green : 8;

    };

    struct {

        uint32_t	img_stride : 20;
        uint32_t	reserved1 : 4;
        uint32_t	ckey_blue : 8;
    };

    struct {
        uint32_t	img_width : 16;
        uint32_t	img_height : 16;
    };

    uint32_t	img_base_adr;
    union {
        uint32_t img_lut_adr;
        uint32_t img_a_color;
    };

    uint32_t	img_im00;
    uint32_t	img_im01;
    uint32_t	img_im02;
    uint32_t	img_im10;
    uint32_t	img_im11;
    uint32_t	img_im12;
    uint32_t	img_im20;
    uint32_t	img_im21;

    uint32_t    rle_limit;
} jlvg_hw_fragimage_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
