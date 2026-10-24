#ifndef DEVICE_IIC_H
#define DEVICE_IIC_H

#include "generic/typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "os/os_api.h"
#include "asm/iic.h"


/**
 * \name IIC dev_ioctl funciton command
 * \{
 */
#define IIC_MAGIC                         'I'
#define IOCTL_IIC_START                   _IO(IIC_MAGIC,1)                  ///< iic start，收发数据前需要调用该命令
#define IOCTL_IIC_STOP                    _IO(IIC_MAGIC,2)                  ///< iic stop，收发结束后需要调用该命令
#define IOCTL_IIC_TX_BYTE                _IOW(IIC_MAGIC,3, u8)              ///< 发送1 byte数据
#define IOCTL_IIC_TX_BYTE_WITH_START_BIT _IOW(IIC_MAGIC,4, u8)              ///< 发送start + data
#define IOCTL_IIC_TX_BYTE_WITH_STOP_BIT  _IOW(IIC_MAGIC,5, u8)              ///< 发送1 byte数据，并发送stop
#define IOCTL_IIC_RX_BYTE                _IOR(IIC_MAGIC,6, u8 *)            ///< 接收1 byte数据，发送ack
#define IOCTL_IIC_RX_BYTE_WITH_ACK       _IOR(IIC_MAGIC,7, u8 *)            ///< 接收1 byte数据，发送ack
#define IOCTL_IIC_RX_BYTE_WITH_NACK      _IOR(IIC_MAGIC,8, u8 *)            ///< 接收1 byte数据，发送nack
#define IOCTL_IIC_RX_BYTE_WITH_STOP_BIT  _IOR(IIC_MAGIC,9, u8 *)            ///< 接收1 byte数据，发送nack，并发送stop
#define IOCTL_IIC_SET_BAUDRATE           _IOW(IIC_MAGIC,10, u32)            ///< 设置波特率
#define IOCTL_IIC_TX_STOP_BIT             _IO(IIC_MAGIC,11)                 ///< 发送stop信号。仅iic0和软iic支持
/* 下面为增强型iic专用命令 */
#define IOCTL_IIC_SET_IRQ_CALLBACK       _IOW(IIC_MAGIC,12, void (*)(void)) ///< 设置中断回调函数
#define IOCTL_IIC_SET_SLAVE_RX_PREPARE   _IOW(IIC_MAGIC,13, u32)            ///< 从机：接收准备
#define IOCTL_IIC_GET_SLAVE_TX_ACK       _IOR(IIC_MAGIC,14, u8 *)           ///< 从机：检测是否收到ack
#define IOCTL_IIC_GET_PNDING            _IOWR(IIC_MAGIC,15, u32 *)          ///< 查询指定pnd的状态
#define IOCTL_IIC_CLR_PNDING             _IOW(IIC_MAGIC,16, u32)            ///< 清除指定pnd的信号
/* \} name */


/**
 * \name IIC device type
 * \{
 */
typedef enum {
    IIC_TYPE_HW_PLUS,                  ///< hardware iic 增强型 (792n中的iic0)
    IIC_TYPE_HW,                       ///< hardware iic
    IIC_TYPE_SW,                       ///< software iic
} iic_device_type_t;
/* \} name */


/**
 * \name software IIC config item
 * \{
 */
typedef struct {
    u8 clk_pin;                        ///< SCL引脚IO
    u8 dat_pin;                        ///< SDA引脚IO
    u32 sw_iic_delay;                  ///< 软IIC波形脉宽延时，该参数影响通讯速率。单位:nop。
    u32 sw_iic_byte_gap_time;          ///< 软IIC发送数据字节之间的间隔。单位:nop。
    gpio_drive_strength_t hd_level;    ///< IO强驱档位
} iic_software_t;
/* \} name */


/// \cond DO_NOT_DOCUMENT
struct iic_operations;


typedef struct {
    u8 id;                             ///< 设备id号
    u8 type;                           ///< 设备类型
    u8 open_status;                    ///< 设备是否打开的标记
    struct list_head entry;            ///< list链表
    const void *hw;                    ///< 指向对应IIC设备的board的配置数据
    struct device dev;
    const struct iic_operations *ops;
    OS_MUTEX mutex;
} iic_device_t;


struct iic_operations {
    iic_device_type_t type;
    int (*open)(iic_device_t *);
    int (*read)(iic_device_t *, void *buf, int len);
    int (*write)(iic_device_t *, void *buf, int len);
    int (*ioctl)(iic_device_t *, int cmd, int arg);
    int (*close)(iic_device_t *);
};


struct iic_platform_data {
    iic_device_type_t type;            ///< iic设备类型
    iic_device_t *p_iic_device;        ///< iic设备
    u32 data[0];
};


struct sw_iic_platform_data {
    struct iic_platform_data head;
    iic_software_t iic;
};


struct hw_iic_platform_data {
    struct iic_platform_data head;
    iic_hardware_t iic;
};


#define SW_IIC_PLATFORM_DATA_BEGIN(data) \
    static int link_##data(void) {\
        extern int sw_iic_ops_link(void);\
        return sw_iic_ops_link();\
    }\
    platform_initcall(link_##data);\
    static iic_device_t _iic_device_##data;\
    static const struct sw_iic_platform_data data = { \
        .head = { \
            .type = IIC_TYPE_SW, \
            .p_iic_device = &_iic_device_##data,\
        }, \
        .iic = {

#define SW_IIC_PLATFORM_DATA_END() \
        }, \
    };


#define REGISTER_IIC_DEVICE(name) \
    int name##_link(void){return 0;}\
    static const struct iic_operations name SEC_USED(.iic)

extern const struct iic_operations iic_device_begin[], iic_device_end[];

#define list_for_each_iic_device_ops(p) \
    for (p=iic_device_begin; p<iic_device_end; p++)

extern const struct device_operations iic_dev_ops;
/// \endcond

#endif

