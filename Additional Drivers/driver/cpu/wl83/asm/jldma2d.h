


#ifndef __JLDMA2D_H__
#define __JLDMA2D_H__


#include "dma2d_rcf.h"

typedef enum {
    JLDMA2D_FORMAT_ARGB8888 = 0x00,
    JLDMA2D_FORMAT_ARGB8565 = 0x01,
    JLDMA2D_FORMAT_ARGB1555 = 0x02,
    JLDMA2D_FORMAT_ARGB4444 = 0x03,
    JLDMA2D_FORMAT_RGB888   = 0x04,
    JLDMA2D_FORMAT_RGB565   = 0x05,

    JLDMA2D_FORMAT_YUV422_BT601 = 0x06,
    JLDMA2D_FORMAT_YUV422_BT709 = 0x07,

    JLDMA2D_FORMAT_AL88     = 0x08,
    JLDMA2D_FORMAT_AL44     = 0x10,
    JLDMA2D_FORMAT_AL22     = 0x11,
    JLDMA2D_FORMAT_L8       = 0x12,
    JLDMA2D_FORMAT_L4       = 0x13,
    JLDMA2D_FORMAT_L2       = 0x14,
    JLDMA2D_FORMAT_L1       = 0x15,
    JLDMA2D_FORMAT_A8       = 0x16,
    JLDMA2D_FORMAT_A4       = 0x17,
    JLDMA2D_FORMAT_A2       = 0x18,
    JLDMA2D_FORMAT_A1       = 0x19,
} jldma2d_format_t;


typedef enum {
    JLDMA2D_CLUT_FORMAT_ARGB8888 = 0x00,
    JLDMA2D_CLUT_FORMAT_ARGB8565 = 0x01,
    JLDMA2D_CLUT_FORMAT_RGB888   = 0x02,
    JLDMA2D_CLUT_FORMAT_RGB565   = 0x05,
} jldma2d_clut_format_t;


#define JLDMA2D_NO_MODIF_ALPHA        0x00000000U  /*!< No modification of the alpha channel value                     */
#define JLDMA2D_REPLACE_ALPHA         0x00000001U  /*!< Replace original alpha channel value by programmed alpha value */
#define JLDMA2D_COMBINE_ALPHA         0x00000002U  /*!< Replace original alpha channel value by programmed alpha value
                                                      with original alpha channel value                              */

#define JLDMA2D_REGULAR_ALPHA         0x00000000U  /*!< No modification of the alpha channel value */
#define JLDMA2D_INVERTED_ALPHA        0x00000001U  /*!< Invert the alpha channel value */

#endif
