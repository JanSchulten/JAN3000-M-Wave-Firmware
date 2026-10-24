#ifndef __HWAPI_H
#define __HWAPI_H

//#include "openvg.h"
#include "gpu/jlvg_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VG_HW_COORD_INT_BITS		(16)   //signed 16bit
#define VG_HW_COORD_FRAC_BITS		(8)
#define VG_HW_COORD_ONE				(1<<VG_HW_COORD_FRAC_BITS)
#define VG_HW_COORD_FRAC_MASK		(VG_HW_COORD_ONE - 1)
#define VG_HW_COORD_HALF			(1<<(VG_HW_COORD_FRAC_BITS-1))

#define VG_HW_GRADIENT_FRAC_BITS	(14)
#define VG_HW_GRADIENT_ONE			(1<<VG_HW_GRADIENT_FRAC_BITS)
#define VG_HW_GRADIENT_FRAC_MASK	(VG_HW_GRADIENT_ONE-1)
#define VG_HW_GRADIENT_HALF			(1<<(VG_HW_GRADIENT_FRAC_BITS-1))




typedef float          VGHWfloat;
typedef signed char    VGHWbyte;
typedef unsigned char  VGHWubyte;
typedef signed short   VGHWshort;
typedef unsigned short   VGHWushort;
typedef signed int     VGHWint;
typedef unsigned int   VGHWuint;
typedef unsigned int   VGHWbitfield;

typedef VGHWuint VGHW_Handle_TypeDef;

typedef enum {
    VGHW_FALSE = 0,
    VGHW_TRUE = 1,
} VGHW_Boolean_TypeDef;

// typedef enum { //EXTEND_COLOR_FORMAT
// VGHW_FORMAT_ARGB8888 =	    0x00,
// VGHW_FORMAT_ARGB8565 =	    0x01,
// VGHW_FORMAT_ARGB1555 =	    0x02,  //newly added
// VGHW_FORMAT_ARGB4444 =	    0x03, //newly added
// VGHW_FORMAT_RGB888 =        0x04,
// VGHW_FORMAT_RGB565 =	    0x05,
// VGHW_FORMAT_YUV422_BT601 =	0x06, //newly added
// VGHW_FORMAT_YUV422_BT709 =  0x07, //newly added
// VGHW_FORMAT_AL88 =		    0x08,
// VGHW_FORMAT_AL44 =	      	0x10,
// VGHW_FORMAT_AL22 =		    0x11,
// VGHW_FORMAT_L8 =		    0x12,
// VGHW_FORMAT_L4 =		    0x13,
// VGHW_FORMAT_L2 =		    0x14,
// VGHW_FORMAT_L1 =		    0x15, //newly added
// VGHW_FORMAT_A8 =		    0x16,
// VGHW_FORMAT_A4 =		    0x17,
// VGHW_FORMAT_A2 =		    0x18,
// VGHW_FORMAT_A1 =		    0x19,

// } VGHW_ImageFormat_TypeDef;
#define VGHW_ImageFormat_TypeDef jlvg_hw_imageformat_t


// typedef enum {
// VGHW_CLUT_FORMAT_ARGB8888  = 0x00,
// VGHW_CLUT_FORMAT_ARGB8565  = 0x01,
// VGHW_CLUT_FORMAT_RGB888    = 0x02,
// VGHW_CLUT_FORMAT_RGB565    = 0x03,
// } VGHW_ClutFormat_TypeDef;
#define VGHW_ClutFormat_TypeDef jlvg_hw_clutformat_t

// typedef enum {
// VGHW_COLOR_RAMP_SPREAD_PAD = 0,
// VGHW_COLOR_RAMP_SPREAD_REFLECT = 1,
// VGHW_COLOR_RAMP_SPREAD_REPEAT = 2,
// } VGHW_ColorRampSpreadMode_TypeDef;
#define VGHW_ColorRampSpreadMode_TypeDef  jlvg_hw_spreadmode_t

// typedef enum {
// VGHW_PAINT_TYPE_COLOR = 0,
// VGHW_PAINT_TYPE_LINEAR_GRADIENT = 1,
// VGHW_PAINT_TYPE_RADIAL_GRADIENT = 2,
// } VGHW_PaintType_TypeDef;
#define VGHW_PaintType_TypeDef jlvg_hw_painttype_t

// typedef enum {
// VGHW_RENDERING_QUALITY_NONANTIALIASED = 0,
// VGHW_RENDERING_QUALITY_MSAA_4X = 1,
// VGHW_RENDERING_QUALITY_MSAA_8X = 2,
// } VGHW_RenderingQuality_TypeDef;
#define VGHW_RenderingQuality_TypeDef jlvg_hw_rendingquality_t

