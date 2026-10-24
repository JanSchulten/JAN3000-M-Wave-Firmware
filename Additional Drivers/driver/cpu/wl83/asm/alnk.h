#ifndef __AUDIO_ALNK_H__
#define __AUDIO_ALNK_H__

#include "generic/typedef.h"

/**
 * \name 可使用ALNK模块数量
 * \{
 */
#define	MAX_ALNK_NUM                   1
/* \} name */

/**
 * \name ALNK输入通道
 * \{
 */
#define	ALNK_INPUT_CHANNEL0            BIT(0) /*!< 输入通道0*/
#define	ALNK_INPUT_CHANNEL1            BIT(1) /*!< 输入通道1*/
#define	ALNK_INPUT_CHANNEL2            BIT(2) /*!< 输入通道2*/
#define	ALNK_INPUT_CHANNEL3            BIT(3) /*!< 输入通道3*/
/* \} name */

/**
 * \name ALNK输出通道
 * \{
 */
#define	ALNK_OUTPUT_CHANNEL0           BIT(0) /*!< 输出通道0*/
#define	ALNK_OUTPUT_CHANNEL1           BIT(1) /*!< 输出通道1*/
#define	ALNK_OUTPUT_CHANNEL2           BIT(2) /*!< 输出通道2*/
#define	ALNK_OUTPUT_CHANNEL3           BIT(3) /*!< 输出通道3*/
/* \} name */

/**
 * \name ALNK数据位宽
 * \{
 */
#define	ALNK_DATA_WIDTH_24BIT_CHANNEL0 BIT(0) /*!< 通道0使用24bit数据位宽*/
#define	ALNK_DATA_WIDTH_24BIT_CHANNEL1 BIT(1) /*!< 通道1使用24bit数据位宽*/
#define	ALNK_DATA_WIDTH_24BIT_CHANNEL2 BIT(2) /*!< 通道2使用24bit数据位宽*/
#define	ALNK_DATA_WIDTH_24BIT_CHANNEL3 BIT(3) /*!< 通道3使用24bit数据位宽*/
/* \} name */

/**
 * \name ALNK工作模式
 * \{
 */
#define	ALNK_BASIC_IIS_MODE            1 /*!< IIS模式 */
#define	ALNK_BASIC_LEFT_MODE           2 /*!< 左对齐模式 */
#define	ALNK_BASIC_RIGHT_MODE          3 /*!< 右对齐模式 */
#define	ALNK_EXTEND_SHORT_FRAME_DSP0_MODE (BIT(2) | 1) /*!< DSP0模式 */
#define	ALNK_EXTEND_SHORT_FRAME_DSP1_MODE (BIT(2) | 2) /*!< DSP1模式 */
/* \} name */

/**
 * \name ALNK MCLK时钟源选择
 * \{
 */
#define	ALNK_MCLK_FROM_EXTEND_INPUT    0 /*!< 从外部输入MCLK */
#define	ALNK_MCLK_FROM_SYS_CLOCK       1 /*!< 系统时钟 */
#define	ALNK_MCLK_FROM_OSC_CLOCK       2 /*!< OSC CLK */
#define	ALNK_MCLK_FROM_PLL_ALNK_CLOCK  3 /*!< PLL ALNK CLK */
/* \} name */

/**
 * \name ALNK DMA工作模式
 * \{
 */
#define	ALNK_DMA_PINGPANG_BUFFER       0 /*!< 乒乓缓存 */
#define	ALNK_DMA_CIRCULAR_BUFFER       1 /*!< 循环缓存 */
/* \} name */


/**
 * @brief struct ALNK参数设置结构体
 */
struct alnk_platform_data {
    int port[7];               /*!< [0]MCLK [1]LRCLK [2]SCLK [3]DAT0 [4]DAT1 [5]DAT2 [6]DAT3 */
    u8 channel_in;             /*!< 输入通道 BIT(X) */
    u8 channel_out;            /*!< 输出通道 BIT(X) */
    u8 data_width;             /*!< 数据位宽 BIT(X)*/
    u8 mode;                   /*!< 工作模式 */
    u8 dma_mode;               /*!< 0:乒乓缓存 1:循环缓存 */
    u8 slave_mode;             /*!< 0:主机模式 1:从机模式 */
    u8 mclk_src;               /*!< mclk时钟源选择 */
    u8 update_edge;            /*!< 1:SCLK上升沿更新数据，下降沿采样数据 0:SCLK下降沿更新数据，上升沿采样数据 */
    u8 data_sel;               /*!< 当输入单通道数据时选择获取的是alnk模块的左通道数据还是右通道数据   0:左通道   1:右通道 */
    u8 dec_width_16_to_24;     /*!< 将解码器输出的16bit数据扩展成alnk的24bit输出 */
    u8 enc_width_24_to_32;     /*!< 将alnk输入的24bit数据扩展成32bit输出到编码器 */
    u8 keep_alive;             /*!< 保持alnk模块不关闭一直输出时钟 */
    u16 dump_points_num;       /*!< 丢弃刚打开alnk硬件时的数据点数 */
    u16 sr_points;             /*!< 多少个采样点进一次中断，一般不建议修改 */
    u8 f32e;                   /*!< 是否把每帧数据的SCLK个数设成32 */
};

/**
 * @brief alnk打开通道
 *
 * @param channel 通道 BIT(x)
 * @param index 0：alnk0 1：alnk1
 */
void alnk_channel_on(u8 channel, u8 index);

/**
 * @brief alnk关闭通道
 *
 * @param channel 通道 BIT(x)
 * @param index 0：alnk0 1：alnk1
 */
void alnk_channel_off(u8 channel, u8 index);

/**
 * @brief 打开alnk
 *
 * @param alnk_platform_data *pd alnk配置参数结构体参数指针
 * @param index 0：alnk0 1：alnk1
 *
 * @return 0:成功  -1:失败
 */
int alnk_open(struct alnk_platform_data *pd, u8 index);

/**
 * @brief 关闭alnk
 *
 * @param index 0：alnk0 1：alnk1
 */
void alnk_close(u8 index);

/**
 * @brief alnk采样率设置
 *
 * @param sample_rate 采样率
 * @param index 0：alnk0 1：alnk1
 *
 * @return 0:成功  -1:失败
 */
int alnk_set_sample_rate(int sample_rate, u8 index);

/**
 * @brief alnk设置输入通道数据的中断回调
 *
 * @param priv 传入指针
 * @param cb 回调函数
 * @param index 0：alnk0 1：alnk1
 */
void alnk_set_enc_data_handler(void *priv, void (*cb)(void *, u8 *data, int len, u8), u8 index);

/**
 * @brief alnk设置输出通道数据的中断回调
 *
 * @param priv 传入指针
 * @param cb 回调函数
 * @param index 0：alnk0 1：alnk1
 */
void alnk_set_dec_data_handler(void *priv, void (*cb)(void *, u8 *data, int len, u8), u8 index);

/**
 * @brief alnk中断函数
 *
 * @param index 0：alnk0 1：alnk1
 */
void alnk_irq_handler(u8 index);

#endif

