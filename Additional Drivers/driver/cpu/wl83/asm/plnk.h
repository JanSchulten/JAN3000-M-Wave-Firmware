#ifndef __AUDIO_PLNK_H__
#define __AUDIO_PLNK_H__

#include "generic/typedef.h"

/**
 * \name 可使用PDMLINK模块数量
 * \{
 */
#define		MAX_PLNK_NUM		1	/*!< 可使用PDMLINK的模块数量   */
/* \} name */

/**
 * \name 输入通道选择
 * \{
 */
#define PLNK_MIC_MASK					(BIT(0) | BIT(1)) /*!< 通道掩码 */
#define PLNK_CH_MIC_DAT0				BIT(0)	/*!< 两个MIC共用DAT0也使用该宏 */
#define PLNK_CH_MIC_DAT1				BIT(1)  /*!< 两个MIC共用DAT1也使用该宏 */
#define PLNK_CH_MIC_DOUBLE				(BIT(0) | BIT(1)) /*!< 使用双通道(通道0和通道1) */
/* \} name */

/**
 * @brief enum PLNK通道数据模式
 */
typedef enum {
    DATA0_SCLK_RISING_EDGE_SAMPLE, /*!< 通道0数据在SCLK的上升沿采样 */
    DATA0_SCLK_FALLING_EDGE_SAMPLE,/*!< 通道0数据在SCLK的下降沿采样 */
    DATA1_SCLK_RISING_EDGE_SAMPLE, /*!< 通道1数据在SCLK的上升沿采样 */
    DATA1_SCLK_FALLING_EDGE_SAMPLE,/*!< 通道0数据在SCLK的下降沿采样 */
} PLNK_CH_MD;

/**
 * @brief struct PLNK参数设置结构体
 */
struct plnk_platform_data {
    int sclk_io;                        /*!< SCLK IO */
    int dat0_io;                        /*!< DAT0 IO */
    int dat1_io;                        /*!< DAT1 IO */
    u32 sclk_fre;                       /*!< SCLK 频率，推荐2M */
    u8 ch0_mode;                        /*!< CH0 MD */
    u8 ch1_mode;                        /*!< CH1 MD */
    u8 order;                           /*!< CIC1抽取滤波器的阶数1~3 */
    u8 dfdly_m;                         /*!< CIC1抽取滤波器差分延迟因子1~2 */
    u8 hw_channel;                      /*!< 输入通道选择 */
    u8 share_data_io;                   /*!< 双数字麦是否共用同一个数据输入IO */
    u8 dc_cancelling_filter;            /*!< 去直流滤波器等级0-15 */
    u16 sr_points;                      /*!< 每次中断起来时的采样点数 */
    u16 init_delay_ms;                  /*!< 数字麦初始化后等待SCLK稳定的延时ms */
    u32 dump_points_num;                /*!< 丢弃刚打开硬件时的数据点数 */
};

/**
 * @brief 初始化PLNK
 * @param pd plnk配置参数结构体参数指针
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_open(const struct plnk_platform_data *pd, u8 index);

/**
 * @brief 启动PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_on(u8 index);

/**
 * @brief 停止PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_off(u8 index);

/**
 * @brief 关闭PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_close(u8 index);

/**
 * @brief 设置PLNK采样率
 * @param sample_rate 采样率
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_set_sample_rate(int sample_rate, u8 index);

/**
 * @brief 设置PLNK中断处理函数中的回调函数，用于处理采集到的数据
 * @param priv 回调函数的私有指针
 * @param cb 回调函数
 * @param index 0:plnk0  1:plnk1
 */
void plnk_set_handler_cb(void *priv, void (*cb)(void *, u8 *data, int len), u8 index);

/**
 * @brief PLNK中断处理函数
 * @param index 0:plnk0  1:plnk1
 */
void plnk_irq_handler(u8 index);

#endif

