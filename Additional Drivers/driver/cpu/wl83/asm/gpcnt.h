#ifndef __ASM_GPCNT_H__
#define __ASM_GPCNT_H__

/*
 * 该模块使用说明
 * 选择一个时钟源进行计算另一个时钟源输入的脉冲数量
 * 使用方法如gpnt_test();
 */
#include "generic/typedef.h"
#include "generic/ioctl.h"


/**
 * \name GPCNT dev_ioctl funciton selest
 * \{
 */
#define GPCNT_MAGIC									'G'
#define IOCTL_GPCNT_GET_CNT							_IOR(GPCNT_MAGIC, 1, u32 *)					///<  获取另一时钟源输入的脉冲数量
#define IOCTL_GPCNT_SET_CYCLE						_IOW(GPCNT_MAGIC, 2, u8)					///<  设置主时钟周期
#define IOCTL_GPCNT_SET_GPIO						_IOW(GPCNT_MAGIC, 3, int)					///<  设置次时钟输入IO
#define IOCTL_GPCNT_SET_GSS_CLK_SOURCE				_IOW(GPCNT_MAGIC, 4, gpcnt_clk_source_t)	///<  设置主时钟源
#define IOCTL_GPCNT_SET_CSS_CLK_SOURCE				_IOW(GPCNT_MAGIC, 5, gpcnt_clk_source_t)	///<  设置次时钟源


/* \} name */

/**
 * \name GPCNT CLK source
 * \{
 */
typedef enum {
    GPCNT_GSS_CSS_RC_16M = 1,
    GPCNT_GSS_CSS_RC_250K,
    GPCNT_GSS_CSS_LRC_CLK,
    GPCNT_GSS_CSS_RTC_OSC,
    GPCNT_GSS_CSS_HSB_CLK,
    GPCNT_GSS_CSS_LSB_CLK,
    GPCNT_GSS_CSS_P33_CLK_DBG,
    GPCNT_GSS_CSS_XOSC_24M,
    GPCNT_GSS_CSS_PAT_CLK,
    GPCNT_GSS_CSS_ALLPLL_D1P0,
    GPCNT_GSS_CSS_PLL_480M,
    GPCNT_GSS_CSS_PLL_192M,
    GPCNT_GSS_CSS_PLL_160M,
    GPCNT_GSS_CSS_PLL_120M,
    GPCNT_GSS_CSS_PLL_96M,
    GPCNT_GSS_CSS_STD_48M,
    GPCNT_GSS_CSS_STD_24M,
    GPCNT_GSS_CSS_ASS_DBG_CLK,
    GPCNT_GSS_CSS_INPUT_CAP_MUX,
    GPCNT_GSS_CSS_INPUT_CLK_MUX,
} gpcnt_clk_source_t;
/* \} name */

/**
 * \name GPCNT platform data
 * \{
 */
struct gpcnt_platform_data {
    int gpio;
    gpcnt_clk_source_t gss_clk_source;  ///<  主时钟源
    gpcnt_clk_source_t css_clk_source;  ///<  次时钟源
    u8 cycle;                           ///<  主时钟周期数 = 32*2^CYCLE_GPCNT，取值范围0-15
};
/* \} name */

extern const struct device_operations gpcnt_dev_ops;

#endif
