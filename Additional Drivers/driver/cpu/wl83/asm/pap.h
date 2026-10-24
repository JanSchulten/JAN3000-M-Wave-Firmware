#ifndef __ASM_PAP_H__
#define __ASM_PAP_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"
#include "os/os_api.h"

/**
 * \name pap dev_ioctl funciton command
 * \{
 */
#define PAP_MAGIC                      'P'
#define IOCTL_PAP_ENABLE               _IO(PAP_MAGIC, 1)                     ///< 使能PAP
#define IOCTL_PAP_DISABLE              _IO(PAP_MAGIC, 2)                     ///< 关闭PAP
#define IOCTL_PAP_SET_IRQ_CB           _IOW(PAP_MAGIC, 3, void (*)(void))    ///< 设置中断回调函数
#define IOCTL_PAP_SET_DMA_WRITE_ASYNC  _IOW(PAP_MAGIC, 4, u8)                ///< 设置是否异步dma发送
/* \} name */


/**
 * \name pap data width config
 * \{
 */
typedef enum {
    PAP_DATAWIDTH_8BIT,   ///< 数据位宽:8bit模式
    PAP_DATAWIDTH_16BIT,  ///< 数据位宽:16bit模式
} pap_datawidth_t;
/* \} name */


/**
 * \name pap endian config
 * \{
 */
typedef enum {
    PAP_ENDIAN_LE,        ///< 小端。位宽为8bit时，数据在低8位IO输出；位宽为16bit时，低位数据在低端口IO输出
    PAP_ENDIAN_BE,        ///< 大端。位宽为8bit时，数据在高8位IO输出；位宽为16bit时，低位数据在高端口IO输出
} pap_endian_t;
/* \} name */


/**
 * \name pap tx/rx (data number under 3 bytes) cycle times
 * \{
 */
typedef enum {
    PAP_CYCLE_ONE,        ///< 循环收/发一次
    PAP_CYCLE_TWO,        ///< 循环收/发两次
} pap_cycle_t;
/* \} name */


/**
 * \name pap RD pin polarity
 * \{
 */
typedef enum {
    PAP_RD_LEVEL_LOW,     ///< 读信号：空闲1电平，有效0电平
    PAP_RD_LEVEL_HIGH,    ///< 读信号：空闲0电平，有效1电平
} pap_pre_t;
/* \} name */


/**
 * \name pap WR pin polarity
 * \{
 */
typedef enum {
    PAP_WR_LEVEL_LOW,     ///< 写信号：空闲1电平，有效0电平
    PAP_WR_LEVEL_HIGH     ///< 写信号：空闲0电平，有效1电平,
} pap_pwe_t;
/* \} name */


/**
 * \name pap data port select
 * \{
 */
typedef enum {
    PAP_PORT_A,           ///< PAP_D0->PA04 ~ PAP_D7->PA11, PAP_D8->PB0 ~ PAP_D15->PB07
    PAP_PORT_B,           ///< PAP_D0->PB10 ~ PAP_D5->PB15, PAP_D6->PC0 ~ PAP_D7->PC1, PAP_D8->PB0 ~ PAP_D15->PB7
} pap_port_sel_t;
/* \} name */


/**
 * \name pap setup timing config
 * \{
 */
typedef enum {
    PAP_TS_0_CLK,         ///< 建立时间0个系统时钟宽度
    PAP_TS_1_CLK,         ///< 建立时间1个系统时钟宽度
    PAP_TS_2_CLK,         ///< 建立时间2个系统时钟宽度
    PAP_TS_3_CLK,         ///< 建立时间3个系统时钟宽度
} pap_ts_t;
/* \} name */


/**
 * \name pap hold timing config
 * \{
 */
typedef enum {
    PAP_TH_0_CLK,         ///< 保持时间0个系统时钟宽度
    PAP_TH_1_CLK,         ///< 保持时间1个系统时钟宽度
    PAP_TH_2_CLK,         ///< 保持时间2个系统时钟宽度
    PAP_TH_3_CLK,         ///< 保持时间3个系统时钟宽度
} pap_th_t;
/* \} name */


/**
 * \name pap WR/RD signel timing config
 * \{
 */
typedef enum {
    PAP_TW_16_CLK,        ///< 读写使能信号16个系统时钟宽度
    PAP_TW_1_CLK,         ///< 读写使能信号1个系统时钟宽度
    PAP_TW_2_CLK,         ///< 读写使能信号2个系统时钟宽度
    PAP_TW_3_CLK,         ///< 读写使能信号3个系统时钟宽度
    PAP_TW_4_CLK,         ///< 读写使能信号4个系统时钟宽度
    PAP_TW_5_CLK,         ///< 读写使能信号5个系统时钟宽度
    PAP_TW_6_CLK,         ///< 读写使能信号6个系统时钟宽度
    PAP_TW_7_CLK,         ///< 读写使能信号7个系统时钟宽度
    PAP_TW_8_CLK,         ///< 读写使能信号8个系统时钟宽度
    PAP_TW_9_CLK,         ///< 读写使能信号9个系统时钟宽度
    PAP_TW_10_CLK,        ///< 读写使能信号10个系统时钟宽度
    PAP_TW_11_CLK,        ///< 读写使能信号11个系统时钟宽度
    PAP_TW_12_CLK,        ///< 读写使能信号12个系统时钟宽度
    PAP_TW_13_CLK,        ///< 读写使能信号13个系统时钟宽度
    PAP_TW_14_CLK,        ///< 读写使能信号14个系统时钟宽度
    PAP_TW_15_CLK,        ///< 读写使能信号15个系统时钟宽度
} pap_tw_t;
/* \} name */



//timing_setup(ts) timing_hold(th) timing_width(tw):数据传输配置
/*
写时钟信号:
                    __________               _________
pwe=H: ____________|          |_____________|         |__________
pwe=L: ____________            _____________           __________
                   |__________|             |_________|

数据信号:
             ________________________                       ______
data:  _____|                        |_____________________|

		    |      |          |      |
ts,tw,th:   |      |          |      |
		    |<-ts->|<---tw--->|<-th->|
		    |      |          |      |

ts:0-3,ts个lsb系统时钟宽度
tw:0-15,0-->16个lsb系统时钟宽度，1-15：tw个lsb系统时钟宽度
th:0-3,th个lsb系统时钟宽度
*/


/**
 * \name pap platform data
 * \{
 */
typedef struct {
    u8 rd_en;                     ///< 是否使用rd引脚读取数据
    int we_port;                  ///< we引脚
    int re_port;                  ///< re引脚
    pap_ts_t timing_setup;        ///< 数据建立时间
    pap_th_t timing_hold;         ///< 数据保持时间
    pap_tw_t timing_width;        ///< 读/写使能信号宽度
    pap_datawidth_t datawidth;    ///< 数据port位宽
    pap_endian_t endian;          ///< 数据端序
    pap_cycle_t cycle;            ///< 数据量小于3byte时，循环收/发次数
    pap_port_sel_t port_sel;      ///< 数据输出端口选择
    pap_pre_t pre;                ///< 读极性
    pap_pwe_t pwe;                ///< 写极性
} pap_platform_data_t;
/* \} name */


/// \cond DO_NOT_DOCUMENT

extern const struct device_operations pap_dev_ops;


#define PAP_PLATFORM_DATA_BEGIN(pap_data) \
    static const pap_platform_data_t pap_data = { \

#define PAP_PLATFORM_DATA_END() \
    };

/// \endcond

#endif
