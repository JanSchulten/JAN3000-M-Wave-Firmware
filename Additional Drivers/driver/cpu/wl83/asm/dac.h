#ifndef __AUDIO_DAC_H__
#define __AUDIO_DAC_H__

#include "generic/typedef.h"

/**
 * \name DAC通道选择
 * \{
 */
#define AUDIO_DAC_CH_L          BIT(0) /*!< 通道0 左声道 */
#define AUDIO_DAC_CH_R          BIT(1) /*!< 通道1 右声道 */
/* \} name */

/**
 * \name 淡入淡出状态
 * \{
 */
#define FADE_DISABLE            0   /*!< 淡入淡出失能 */
#define FADE_VOL_OUT            1   /*!< 淡入淡出模拟音量淡出 */
#define FADE_VOL_IN             2   /*!< 淡入淡出模拟音量淡入 */
#define FADE_VOL_UPDOWN         3   /*!< 淡入淡出模拟音量调节 */
/* \} name */

/**
 * @brief struct AUDIO_DAC参数设置结构体
 */
struct dac_platform_data {
    int pa_mute_port;           /*!< 功放MUTE IO */
    u8 pa_auto_mute;            /*!< 关闭DAC时是否自动MUTE功放  0:不mute功放 1:MUTE功放 */
    u8 pa_mute_value;           /*!< MUTE电平值 0:低电平 1:高电平 */
    u8 trim_en;                 /*!< 是否每次开机都trim dac */
    u8 differ_output;           /*!< 是否使用硬件差分输出模式  0:不使用 1:使用 */
    u8 hw_channel;              /*!< 硬件DAC模拟通道 */
    u8 ch_num;                  /*!< 数字通道，软件输出通道数 */
    u8 auto_src_enable;         /*!< 根据当前解码的最高采样率自动调整最佳DAC采样率 */
    u8 fade_enable;             /*!< 数字音量淡入淡出使能位 */
    u8 fade_points_step;        /*!< 数字音量淡入淡出每隔多少个采样点更新一次音量 */
    u8 fade_volume_step;        /*!< 数字音量淡入淡出音量每次更新的步进 */
    u16 mute_delay_ms;          /*!< MUTE延时 */
    u16 fade_delay_ms;          /*!< 模拟音量淡入淡出延时 */
    u16 sr_points;              /*!< 多少个采样点进一次中断，一般不建议修改 */
    u16 vcm_init_delay_ms;      /*!< VCM电压初始化后等待稳定的延时ms */
    u32 poweron_delay;          /*!< 低功耗睡眠时恢复DAC模块的延迟us */
    void (*pa_mute_cb)(u8 mute);/*!< 功放mute用户操作回调 */
};

/**
 * @brief dac_early_init, dac初始化
 *
 * @param hw_channel 硬件dac通道配置BIT(x)
 * @param vcm_cap VCM是否外挂电容
*/
void dac_early_init(u8 hw_channel, u8 vcm_cap);

/**
 * @brief 打开dac模块
 *
 * @param struct dac_platform_data *pd dac配置参数结构体参数指针
*/
int dac_open(const struct dac_platform_data *pd);

/**
 * @brief dac模块使能
*/
int dac_on(void);

/**
 * @brief dac模块失能
*/
int dac_off(void);

/**
 * @brief 关闭dac模块
*/
int dac_close(void);

/**
 * @brief 低功耗休眠起来后恢复dac模块
*/
void dac_poweron(void);

/**
 * @brief 低功耗进入前挂起dac模块
*/
void dac_poweroff(void);

/**
 * @brief dac数字音量设置
 *
 * @param volume 音量0-100
*/
void dac_set_volume(s16 volume);

/**
 * @brief dac采样率设置
 *
 * @param sample_rate 采样率
*/
int dac_set_sample_rate(u32 sample_rate);

/**
 * @brief 获取当前dac采样率
 *
 * @return DAC当前采样率
*/
int dac_get_sample_rate(void);

/**
 * @brief dac中断函数
*/
void dac_irq_handle(void);

/**
 * @brief dac中断的回调函数设置
 *
 * @param priv 传入指针
 * @param handler 传入的回调函数
*/
void dac_set_data_handler(void *priv, void (*handler)(void *, u8 *data, int len));

/**
 * @brief dac模拟增益设置
 *
 * @param gain 模拟增益0-31
 * @param fade_opt 淡入淡出状态位
 * @note 本芯片无效
*/
void dac_set_gain(u8 gain, u8 fade_opt);

/**
 * @brief dac直通模拟增益设置
 *
 * @param gain 模拟增益0-31
 * @note 本芯片无效
*/
void dac_direct_set_gain(u8 gain);

/**
 * @brief dac模拟增益淡入淡出设置
 *
 * @param enable 0：关闭 1：使能
 * @note 本芯片无效
*/
void dac_fade_inout_set(u8 enable);

/**
 * @brief 读取DAC DMA BUF的单通道数据
 *
 * @param read_pos 读指针
 * @param points_offset 偏移量
 * @param data 数据指针
 * @param len 数据长度
 *
 * @return 数据长度
*/
int audio_dac_read_single_channel(u16 *read_pos, s16 points_offset, void *data, int len);

#endif

