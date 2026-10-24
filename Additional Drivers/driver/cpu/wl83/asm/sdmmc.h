#ifndef __ASM_SDMMC_H__
#define __ASM_SDMMC_H__

#include "device/device.h"
#include "sdio/sdmmc.h"

/**
 * \name SDIO platform data
 * \{
 */
struct sdmmc_platform_data {
    char port[6];                   ///<  SDIO，按顺序对应CLK  CMD  SD0  SD1  SD2  SD3
    u8 hd_level;                    ///<  SDIO强驱等级
    u8 index;                       ///<  中断CPU绑定
    u8 port_group;                  ///<  固定IO组A、B、C、D
    u8 irq;                         ///<  中断号
    u8 data_width;                  ///<  1-单线模式；4-四线模式
    u8 priority;                    ///<  中断优先级
    u8 detect_mode;                 ///<  检测模式：命令检测、时钟检测、IO检测
    u8 detect_io;                   ///<  IO检测模式下配置IO
    u8 detect_io_level;             ///<  IO检卡上线的电平(0/1)，IO检测模式下有效
    u8 detect_time_interval;        ///<  定时扫描时间设置
    u8 wait_busy_timeout;           ///<  读写卡超时时间设置
    u16 detect_timeout;             ///<  检测模式超时设置
    u32 speed;                      ///<  SDIO时钟
    volatile u32 *sfr;              ///<  SD卡基本配置寄存器
    int (*detect_func)(const struct sdmmc_platform_data *);            ///<  检测模式下的检测函数，可外部实现。
    void (*port_init)(const struct sdmmc_platform_data *, int mode);   ///<  SDIO初始化函数
    void (*power)(u8 on);           ///<  SDPGAT控制开关函数
};
/* \} name */

/// \cond DO_NOT_DOCUMENT
#define SD0_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_platform_data data = {

#define SD0_PLATFORM_DATA_END() \
	.irq 					= IRQ_SD0_IDX, \
	.index 					= 0, \
    .sfr                    = &JL_SD0->CON0, \
	.port_init 				= sdmmc_port_init, \
	.detect_time_interval 	= 250, \
	.detect_timeout     	= 1000, \
};

#define SD1_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_platform_data data = {

#define SD1_PLATFORM_DATA_END() \
	.irq 					= IRQ_SD1_IDX, \
	.index 					= 1, \
    .sfr                    = &JL_SD1->CON0, \
	.port_init 				= sdmmc_port_init, \
	.detect_time_interval 	= 250, \
	.detect_timeout     	= 1000, \
};

void sdmmc_port_init(const struct sdmmc_platform_data *, int mode);

int sdmmc_clk_detect(const struct sdmmc_platform_data *);

int sdmmc_io_detect(const struct sdmmc_platform_data *);

int sdmmc_cmd_detect(const struct sdmmc_platform_data *);

void sd_set_power(u8 enable);
/// \endcond

/**
 * \name dmsdx platform data
 * \{
 */
struct dmsdx_platform_data {
    const char *dmsdx_name;         ///<  MSDx标识符
    u32 block_offset_start;         ///<  分区BLOCK偏移量起始
    u32 block_offset_end;           ///<  分区BLOCK偏移量结束块
    struct device *dmsdx_device;    ///<  存储device设备指针
    struct device dev;              ///<  存储device设备结构体
    struct list_head entry;         ///<  链表
};
/* \} name */

/// \cond DO_NOT_DOCUMENT
extern const struct device_operations sd_dev_ops;
extern const struct device_operations dmsd_dev_ops;
/// \endcond

#endif

