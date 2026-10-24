#ifndef __ASM_GPIO_H__
#define __ASM_GPIO_H__

#include "asm/cpu.h"

#define IO_GROUP_NUM 		16

#define IO_PORTA_00 				(IO_GROUP_NUM * 0 + 0)
#define IO_PORTA_01 				(IO_GROUP_NUM * 0 + 1)
#define IO_PORTA_02 				(IO_GROUP_NUM * 0 + 2)
#define IO_PORTA_03 				(IO_GROUP_NUM * 0 + 3)
#define IO_PORTA_04 				(IO_GROUP_NUM * 0 + 4)
#define IO_PORTA_05 				(IO_GROUP_NUM * 0 + 5)
#define IO_PORTA_06 				(IO_GROUP_NUM * 0 + 6)
#define IO_PORTA_07 				(IO_GROUP_NUM * 0 + 7)
#define IO_PORTA_08 				(IO_GROUP_NUM * 0 + 8)
#define IO_PORTA_09 				(IO_GROUP_NUM * 0 + 9)
#define IO_PORTA_10 				(IO_GROUP_NUM * 0 + 10)
#define IO_PORTA_11 				(IO_GROUP_NUM * 0 + 11)
#define IO_PORTA_12 				(IO_GROUP_NUM * 0 + 12)
#define IO_PORTA_13 				(IO_GROUP_NUM * 0 + 13)
#define IO_PORTA_14 				(IO_GROUP_NUM * 0 + 14)
#define IO_PORTA_15 				(IO_GROUP_NUM * 0 + 15)
#define IO_PORTA_MASK               0xffff

#define IO_PORTB_00 				(IO_GROUP_NUM * 1 + 0)
#define IO_PORTB_01 				(IO_GROUP_NUM * 1 + 1)
#define IO_PORTB_02 				(IO_GROUP_NUM * 1 + 2)
#define IO_PORTB_03 				(IO_GROUP_NUM * 1 + 3)
#define IO_PORTB_04 				(IO_GROUP_NUM * 1 + 4)
#define IO_PORTB_05 				(IO_GROUP_NUM * 1 + 5)
#define IO_PORTB_06 				(IO_GROUP_NUM * 1 + 6)
#define IO_PORTB_07 				(IO_GROUP_NUM * 1 + 7)
#define IO_PORTB_08 				(IO_GROUP_NUM * 1 + 8)
#define IO_PORTB_09 				(IO_GROUP_NUM * 1 + 9)
#define IO_PORTB_10 				(IO_GROUP_NUM * 1 + 10)
#define IO_PORTB_11 				(IO_GROUP_NUM * 1 + 11)
#define IO_PORTB_12 				(IO_GROUP_NUM * 1 + 12)
#define IO_PORTB_13 				(IO_GROUP_NUM * 1 + 13)
#define IO_PORTB_14 				(IO_GROUP_NUM * 1 + 14)
#define IO_PORTB_15 				(IO_GROUP_NUM * 1 + 15)
#define IO_PORTB_MASK               0xffff

#define IO_PORTC_00 				(IO_GROUP_NUM * 2 + 0)
#define IO_PORTC_01 				(IO_GROUP_NUM * 2 + 1)
#define IO_PORTC_02 				(IO_GROUP_NUM * 2 + 2)
#define IO_PORTC_03 				(IO_GROUP_NUM * 2 + 3)
#define IO_PORTC_04 				(IO_GROUP_NUM * 2 + 4)
#define IO_PORTC_05 				(IO_GROUP_NUM * 2 + 5)
#define IO_PORTC_06 				(IO_GROUP_NUM * 2 + 6)
#define IO_PORTC_07 				(IO_GROUP_NUM * 2 + 7)
#define IO_PORTC_08 				(IO_GROUP_NUM * 2 + 8)
#define IO_PORTC_09 				(IO_GROUP_NUM * 2 + 9)
#define IO_PORTC_10 				(IO_GROUP_NUM * 2 + 10)
#define IO_PORTC_11 				(IO_GROUP_NUM * 2 + 11)
#define IO_PORTC_12 				(IO_GROUP_NUM * 2 + 12)
#define IO_PORTC_13 				(IO_GROUP_NUM * 2 + 13)
#define IO_PORTC_MASK               0x3fff

