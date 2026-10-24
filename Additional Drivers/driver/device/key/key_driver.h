#ifndef __KEY_DRIVER_H__
#define __KEY_DRIVER_H__

#include "generic/typedef.h"
#include "event/key_event.h"

/**
 * \name 按键类型
 * \{
 */
typedef enum {
    KEY_DRIVER_TYPE_IO = 0x0,
    KEY_DRIVER_TYPE_AD,
    KEY_DRIVER_TYPE_RTCVDD_AD,
    KEY_DRIVER_TYPE_IR,
    KEY_DRIVER_TYPE_TOUCH,
    KEY_DRIVER_TYPE_CTMU_TOUCH,
    KEY_DRIVER_TYPE_RDEC,
    KEY_DRIVER_TYPE_SLIDEKEY,
    KEY_DRIVER_TYPE_SOFTKEY,
    KEY_DRIVER_TYPE_BRIGHTNESS,
    KEY_DRIVER_TYPE_UART,
    KEY_DRIVER_TYPE_ASR,
    KEY_DRIVER_TYPE_AVCTP,

    KEY_DRIVER_TYPE_MAX,
} key_driver_type_t;
/* \} name */

/**
 * \name 按键扫描参数
 * \{
 */
struct key_driver_para {
    const u32 scan_time;    ///< 按键扫描频率, 单位ms
    u8 last_key;            ///< 上一次get_value按键值
//== 用于消抖类参数
    u8 filter_value;        ///< 用于按键消抖
    u8 filter_cnt;          ///< 用于按键消抖时的累加值
    const u8 filter_time;   ///< 当filter_cnt累加到base_cnt值时, 消抖有效
//== 用于判定长按和HOLD事件参数
    const u16 long_time;    ///< 按键判定长按数量
    const u16 hold_time;    ///< 按键判定HOLD数量
    u16 press_cnt;          ///< 与long_time和hold_time对比, 判断long_event和hold_event
//== 用于判定连击事件参数
    u8 click_cnt;           ///< 单击次数
    u8 click_delay_cnt;     ///< 按键被抬起后等待连击事件延时计数
    const u8 click_delay_time; ///< 按键被抬起后等待连击事件延时数量
    u8 notify_value;       ///< 在延时的待发送按键值
    u8 key_type;           ///< 按键类型
};
/* \} name */

/// \cond DO_NOT_DOCUMENT
//组合按键映射按键值
struct key_remap {
    u8 bit_value;
    u8 remap_value;
};

struct key_remap_data {
    u8 remap_num;
    const struct key_remap *table;
};

struct key_driver_ops {
    const struct key_driver_para *param;
    u8(*get_value)(void);
    int (*key_init)(void);
};

#define REGISTER_KEY_DRIVER_OPS(key_ops) \
        static const struct key_driver_ops key_ops SEC_USED(.key_operation)

extern const struct key_driver_ops key_driver_ops_begin[];
extern const struct key_driver_ops key_driver_ops_end[];

#define list_for_each_key_driver(p) \
    for (p = key_driver_ops_begin; p < key_driver_ops_end; p++)

/// \endcond


/* --------------------------------------------------------------------------*/
/**
 * @brief 按键初始化函数，初始化所有注册的按键驱动
 */
/* ----------------------------------------------------------------------------*/
int key_driver_init(void);

#endif

