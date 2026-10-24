
//===============================================================================//
//
//      input IO define
//
//===============================================================================//
#define PA0_IN    1
#define PA1_IN    2
#define PA2_IN    3
#define PA3_IN    4
#define PA4_IN    5
#define PA5_IN    6
#define PA6_IN    7
#define PA7_IN    8
#define PA8_IN    9
#define PA9_IN    10
#define PA10_IN   11
#define PA11_IN   12
#define PA12_IN   13
#define PA13_IN   14
#define PA14_IN   15
#define PA15_IN   16
#define PB0_IN    17
#define PB1_IN    18
#define PB2_IN    19
#define PB3_IN    20
#define PB4_IN    21
#define PB5_IN    22
#define PB6_IN    23
#define PB7_IN    24
#define PB8_IN    25
#define PB9_IN    26
#define PB10_IN   27
#define PB11_IN   28
#define PB12_IN   29
#define PB13_IN   30
#define PB14_IN   31
#define PB15_IN   32
#define PC0_IN    33
#define PC1_IN    34
#define PC2_IN    35
#define PC3_IN    36
#define PC4_IN    37
#define PC5_IN    38
#define PC6_IN    39
#define PC7_IN    40
#define PC8_IN    41
#define PC9_IN    42
#define PC10_IN   43
#define PC11_IN   44
#define PC12_IN   45
#define PC13_IN   46
#define PD0_IN    47
#define PD1_IN    48
#define PD2_IN    49
#define PD3_IN    50
#define PD4_IN    51
#define PD5_IN    52
#define PD6_IN    53
#define PD7_IN    54
#define PD8_IN    55
#define PD9_IN    56
#define PD10_IN   57
#define PD11_IN   58
#define PD12_IN   59
#define PD13_IN   60
#define PD14_IN   61
#define PD15_IN   62
#define PE0_IN    63
#define PE1_IN    64
#define PE2_IN    65
#define PE3_IN    66
#define PE4_IN    67
#define PE5_IN    68
#define PE6_IN    69
#define PE7_IN    70
#define PE8_IN    71
#define PE9_IN    72
#define PE10_IN   73
#define PE11_IN   74
#define PE12_IN   75
#define PE13_IN   76
#define PE14_IN   77
#define PE15_IN   78
#define FUSBDP_IN 79
#define FUSBDM_IN 80
#define HUSBDP_IN 81
#define HUSBDM_IN 82
#define PV0_IN    83
#define PV1_IN    84


//===============================================================================//
//
//      function input select sfr
//
//===============================================================================//
typedef struct {
    ___RW ___u8 FI_GP_ICH0;
    ___RW ___u8 FI_GP_ICH1;
    ___RW ___u8 FI_GP_ICH2;
    ___RW ___u8 FI_GP_ICH3;
    ___RW ___u8 FI_GP_ICH4;
    ___RW ___u8 FI_GP_ICH5;
    ___RW ___u8 FI_GP_ICH6;
    ___RW ___u8 FI_GP_ICH7;
    ___RW ___u8 FI_GP_ICH8;
    ___RW ___u8 FI_GP_ICH9;
    ___RW ___u8 FI_GP_ICH10;
    ___RW ___u8 FI_GP_ICH11;
    ___RW ___u8 FI_CAN_RX;
    ___RW ___u8 FI_SPI0_CLK;
    ___RW ___u8 FI_SPI0_DA0;
    ___RW ___u8 FI_SPI0_DA1;
    ___RW ___u8 FI_SPI0_DA2;
    ___RW ___u8 FI_SPI0_DA3;
    ___RW ___u8 FI_SPI1_CLK;
    ___RW ___u8 FI_SPI1_DA0;
    ___RW ___u8 FI_SPI1_DA1;
    ___RW ___u8 FI_SPI1_DA2;
    ___RW ___u8 FI_SPI1_DA3;
    ___RW ___u8 FI_SPI2_CLK;
    ___RW ___u8 FI_SPI2_DA0;
    ___RW ___u8 FI_SPI2_DA1;
    ___RW ___u8 FI_IIC0_SCL;
    ___RW ___u8 FI_IIC0_SDA;
    ___RW ___u8 FI_IIC1_SCL;
    ___RW ___u8 FI_IIC1_SDA;
    ___RW ___u8 FI_IIC2_SCL;
    ___RW ___u8 FI_IIC2_SDA;
    ___RW ___u8 FI_UART0_RX;
    ___RW ___u8 FI_UART1_RX;
    ___RW ___u8 FI_UART2_RX;
    ___RW ___u8 FI_UART3_RX;
    ___RW ___u8 FI_UART4_RX;
    ___RW ___u8 FI_QDEC0_A;
    ___RW ___u8 FI_QDEC0_B;
    ___RW ___u8 FI_ALNK0_MCLK;
    ___RW ___u8 FI_ALNK0_LRCK;
    ___RW ___u8 FI_ALNK0_SCLK;
    ___RW ___u8 FI_ALNK0_DAT0;
    ___RW ___u8 FI_ALNK0_DAT1;
    ___RW ___u8 FI_ALNK0_DAT2;
    ___RW ___u8 FI_ALNK0_DAT3;
    ___RW ___u8 FI_TOTAL;
} JL_IMAP_TypeDef;

#define JL_IMAP_BASE      (ls_base + map_adr(0x6c, 0x00))
#define JL_IMAP           ((JL_IMAP_TypeDef *)JL_IMAP_BASE)