#define IO_PORTD_00 				(IO_GROUP_NUM * 3 + 0)
#define IO_PORTD_01 				(IO_GROUP_NUM * 3 + 1)
#define IO_PORTD_02 				(IO_GROUP_NUM * 3 + 2)
#define IO_PORTD_03 				(IO_GROUP_NUM * 3 + 3)
#define IO_PORTD_04 				(IO_GROUP_NUM * 3 + 4)
#define IO_PORTD_05 				(IO_GROUP_NUM * 3 + 5)
#define IO_PORTD_06 				(IO_GROUP_NUM * 3 + 6)
#define IO_PORTD_07 				(IO_GROUP_NUM * 3 + 7)
#define IO_PORTD_08 				(IO_GROUP_NUM * 3 + 8)
#define IO_PORTD_09 				(IO_GROUP_NUM * 3 + 9)
#define IO_PORTD_10 				(IO_GROUP_NUM * 3 + 10)
#define IO_PORTD_11 				(IO_GROUP_NUM * 3 + 11)
#define IO_PORTD_12 				(IO_GROUP_NUM * 3 + 12)
#define IO_PORTD_13 				(IO_GROUP_NUM * 3 + 13)
#define IO_PORTD_14 				(IO_GROUP_NUM * 3 + 14)
#define IO_PORTD_15 				(IO_GROUP_NUM * 3 + 15)
#define IO_PORTD_MASK               0xffff

#define IO_PORTE_00 				(IO_GROUP_NUM * 4 + 0)
#define IO_PORTE_01 				(IO_GROUP_NUM * 4 + 1)
#define IO_PORTE_02 				(IO_GROUP_NUM * 4 + 2)
#define IO_PORTE_03 				(IO_GROUP_NUM * 4 + 3)
#define IO_PORTE_04 				(IO_GROUP_NUM * 4 + 4)
#define IO_PORTE_05 				(IO_GROUP_NUM * 4 + 5)
#define IO_PORTE_06 				(IO_GROUP_NUM * 4 + 6)
#define IO_PORTE_07 				(IO_GROUP_NUM * 4 + 7)
#define IO_PORTE_08 				(IO_GROUP_NUM * 4 + 8)
#define IO_PORTE_09 				(IO_GROUP_NUM * 4 + 9)
#define IO_PORTE_10 				(IO_GROUP_NUM * 4 + 10)
#define IO_PORTE_11 				(IO_GROUP_NUM * 4 + 11)
#define IO_PORTE_12 				(IO_GROUP_NUM * 4 + 12)
#define IO_PORTE_13 				(IO_GROUP_NUM * 4 + 13)
#define IO_PORTE_14 				(IO_GROUP_NUM * 4 + 14)
#define IO_PORTE_15 				(IO_GROUP_NUM * 4 + 15)
#define IO_PORTE_MASK               0xffff

#define IO_PORTF_00 				(IO_GROUP_NUM * 5 + 0)
#define IO_PORTF_01 				(IO_GROUP_NUM * 5 + 1)
#define IO_PORTF_02 				(IO_GROUP_NUM * 5 + 2)
#define IO_PORTF_03 				(IO_GROUP_NUM * 5 + 3)
#define IO_PORTF_04 				(IO_GROUP_NUM * 5 + 4)
#define IO_PORTF_05 				(IO_GROUP_NUM * 5 + 5)
#define IO_PORTF_MASK               0x003f

#define IO_PORT_USB_DPA             (IO_GROUP_NUM * 6 + 0)
#define IO_PORT_USB_DMA             (IO_GROUP_NUM * 6 + 1)
#define IO_PORT_FUSB_MASK           0x0003

