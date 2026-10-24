#ifndef __AUDIO_SRC_H__
#define __AUDIO_SRC_H__

#include "generic/typedef.h"

/**
 * \name 变采样类型
 * \{
 */
#define AUDIO_ONLY_RESAMPLE         1 /*!< 仅变采样 */
#define AUDIO_SYNC_RESAMPLE         2 /*!< 同步变采样 */
#define BIND_AUDSYNC                0x10 /*!< 同步变采样掩码 */
/* \} name */

/**
 * \name 变采样数据类型
 * \{
 */
#define AUDIO_SAMPLE_FMT_16BIT      0 /*!< 16BIT 数据类型 */
#define AUDIO_SAMPLE_FMT_24BIT      1 /*!< 24BIT 数据类型 */
/* \} name */

/**
 * @brief
 *
 * @param
 *
 * @return
*/

/**
 * @brief 打开SRC,配置通道，输入输出采样率和采样类型
 *
 * @param channel :通道数
 * @param in_sample_rate :输入采样率
 * @param out_sample_rate :输出采样率
 * @param type : 变采样类型
 *
 * @return 变采样句柄指针 *resample:成功  NULL:失败
*/
void *src_base_open(u8 channel, int in_sample_rate, int out_sample_rate, u8 type);

/**
 * @brief 输出数据回调函数设置
 *
 * @param resample :变采样句柄指针
 * @param priv :回调函数参数
 * @param data :变采样输出数据
 * @param len :变采样输出数据长度
 *
 * @return 0:成功
*/
int src_base_set_output_handler(void *resample,
                                void *priv,
                                int (*handler)(void *priv, void *data, int len));
/**
 * @brief 设置变采样通道数
 *
 * @param resample :变采样句柄指针
 * @param channel :变采样通道数
 *
 * @return 0:成功
*/
int src_base_set_channel(void *resample, u8 channel);

/**
 * @brief 设置变采样输入buf和长度
 *
 * @param resample :变采样句柄指针
 * @param buf :输入buf
 * @param len :输入长度
 *
 * @return 0:成功
*/
int src_base_set_in_buffer(void *resample, void *buf, int len);

/**
 * @brief 设置变采样输入buf和长度
 *
 * @param resample :变采样句柄指针
 * @param buf :输入buf
 * @param len :输入长度
 *
 * @return 0:成功
*/
int src_base_set_input_buff(void *resample, void *buf, int len);

/**
 * @brief 设置变采样输出buf和长度
 *
 * @param resample :变采样句柄指针
 * @param buf :输出buf
 * @param len :输出长度
 *
 * @return 0:成功
*/
int src_base_set_output_buff(void *resample, void *buf, int len);

/**
 * @brief 设置变采样的输入输出采样率
 *
 * @param resample :变采样句柄指针
 * @param in_rate :输入采样率
 * @param out_rate :输出采样率
 *
 * @return 0:成功
*/
int src_base_resample_config(void *resample, int in_rate, int out_rate);

/**
 * @brief 变采样输入数据进行变采样处理
 *
 * @param resample :变采样句柄指针
 * @param data :输入数据
 * @param len :输入长度
 *
 * @return len:成功处理了输入数据的长度 0:未成功进行边采样
*/
int src_base_write(void *resample, void *data, int len);

/**
 * @brief 停止变采样
 *
 * @param resample :变采样句柄指针
 *
 * @return 0:成功
*/
int src_base_stop(void *resample);

/**
 * @note 未使用
*/
int src_base_run_scale(void *resample);

/**
 * @brief 获取变采样输入帧数
 *
 * @param resample :变采样句柄指针
 *
 * @return 变采样输入帧数
*/
int src_base_input_frames(void *resample);

/**
 * @brief 获取变采样输出帧数
 *
 * @param resample :变采样句柄指针
 *
 * @return 变采样输出帧数
*/
u32 src_base_out_frames(void *resample);



///  \cond DO_NOT_DOCUMENT
float src_base_position(void *resample);
int src_base_scale_output(void *resample, int in_sample_rate, int out_sample_rate, int frames);
int src_base_bufferd_frames(void *resample);
int audio_src_base_set_slience(void *resample, u8 slience, int fade_time);
/// \endcond


/**
 * @brief 设置中断回调函数
 *
 * @param resample :变采样句柄指针
 * @param priv :回调函数参数
 * @param callback :回调函数
 *
 * @return 0 :成功 非0:失败
*/
int src_base_wait_irq_callback(void *resample, void *priv, void (*callback)(void *));

/**
 * @brief 关闭src
 *
 * @param resample :变采样句柄指针
*/
void src_base_close(void *resample);

/**
 * @brief 将数据变采样输出完
 *
 * @param resample :变采样句柄指针
 *
 * @return 0 :成功 非0:失败
*/
int src_base_push_data_out(void *resample);

#endif

