#ifndef __DEVICE_UART_H__
#define __DEVICE_UART_H__

#include "generic/typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "os/os_api.h"
#include "asm/uart.h"
#include "asm/gpio.h"

/// \cond DO_NOT_DOCUMENT
#define UART_DISABLE        0x00000000
#define UART_DMA_SUPPORT 	0x00000001
#define UART_TX_USE_DMA 	0x00000003
#define UART_RX_USE_DMA 	0x00000005
#define UART_DEBUG 			0x00000008
/// \endcond

/**
 * \name uart_platform_data
 * \{
 */
struct uart_platform_data {
    u8  irq;                  				  ///<  中断号，在注册的时候已经对应设置好了
    u8  flow_ctl_enable;      				  ///<  1:使能串口流控
    u8  rts_hw_enable;        				  ///<  1:使能硬件RTS
    u8  rts_idle_level;       				  ///<  0:RTS空闲电平为低; 1:RTS空闲电平为高
    u8  cts_idle_level;       				  ///<  0:CTS空闲电平为低; 1:CTS空闲电平为高
    u8  rx_thresh;            				  ///<  接收BUF触发流控的比值0-100
    u8  disable_tx_irq;       				  ///<  1:不使用发送中断
    u8  disable_rx_irq;       				  ///<  1:不使用中断接收
    u8  disable_ot_irq;       				  ///<  1:不使用超时中断
    u8  tx_pin_hd_level;                      ///<  tx pin强驱等级

    int tx_pin;               				  ///<  发送引脚，不配置需设置-1
    int rx_pin;               				  ///<  接收引脚，不配置需设置-1
    int rts_pin;              				  ///<  串口一控制流RTS脚，不配置需设置-1
    int cts_pin;              				  ///<  串口一控制流CTS脚，不配置需设置-1
    int flags;             				      ///<  串口用作打印
    u32 baudrate;             				  ///<  波特率设置。理论范围：CLK/3 ~ CLK/4/(0xFFFF+1) -> CLK/3 ~ CLK/262144

    u32 max_continue_recv_cnt;				  ///<  连续接收最大字节，范围：0 ~ 0xFFFFFFFF
    u32 idle_sys_clk_cnt;     				  ///<  超时计数器。在指定时间里没有收到数据，则产生超时中断，范围：0 ~ 0xFFFFFFFF
    uart_parity_t parity;       		 	  ///<  奇偶校验位。可选：UART_PARITY_DISABLE、UART_PARITY_EVEN、UART_PARITY_ODD
};
/* \} name */

/**
 * \name UART error
 * \{
 */
typedef enum {
    UART_CIRCULAR_BUFFER_WRITE_OVERLAY = -1, ///<  循环buf写满
    UART_RECV_TIMEOUT = -2,				  	 ///<  接收超时
    UART_RECV_EXIT = -3,				     ///<  接收终止退出
} uart_event_t;
/* \} name */

/**
 * \name UART IRQ event
 * \{
 */
typedef enum {
    UART_IRQ_EVENT_NULL,					///<  0.无
    UART_IRQ_EVENT_TX,						///<  1.TX pnd
    UART_IRQ_EVENT_RX, 						///<  2.RX pnd,当RX DMA连续收到的数据超过RXCNT的设定值时
    UART_IRQ_EVENT_OT, 						///<  3.OverTime pnd,当RX DMA收到一包数据后经过OTCNT个CLK后未再收到新数据时
    UART_IRQ_EVENT_CTS,						///<  4.CTS pnd
    UART_IRQ_EVENT_CHK, 					///<  5.CHK pnd
} uart_irq_event_t;
/* \} name */

/**
 * \name UART dev_ioctl funciton selest
 * \{
 */
#define UART_MAGIC                                'U'
#define IOCTL_UART_FLUSH                          _IO(UART_MAGIC,1)                 ///<  串口重载
#define IOCTL_UART_SET_SEND_BLOCK                 _IOW(UART_MAGIC,2,bool)           ///<  设置串口发送阻塞
#define IOCTL_UART_SET_RECV_BLOCK                 _IOW(UART_MAGIC,3,bool)           ///<  设置串口接收阻塞
#define IOCTL_UART_SET_RECV_ALL                   _IOW(UART_MAGIC,4,bool)           ///<  设置串口等待接收满才退出(需先使用上一条命令配置UART为阻塞接收)
#define IOCTL_UART_SET_RECV_TIMEOUT               _IOW(UART_MAGIC,5,u32)            ///<  设置串口接收超时
#define IOCTL_UART_SET_RECV_TIMEOUT_CB            _IOW(UART_MAGIC,6,int (*)(void))  ///<  设置串口超时之后的回调函数
#define IOCTL_UART_GET_RECV_CNT                   _IOR(UART_MAGIC,7,u32 *)          ///<  获取串口循环buf中已接收到未被取走的数据的累计值
#define IOCTL_UART_START                          _IO(UART_MAGIC,8)                 ///<  开启串口
#define IOCTL_UART_STOP                           _IO(UART_MAGIC,9)                 ///<  暂停串口
#define IOCTL_UART_SET_CIRCULAR_BUFF_ADDR         _IOW(UART_MAGIC,10,u32)           ///<  设置串口循环buf地址
#define IOCTL_UART_SET_CIRCULAR_BUFF_LENTH        _IOW(UART_MAGIC,11,u32)           ///<  设置串口循环buf长度
#define IOCTL_UART_SET_BAUDRATE                   _IOW(UART_MAGIC,12,u32)           ///<  串口运行过程中修改波特率
#define IOCTL_UART_FLOW_CTL_RTS_SUSPEND           _IO(UART_MAGIC,13)                ///<  串口控制流挂起
#define IOCTL_UART_FLOW_CTL_RTS_RESUME            _IO(UART_MAGIC,14)                ///<  串口控制流恢复
#define IOCTL_UART_SET_IRQ_EVENT_CB               _IOW(UART_MAGIC,15,void (*)(uart_irq_event_t)) ///<  设置中断事件回调函数
/* \} name */


extern const struct device_operations uart_dev_ops;

extern int uart_init(const struct uart_platform_data *data);

extern void uart_debug_suspend(void);

extern void uart_debug_resume(void);

/// \cond DO_NOT_DOCUMENT
extern void put_bin(char *str, u32 bin);
#define putbin(x) 	put_bin(#x,x)
#define putline()   printf("-> %s %d \r\n",__func__,__LINE__)
#define putint(x)   printf("-> %s = 0x%x \r\n",#x,x)
/// \endcond

#endif