#define IO_PORT_USB_DPB             (IO_GROUP_NUM * 7 + 0)
#define IO_PORT_USB_DMB             (IO_GROUP_NUM * 7 + 1)
#define IO_PORT_HUSB_MASK           0x0003

#define USB_IO_OFFSET               4
#define IO_PORT_DP                  (IO_PORT_USB_DPA)
#define IO_PORT_DM                  (IO_PORT_USB_DMA)

#define IO_MAX_NUM                  (IO_PORT_USB_DMB + 1)

#define IO_PORT_PR_00               (IO_GROUP_NUM * 8 + 0)
#define IO_PORT_PR_01               (IO_GROUP_NUM * 8 + 1)

#define IO_PORT_PV_00               (IO_GROUP_NUM * 9 + 0)
#define IO_PORT_PV_01               (IO_GROUP_NUM * 9 + 1)

#define GPIOA                       (IO_GROUP_NUM * 0)
#define GPIOB                       (IO_GROUP_NUM * 1)
#define GPIOC                       (IO_GROUP_NUM * 2)
#define GPIOD                       (IO_GROUP_NUM * 3)
#define GPIOE                       (IO_GROUP_NUM * 4)
#define GPIOF                       (IO_GROUP_NUM * 5)
#define GPIOFUSB                    (IO_GROUP_NUM * 6)
#define GPIOHUSB                    (IO_GROUP_NUM * 7)
#define GPIOR                       (IO_GROUP_NUM * 8)
#define GPIOV                       (IO_GROUP_NUM * 9)


typedef enum {
    GPIO_DEINIT = 0,              //IO初始状态
    GPIO_HIGHZ = 1,               //高阻模式

    GPIO_OUTPUT_LOW = 2,
    GPIO_OUTPUT_HIGH = 3,

    GPIO_INPUT_FLOATING = 0x10,   //浮空输入
    GPIO_INPUT_PULL_UP_10K = 0x11,
    GPIO_INPUT_PULL_UP_100K,
    GPIO_INPUT_PULL_UP_1M,

    GPIO_INPUT_PULL_DOWN_10K = 0x21,
    GPIO_INPUT_PULL_DOWN_100K,
    GPIO_INPUT_PULL_DOWN_1M,
} gpio_mode_t;

typedef enum {
    GPIO_PULL_UP_DISABLE,
    GPIO_PULL_UP_10K,
    GPIO_PULL_UP_100K,          ///< WL83不支持该档位
    GPIO_PULL_UP_1M,            ///< WL83不支持该档位
} gpio_pull_up_mode_t;

typedef enum {
    GPIO_PULL_DOWN_DISABLE,
    GPIO_PULL_DOWN_10K,
    GPIO_PULL_DOWN_100K,        ///< WL83不支持该档位
    GPIO_PULL_DOWN_1M,          ///< WL83不支持该档位
} gpio_pull_down_mode_t;

typedef enum {
    GPIO_DRIVE_STRENGTH_2p5mA,	///< 最大驱动电流 2.5mA
    GPIO_DRIVE_STRENGTH_8p0mA,	///< 最大驱动电流 8.0mA
    GPIO_DRIVE_STRENGTH_18p5mA,	///< 最大驱动电流 18.5mA
    GPIO_DRIVE_STRENGTH_24p0mA,	///< 最大驱动电流 24.0mA
} gpio_drive_strength_t;

struct gpio_reg {
    volatile unsigned long in;
    volatile unsigned long out;
    volatile unsigned long dir;
    volatile unsigned long die;
    volatile unsigned long dieh;
    volatile unsigned long pu0;
    volatile unsigned long pu1;
    volatile unsigned long pd0;
    volatile unsigned long pd1;
    volatile unsigned long hd0;
    volatile unsigned long hd1;
    volatile unsigned long spl;
    volatile unsigned long con;
};

