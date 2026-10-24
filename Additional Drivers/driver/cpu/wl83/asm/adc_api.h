#ifndef __ADC_API_H__
#define __ADC_API_H__

#define ADC_MAX_CH          8

/// \cond DO_NOT_DOCUMENT
#define ADC_IO_CH           0u
#define ADC_RWF_CH          1u
#define ADC_PMU_CH          2u
#define ADC_RTC_CH          3u
#define ADC_AUDIO_CH        4u
#define ADC_SYSPLL_CH       5u
#define ADC_HUB_CH          6u
#define ADC_DDRPLL_CH       7u
/// \endcond

/**
 * \name AD channel define
 * \{
 */
typedef enum {
    ADC_IO_CH_PD00,
    ADC_IO_CH_PD01,
    ADC_IO_CH_PD06,
    ADC_IO_CH_PD07,
    ADC_IO_CH_PD08,
    ADC_IO_CH_PD09,
    ADC_IO_CH_PD10,
    ADC_IO_CH_PE04,
    ADC_IO_CH_PE05,
    ADC_IO_CH_PE06,
    ADC_IO_CH_PE07,
    ADC_IO_CH_PE08,
    ADC_IO_CH_PE09,
    ADC_IO_CH_PC13,
    ADC_IO_CH_DP,
    ADC_IO_CH_DM,
    ADC_IO_CH_MAX,
} adc_io_ch_t;

#define ADC_PMU_CH_VBG      (0x0 | (ADC_PMU_CH << 16))
#define ADC_PMU_CH_VDC14    (0x1 | (ADC_PMU_CH << 16))
#define ADC_PMU_CH_SYSVDD   (0x2 | (ADC_PMU_CH << 16))
#define ADC_PMU_CH_VTEMP    (0x3 | (ADC_PMU_CH << 16))
#define ADC_PMU_CH_WVDD     (0x4 | (ADC_PMU_CH << 16))
#define ADC_PMU_CH_VBAT     (0x5 | (ADC_PMU_CH << 16))    //1/4 vbat
#define ADC_PMU_CH_LDO5V    (0x6 | (ADC_PMU_CH << 16))    //1/4 LDO 5V

#define AD_AUDIO_CH_VCM     (0x0 | (ADC_AUDIO_CH << 16))
#define AD_AUDIO_CH_VOUTL   (0x1 | (ADC_AUDIO_CH << 16))
#define AD_AUDIO_CH_VOUTR   (0x2 | (ADC_AUDIO_CH << 16))
#define AD_AUDIO_CH_DACVDD  (0x3 | (ADC_AUDIO_CH << 16))
/* \} name */

/**
 * @brief adc_init, adc初始化
*/
void adc_init(void);

/**
 * @brief adc_suspend, adc检测挂起
*/
void adc_suspend(void);

/**
 * @brief adc_resume, adc检测恢复
*/
void adc_resume(void);

/**
 * @brief adc_get_value, 从队列中获取adc通道测得的数值
 * @param ch : ADC通道号
 * @return 当前通道的AD值
*/
u32 adc_get_value(u32 ch);

/**
 * @brief adc_add_sample_ch, 添加adc采样通道到采样队列中
 * @param ch : ADC通道号
 * @return 当前通道值
*/
u32 adc_add_sample_ch(u32 ch);

/**
 * @brief adc_remove_sample_ch, 从采样队列中移除adc采样通道
 * @param ch : ADC通道号
 * @return 当前通道值
*/
u32 adc_remove_sample_ch(u32 ch);

/**
 * @brief adc_get_voltage, 换算电压的公式函数, 获取adc通道电压值，如果测得与实际不符，则需留意芯片是否trim过，trim值是否正确。
 * @param ch : ADC通道号
 * @return 当前通道的电压值，单位mv
*/
u32 adc_get_voltage(u32 ch);

/**
 * @brief adc_value_to_voltage, 采样值换算电压，如果换算后与实际不符，则需留意芯片是否trim过，trim值是否正确。
 * @param adc_vbg : 基准电压采样值
 * @param adc_ch_val : 需要转换的通道的AD值
 * @return 转换后的电压值，单位mv
*/
u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_ch_val);

/**
 * @brief adc_get_value_blocking, 阻塞式采集一个指定通道的adc原始值
 * @param ch : ADC通道号
 * @return 当前通道的AD值
*/
u32 adc_get_value_blocking(u32 ch);

/**
 * @brief adc_get_voltage_blocking, 阻塞式采集一个指定通道的电压值（经过均值滤波处理）
 * @param ch : ADC通道号
 * @return 转换后的电压值，单位mv
*/
u32 adc_get_voltage_blocking(u32 ch);

/**
 * @brief adc_sample, 启动采样一次
 * @param ch : ADC通道号
 * @param block : 是否堵塞
 * @return 堵塞时返回当前通道的AD值，非堵塞时返回上一次测量的AD值
 * @note 内部接口，用户请勿直接使用该接口
*/
u32 adc_sample(u32 ch, u8 block);

/**
 * @brief adc_io2ch, adc io转换成对应adc通道序号
 * @param gpio : adc io
 * @return 对应通道序号
*/
adc_io_ch_t adc_io2ch(int gpio);

#endif