// typedef enum {
// VGHW_IMAGE_QUALITY_BILINEAR = 0,
// VGHW_IMAGE_QUALITY_NONANTIALIASED = 1,
// } VGHW_ImageQuality_TypeDef;
#define VGHW_ImageQuality_TypeDef jlvg_hw_imagequality_t

// typedef enum {
// VGHW_DRAW_IMAGE_NORMAL = 0,
// VGHW_DRAW_IMAGE_MULTIPLY = 1,
// VGHW_DRAW_IMAGE_STENCIL = 2,
// } VGHW_ImageMode_TypeDef;
#define VGHW_ImageMode_TypeDef jlvg_hw_imagemode_t

// typedef enum {
// VGHW_BLEND_SRC = 0,
// VGHW_BLEND_SRC_OVER = 1,
// VGHW_BLEND_DST_OVER = 2,
// VGHW_BLEND_SRC_IN = 3,
// VGHW_BLEND_DST_IN = 4,
// VGHW_BLEND_MULTIPLY = 5,
// VGHW_BLEND_SCREEN = 6,
// VGHW_BLEND_ADDITIVE = 7,

// } VGHW_BlendMode_TypeDef;
#define VGHW_BlendMode_TypeDef jlvg_hw_blendmode_t

// typedef enum {
// VGHW_NONE          = 0,
// VGHW_PADDING_MODE  = 1,
// VGHW_BLENDING_MODE = 2,
// } VGHW_BgMode_TypeDef;
#define VGHW_BgMode_TypeDef jlvg_hw_bgmode_t

// typedef enum {
// VGHW_FRAG_IMAGE = 0,
// VGHW_FRAG_PAINT_MULTIPLY_IMAGE = 1,
// VGHW_FRAG_PAINT_STENCIL_IMAGE = 2,
// VGHW_FRAG_PAINT = 3,
// } VGHW_PaintMode_TypeDef;
#define VGHW_PaintMode_TypeDef jlvg_hw_paintmode_t

// typedef enum {
// VGHW_NON_ZERO = 0,
// VGHW_EVEN_ODD = 1,
// } VGHW_FillRule_TypeDef;
#define VGHW_FillRule_TypeDef jlvg_hw_fillrule_t

// typedef enum {
// VGHW_TILE_FILL = 0,
// VGHW_TILE_PAD = 1,
// VGHW_TILE_REPEAT = 2,
// VGHW_TILE_REFLECT = 3,

// } VGHW_TilingMode_TypeDef;
#define VGHW_TilingMode_TypeDef jlvg_hw_tilingmode_t

// typedef enum {
// VGHW_CLOSE_PATH = (0),
// VGHW_MOVE_TO = (1),
// VGHW_LINE_TO = (2),
// VGHW_HLINE_TO = (3),
// VGHW_VLINE_TO = (4),
// VGHW_QUAD_TO = (5),
// VGHW_SQUAD_TO = (6),
// VGHW_CUBIC_TO = (7),
// VGHW_SCUBIC_TO = (8),
// VGHW_SCCWARC_TO = (9),
// VGHW_SCWARC_TO = (10),
// VGHW_LCCWARC_TO = (11),
// VGHW_LCWARC_TO = (12),
// } VGHW_PathSegment_TypeDef;
#define VGHW_PathSegment_TypeDef jlvg_hw_pathtype_t

typedef struct {
    VGHWuint xbus_seg : 2;
} VGHW_XBUS_CON0_TypeDef;

typedef struct {
    VGHWuint grp_sel_wb : 1;
    VGHWuint reserved : 15;
    VGHWuint grp_sel_l1 : 1;
    VGHWuint grp_sel_l2_path : 1;
    VGHWuint grp_sel_l2_frag : 1;
    VGHWuint grp_sel_dch : 1;
} VGHW_XBUS_CON1_TypeDef;

typedef struct {
    VGHWuint vtx_done_pnd : 1;
    VGHWuint frg_done_pnd : 1;
    VGHWuint blk_done_pnd : 1;
    VGHWuint win_done_pnd : 1;
    VGHWuint wcnt_ov_pnd : 1;
} VGHW_PND_CON_TypeDef;

typedef struct {
    VGHWuint vtx_done_clr : 1;
    VGHWuint frg_done_clr : 1;
    VGHWuint blk_done_clr : 1;
    VGHWuint win_done_clr : 1;
    VGHWuint wcnt_ov_clr : 1;
} VGHW_PND_CLR_TypeDef;

