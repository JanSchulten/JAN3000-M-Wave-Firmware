#ifndef __DEVICE_GPIO_H__
#define __DEVICE_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "asm/gpio.h"

///  \cond DO_NOT_DOCUMENT
#define GPIO2PORT(gpio)    (gpio / IO_GROUP_NUM)
/// \endcond

/**
 * @brief gpio_port_lock，自旋锁，即不能被另一个cpu打断，也不能被中断打断
 *
 * @param gpio 参考宏GPIO2PORT(IO_PORTx_xx)，如GPIO2PORT(IO_PORTA_00)
 *
 */
void gpio_port_lock(unsigned int port);

/**
 * @brief gpio_port_unlock，解除自旋锁
 *
 * @param gpio 参考宏GPIO2PORT(IO_PORTx_xx)，如GPIO2PORT(IO_PORTA_00)
 */
void gpio_port_unlock(unsigned int port);

/**
 * @brief gpio_direction_input，将引脚直接设为输入模式
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return 0：成功  非0：失败
 */
int gpio_direction_input(unsigned int gpio);
///  \cond DO_NOT_DOCUMENT
int __gpio_direction_input(unsigned int gpio);
/// \endcond

/**
 * @brief gpio_direction_output，设置引脚的方向为输出，并设置一下电平
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1：输出高电平，  0：输出低电平
 *
 * @return 0：成功  非0：失败
 */
int gpio_direction_output(unsigned int gpio, int value);
///  \cond DO_NOT_DOCUMENT
int __gpio_direction_output(unsigned int gpio, int value);
/// \endcond

/**
 * @brief gpio_set_pull_up，设置引脚的上拉电阻等级
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param mode gpio_pull_up_mode_t
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_pull_up(unsigned int gpio, gpio_pull_up_mode_t mode);
///  \cond DO_NOT_DOCUMENT
int __gpio_set_pull_up(unsigned int gpio, gpio_pull_up_mode_t mode);
/// \endcond

/**
 * @brief gpio_set_pull_down，设置引脚的下拉电阻等级
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param mode gpio_pull_down_mode_t
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_pull_down(unsigned int gpio, gpio_pull_down_mode_t mode);
///  \cond DO_NOT_DOCUMENT
int __gpio_set_pull_down(unsigned int gpio, gpio_pull_down_mode_t mode);
/// \endcond

/**
 * @brief gpio_set_hd, 设置引脚的强驱等级
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param mode gpio_drive_strength_t
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_hd(unsigned int gpio, gpio_drive_strength_t mode);
///  \cond DO_NOT_DOCUMENT
int __gpio_set_hd(unsigned int gpio, gpio_drive_strength_t mode);
/// \endcond

/**
 * @brief gpio_set_die，设置引脚为数字功能还是模拟功能，比如引脚作为ADC的模拟输入，则die要置0
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1：数字输入功能  0：跟电压相关的模拟功能
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_die(unsigned int gpio, int value);
///  \cond DO_NOT_DOCUMENT
int __gpio_set_die(unsigned int gpio, int value);
/// \endcond

/**
 * @brief gpio_set_dieh，设置引脚P33输入功能
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param dir 1：打开 0：关闭
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_dieh(unsigned int gpio, int value);

/**
 * @brief gpio_set_direction，设置引脚方向
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param dir 1：输入 0：输出
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_direction(unsigned int gpio, unsigned int dir);
///  \cond DO_NOT_DOCUMENT
int __gpio_set_direction(unsigned int gpio, unsigned int dir);
/// \endcond

/**
 * @brief gpio_read，读取引脚的输入电平，引脚为数字输入模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return 电平值
 */
int gpio_read(unsigned int gpio);
///  \cond DO_NOT_DOCUMENT
int __gpio_read(unsigned int gpio);
/// \endcond

/**
 * @brief gpio_set_spl，设置引脚输出时是否维持上下拉电阻
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param keep 1：维持 0：不维持
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_spl(unsigned int gpio, int keep);

/**
 * @brief gpio_set_mode，设置引脚工作模式
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param mode gpio_mode_t
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_mode(unsigned int gpio, gpio_mode_t mode);

/**
 * @brief gpio_enable_function，打开引脚映射功能
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param fn gpio_function_t
 * @param io_init 是否使用映射功能的默认IO初始化状态
 *
 * @return 0：成功  非0：失败
 */
int gpio_enable_function(unsigned int gpio, gpio_function_t fn, int io_init);

/**
 * @brief gpio_disable_function，关闭引脚映射功能
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param fn gpio_function_t
 * @param io_init 是否还原IO的初始状态（输入高阻）
 *
 * @return 0：成功  非0：失败
 */
int gpio_disable_function(unsigned int gpio, gpio_function_t fn, int io_uninit);

/**
 * @brief usb_iomode, usb引脚设为普通IO
 *
 * @param usb_id usb名称号，0是usb0，1是usb1
 * @param enable 1：使能；0：关闭
 */
void usb_iomode(unsigned char usb_id, int enable);

/**
 * @brief gpio_latch_en，锁定引脚状态，解锁前IO电平状态不会再改变
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param en 1：锁定；0：解锁
 */
void gpio_latch_en(unsigned int gpio, int enable);

#ifdef __cplusplus
}
#endif
#endif
