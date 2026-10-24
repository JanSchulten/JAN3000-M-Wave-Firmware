#ifndef __LCD_COMMON_H__
#define __LCD_COMMON_H__

#include "typedef.h"

//<帧模式>
enum BUFFER_MODE {
    BUFFER_FRAME,
    BUFFER_DOUBLE,
};

//<扫描方式>
enum INTERLACED {
    INTERLACED_NONE,		//非隔行
    INTERLACED_TIMING,		//时序隔行
    INTERLACED_DATA,		//数据隔行
    INTERLACED_ALL,			//(INTERLACED_DATA|INTERLACED_TIMING)时序数据隔行
};

//<input format>
enum IN_FORMAT {
    SAMP_YUV420,
    SAMP_YUV422,
    SAMP_YUV444,
    SAMP_YUV422_SPECIAL,
    SAMP_RGB_TO_MIPI,
};

//<输出数据格式>
enum OUT_FORMAT {
    FORMAT_RGB333 = 2,
    FORMAT_RGB666,
    FORMAT_RGB888,
    FORMAT_RGB565,
    FORMAT_YUV888,
    FORMAT_YUV422,
    FORMAT_ITU656, //YUV422同时使能
};

//<访问长度>
enum LEN_CFG {
    LEN_512,
    LEN_256,
};

enum ODD_EVEN_FILED {
    ODD_FILED, /*默认*/
    EVEN_FILED,
    SAME_FILED,
};

//<旋转参数配置>
enum ROTATE_CONFIG {
    ROTATE_0 = 0,
    ROTATE_90 = 90,
    ROTATE_180 = 180,
    ROTATE_270 = 270,
};


struct color_correct {
    float r_gain;
    float g_gain;
    float b_gain;
    float bright_gain;
    float contrast_gain;
    float saturation_gain;
    float mean;
    float angle;
    float mode;
};

/*
 *TIPS:由于avout对时钟的要求比较高,想要比较好的效果,建议使用12M的晶振
 * */
enum PLL4_SRC {
    PLL4_SRC_XOSC0 = 1, /*PG14、PG15脚挂12M晶振*/
    PLL4_SRC_XOSC1,     /*PB14、PB15脚挂12M晶振*/
    PLL4_SRC_RTCOSC,
    PLL4_SRC_HTC,
    PLL4_SRC_OSC,
    PLL4_SRC_x12M,
};

enum LCD_IF {
    LCD_MCU_SINGLE_FRAME,
    LCD_MIPI,
    LCD_MCU,
    LCD_RGB,
    LCD_SPI,
};

struct pll4_info {
    u16 pll_freq;
};

struct basic_info {
    u16 x;                                     ///< 显存在屏的水平方向偏移
    u16 y;                                     ///< 显存在屏的垂直方向偏移
    u16 xres;                                  ///< 显存水平分辨率
    u16 yres;                                  ///< 显存垂直分辨率
    u16 target_xres;                           ///< 屏水平分辨率
    u16 target_yres;                           ///< 屏垂直分辨率
    u8 interpolation_en;                       ///< 屏幕插值使能

    enum LCD_IF drive_mode;                    ///< 驱动模式
    u8 in_rbs;                                 ///< 输入数据RB交换
    u8 in_bt601;                               ///< yuv422输入转换
    u8 out_rbs;                                ///< 输出数据RB交换
    u8 dither_en;                              ///< 0低位截断，1 dither
    u8 in_fmt;                                 ///< 输入数据格式
    u8 out_fmt;                                ///< 输出数据格式
    u8 out_bt601;                              ///< yuv422输出转换

    u32 buf_addr;                              ///< 显示缓存
    enum BUFFER_MODE mode;
    enum INTERLACED interlaced_mode;           ///< 隔行模式

    u8  test_mode;                             ///< 测试模式(纯色)使能
    u32 test_mode_color;                       ///< 测试模式颜色设置
    u32 bg_color;                              ///< 背景颜色

    enum IN_FORMAT sample;                     ///< 采样方式 YUV420/YUV422/YUV444
    enum OUT_FORMAT format;                    ///< 输出数据格式
    struct color_correct adjust;               ///< 颜色校正参数

    enum LEN_CFG len;                          ///< 访问的块长度
    enum ODD_EVEN_FILED interlaced_1st_filed;  ///< 首场是奇场or偶场,INTERLACED_DATA 时有效

    u16 rotate;                                ///< 旋转角度(90.0 / 180.0 / 270.0)
};

#endif
