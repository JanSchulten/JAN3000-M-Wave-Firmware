#ifndef __ASM_HWI_H__
#define __ASM_HWI_H__

#include "generic/typedef.h"

/// \cond DO_NOT_DOCUMENT
// #define CPU_IRQ_USAGE_DEBUG

struct irq_info {
    u8 index;
    u8 prio;
    u8 cpu_id;
};

/// \endcond
/**
 * \name IRQ select
 * \{
 */
#define IRQ_SDTAP_IDX      0
#define IRQ_EXCEPTION_IDX  1
#define IRQ_SYSCALL_IDX    2
#define IRQ_TICK_TMR_IDX   3
#define IRQ_TIMER0_IDX     4
#define IRQ_TIMER1_IDX     5
#define IRQ_TIMER2_IDX     6
#define IRQ_UART0_IDX      7
#define IRQ_UART1_IDX      8
#define IRQ_UART2_IDX      9
#define IRQ_SPI0_IDX       10
#define IRQ_SPI1_IDX       11
#define IRQ_IIC0_IDX       12
#define IRQ_IIC1_IDX       13
#define IRQ_PORT_IDX       14
#define IRQ_SARADC_IDX     15
#define IRQ_GPC_IDX        16
#define IRQ_USB_SOF_IDX    17
#define IRQ_USB_CTRL_IDX   18
#define IRQ_TIMER3_IDX     19
#define IRQ_MCPWM_TMR_IDX  20
#define IRQ_PMU_SOFT_IDX   21
#define IRQ_PMU_PND0_IDX   22
#define IRQ_PMU_PND1_IDX   23
#define IRQ_QDEC_IDX       24
#define IRQ_PAP_IDX        25
#define IRQ_WWDG_IDX       26
#define IRQ_MCPWM_CHX_IDX  27
#define IRQ_SPI2_IDX       28
#define IRQ_SD0_IDX        29
#define IRQ_SHA_IDX        30
#define IRQ_ETMMAC_IDX     31
#define IRQ_IIC2_IDX       34
#define IRQ_SD1_IDX        35
#define IRQ_CAN_IDX        36
#define IRQ_LEDC_IDX       37
#define IRQ_LRCT_IDX       38
#define IRQ_LED_IDX        39
#define IRQ_TIMER4_IDX     40
#define IRQ_TIMER5_IDX     41
#define IRQ_UART3_IDX      42
#define IRQ_UART4_IDX      43

#define IRQ_UDMA0_IDX      52
#define IRQ_UDMA1_IDX      53
#define IRQ_SRC_IDX        54
#define IRQ_AES_IDX        55
#define IRQ_EQ_IDX         56
#define IRQ_DCP_IDX        57
#define IRQ_DVB0_IDX       58
#define IRQ_DVB1_IDX       59

#define IRQ_USB1_CTRL_IDX  60
#define IRQ_USB1_SOF_IDX   61
#define IRQ_USB1_TRIM_IDX  62

#define IRQ_JPG1_IDX       71
#define IRQ_JPG0_IDX       72
#define IRQ_ISC_IDX        73
#define IRQ_ISP0_IDX       74
#define IRQ_REP_IDX        75
#define IRQ_IMC_IDX        76
#define IRQ_DMM_IDX        77
#define IRQ_DPI_IDX        78
#define IRQ_CSI_IDX        79
#define IRQ_DSI_IDX        80
#define IRQ_MCV_IDX        81
#define IRQ_DMA2D_IDX      82
#define IRQ_GPU_IDX        83

#define IRQ_SCALE_IDX      84

#define IRQ_PLNK_IDX       90
#define IRQ_ALNK_IDX       91
#define IRQ_AUDIO_ADC_IDX  92
#define IRQ_AUDIO_DAC_IDX  93

#define IRQ_BT_TIMEBASE_IDX 100
#define IRQ_BLE_RX_IDX     101
#define IRQ_BLE_EVENT_IDX  102
#define IRQ_BT_CLKN_IDX    103
#define IRQ_BREDR_IDX      104
#define IRQ_BT_RXMCH_IDX   105
#define IRQ_SRC_SYNC_IDX   106
#define IRQ_WF_IDX         107
#define IRQ_WF_RSVD_IDX    108

#define IRQ_FFT_IDX        116

#define IRQ_SOFT0_IDX      120
#define IRQ_SOFT1_IDX      121
#define IRQ_SOFT2_IDX      122
#define IRQ_SOFT3_IDX      123
#define IRQ_SOFT4_IDX      124
#define IRQ_SOFT5_IDX      125
#define IRQ_SOFT6_IDX      126
#define IRQ_SOFT7_IDX      127


extern u32 _TEE_USER_IRQ_BEGIN[];

#define ISR_ENTRY          0x11fe00
/* \} name */

/// \cond DO_NOT_DOCUMENT
void bit_clr_ie(unsigned char index, u8 cpuid);
void bit_set_ie(unsigned char index, u8 cpuid);
void reg_set_ip(unsigned char index, unsigned char dat, u8 cpuid);
/// \endcond

/**
 * @brief request_irq, 中断注册
 * @param index : 使用的中断号，例如IRQ_TIMER0_IDX 等
 * @param priority : 中断优先级0~7，越大优先级越高
 * @param handler : 中断函数
 * @param cpu_id : 中断注册到的cpu, 0是注册到cpu0,1是注册到cpu1，cpuid为0xff时注册到当前CPU。
 */
void request_irq(unsigned char index, unsigned char priority, void (*handler)(void), unsigned char cpu_id);

/**
 * @brief unrequest_irq, 解除中断注册
 * @param index : 使用的中断号，例如IRQ_TIMER0_IDX 等
 * @param cpu_id : 中断注册到的cpu, 0是注册到cpu0,1是注册到cpu1
 */
void unrequest_irq(u8 index, u8 cpu_id);

/// \cond DO_NOT_DOCUMENT
u8 check_pending(unsigned char index, u8 cpuid);
void interrupt_init(void);
void dump_cpu_irq_usage(void);
u8 irq_read(u32 index);

#define irq_disable(x)  bit_clr_ie(x, 0)
#define irq_enable(x)   bit_set_ie(x, 0)

//#define IRQ_TIME_COUNT_EN

#ifdef IRQ_TIME_COUNT_EN
void irq_handler_enter(int irq);

void irq_handler_exit(int irq);

void irq_handler_times_dump();
#else

#define irq_handler_enter(irq)      do { }while(0)
#define irq_handler_exit(irq)       do { }while(0)
#define irq_handler_times_dump()    do { }while(0)

#endif
/// \endcond

#endif
