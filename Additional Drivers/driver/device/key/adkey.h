#ifndef __DEVICE_ADKEY_H__
#define __DEVICE_ADKEY_H__

#include "generic/typedef.h"

///  \cond DO_NOT_DOCUMENT
#define ADKEY_MAX_NUM 10

#define ADKEY_MAX_NUM 10
/// \endcond

/**
 * \name adkey_platform_data
 * \{
 */
struct adkey_platform_data {
    u8 adkey_pin;                   ///<  ad按键引脚
    u8 extern_up_en;                ///<  是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u32 ad_channel;                 ///<  ad通道
    u16 ad_value[ADKEY_MAX_NUM];    ///<  ad值
    u8  key_value[ADKEY_MAX_NUM];   ///<  key值
};
/* \} name */

///  \cond DO_NOT_DOCUMENT
struct adkey_rtcvdd_platform_data {
    u8 adkey_pin;
    u8 adkey_num;
    u32 ad_channel;
    u32 extern_up_res_value;        //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u16 res_value[ADKEY_MAX_NUM]; 	//电阻值, 从 [大 --> 小] 配置
    u8  key_value[ADKEY_MAX_NUM];
};
/// \endcond

#endif