#define INPUT_GP_ICH_MAX_NUM  12
#define OUTPUT_GP_OCH_MAX_NUM 12

typedef enum {
    OUTPUT_CH_SIGNAL_TIMER0_PWM,
    OUTPUT_CH_SIGNAL_TIMER1_PWM,
    OUTPUT_CH_SIGNAL_TIMER2_PWM,
    OUTPUT_CH_SIGNAL_TIMER3_PWM,
    OUTPUT_CH_SIGNAL_TIMER4_PWM,
    OUTPUT_CH_SIGNAL_TIMER5_PWM,
    OUTPUT_CH_SIGNAL_CLOCK_OUT0,
    OUTPUT_CH_SIGNAL_CLOCK_OUT1,
    OUTPUT_CH_SIGNAL_CLOCK_OUT2,
    OUTPUT_CH_SIGNAL_CLOCK_OUT3,
    OUTPUT_CH_SIGNAL_UART1_RTS,
    OUTPUT_CH_SIGNAL_UART2_RTS,
    OUTPUT_CH_SIGNAL_PAP_WE,
    OUTPUT_CH_SIGNAL_PAP_RE,
    OUTPUT_CH_SIGNAL_GP_ICH0,
    OUTPUT_CH_SIGNAL_GP_ICH1,
    OUTPUT_CH_SIGNAL_ADC_DMIC_SCLK,
    OUTPUT_CH_SIGNAL_PLNK_SCLK,

    OUTPUT_CH_SIGNAL_FUSB_DEBUG,
    OUTPUT_CH_SIGNAL_P33_CLK_DEBUG,
    OUTPUT_CH_SIGNAL_P33_SIG_DEBUG0,
    OUTPUT_CH_SIGNAL_P33_SIG_DEBUG1,
    OUTPUT_CH_SIGNAL_P11_CLK_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_CLK_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_DATO0_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_DATO1_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_DATO2_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_DATO3_DEBUG,
    OUTPUT_CH_SIGNAL_ASS_DATO4_DEBUG,
    OUTPUT_CH_SIGNAL_WL_AMPE,
    OUTPUT_CH_SIGNAL_WL_LNAE,
    OUTPUT_CH_SIGNAL_WL_INT_ACTIVE,
    OUTPUT_CH_SIGNAL_WL_INT_STATUS,
    OUTPUT_CH_SIGNAL_WL_INT_FREQ,
    OUTPUT_CH_SIGNAL_WL_TRSW,
    OUTPUT_CH_SIGNAL_WL_TRSWN,
    OUTPUT_CH_SIGNAL_WL_ANT_ID0,
    OUTPUT_CH_SIGNAL_WL_ANT_ID1,
    OUTPUT_CH_SIGNAL_WL_ANT_ID2,
    OUTPUT_CH_SIGNAL_WL_ANT_ID3,
    OUTPUT_CH_SIGNAL_WL_TEST_CLK,
    OUTPUT_CH_SIGNAL_WPAN_RFEN,
    OUTPUT_CH_SIGNAL_WLAN_RFEN,

    OUTPUT_CH_SIGNAL_WL_DEBUG = 45,
} output_ch_signal_t;

