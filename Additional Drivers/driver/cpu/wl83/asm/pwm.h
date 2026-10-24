#ifndef __ASM_PWM_H__
#define __ASM_PWM_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"
#include "device/device.h"

#define MCPWM_MAX_NUM		4	//0-3，MPWM最多4个通道

/**
 * \name MCPWMCHx_H
 * \{
 */
#define PWMCH0_H  		BIT(0)
#define PWMCH1_H  		BIT(1)
#define PWMCH2_H  		BIT(2)
#define PWMCH3_H  		BIT(3)
// #define PWMCH4_H  		BIT(4)
// #define PWMCH5_H  		BIT(5)
// #define PWMCH6_H  		BIT(6)
// #define PWMCH7_H  		BIT(7)
/* \} name */

/**
 * \name MCPWMCHx_L
 * \{
 */
#define PWM_CHL_OFFSET	MCPWM_MAX_NUM
#define PWMCH0_L  		BIT(PWM_CHL_OFFSET + 0)
#define PWMCH1_L  		BIT(PWM_CHL_OFFSET + 1)
#define PWMCH2_L  		BIT(PWM_CHL_OFFSET + 2)
#define PWMCH3_L  		BIT(PWM_CHL_OFFSET + 3)
// #define PWMCH4_L  		BIT(PWM_CHL_OFFSET + 4)
// #define PWMCH5_L  		BIT(PWM_CHL_OFFSET + 5)
// #define PWMCH6_L  		BIT(PWM_CHL_OFFSET + 6)
// #define PWMCH7_L  		BIT(PWM_CHL_OFFSET + 7)
/* \} name */

/**
 * \name TIMER PWM 最多四个通道
 * \{
 */
#define TIME_PWM_MAX    4
#define TIME_PWM_OFFSET         (MCPWM_MAX_NUM * 2)
#define PWM_TIMER2_OPCH         BIT(TIME_PWM_OFFSET + 0)
#define PWM_TIMER3_OPCH         BIT(TIME_PWM_OFFSET + 1)
#define PWM_TIMER4_OPCH         BIT(TIME_PWM_OFFSET + 2)
#define PWM_TIMER5_OPCH         BIT(TIME_PWM_OFFSET + 3)
/* \} name */

/**
 * \name PWM dev_ioctl CMD
 * \{
 */
#define PWM_MAGIC                       'P'
#define IOCTL_PWM_GET_DUTY              _IOR(PWM_MAGIC,1,pwm_config_t *)                ///<  获取占空比，通过读取结构体形参中的duty获取，获取的pwm通道占空比是看传参中的pwm_ch，不支持多通道
#define IOCTL_PWM_SET_DUTY              _IOW(PWM_MAGIC,2,pwm_config_t *)                ///<  设置占空比，通过结构体形参中的duty设置，设置的PWM通道占空比是看传参中的pwm_ch，其他传参pwm_config_t *的IOCTL同理。支持多通道
#define IOCTL_PWM_SET_STOP              _IOW(PWM_MAGIC,3,pwm_config_t *)                ///<  停止，支持多通道
#define IOCTL_PWM_SET_RUN               _IOW(PWM_MAGIC,4,pwm_config_t *)                ///<  运行，支持多通道
#define IOCTL_PWM_SET_FORDIRC           _IOW(PWM_MAGIC,5,pwm_config_t *)                ///<  正向，支持多通道
#define IOCTL_PWM_SET_REVDIRC           _IOW(PWM_MAGIC,6,pwm_config_t *)                ///<  反向，支持多通道
#define IOCTL_PWM_SET_DEATH_TIME        _IOW(PWM_MAGIC,7,pwm_config_t *)                ///<  死区时间为系统频率周期时间的整数倍，最大值31，支持多通道
#define IOCTL_PWM_SET_FREQ              _IOW(PWM_MAGIC,8,pwm_config_t *)                ///<  设置频率，支持多通道
#define IOCTL_PWM_SET_REMOV_CHANNEL     _IOW(PWM_MAGIC,9,struct pwm_platform_data *)    ///<  删除通道，与IOCTL_PWM_SET_ADD_CHANNEL成对使用
#define IOCTL_PWM_SET_ADD_CHANNEL       _IOW(PWM_MAGIC,10,struct pwm_platform_data *)   ///<  增加通道，与IOCTL_PWM_SET_REMOV_CHANNEL成对使用
/* \} name */

/**
 * \name PWM config data
 * \{
 */
typedef struct {
    float duty;                         ///< 用于带2位小数点占空比的PWM，0.00%-100.00%
    u8 deathtime;                       ///< 死区时间，最大值31
    u8  point_bit;                      ///< 小数点精度:0-2
    u32 pwm_ch;                         ///< PWM通道使能
    u32 freq;                           ///< PWM输出频率配置
} pwm_config_t;
/* \} name */

/**
 * \name PWM platform data
 * \{
 */
struct pwm_platform_data {
    int timer_pwm_port[TIME_PWM_MAX];   ///< TIMER PWM IO口配置
    int mcpwm_port[MCPWM_MAX_NUM * 2];  ///< MCPWM IO配置
    pwm_config_t pwm_config;            ///< PWM参数配置
    u8 hd_level;                        ///< IO强驱等级
};
/* \} name */

///  \cond DO_NOT_DOCUMENT
#define PWM_PLATFORM_DATA_BEGIN(data) \
    static const struct pwm_platform_data data = {

#define PWM_PLATFORM_DATA_END()     };
/// \endcond

extern const struct device_operations pwm_dev_ops;

#endif

