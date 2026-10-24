#ifndef __ASM_SPI_H__
#define __ASM_SPI_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"

#define SPI_MAX_SIZE	(0x7FFFFFFF - 1)     ///< (2^31 - 1)//由于该宏可能赋值给有符合变量，因此使用2^31 - 1


/**
 * \name SPI dev_ioctl funciton selest
 * \{
 */
#define SPI_MAGIC                         'S'
#define IOCTL_SPI_SET_CS_STATUS           _IOW(SPI_MAGIC, 1, u8)                     ///< 设置cs状态
#define IOCTL_SPI_SET_CS_PORT_FUNC        _IOW(SPI_MAGIC, 2, void (*)(u8))           ///< 设置cs函数
#define IOCTL_SPI_SEND_BYTE               _IOW(SPI_MAGIC, 3, u8)                     ///< 发送一个字节
#define IOCTL_SPI_READ_BYTE               _IOR(SPI_MAGIC, 4, u8 *)                   ///< 读取一个字节
#define IOCTL_SPI_SEND_CMD_2BIT_MODE      _IOW(SPI_MAGIC, 5, spi_2bit_data_t *)      ///< 以2bit模式发命令
#define IOCTL_SPI_SET_USE_SEM             _IOW(SPI_MAGIC, 6, u8)                     ///< 设置是否使用信号量,0:使用 非0:不使用
// #define IOCTL_SPI_SET_TRIGGER_WAIT_SEM_TIME_THRE    _IOW(SPI_MAGIC, 7, u32)          ///< 设置TX使用信号量的时间阈值(该功能取消)
#define IOCTL_SPI_SET_IRQ_FUNC            _IOW(SPI_MAGIC, 8, void (*)(void))         ///< 设置中断函数
#define IOCTL_SPI_SET_ASYNC_SEND          _IOW(SPI_MAGIC, 9, u8)                     ///< 设置是否同步方式发包
#define IOCTL_SPI_WAIT_ASYNC_SEND_END      _IO(SPI_MAGIC, 10)                        ///< 等待非同步发送结束
#define IOCTL_SPI_SET_ASYNC_RECV          _IOW(SPI_MAGIC, 11, u8)                    ///< 设置是否同步方式发包
#define IOCTL_SPI_WAIT_ASYNC_RECV_END      _IO(SPI_MAGIC, 12)                        ///< 等待非同步发送结束
#define IOCTL_SPI_SET_SYNC_TIMEOUT        _IOW(SPI_MAGIC, 13,u32)                    ///< 设置同步等待超时时间，单位ms
#define IOCTL_SPI_FDX_TR_DATA             _IOR(SPI_MAGIC, 14,spi_fdx_data_t *)       ///< 全双工收发数据，仅STD模式支持
#define IOCTL_SPI_SET_IRQ_EVENT_CB        _IOW(SPI_MAGIC, 15,void (*)(spi_irq_event_t))  ///< 设置中断事件回调函数
#define IOCTL_SPI_SET_BLOCK_DATA_CFG      _IOW(SPI_MAGIC, 16,spi_block_data_cfg_t *) ///< 设置block data的配置
#define IOCTL_SPI_READ_BLOCK_DATA         _IOR(SPI_MAGIC, 17,int *)                  ///< 读取block data
#define IOCTL_SPI_FREE_BLOCK_DATA          _IO(SPI_MAGIC, 18)                        ///< 释放一个block data
#define IOCTL_SPI_SET_BAUDRATE            _IOW(SPI_MAGIC, 19,u32)                    ///< 设置波特率
#define IOCTL_SPI_9BIT_EN                 _IOW(SPI_MAGIC, 20,u8)                     ///< 9BIT模式开/关，0:关 非0:开，推屏用
#define IOCTL_SPI_9BIT_DATA               _IOW(SPI_MAGIC, 21,u8)                     ///< 9BIT数据，0:低 非0:高，推屏用
/* \} name */


/**
 * \name SPI master or slave mode configuration parameters
 * \{
 */
typedef enum {
    SPI_MODE_MASTER = 1,
    SPI_MODE_SLAVE,
} spi_role_t;
/* \} name */

/**
 * \name SPI clk/updata/sample level and edge configuration parameters
 * \{
 */