typedef enum {
    INPUT_CH_SIGNAL_TIMER0_CIN,
    INPUT_CH_SIGNAL_TIMER1_CIN,
    INPUT_CH_SIGNAL_TIMER2_CIN,
    INPUT_CH_SIGNAL_TIMER3_CIN,
    INPUT_CH_SIGNAL_TIMER4_CIN,
    INPUT_CH_SIGNAL_TIMER5_CIN,

    INPUT_CH_SIGNAL_TIMER0_CAP,
    INPUT_CH_SIGNAL_TIMER1_CAP,
    INPUT_CH_SIGNAL_TIMER2_CAP,
    INPUT_CH_SIGNAL_TIMER3_CAP,
    INPUT_CH_SIGNAL_TIMER4_CAP,
    INPUT_CH_SIGNAL_TIMER5_CAP,

    INPUT_CH_SIGNAL_MCPWM0_CK,
    INPUT_CH_SIGNAL_MCPWM1_CK,
    INPUT_CH_SIGNAL_MCPWM2_CK,
    INPUT_CH_SIGNAL_MCPWM3_CK,
    INPUT_CH_SIGNAL_MCPWM0_FP,
    INPUT_CH_SIGNAL_MCPWM1_FP,

    INPUT_CH_SIGNAL_MCPWM2_FP,
    INPUT_CH_SIGNAL_MCPWM3_FP,
    INPUT_CH_SIGNAL_UART1_CTS,
    INPUT_CH_SIGNAL_UART2_CTS,
    INPUT_CH_SIGNAL_CAP,
    INPUT_CH_SIGNAL_CLK_PIN,

    INPUT_CH_SIGNAL_EXT_CLK,
    INPUT_CH_SIGNAL_ADC_DMIC_IDAT0,
    INPUT_CH_SIGNAL_ADC_DMIC_IDAT1,
    INPUT_CH_SIGNAL_PLNK_IDAT0,
    INPUT_CH_SIGNAL_PLNK_IDAT1,
    INPUT_CH_SIGNAL_ASS_DBG_CLKI,

    INPUT_CH_SIGNAL_ASS_DBG_DATI,
    INPUT_CH_SIGNAL_WLC_EXT_ACTIVE,
    INPUT_CH_SIGNAL_WLC_EXT_STATUS,
    INPUT_CH_SIGNAL_WLC_EXT_FREQ,

    INPUT_CH_SIGNAL_EXTI,
} input_ch_signal_t;

typedef enum {
    INPUT_CH_TYPE_GP_ICH,
    INPUT_CH_TYPE_TIME2_PWM = INPUT_CH_TYPE_GP_ICH + INPUT_GP_ICH_MAX_NUM,
    INPUT_CH_TYPE_TIME3_PWM,
    INPUT_CH_TYPE_FUSB_DP,
    INPUT_CH_TYPE_FUSB_DM,
    INPUT_CH_TYPE_HUSB_DP,
    INPUT_CH_TYPE_HUSB_DM,
} input_ch_type_t;

