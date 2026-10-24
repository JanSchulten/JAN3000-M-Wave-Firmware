#ifndef __DEVICE_TOUCH_KEY_H__
#define __DEVICE_TOUCH_KEY_H__

#include "generic/typedef.h"

/**
 * \name 计数参考时钟选择, 频率越高, 精度越高
 * \{
 */
typedef enum {
    TOUCH_KEY_CLK_STD_48M = 0,
    TOUCH_KEY_CLK_MUX_IN,
    TOUCH_KEY_CLK_PLL_480M,
    TOUCH_KEY_CLK_ALLPLL_D1P0,
} touch_key_clk_t;
/* \} name */

/**
 * \name touch_key_port_config
 * \{
 */
struct touch_key_port {
    u8 port;            ///< 触摸按键IO
    u8 key_value;       ///< 按键返回值
    u16 press_delta;    ///< 按下判决门限
};
/* \} name */

/**
 * \name touch_key_platform_data
 * \{
 */
struct touch_key_platform_data {
    u8 num;             ///< 触摸按键个数
    u8 change_gain;     ///< 变化放大倍数
    s16 release_cfg0;   ///< 释放判决门限0
    s16 release_cfg1;   ///< 释放判决门限1
    touch_key_clk_t clk_src;    ///< 时钟源选择
    const struct touch_key_port *port_list; ///< 触摸按键列表
};
/* \} name */

#endif