typedef enum {
    SPI_SCLK_H_UPH_SMPL = 1,
    SPI_SCLK_H_UPL_SMPH,
    SPI_SCLK_H_UPH_SMPH,
    SPI_SCLK_H_UPL_SMPL,
    SPI_SCLK_L_UPH_SMPL,
    SPI_SCLK_L_UPL_SMPH,
    SPI_SCLK_L_UPH_SMPH,
    SPI_SCLK_L_UPL_SMPL,
} spi_attr_t;
/* \} name */

/**
 * \name SPI mode configuration parameters
 * \{
 */
typedef enum {
    SPI_STD_MODE = 1,              ///< 标准模式，DI、DO、CLK
    SPI_1WIRE_MODE,
    SPI_DUAL_MODE,
    SPI_QUAD_MODE,
} spi_mode_t;
/* \} name */

/**
 * \name SPI IRQ event configuration parameters
 * \{
 */
typedef enum {
    SPI_IRQ_EVENT_NULL,            ///< 0.NULL
    SPI_IRQ_EVENT_TX,              ///< 1.send
    SPI_IRQ_EVENT_RX,              ///< 2.receive
    SPI_IRQ_EVENT_UF,              ///< 3.UnderFlow
    SPI_IRQ_EVENT_OF,              ///< 4.OverFlow
    SPI_IRQ_EVENT_BLOCK_RECV,      ///< 5.block receive data
} spi_irq_event_t;
/* \} name */

/**
 * \name SPI io config item
 * \{
 */
typedef struct {
    int cs_pin;
    int di_pin;
    int do_pin;
    int clk_pin;
    int d2_pin;
    int d3_pin;
} spi_io_t;
/* \} name */

/**
 * \name SPI 2bit mode data config item
 * \{
 */
typedef struct {
    u32 len;
    u8 *buf;
} spi_2bit_data_t;
/* \} name */

/**
 * \name SPI block data config item
 * \{
 */
typedef struct {
    u8 *buf;                       ///< block的buf地址
    u32 buf_size;                  ///< block的buf空间大小
    u32 dma_max_cnt;               ///< 一次dma的最大数量，通常被配置为SPI_MAX_SIZE
    u32 first_dma_size;            ///< 每个block第一次dma接收数据的大小
    u32 block_size;                ///< 一个block的大小
} spi_block_data_cfg_t;
/* \} name */

/**
 * \name SPI TX RX full duplex data transmission and reception config item
 * \{
 */
typedef struct {
    u8 *send_buf;                  ///< 发送buf
    u8 *recv_buf;                  ///< 接收buf
    u32 len;                       ///< 收发数据量
    void (*callback)(u8);          ///< 数据准备通知回调
} spi_fdx_data_t;
/* \} name */

/**
 * \name SPI platform data
 * \{
 */
struct spi_platform_data {
    spi_mode_t mode;               ///< 模式
    u8 irq;                        ///< 中断号，每个SPI为固定值
    u8 priority;                   ///< 中断优先级
    u8 cpu_id;                     ///< 中断注册所在cpu
    u8 hd_level;                   ///< IO强驱等级
    u8 uf_ie_enable;               ///< dma fifo UnderFlow irq enable(for debug)
    u8 of_ie_enable;               ///< dma fifo OverFlow irq enable(for debug)
    u32 clk;                       ///< 波特率配置
    u32 sync_wait_timeout;         ///< 同步等待超时时间，单位ms
    spi_role_t role;               ///< 主、从机配置
    spi_attr_t attr;               ///< clk/updata/sample的电平/边沿配置
    spi_io_t io;                   ///< 使用的引脚配置
    JL_SPI_TypeDef *reg;           ///< 使用的寄存器组
};
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define SPI0_PLATFORM_DATA_BEGIN(spi0_data) \
    static const struct spi_platform_data spi0_data = { \

#define SPI0_PLATFORM_DATA_END() \
    .irq = IRQ_SPI0_IDX, \
    .reg = JL_SPI0, \
};

#define SPI1_PLATFORM_DATA_BEGIN(spi1_data) \
    static const struct spi_platform_data spi1_data = { \

#define SPI1_PLATFORM_DATA_END() \
    .irq = IRQ_SPI1_IDX, \
    .reg = JL_SPI1, \
};

#define SPI2_PLATFORM_DATA_BEGIN(spi2_data) \
    static const struct spi_platform_data spi2_data = { \

#define SPI2_PLATFORM_DATA_END() \
    .irq = IRQ_SPI2_IDX, \
    .reg = JL_SPI2, \
};

extern const struct device_operations spi_dev_ops;
/// \endcond

#endif