typedef struct {
    VGHWuint vtx_done_ie : 1;
    VGHWuint frg_done_ie : 1;
    VGHWuint blk_done_ie : 1;
    VGHWuint win_done_ie : 1;
    VGHWuint wcnt_ov_ie : 1;
} VGHW_PND_IE_TypeDef;

typedef struct {
    VGHWuint gpu_start : 1;
    VGHWuint gpu_pause : 1;
    VGHWuint gpu_status : 2;
    VGHWuint mark_auto_del : 1;
    VGHWuint mark_clean : 1;
    VGHWuint reserved : 2;
    VGHWuint sta0 : 4;
    VGHWuint sta1 : 4;
    VGHWuint sta2 : 4;
    VGHWuint sta3 : 4;
} VGHW_STA_CON_TypeDef;

typedef struct {
    VGHWuint out_color_fmt : 5;
    VGHWuint out_adr_mode : 1;
    VGHWuint max_bez_div : 3;
    VGHWuint max_msaa_lvl : 2;
    VGHWuint dither_en : 1;
    VGHWuint bg_mode : 2;
    VGHWuint out_rb_swap : 1;
    VGHWuint out_premult : 1;
    VGHWuint cache_size : 2;
    VGHWuint sp_num : 1;
} VGHW_WIN_CON_TypeDef;

typedef struct {
    VGHWuint win_base_adr;
} VGHW_WIN_BASE_TypeDef;

typedef struct {
    VGHWuint win_stride : 20;
} VGHW_WIN_STRIDE_TypeDef;

typedef struct {
    VGHWuint win_x_min : 12;
    VGHWuint reserved0 : 4;
    VGHWuint win_x_max : 12;
} VGHW_WIN_X_TypeDef;


typedef struct {
    VGHWuint win_y_min : 12;
    VGHWuint reserved1 : 4;
    VGHWuint win_y_max : 12;
} VGHW_WIN_Y_TypeDef;

typedef struct {
    VGHWuint inst_base_adr;
} VGHW_INST_BASE_TypeDef;

typedef struct {
    VGHWuint pad_color;
} VGHW_PAD_COLOR_TypeDef;

typedef struct {
    VGHWuint paint_lut_adr : 11;
} VGHW_PLUT_PTR_TypeDef;

typedef struct {
    VGHWuint paint_lut_dat;
} VGHW_PLUT_DAT_TypeDef;


typedef struct {
    VGHW_XBUS_CON0_TypeDef XBUS_CON0;
    VGHW_XBUS_CON1_TypeDef XBUS_CON1;
    VGHW_PND_CON_TypeDef PND_CON;
    VGHW_PND_CLR_TypeDef PND_CLR;
    VGHW_PND_IE_TypeDef PND_IE;
    VGHW_STA_CON_TypeDef STA_CON;
    VGHW_WIN_CON_TypeDef WIN_CON;
    VGHW_WIN_BASE_TypeDef WIN_BASE;
    VGHW_WIN_STRIDE_TypeDef WIN_STRIDE;
    VGHW_WIN_X_TypeDef WIN_X;
    VGHW_WIN_Y_TypeDef WIN_Y;
    VGHW_INST_BASE_TypeDef INST_BASE;
    VGHW_PAD_COLOR_TypeDef PAD_COLOR;
    VGHW_PLUT_PTR_TypeDef PLUT_PTR;
    VGHW_PLUT_DAT_TypeDef PLUT_DAT;

} VGHW_SFR_TypeDef;


typedef struct {
    //0
    struct {
        VGHWuint		blend_mode : 4;
        VGHWuint		paint_mode : 2;
        VGHWuint		msaa_lvl : 2;

        VGHWuint		gb_alpha: 8;
        VGHWuint		seg_cmd_len: 16;
    };

    //1
    VGHWuint next_inst_adr;

    //2
    VGHWuint frag_base_adr;

    //3
    VGHWuint seg_base_adr;

    //4
    struct {
        VGHWuint		act_x_min : 12;
        VGHWuint        reserved0 : 4;
        VGHWuint		act_x_max : 12;
    };

    //5
    struct {
        VGHWuint		act_y_min : 12;
        VGHWuint        reserved1 : 4;
        VGHWuint		act_y_max : 12;
    };

} VGHW_CMD_TypeDef;

