#ifndef __AUDIO_ADC_H__
#define __AUDIO_ADC_H__

#include "generic/typedef.h"

/**
 * @brief struct AUDIO_ADC参数设置结构体
 */
struct adc_platform_data {
    int mic_port[4];     /*!< 用到的mic的io组 [0]MIC0P [1]MIC0N [2]MIC1P [3]MIC1N */
    int linein_port[4];  /*!< 用到的linein的io组 [0]AUX0P [1]AUX0N [2]AUX1P [3]AUX1N */
    u8 mic_ch_num;          /*!< 用到的mic通道数 */
    u8 mic_share_micbias;   /*!< 共用micbias使用 */
    u8 mic_other_power_enable; /*!< 不使用micbias使能普通io或外部ldo供电做mic电源 */
    int mic_other_power_port[2];  /*!< 不使用micbias使用普通io或外部ldo供电做mic供电电源 */
    u8 linein_ch_num;       /*!< 用到的linein通道数 */
    u8 isel;                /*!< AD电流档，一般没有特殊要求不建议改动 */
    u8 inside_bias_resistor;/*!< MIC BIAS是否使用内置电阻 */
    u8 all_channel_open;    /*!< 所有AD通道打开，使用多路AD时建议打开 */
    u8 dmic_enable;         /*!< 数字麦通道使能，此时模拟麦通道会被替换 */
    u16 sr_points;          /*!< 多少个采样点进一次中断，一般不建议修改 */
    u16 dump_num;           /*!< 打开adc时丢掉点数 */
    u16 dmic_init_delay_ms; /*!< 数字麦初始化后等待SCLK稳定的延时ms */
    u32 dmic_sclk_fre;      /*!< 数字麦的sclk输出频率 */
    int dmic_io_sclk;       /*!< 数字麦的sclk输出引脚 */
    int dmic_io_idat0;      /*!< 数字麦的dat输入引脚0 */
    int dmic_io_idat1;      /*!< 数字麦的dat输入引脚0 */
};

/**
 * @brief ADC多路复用时的模拟增益设置
 *
 * @param source 采样源 "mic" "linein"
 * @param channel_bit_map 通道选择
 * @param gain 模拟增益  0-100
 */
void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);

/**
 * @brief ADC多路复用时打开通道
 *
 * @param source 采样源 "mic" "linein"
 * @param channel_bit_map 通道选择
 * @param rec_linein_channel_bit_map linein回采通道选择
 */
void adc_multiplex_open(const char *source, u8 channel_bit_map, u8 rec_linein_channel_bit_map);

/**
 * @brief 打开ADC
 *
 * @param source 采样源 "mic" "linein"
 * @param adc_platform_data *pd adc配置参数结构体指针
 */
int adc_open(const char *source, struct adc_platform_data *pd);

/**
 * @brief ADC采样率设置
 *
 * @param sample_rate  采样率
 */
int adc_set_sample_rate(int sample_rate);

/**
 * @brief ADC模块使能
 *
 * @param source 采样源 "mic" "linein"
 */
int adc_start(const char *source);

/**
 * @brief ADC中断函数
 */
void adc_irq_handler(void);

/**
 * @brief ADC模块失能
 */
int adc_stop(void);

/**
 * @brief 关闭ADC
 */
int adc_close(void);

/**
 * @brief 设置ADC中断的回调函数
 *
 * @param priv 传入指针
 * @param handler 回调函数
*/
void adc_set_data_handler(void *priv, void (*handler)(void *, u8 *data, int len));

/**
 * @brief ADC增益设置
 *
 * @param source 采样源 "mic" "linein"
 * @param gain 模拟增益  0-100
 */
int adc_set_gain(const char *source, u8 gain);

/**
 * @brief LINEIN模拟直通到DACL/R
 *
 * @param enable_dacl 直通使能DACL位
 * @param enable_dacr 直通使能DACR位
 */
void linein_to_fdac(u8 enable_dacl, u8 enable_dacr);

/**
 * @brief MIC模拟直通到DACL/R
 *
 * @param channel_bit_map 通道选择
 * @param enable_dacl 直通使能DACL位
 * @param enable_dacr 直通使能DACR位
 */
void mic_to_fdac(u8 channel_bit_map, u8 enable_dacl, u8 enable_dacr);

/**
 * @brief vcm电压初始化
 *
 * @param delay_ms 毫秒级稳定延时
 */
void vcm_early_init(u32 delay_ms);

/**
 * @brief linein模拟直通DAC的模拟增益控制开关
 *
 * @param enable 1：使能 0：关闭
 */
void linein_to_fdac_high_gain(u8 enable);

#endif