typedef enum {
    GPIO_FUNC_GPADC,

    GPIO_FUNC_MCPWM0_H,
    GPIO_FUNC_MCPWM1_H,
    GPIO_FUNC_MCPWM2_H,
    GPIO_FUNC_MCPWM3_H,
    GPIO_FUNC_MCPWM0_L,
    GPIO_FUNC_MCPWM1_L,
    GPIO_FUNC_MCPWM2_L,
    GPIO_FUNC_MCPWM3_L,
    GPIO_FUNC_LEDC0_DO,
    GPIO_FUNC_LEDC1_DO,
    GPIO_FUNC_LEDC2_DO,
    GPIO_FUNC_CAN_TX,
    GPIO_FUNC_CAN_RX,
    GPIO_FUNC_UART0_TX,
    GPIO_FUNC_UART1_TX,
    GPIO_FUNC_UART2_TX,
    GPIO_FUNC_UART3_TX,
    GPIO_FUNC_UART4_TX,
    GPIO_FUNC_ALNK_MCLK_OUTPUT,
    GPIO_FUNC_ALNK_LRCLK_OUTPUT,
    GPIO_FUNC_ALNK_SCLK_OUTPUT,
    GPIO_FUNC_ALNK_DAT0_OUTPUT,
    GPIO_FUNC_ALNK_DAT1_OUTPUT,
    GPIO_FUNC_ALNK_DAT2_OUTPUT,
    GPIO_FUNC_ALNK_DAT3_OUTPUT,
    GPIO_FUNC_UART0_RX,
    GPIO_FUNC_UART1_RX,
    GPIO_FUNC_UART2_RX,
    GPIO_FUNC_UART3_RX,
    GPIO_FUNC_UART4_RX,
    GPIO_FUNC_QDEC0_A,
    GPIO_FUNC_QDEC0_B,
    GPIO_FUNC_ALNK_MCLK_INPUT,
    GPIO_FUNC_ALNK_LRCLK_INPUT,
    GPIO_FUNC_ALNK_SCLK_INPUT,
    GPIO_FUNC_ALNK_DAT0_INPUT,
    GPIO_FUNC_ALNK_DAT1_INPUT,
    GPIO_FUNC_ALNK_DAT2_INPUT,
    GPIO_FUNC_ALNK_DAT3_INPUT,
    GPIO_FUNC_SPI0_CLK,
    GPIO_FUNC_SPI0_DA0,
    GPIO_FUNC_SPI0_DA1,
    GPIO_FUNC_SPI0_DA2,
    GPIO_FUNC_SPI0_DA3,
    GPIO_FUNC_SPI1_CLK,
    GPIO_FUNC_SPI1_DA0,
    GPIO_FUNC_SPI1_DA1,
    GPIO_FUNC_SPI1_DA2,
    GPIO_FUNC_SPI1_DA3,
    GPIO_FUNC_SPI2_CLK,
    GPIO_FUNC_SPI2_DA0,
    GPIO_FUNC_SPI2_DA1,
    GPIO_FUNC_IIC0_SCL,
    GPIO_FUNC_IIC0_SDA,
    GPIO_FUNC_IIC1_SCL,
    GPIO_FUNC_IIC1_SDA,
    GPIO_FUNC_IIC2_SCL,
    GPIO_FUNC_IIC2_SDA,
} gpio_function_t;

