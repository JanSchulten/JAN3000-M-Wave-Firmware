#ifndef __ASM_IIC_H__
#define __ASM_IIC_H__

#include "generic/typedef.h"
#include "asm/gpio.h"


/**
 * \name IIC error return
 * \{
 */
typedef enum {
    IIC_ROLE_MASTER = 0,                  ///< 主机模式
    IIC_ROLE_SLAVE = 1,                   ///< 从机模式
} iic_role_t;
/* \} name */


/**
 * \name IIC error return
 * \{
 */
typedef enum {
    IIC_FILTER_CLOSE = 0,                 ///< 关闭滤波器
    IIC_FILTER_1BAUD = 1,                 ///< 滤除1*Tbaud以下的尖峰脉宽
    IIC_FILTER_2BAUD = 2,                 ///< 滤除2*Tbaud以下的尖峰脉宽
    IIC_FILTER_3BAUD = 3,                 ///< 滤除3*Tbaud以下的尖峰脉宽
} iic_filter_t;
/* \} name */


/**
 * \name IIC error return value
 * \{
 */
typedef enum {
    IIC_OK = 0,
    IIC_ERROR_INIT_FAIL = -1,             ///< 初始化失败
    IIC_ERROR_NO_INIT = -2,               ///< 未初始化
    IIC_ERROR_SUSPEND_FAIL = -3,          ///< 挂起失败
    IIC_ERROR_RESUME_FAIL = -4,           ///< 恢复失败
    IIC_ERROR_BUSY = -5,                  ///< 设备忙(已被open过)
    IIC_ERROR_PARAM_ERROR = -6,           ///< 参数错误
    IIC_ERROR_DEV_ADDR_ACK_ERROR = -7,    ///< 发送设备地址，未检测到ACK
    IIC_ERROR_REG_ADDR_ACK_ERROR = -8,    ///< 发送寄存器地址，未检测到ACK
    IIC_ERROR_INDEX_ERROR = -9,           ///< IIC ID错误
    IIC_ERROR_BAUD_ERROR = -10,           ///< 波特率参数错误
    IIC_ERROR_TX_DATA_ACK_ERROR = -11,    ///< 发送数据，未检测的ACK
} iic_error_t;
/* \} name */


/**
 * \name IIC slave receive state
 * \{
 */
typedef enum {
    IIC_SLAVE_RX_PREPARE_TIMEOUT = -1,    ///< 接收准备超时
    IIC_SLAVE_RX_PREPARE_OK = 0,          ///< 接收准备完成
    IIC_SLAVE_RX_PREPARE_END_OK = 1,      ///< 接收结束
    IIC_SLAVE_RX_ADDR_NO_MATCH = -2,      ///< 接收地址未匹配
    IIC_SLAVE_RX_ADDR_TX = 2,             ///< 接收到匹配地址，方向为TX
    IIC_SLAVE_RX_ADDR_RX = 3,             ///< 接收到匹配地址，方向为RX
    IIC_SLAVE_RX_DATA = 4,                ///< 接收到数据
} iic_slave_rx_state_t;
/* \} name */


/**
 * \name IIC_plus pending type
 * \{
 */
typedef enum {
    IIC_PND_TASK_DONE       = BIT(20),    ///< task事务执行完成
    IIC_PND_START           = BIT(21),    ///< 检测到restart信号
    IIC_PND_STOP            = BIT(22),    ///< 检测到stop信号
    IIC_PND_RXACK           = BIT(23),    ///< 发送addr/data后检测到ack
    IIC_PND_RXNACK          = BIT(24),    ///< 发送addr/data后检测到nack
    IIC_PND_ADR_MATCH       = BIT(25),    ///< 作为从机接收到与addr相符合的地址
    IIC_PND_RXDATA_DONE     = BIT(26),    ///< 接收到1 Byte数据
    IIC_PND_TXTASK_LOAD     = BIT(27),    ///< TX相关的task事务加载完成。load pnd会比task done pnd更早起来
    IIC_PND_RXTASK_LOAD     = BIT(28),    ///< RX相关的task事务加载完成。
} iic_plus_pnd_typedef_t;
/* \} name */


/**
 * \name hardware IIC config item
 * \{
 */
typedef struct {
    u8 index;                             ///< IIC ID号，792N有3个IIC，ID范围0~2
    iic_role_t role;                      ///< 主机or从机
    gpio_drive_strength_t hd_level;       ///< IO强驱档位
    u32 clk_pin;                          ///< CLK使用的引脚
    u32 dat_pin;                          ///< DATA使用的引脚
    iic_filter_t io_filter;               ///< 滤波器选择
    u8 internal_pull_up_enable;           ///< 开内部上拉。0:disable 1:enable
    u8 fast_resp;                         ///< 快速应答模式。0:常规 1:快速
    u8 slave_addr;                        ///< 从机地址
    u8 adv_enable;                        ///< 响应广播地址使能。0:disable 1:enable
    u8 ie_enable;                         ///< 中断使能。0:disable 1:enable
    u8 stretch_disable;                   ///< 时钟延展功能使能。0:disable 1:enable
    u32 baudrate;                         ///< 波特率
    u32 timeout_cnt;                      ///< 等待各种PND的超时值。时基：10ms
    JL_IIC_TypeDef *reg;                  ///< 增强型IIC寄存器地址(792N的iic0)
    JL_IICDMA_TypeDef *dma_reg;           ///< 普通型IIC寄存器地址(792N的iic1、iic2)
} iic_hardware_t;
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define HW_IIC0_PLATFORM_DATA_BEGIN(data) \
    static int link_##data(void) {\
    extern int hw_iic_ops_link(void);\
    return hw_iic_ops_link(); }\
    platform_initcall(link_##data);\
    static iic_device_t _iic_device_##data;\
    static const struct hw_iic_platform_data data = { \
        .head = { \
            .type = IIC_TYPE_HW_PLUS, \
            .p_iic_device = &_iic_device_##data,\
        }, \
        .iic = {

#define HW_IIC0_PLATFORM_DATA_END() \
            .index = 0, \
            .reg = JL_IIC0, \
        }, \
    };


#define HW_IIC1_PLATFORM_DATA_BEGIN(data) \
    static int link_##data(void) {\
    extern int hw_iic_ops_link(void);\
    return hw_iic_ops_link(); }\
    platform_initcall(link_##data);\
    static iic_device_t _iic_device_##data;\
    static const struct hw_iic_platform_data data = { \
        .head = { \
            .type = IIC_TYPE_HW, \
            .p_iic_device = &_iic_device_##data,\
        }, \
        .iic = {

#define HW_IIC1_PLATFORM_DATA_END() \
            .index = 1, \
            .dma_reg = JL_IIC1, \
        }, \
    };


#define HW_IIC2_PLATFORM_DATA_BEGIN(data) \
    static int link_##data(void) {\
    extern int hw_iic_ops_link(void);\
    return hw_iic_ops_link(); }\
    platform_initcall(link_##data);\
    static iic_device_t _iic_device_##data;\
    static const struct hw_iic_platform_data data = { \
        .head = { \
            .type = IIC_TYPE_HW, \
            .p_iic_device = &_iic_device_##data,\
        }, \
        .iic = {

#define HW_IIC2_PLATFORM_DATA_END() \
            .index = 2, \
            .dma_reg = JL_IIC2, \
        }, \
    };
/// \endcond


#endif

