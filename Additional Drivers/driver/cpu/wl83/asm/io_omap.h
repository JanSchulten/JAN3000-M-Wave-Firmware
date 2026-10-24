
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define FO_GP_OCH0        ((0 << 2)|BIT(1))
#define FO_GP_OCH1        ((1 << 2)|BIT(1))
#define FO_GP_OCH2        ((2 << 2)|BIT(1))
#define FO_GP_OCH3        ((3 << 2)|BIT(1))
#define FO_GP_OCH4        ((4 << 2)|BIT(1))
#define FO_GP_OCH5        ((5 << 2)|BIT(1))
#define FO_GP_OCH6        ((6 << 2)|BIT(1))
#define FO_GP_OCH7        ((7 << 2)|BIT(1))
#define FO_GP_OCH8        ((8 << 2)|BIT(1))
#define FO_GP_OCH9        ((9 << 2)|BIT(1))
#define FO_GP_OCH10       ((10 << 2)|BIT(1))
#define FO_GP_OCH11       ((11 << 2)|BIT(1))
#define FO_MCPWM0_H       ((12 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM1_H       ((13 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM2_H       ((14 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM3_H       ((15 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM0_L       ((16 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM1_L       ((17 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM2_L       ((18 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM3_L       ((19 << 2)|BIT(1)|BIT(0))
#define FO_LEDC0_DO       ((20 << 2)|BIT(1)|BIT(0))
#define FO_LEDC1_DO       ((21 << 2)|BIT(1)|BIT(0))
#define FO_LEDC2_DO       ((22 << 2)|BIT(1)|BIT(0))
#define FO_CAN_TX         ((23 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_CLK       ((24 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA0       ((25 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA1       ((26 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA2       ((27 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA3       ((28 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_CLK       ((29 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA0       ((30 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA1       ((31 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA2       ((32 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA3       ((33 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_CLK       ((34 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA0       ((35 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA1       ((36 << 2)|BIT(1)|BIT(0))
#define FO_IIC0_SCL       ((37 << 2)|BIT(1)|BIT(0))
#define FO_IIC0_SDA       ((38 << 2)|BIT(1)|BIT(0))
#define FO_IIC1_SCL       ((39 << 2)|BIT(1)|BIT(0))
#define FO_IIC1_SDA       ((40 << 2)|BIT(1)|BIT(0))
#define FO_IIC2_SCL       ((41 << 2)|BIT(1)|BIT(0))
#define FO_IIC2_SDA       ((42 << 2)|BIT(1)|BIT(0))
#define FO_UART0_TX       ((43 << 2)|BIT(1)|BIT(0))
#define FO_UART1_TX       ((44 << 2)|BIT(1)|BIT(0))
#define FO_UART2_TX       ((45 << 2)|BIT(1)|BIT(0))
#define FO_UART3_TX       ((46 << 2)|BIT(1)|BIT(0))
#define FO_UART4_TX       ((47 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_MCLK     ((48 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_LRCK     ((49 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_SCLK     ((50 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT0     ((51 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT1     ((52 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT2     ((53 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT3     ((54 << 2)|BIT(1)|BIT(0))

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    ___RW ___u8 PA0_OUT;
    ___RW ___u8 PA1_OUT;
    ___RW ___u8 PA2_OUT;
    ___RW ___u8 PA3_OUT;
    ___RW ___u8 PA4_OUT;
    ___RW ___u8 PA5_OUT;
    ___RW ___u8 PA6_OUT;
    ___RW ___u8 PA7_OUT;
    ___RW ___u8 PA8_OUT;
    ___RW ___u8 PA9_OUT;
    ___RW ___u8 PA10_OUT;
    ___RW ___u8 PA11_OUT;
    ___RW ___u8 PA12_OUT;
    ___RW ___u8 PA13_OUT;
    ___RW ___u8 PA14_OUT;
    ___RW ___u8 PA15_OUT;
    ___RW ___u8 PB0_OUT;
    ___RW ___u8 PB1_OUT;
    ___RW ___u8 PB2_OUT;
    ___RW ___u8 PB3_OUT;
    ___RW ___u8 PB4_OUT;
    ___RW ___u8 PB5_OUT;
    ___RW ___u8 PB6_OUT;
    ___RW ___u8 PB7_OUT;
    ___RW ___u8 PB8_OUT;
    ___RW ___u8 PB9_OUT;
    ___RW ___u8 PB10_OUT;
    ___RW ___u8 PB11_OUT;
    ___RW ___u8 PB12_OUT;
    ___RW ___u8 PB13_OUT;
    ___RW ___u8 PB14_OUT;
    ___RW ___u8 PB15_OUT;
    ___RW ___u8 PC0_OUT;
    ___RW ___u8 PC1_OUT;
    ___RW ___u8 PC2_OUT;
    ___RW ___u8 PC3_OUT;
    ___RW ___u8 PC4_OUT;
    ___RW ___u8 PC5_OUT;
    ___RW ___u8 PC6_OUT;
    ___RW ___u8 PC7_OUT;
    ___RW ___u8 PC8_OUT;
    ___RW ___u8 PC9_OUT;
    ___RW ___u8 PC10_OUT;
    ___RW ___u8 PC11_OUT;
    ___RW ___u8 PC12_OUT;
    ___RW ___u8 PC13_OUT;
    ___RW ___u8 PD0_OUT;
    ___RW ___u8 PD1_OUT;
    ___RW ___u8 PD2_OUT;
    ___RW ___u8 PD3_OUT;
    ___RW ___u8 PD4_OUT;
    ___RW ___u8 PD5_OUT;
    ___RW ___u8 PD6_OUT;
    ___RW ___u8 PD7_OUT;
    ___RW ___u8 PD8_OUT;
    ___RW ___u8 PD9_OUT;
    ___RW ___u8 PD10_OUT;
    ___RW ___u8 PD11_OUT;
    ___RW ___u8 PD12_OUT;
    ___RW ___u8 PD13_OUT;
    ___RW ___u8 PD14_OUT;
    ___RW ___u8 PD15_OUT;
    ___RW ___u8 PE0_OUT;
    ___RW ___u8 PE1_OUT;
    ___RW ___u8 PE2_OUT;
    ___RW ___u8 PE3_OUT;
    ___RW ___u8 PE4_OUT;
    ___RW ___u8 PE5_OUT;
    ___RW ___u8 PE6_OUT;
    ___RW ___u8 PE7_OUT;
    ___RW ___u8 PE8_OUT;
    ___RW ___u8 PE9_OUT;
    ___RW ___u8 PE10_OUT;
    ___RW ___u8 PE11_OUT;
    ___RW ___u8 PE12_OUT;
    ___RW ___u8 PE13_OUT;
    ___RW ___u8 PE14_OUT;
    ___RW ___u8 PE15_OUT;
    ___RW ___u8 FUSBDP_OUT;
    ___RW ___u8 FUSBDM_OUT;
    ___RW ___u8 HUSBDP_OUT;
    ___RW ___u8 HUSBDM_OUT;
} JL_OMAP_TypeDef;

#define JL_OMAP_BASE      (ls_base + map_adr(0x68, 0x00))
#define JL_OMAP           ((JL_OMAP_TypeDef *)JL_OMAP_BASE)