typedef enum {
    PFI_GP_ICH0 = ((u32)(&(JL_IMAP->FI_GP_ICH0))),
    PFI_GP_ICH1 = ((u32)(&(JL_IMAP->FI_GP_ICH1))),
    PFI_GP_ICH2 = ((u32)(&(JL_IMAP->FI_GP_ICH2))),
    PFI_GP_ICH3 = ((u32)(&(JL_IMAP->FI_GP_ICH3))),
    PFI_GP_ICH4 = ((u32)(&(JL_IMAP->FI_GP_ICH4))),
    PFI_GP_ICH5 = ((u32)(&(JL_IMAP->FI_GP_ICH5))),
    PFI_GP_ICH6 = ((u32)(&(JL_IMAP->FI_GP_ICH6))),
    PFI_GP_ICH7 = ((u32)(&(JL_IMAP->FI_GP_ICH7))),
    PFI_GP_ICH8 = ((u32)(&(JL_IMAP->FI_GP_ICH8))),
    PFI_GP_ICH9 = ((u32)(&(JL_IMAP->FI_GP_ICH9))),
    PFI_GP_ICH10 = ((u32)(&(JL_IMAP->FI_GP_ICH10))),
    PFI_GP_ICH11 = ((u32)(&(JL_IMAP->FI_GP_ICH11))),
    PFI_SPI0_CLK = ((u32)(&(JL_IMAP->FI_SPI0_CLK))),
    PFI_SPI0_DA0 = ((u32)(&(JL_IMAP->FI_SPI0_DA0))),
    PFI_SPI0_DA1 = ((u32)(&(JL_IMAP->FI_SPI0_DA1))),
    PFI_SPI0_DA2 = ((u32)(&(JL_IMAP->FI_SPI0_DA2))),
    PFI_SPI0_DA3 = ((u32)(&(JL_IMAP->FI_SPI0_DA3))),
    PFI_SPI1_CLK = ((u32)(&(JL_IMAP->FI_SPI1_CLK))),
    PFI_SPI1_DA0 = ((u32)(&(JL_IMAP->FI_SPI1_DA0))),
    PFI_SPI1_DA1 = ((u32)(&(JL_IMAP->FI_SPI1_DA1))),
    PFI_SPI1_DA2 = ((u32)(&(JL_IMAP->FI_SPI1_DA2))),
    PFI_SPI1_DA3 = ((u32)(&(JL_IMAP->FI_SPI1_DA3))),
    PFI_SPI2_CLK = ((u32)(&(JL_IMAP->FI_SPI2_CLK))),
    PFI_SPI2_DA0 = ((u32)(&(JL_IMAP->FI_SPI2_DA0))),
    PFI_SPI2_DA1 = ((u32)(&(JL_IMAP->FI_SPI2_DA1))),
    PFI_IIC0_SCL = ((u32)(&(JL_IMAP->FI_IIC0_SCL))),
    PFI_IIC0_SDA = ((u32)(&(JL_IMAP->FI_IIC0_SDA))),
    PFI_IIC1_SCL = ((u32)(&(JL_IMAP->FI_IIC1_SCL))),
    PFI_IIC1_SDA = ((u32)(&(JL_IMAP->FI_IIC1_SDA))),
    PFI_IIC2_SCL = ((u32)(&(JL_IMAP->FI_IIC2_SCL))),
    PFI_IIC2_SDA = ((u32)(&(JL_IMAP->FI_IIC2_SDA))),
    PFI_UART0_RX = ((u32)(&(JL_IMAP->FI_UART0_RX))),
    PFI_UART1_RX = ((u32)(&(JL_IMAP->FI_UART1_RX))),
    PFI_UART2_RX = ((u32)(&(JL_IMAP->FI_UART2_RX))),
    PFI_UART3_RX = ((u32)(&(JL_IMAP->FI_UART3_RX))),
    PFI_UART4_RX = ((u32)(&(JL_IMAP->FI_UART4_RX))),
    PFI_QDEC0_A  = ((u32)(&(JL_IMAP->FI_QDEC0_A))),
    PFI_QDEC0_B  = ((u32)(&(JL_IMAP->FI_QDEC0_B))),
    PFI_CAN_RX   = ((u32)(&(JL_IMAP->FI_CAN_RX))),
    PFI_ALNK_MCLK = ((u32)(&(JL_IMAP->FI_ALNK0_MCLK))),
    PFI_ALNK_LRCK = ((u32)(&(JL_IMAP->FI_ALNK0_LRCK))),
    PFI_ALNK_SCLK = ((u32)(&(JL_IMAP->FI_ALNK0_SCLK))),
    PFI_ALNK_DAT0 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT0))),
    PFI_ALNK_DAT1 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT1))),
    PFI_ALNK_DAT2 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT2))),
    PFI_ALNK_DAT3 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT3))),
    PFI_TOTAl = ((u32)(&(JL_IMAP->FI_TOTAL))),
} pfi_table_t;


u32 *gpio2crossbar_outreg(u32 gpio);

u32 gpio2crossbar_inport(u32 gpio);

int gpio_och_sel_output_signal(u32 gpio, output_ch_signal_t signal);

int gpio_och_disable_output_signal(u32 gpio, output_ch_signal_t signal);

int gpio_ich_sel_input_signal(u32 gpio, input_ch_signal_t signal, input_ch_type_t type);

int gpio_ich_disable_input_signal(u32 gpio, input_ch_signal_t signal);

int gpio_set_fun_input_port(u32 gpio, pfi_table_t pfun);

int gpio_disable_fun_input_port(pfi_table_t pfun);

u16 gpio_get_gp_och_use_flag(void);

u16 gpio_get_gp_ich_use_flag(void);


#if 0
#define     IO_DEBUG_0(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     IO_DEBUG_1(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}
#define     IO_DEBUG_TOGGLE(i,x)  {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT ^= BIT(x);}
#else
#define     IO_DEBUG_0(i,x)         {}
#define     IO_DEBUG_1(i,x)         {}
#define     IO_DEBUG_TOGGLE(i,x)    {}
#endif

#endif