typedef struct {
    VGHWuint		trans_m00;
    VGHWuint		trans_m01;
    VGHWuint		trans_m02;
    VGHWuint		trans_m10;
    VGHWuint		trans_m11;
    VGHWuint		trans_m12;
    struct {
        VGHWuint		fill_rule : 1;
        VGHWuint		reserved : 7;
        VGHWuint		seg_byte0 : 8;
        VGHWuint		seg_byte1 : 8;
        VGHWuint		seg_byte2 : 8;
    };
} VGHW_SEG_TypeDef;

typedef struct {
    //0
    struct {
        VGHWuint	paint_type : 2;
        VGHWuint	spread_mode : 2;
        VGHWuint	grad_lut_lvl : 3;

        VGHWuint	reserved0 : 9;
        //TODO
        VGHWuint	grad_lut_base : 11;
    };

    //1
    union {
        struct {
            VGHWuint		inside_radius : 24;
        };

        struct {
            VGHWubyte	pad_blue;
            VGHWubyte	pad_green;
            VGHWubyte	pad_red;
            VGHWubyte	pad_alpha;
        } pad_color;
    };

    VGHWuint		paint_im00;
    VGHWuint		paint_im01;
    VGHWuint		paint_im02;
    VGHWuint		paint_im10;
    VGHWuint		paint_im11;
    VGHWuint		paint_im12;
    VGHWuint		paint_im20;
    VGHWuint		paint_im21;

} VGHW_FragPaint_TypeDef;


typedef struct {
    struct {
        VGHWuint   img_color_fmt : 5;
        VGHWuint   rle_mode : 1; //TODO
        VGHWuint   img_adr_mode : 1; //TODO 0:tile-base 1:line-base
        VGHWuint   quality : 1;
        VGHWuint   premult : 1;
        VGHWuint   rb_swap : 1;
        VGHWuint   clut_format : 2;

        VGHWuint   color_ext : 1; //newly added
        VGHWuint   ckey_en : 1;
        VGHWuint   margin_mode : 1; //newly added
        VGHWuint   reserved0 : 1;
        VGHWuint   ckey_red : 8;
        VGHWuint   ckey_green : 8;

    };

    struct {

        VGHWuint	img_stride : 20;
        VGHWuint	reserved1 : 4;
        VGHWuint	ckey_blue : 8;
    };

    struct {
        VGHWuint	img_width : 16;
        VGHWuint	img_height : 16;
    };

    VGHWuint	img_base_adr;
    union {
        VGHWuint img_lut_adr;
        VGHWuint img_a_color;
    };

    VGHWuint	img_im00;
    VGHWuint	img_im01;
    VGHWuint	img_im02;
    VGHWuint	img_im10;
    VGHWuint	img_im11;
    VGHWuint	img_im12;
    VGHWuint	img_im20;
    VGHWuint	img_im21;

    VGHWuint    rle_limit;
} VGHW_FragImage_TypeDef;

typedef struct {
    VGHWubyte		b;
    VGHWubyte		g;
    VGHWubyte		r;
    VGHWubyte		a;
} VGHW_Color_TypeDef;

typedef struct {
    VGHWint                 x;
    VGHWint                 y;
    VGHWint                 width;
    VGHWint                 height;
} VGHW_Rect_TypeDef;


typedef struct {
    VGHWint					width;
    VGHWint					height;
    VGHWint					stride;
    VGHW_ImageFormat_TypeDef			format;

    VGHWubyte              *data;
    VGHWubyte              *clut;
    VGHWuint				data_size;


    VGHWubyte               premult;
    VGHWubyte               rb_swap;
    VGHW_ImageQuality_TypeDef               quality;
    VGHW_ClutFormat_TypeDef               clut_format;

    VGHWubyte               rle_en;
    VGHWubyte               adr_mode;

    VGHWubyte				ckey_en;
    VGHWubyte				ckey_red;
    VGHWubyte				ckey_green;
    VGHWubyte				ckey_blue;

    VGHWubyte				dither_en;

    VGHWubyte				a_color_alpha;
    VGHWubyte				a_color_red;
    VGHWubyte				a_color_green;
    VGHWubyte				a_color_blue;

    VGHWubyte               bit_extend_mode;
    VGHWubyte               margin_mode;

} VGHW_Image_TypeDef;



VGHW_Handle_TypeDef vghwCreateContext();
void vghwDestroyContext(VGHW_Handle_TypeDef handle);
void vghwRun(VGHW_Handle_TypeDef handle);
VGHW_SFR_TypeDef *vghwGetSfr(VGHW_Handle_TypeDef handle);

VGHWuint vghwGetGradLutPtr(VGHW_Handle_TypeDef handle);
void vghwSetGradLutPtr(VGHW_Handle_TypeDef handle);
void vghwSetGradLutData(VGHW_Handle_TypeDef handle);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
