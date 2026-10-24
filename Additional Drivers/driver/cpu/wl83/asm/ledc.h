#ifndef __ASM_LEDC_H__
#define __ASM_LEDC_H__
#include "typedef.h"


/**
 * \name LEDC dev_ioctl funciton selest
 * \{
 */
#define LEDC_MAGIC                  'L'
#define IOCTL_LEDC_SET_LOOP_TIMES   _IOW(LEDC_MAGIC, 1, u16)             ///< 设置帧循环发送次数(循环中不会发送rst信号)。设置范围0 ~ 0xFFFF
#define IOCTL_LEDC_SET_TX_ASYNC     _IOW(LEDC_MAGIC, 2, u8)              ///< 设置异步发送。0:同步, 1:异步
#define IOCTL_LEDC_WAIT_TX_END       _IO(LEDC_MAGIC, 3)                  ///< (异步发送时)等待发送完成
#define IOCTL_LEDC_SET_IRQ_CB       _IOW(LEDC_MAGIC, 4, void (*)(void))  ///< 设置中断回调函数
/* \} name */


/**
 * \name ledc idle signal level config
 * \{
 */
typedef enum {
    LEDC_IDLE_LEVEL_L,                   ///< LEDC输出空闲信号电平-低
    LEDC_IDLE_LEVEL_H,                   ///< LEDC输出空闲信号电平-高
} ledc_idle_level_t;
/* \} name */


/**
 * \name ledc signal inversion config
 * \{
 */
typedef enum {
    LEDC_OUT_INV_NREV,                   ///< LEDC输出信号电平不反转
    LEDC_OUT_INV_REV,                    ///< LEDC输出信号电平反转
} ledc_out_inv_t;
/* \} name */


/**
 * \name ledc data bits inversion config
 * \{
 */
typedef enum {
    LEDC_BIT_INV_NO,                     ///< LEDC数据BIT不反转
    LEDC_BIT_INV_8BIT,                   ///< LEDC数据 8BIT反转。(0xaa <--> 0x55)
    LEDC_BIT_INV_16BIT,                  ///< LEDC数据16BIT反转。(0xaabb <--> 0xdd55)
    LEDC_BIT_INV_32BIT,                  ///< LEDC数据32BIT反转。(0xaabbccdd <--> 0xbb33dd55)
} ledc_bit_inv_t;
/* \} name */


/**
 * \name ledc time base config
 * \{
 */
typedef enum {
    LEDC_TIME_BASE_21NS,                 ///< LEDC时基21ns
    LEDC_TIME_BASE_42NS,                 ///< LEDC时基42ns
    LEDC_TIME_BASE_63NS,                 ///< LEDC时基63ns
    LEDC_TIME_BASE_125NS,                ///< LEDC时基125ns
    LEDC_TIME_BASE_250NS,                ///< LEDC时基250ns
    LEDC_TIME_BASE_500NS,                ///< LEDC时基500ns
    LEDC_TIME_BASE_1US,                  ///< LEDC时基1us
    LEDC_TIME_BASE_2US,                  ///< LEDC时基2us
    LEDC_TIME_BASE_4US,                  ///< LEDC时基4us
} ledc_time_base_t;
/* \} name */


/**
 * \name ledc platform data
 * \{
 */
typedef struct {
    u8 index;                            ///< 控制器号
    u8 t1h_cnt;                          ///< 1码的高电平时间 = t1h_cnt * time_base，比如4 * 42ns = 168ns
    u8 t1l_cnt;                          ///< 1码的低电平时间 = t1l_cnt * time_base
    u8 t0h_cnt;                          ///< 0码的高电平时间 = t0h_cnt * time_base
    u8 t0l_cnt;                          ///< 0码的低电平时间 = t0l_cnt * time_base
    u8 pnd_time_gap;                     ///< 每一帧最后一bit到起pnding的间隔时间 = pnd_time_gap * time_base
    u32 rst_time_cnt;                    ///< 复位信号时间 = rst_time_cnt * time_base
    int port;                            ///< 输出引脚(使用crossbar)
    ledc_idle_level_t idle_level;        ///< 空闲电平
    ledc_out_inv_t out_inv;              ///< 输出信号反转
    ledc_bit_inv_t bit_inv;              ///< 输出数据取反
    ledc_time_base_t time_base;          ///< 时基
} ledc_platform_data_t;
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define LEDC0_PLATFORM_DATA_BEGIN(ledc0_data) \
    static const ledc_platform_data_t ledc0_data = { \
        .index = 0, \

#define LEDC0_PLATFORM_DATA_END() \
};


#define LEDC1_PLATFORM_DATA_BEGIN(ledc1_data) \
    static const ledc_platform_data_t ledc1_data = { \
        .index = 1, \

#define LEDC1_PLATFORM_DATA_END() \
};


#define LEDC2_PLATFORM_DATA_BEGIN(ledc2_data) \
    static const ledc_platform_data_t ledc2_data = { \
        .index = 2, \

#define LEDC2_PLATFORM_DATA_END() \
};


extern const struct device_operations ledc_dev_ops;
/// \endcond

#endif

