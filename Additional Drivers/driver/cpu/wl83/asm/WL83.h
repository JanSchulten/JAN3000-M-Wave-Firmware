#ifndef __WL83__
#define __WL83__

#include "csfr.h"

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

#define hs_base            0xfe0000
#define ls_base            0xfc0000
#define vd_base            0xf40000
#define as_base            0xf20000
#define wl_base            0xf00000

#define ___RW               volatile       // read write
#define ___RO               volatile const // only read
#define ___WO               volatile       // only write

#define ___u8               unsigned int   // u8  to u32 special for struct
#define ___u16              unsigned int   // u16 to u32 special for struct
#define ___u32              unsigned int

#define ___s8(x)            char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define ___s16(x)           short(x); short(reserved_1_##x)
#define ___s32(x)           int(x)

#define map_adr(grp, adr)  ((64 * grp + adr) * 4)     // grp(0x0-0xff), adr(0x0-0x3f)

//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for hemu
typedef struct {
    ___RW ___u32 WREN;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 MSG0;
    ___RW ___u32 MSG1;
    ___RW ___u32 MSG2;
    ___RW ___u32 MSG3;
    ___RO ___u32 LOG0;
    ___RO ___u32 LOG1;
    ___RO ___u32 LOG2;
    ___RO ___u32 LOG3;
} JL_HEMU_TypeDef;

#define JL_HEMU_BASE                    (hs_base + map_adr(0x00, 0x00))
#define JL_HEMU                         ((JL_HEMU_TypeDef *)JL_HEMU_BASE)

//............. 0x0100 - 0x01ff............ for hsb_mbist
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 SEL;
    ___RW ___u32 BEG;
    ___RW ___u32 END;
    ___RW ___u32 DAT_VLD0;
    ___RW ___u32 DAT_VLD1;
    ___RW ___u32 DAT_VLD2;
    ___RW ___u32 DAT_VLD3;
} JL_HMBIST_TypeDef;

#define JL_HMBIST_BASE                  (hs_base + map_adr(0x01, 0x00))
#define JL_HMBIST                       ((JL_HMBIST_TypeDef *)JL_HMBIST_BASE)

//............. 0x0200 - 0x02ff............ for sfc0
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 USER_CODE;
    ___RW ___u32 OFFSET_ADR;
    ___RW ___u32 DEC_KEY;
    ___RW ___u32 NDEC_SADR;
    ___RW ___u32 NDEC_EADR;
    ___RW ___u32 CLK_CON;
} JL_SFCTZ_TypeDef;

typedef struct {
    ___RW ___u32 FSPG_CON;
    ___RW ___u32 IOMC0;
} JL_IOMCTZ_TypeDef;

typedef struct {
    ___RO ___u32 IN;
    ___RW ___u32 OUT;
    ___RW ___u32 DIR;
    ___RW ___u32 DIE;
    ___RW ___u32 DIEH;
    ___RW ___u32 PU0;
    ___RW ___u32 PU1;
    ___RW ___u32 PD0;
    ___RW ___u32 PD1;
    ___RW ___u32 HD0;
    ___RW ___u32 HD1;
    ___RW ___u32 SPL;
    ___RW ___u32 OUT_BSR;
    ___RW ___u32 DIR_BSR;
    ___RW ___u32 DIE_BSR;
    ___RW ___u32 DIEH_BSR;
    ___RW ___u32 PU0_BSR;
    ___RW ___u32 PU1_BSR;
    ___RW ___u32 PD0_BSR;
    ___RW ___u32 PD1_BSR;
    ___RW ___u32 HD0_BSR;
    ___RW ___u32 HD1_BSR;
    ___RW ___u32 SPL_BSR;
} JL_PORTTZ_TypeDef;

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 BAUD;
    ___RW ___u32 BUF;
    ___WO ___u32 ADR;
    ___RW ___u32 CNT;
    ___RW ___u32 CON1;
    ___RW ___u8  ENCCON;
    ___WO ___u16 ENCKEY;
    ___WO ___u16 ENCADR;
    ___RW ___u32 REG;
    ___WO ___u32 FIFO;
} JL_SPITZ_TypeDef;

#define JL_SFCTZ_BASE                   (hs_base + map_adr(0x02, 0x00))
#define JL_SFCTZ                        ((JL_SFCTZ_TypeDef *)JL_SFCTZ_BASE)

#define JL_IOMCTZ_BASE                  (hs_base + map_adr(0x02, 0x07))
#define JL_IOMCTZ                       ((JL_IOMCTZ_TypeDef *)JL_IOMCTZ_BASE)

#define JL_PFTZ_BASE                    (hs_base + map_adr(0x02, 0x09))
#define JL_PORTF                        ((JL_PORTTZ_TypeDef *)JL_PFTZ_BASE)

#define JL_SPITZ_BASE                   (hs_base + map_adr(0x02, 0x20))
#define JL_SPITZ                        ((JL_SPITZ_TypeDef *)JL_SPITZ_BASE)

//............. 0x0300 - 0x03ff............ for mode
typedef struct {
    ___RW ___u16 MODE_CON;
    ___RW ___u16 PROT_CON;
} JL_MODE_TypeDef;

#define JL_MODE_BASE                    (hs_base + map_adr(0x03, 0x00))
#define JL_MODE                         ((JL_MODE_TypeDef *)JL_MODE_BASE)

//............. 0x0400 - 0x04ff............ for syscfg
typedef struct {
    ___RW ___u32 RST_SRC;
} JL_RST_TypeDef;

#define JL_RST_BASE                     (hs_base + map_adr(0x04, 0x00))
#define JL_RST                          ((JL_RST_TypeDef *)JL_RST_BASE)

typedef struct {
    ___RW ___u32 PWR_CON;
    ___RW ___u32 SYS_CON0;
    ___RW ___u32 SYS_CON1;
    ___RW ___u32 HSB_DIV;
    ___RW ___u32 HSB_SEL;
    ___RW ___u32 LSB_SEL;
    ___RW ___u32 STD_CON0;
    ___RW ___u32 STD_CON1;
    ___RW ___u32 REF_CON0;
    ___RW ___u32 PRP_CON0;
    ___RW ___u32 PRP_CON1;
    ___RW ___u32 PRP_CON2;
    ___RW ___u32 PRP_CON3;
    ___RW ___u32 PRP_TCON;
} JL_CLOCK_TypeDef;

#define JL_CLOCK_BASE                   (hs_base + map_adr(0x04, 0x01))
#define JL_CLOCK                        ((JL_CLOCK_TypeDef *)JL_CLOCK_BASE)

//............. 0x0500 - 0x05ff............ for pll0_ctl
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 NR;
} JL_PLL0_TypeDef;

#define JL_PLL0_BASE                    (hs_base + map_adr(0x05, 0x00))
#define JL_PLL0                         ((JL_PLL0_TypeDef *)JL_PLL0_BASE)

//............. 0x0600 - 0x06ff............ for dmc
typedef struct {
    ___RW ___u32 SDR_CON0;   // RW
    ___WO ___u32 SDR_CON1;   // W-only
    ___RW ___u32 SDR_CON2;   // RW
    ___RW ___u32 SDR_CON3;   // RW
    ___RW ___u32 SDR_CON4;   // RW
    ___WO ___u32 SDR_CON5;   // W-only
    ___RW ___u32 SDR_CON6;   // RW
    ___RW ___u32 SDR_CON7;   // RW
    ___RW ___u32 SDR_CON8;   // RW
    ___RW ___u32 SDR_CON9;   // RW
    ___WO ___u32 SDR_REFREG; // W-only
    ___RO ___u32 SDR_REFSUM; // R-only
    ___RW ___u32 SDR_DBG;    // RW
    ___RW ___u32 PLL_CON;    // RW
    ___RW ___u32 PLL_NF;     // RW
    ___RW ___u32 PLL_NR;     // RW
} JL_DMC_TypeDef;

#define JL_DMC_BASE                     (hs_base + map_adr(0x06, 0x00))
#define JL_DMC                          ((JL_DMC_TypeDef *)JL_DMC_BASE)

//............. 0x0700 - 0x08ff............ for sfc mmu
typedef struct {
    ___RW ___u32 CON0;
    ___WO ___u32 CON1;
    ___RW ___u32 TLB1_BEG;
    ___RW ___u32 TLB1_END;
} JL_SFCMMU_TypeDef;

#define JL_SFC0_MMU_BASE                (hs_base + map_adr(0x07, 0x00))
#define JL_SFC1_MMU_BASE                (hs_base + map_adr(0x08, 0x00))

#define JL_SFC0_MMU                     ((JL_SFCMMU_TypeDef *)JL_SFC0_MMU_BASE)
#define JL_SFC1_MMU                     ((JL_SFCMMU_TypeDef *)JL_SFC1_MMU_BASE)

typedef struct {
    short page: 14;
    short vld: 1;
    short reserved: 1;
} JL_SFCMMU_TLB1_TypeDef;

#define JL_SFC0_MMU_TLB1                ((JL_SFCMMU_TLB1_TypeDef *)(JL_SFC0_MMU->TLB1_BEG))
#define JL_SFC1_MMU_TLB1                ((JL_SFCMMU_TLB1_TypeDef *)(JL_SFC1_MMU->TLB1_BEG))

//............. 0x0900 - 0x09ff............ for wl rfd
//Note: this section define below from JL_WL_RFD

//............. 0x0a00 - 0x0aff............ for tzasc
typedef struct {
    ___RW ___u32 CON[16];
    ___RW ___u32 RGN[16];
    ___RW ___u32 IDA[16];
    ___RW ___u32 IDB[16];
} JL_TZASC_TypeDef;

#define JL_TZASC_BASE                   (hs_base + map_adr(0x0a, 0x00))
#define JL_TZASC                        ((JL_TZASC_TypeDef *)JL_TZASC_BASE)

//............. 0x0b00 - 0x0bff............ for video clk com
typedef struct {
    ___RW ___u32 PUB_CON;
    ___RW ___u32 EVA_CON;
    ___RW ___u32 DPI_CON;
    ___RW ___u32 GPU_CON;
    ___RW ___u32 SEG_CON;
} JL_VDOCK_TypeDef;

#define JL_VDOCK_BASE                   (hs_base + map_adr(0x0b, 0x00))
#define JL_VDOCK                        ((JL_VDOCK_TypeDef *)JL_VDOCK_BASE)

//............. 0x0e00 - 0x0eff............ for dma copy
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 ADR;
    ___RW ___u32 MEMSET_DATL;
    ___RW ___u32 MEMSET_DATH;
} JL_DCP_TypeDef;

#define JL_DCP_BASE                     (hs_base + map_adr(0x0e, 0x00))
#define JL_DCP                          ((JL_DCP_TypeDef *)JL_DCP_BASE)

//............. 0x1000 - 0x10ff............ for mtap
typedef struct {
    ___RW ___u32 MBISTPG_CTL;
    ___RO ___u32 MBISTPG_SO;
    ___RO ___u32 MBISTPG_GO;
    ___RO ___u32 MBISTPG_DONE;
    ___RW ___u32 MBISTPG_SEL;
} JL_MTAP_TypeDef;

#define JL_MTAP_BASE                    (hs_base + map_adr(0x10, 0x00))
#define JL_MTAP                         ((JL_MTAP_TypeDef *)JL_MTAP_BASE)

//............. 0x1100 - 0x11ff............ for sfc1
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 USER_CODE;
    ___RW ___u32 OFFSET_ADR;
    ___RW ___u32 DEC_KEY;
    ___RW ___u32 NDEC_SADR;
    ___RW ___u32 NDEC_EADR;
    ___RW ___u32 CLK_CON;
} JL_SFC_TypeDef;

#define JL_SFC_BASE                     (hs_base + map_adr(0x11, 0x00))
#define JL_SFC1                         ((JL_SFC_TypeDef *)JL_SFC_BASE)

//............. 0x1600 - 0x16ff............ for dvb0
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 RD_BASE;
    ___RW ___u32 RD_CON;
    ___RW ___u32 CON3;
    ___RO ___u32 CON4;
    ___RW ___u32 ERR_L;
    ___RW ___u32 ERR_H;
    ___RW ___u32 ADP_L;
    ___RW ___u32 ADP_H;
    ___RW ___u32 DCW_L;
    ___RW ___u32 DCW_H;
    ___RW ___u32 INT_L;
    ___RW ___u32 INT_H;
    ___RW ___u32 ERR_IE;
    ___RW ___u32 ADP_IE;
    ___RW ___u32 DCW_IE;
    ___RW ___u32 INT_IE;
    ___RW ___u32 REMAINS;
    ___RW ___u32 KEY_EN;
    ___RW ___u32 KEY_SEL;
    ___RW ___u32 KEY_EVEN;
    ___RW ___u32 KEY_ODD;
    ___RW ___u32 IV;
    ___RW ___u32 KEY_MUX0;
    ___WO ___u32 KEY_MUX1;
    ___RW ___u32 KEY_MUX2;
    ___RW ___u32 KEY_MUX3;
    ___RW ___u32 WR_BASE0;
    ___RW ___u32 WR_BASE1;
    ___RW ___u32 WR_BASE2;
    ___RW ___u32 WR_BASE3;
    ___RW ___u32 WR_BASE4;
    ___RW ___u32 WR_BASE5;
    ___RW ___u32 WR_BASE6;
    ___RW ___u32 WR_BASE7;
    ___RW ___u32 WR_LEN0;
    ___RW ___u32 WR_LEN1;
    ___RW ___u32 WR_LEN2;
    ___RW ___u32 WR_LEN3;
    ___RW ___u32 WR_SPND0;
    ___RW ___u32 WR_SPND1;
    ___RW ___u32 WR_SPND2;
    ___RW ___u32 WR_SPND3;
    ___RO ___u32 WR_CNT0;
    ___RO ___u32 WR_CNT1;
    ___RO ___u32 WR_CNT2;
    ___RO ___u32 WR_CNT3;
    ___RW ___u32 WR_INT;
    ___RW ___u32 PIDMUX0;
    ___RW ___u32 PIDMUX1;
    ___RW ___u32 PIDMUX2;
    ___RW ___u32 PIDMUX3;
    ___RW ___u32 PID_EN;
    ___RW ___u32 PID_SEL;
    ___RW ___u32 PID_DATA;
} JL_DVB_TypeDef;

#define JL_DVB0_BASE                    (hs_base + map_adr(0x16, 0x00))
#define JL_DVB0                         ((JL_DVB_TypeDef *)JL_DVB0_BASE)

#define JL_DVB1_BASE                    (hs_base + map_adr(0x17, 0x00))
#define JL_DVB1                         ((JL_DVB_TypeDef *)JL_DVB1_BASE)


//===============================================================================/
//
//      low speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for lemu
typedef struct {
    ___RW ___u32 WREN;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 MSG0;
    ___RW ___u32 MSG1;
    ___RW ___u32 MSG2;
    ___RW ___u32 MSG3;
    ___RO ___u32 LOG0;
    ___RO ___u32 LOG1;
    ___RO ___u32 LOG2;
    ___RO ___u32 LOG3;
} JL_LEMU_TypeDef;

#define JL_LEMU_BASE                    (ls_base + map_adr(0x00, 0x00))
#define JL_LEMU                         ((JL_LEMU_TypeDef *)JL_LEMU_BASE)

//............. 0x0100 - 0x01ff............ for pmu_ctl
typedef struct {
    ___RW ___u32 PMU_CON;
    ___RW ___u32 SPI_CON;
    ___RW ___u32 SPI_DAT;
} JL_PMU_TypeDef;

#define JL_PMU_BASE                     (ls_base + map_adr(0x01, 0x00))
#define JL_PMU                          ((JL_PMU_TypeDef *)JL_PMU_BASE)

//............. 0x0200 - 0x02ff............ for efuse_ctl

//............. 0x0300 - 0x03ff............ for wwdg
typedef struct {
    ___WO ___u32 KEY;
    ___RW ___u32 CR;
    ___RW ___u32 CFG;
    ___RO ___u32 SR;
} JL_WWDG_TypeDef;

#define JL_WWDG_BASE                     (ls_base + map_adr(0x03, 0x00))
#define JL_WWDG                          ((JL_WWDG_TypeDef *)JL_WWDG_BASE)

//............. 0x0400 - 0x05ff............ for udma
typedef struct {
    ___RW ___u32 SADR;
    ___RW ___u32 DADR;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
} JL_UDMA_CH;

typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 SEL0;
    ___RW ___u32 SEL1;
    ___RO ___u32 RESERVED[1];
} JL_DMAGEN_CH;

typedef struct {
    JL_UDMA_CH JL_UDMA_CH0;
    JL_DMAGEN_CH JL_DMAGEN_CH0;
    JL_UDMA_CH JL_UDMA_CH1;
    JL_DMAGEN_CH JL_DMAGEN_CH1;
} JL_UDMA_TypeDef;

#define JL_UDMA_BASE                    (ls_base + map_adr(0x04, 0x00))
#define JL_UDMA                         ((JL_UDMA_TypeDef *)JL_UDMA_BASE)
#define JL_DMAGEN                       ((JL_UDMA_TypeDef *)JL_UDMA_BASE)

//............. 0x0800 - 0x08ff............ for timer
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 CNT;
    ___RW ___u32 PRD;
    ___RW ___u32 PWM;
    ___RW ___u32 IRFLT;
} JL_TIMER_TypeDef;

#define JL_TIMER0_BASE                  (ls_base + map_adr(0x08, 5*0))
#define JL_TIMER1_BASE                  (ls_base + map_adr(0x08, 5*1))
#define JL_TIMER2_BASE                  (ls_base + map_adr(0x08, 5*2))
#define JL_TIMER3_BASE                  (ls_base + map_adr(0x08, 5*3))
#define JL_TIMER4_BASE                  (ls_base + map_adr(0x08, 5*4))
#define JL_TIMER5_BASE                  (ls_base + map_adr(0x08, 5*5))

#define JL_TIMER0                       ((JL_TIMER_TypeDef *)JL_TIMER0_BASE)
#define JL_TIMER1                       ((JL_TIMER_TypeDef *)JL_TIMER1_BASE)
#define JL_TIMER2                       ((JL_TIMER_TypeDef *)JL_TIMER2_BASE)
#define JL_TIMER3                       ((JL_TIMER_TypeDef *)JL_TIMER3_BASE)
#define JL_TIMER4                       ((JL_TIMER_TypeDef *)JL_TIMER4_BASE)
#define JL_TIMER5                       ((JL_TIMER_TypeDef *)JL_TIMER5_BASE)

//............. 0x1000 - 0x10ff............ for irflt
// typedef struct {
// __RW __u32 RFLT_CON;
// } JL_IR_TypeDef;

// #define JL_IR_BASE                      (ls_base + map_adr(0x10, 0x00))
// #define JL_IR                           ((JL_IR_TypeDef *)JL_IR_BASE)

//............. 0x1100 - 0x13ff............ for irflt

//............. 0x1400 - 0x14ff............ for qdec
typedef struct {
    ___RW ___u8  CON;
    ___RW ___u8  DAT;
    ___RW ___u8  SMP;
    ___RW ___u8  DBE;
} JL_QDEC_TypeDef;

#define JL_QDEC0_BASE                   (ls_base + map_adr(0x14, 0x00))
#define JL_QDEC0                        ((JL_QDEC_TypeDef *)JL_QDEC0_BASE)

//............. 0x1500 - 0x17ff............ for qdec

//............. 0x1800 - 0x1aff............ for uart
typedef struct {
    ___RW ___u16 CON0;
    ___RW ___u16 CON1;
    ___RW ___u16 CON2;
    ___RW ___u16 BAUD;
    ___RW ___u8  BUF;
    ___RW ___u32 OTCNT;
    ___RW ___u32 TXADR;
    ___WO ___u16 TXCNT;
    ___RW ___u32 RXSADR;
    ___RW ___u32 RXEADR;
    ___RW ___u32 RXCNT;
    ___RO ___u16 HRXCNT;
    ___RO ___u16 RX_ERR_CNT;
} JL_UART_TypeDef;

#define JL_UART0_BASE                   (ls_base + map_adr(0x18, 0x00))
#define JL_UART1_BASE                   (ls_base + map_adr(0x19, 0x00))
#define JL_UART2_BASE                   (ls_base + map_adr(0x1a, 0x00))
#define JL_UART3_BASE                   (ls_base + map_adr(0x1b, 0x00))
#define JL_UART4_BASE                   (ls_base + map_adr(0x1c, 0x00))
#define JL_UART5_BASE                   (ls_base + map_adr(0x1d, 0x00))
#define JL_UART6_BASE                   (ls_base + map_adr(0x1e, 0x00))
#define JL_UART7_BASE                   (ls_base + map_adr(0x1f, 0x00))

#define JL_UART0                        ((JL_UART_TypeDef *)JL_UART0_BASE)
#define JL_UART1                        ((JL_UART_TypeDef *)JL_UART1_BASE)
#define JL_UART2                        ((JL_UART_TypeDef *)JL_UART2_BASE)
#define JL_UART3                        ((JL_UART_TypeDef *)JL_UART3_BASE)
#define JL_UART4                        ((JL_UART_TypeDef *)JL_UART4_BASE)

//............. 0x2000 - 0x25ff............ for iic
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 PND;
    ___RW ___u32 TX_BUF;
    ___RW ___u32 TASK;
    ___RO ___u32 RX_BUF;
    ___RW ___u32 ADDR;
    ___RW ___u32 BAUD;
    ___RW ___u32 TSU;
    ___RW ___u32 THD;
    ___RO ___u32 IIC_DBG;
    ___RW ___u32 TRIG_EN;
} JL_IIC_TypeDef;

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 BUF;
    ___RW ___u32 BAUD;
    ___RW ___u32 DMA_ADR;
    ___RW ___u32 DMA_CNT;
    ___RW ___u32 DMA_NRATE;
} JL_IICDMA_TypeDef;

#define JL_IIC0_BASE                    (ls_base + map_adr(0x20, 0x00))
#define JL_IIC1_BASE                    (ls_base + map_adr(0x21, 0x00))
#define JL_IIC2_BASE                    (ls_base + map_adr(0x22, 0x00))

#define JL_IIC0                         ((JL_IIC_TypeDef *)JL_IIC0_BASE)
#define JL_IIC1                         ((JL_IICDMA_TypeDef *)JL_IIC1_BASE)
#define JL_IIC2                         ((JL_IICDMA_TypeDef *)JL_IIC2_BASE)

//............. 0x2600 - 0x2bff............ for spi
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 BAUD;
    ___RW ___u32 BUF;
    ___WO ___u32 ADR;
    ___RW ___u32 CNT;
    ___RW ___u32 CON1;
    ___RW ___u8  ENCCON;
    ___WO ___u16 ENCKEY;
    ___WO ___u16 ENCADR;
    ___RW ___u32 REG;
    ___WO ___u32 FIFO;
} JL_SPI_TypeDef;

#define JL_SPI0_BASE                    (ls_base + map_adr(0x26, 0x00))
#define JL_SPI1_BASE                    (ls_base + map_adr(0x27, 0x00))
#define JL_SPI2_BASE                    (ls_base + map_adr(0x28, 0x00))
#define JL_SPI3_BASE                    (ls_base + map_adr(0x29, 0x00))

#define JL_SPI0                         ((JL_SPI_TypeDef *)JL_SPI0_BASE)
#define JL_SPI1                         ((JL_SPI_TypeDef *)JL_SPI1_BASE)
#define JL_SPI2                         ((JL_SPI_TypeDef *)JL_SPI2_BASE)

//............. 0x2c00 - 0x2fff............ for perienc
//typedef struct {
//    __RW __u8  ENCCON ;
//    __WO __u16 ENCKEY ;
//    __WO __u16 ENCADR ;
//} JL_PERIENC_TypeDef;

//#define JL_PERIENC_BASE                 (ls_base + map_adr(0x2c, 0x00))
//#define JL_PERIENC                      ((JL_PERIENC_TypeDef *)JL_PERIENC_BASE)

//............. 0x3000 - 0x33ff............ for sdc
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___WO ___u32 CPTR;
    ___WO ___u32 DPTR;
    ___RW ___u32 CTU_CON;
    ___WO ___u32 CTU_CNT;
    ___RW ___u32 BRK;
    ___RW ___u8  ENCCON;
    ___WO ___u16 ENCKEY;
    ___WO ___u16 ENCADR;
} JL_SD_TypeDef;

#define JL_SD0_BASE                     (ls_base + map_adr(0x30, 0x00))
#define JL_SD1_BASE                     (ls_base + map_adr(0x31, 0x00))
#define JL_SD0                          ((JL_SD_TypeDef *)JL_SD0_BASE)
#define JL_SD1                          ((JL_SD_TypeDef *)JL_SD1_BASE)

//............. 0x3400 - 0x34ff............ for lrct
typedef struct {
    ___WO ___u32 CON;
    ___RW ___u32 NUM;
} JL_LRCT_TypeDef;

#define JL_LRCT_BASE                    (ls_base + map_adr(0x34, 0x00))
#define JL_LRCT                         ((JL_LRCT_TypeDef *)JL_LRCT_BASE)

//............. 0x3500 - 0x35ff............ for pcnt
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 VAL;
} JL_PCNT_TypeDef;

#define JL_PCNT_BASE                    (ls_base + map_adr(0x35, 0x00))
#define JL_PCNT                         ((JL_PCNT_TypeDef *)JL_PCNT_BASE)

//............. 0x3600 - 0x36ff............ for gpcnt0
typedef struct {
    ___RW ___u32 CON;
    ___RO ___u32 NUM;
} JL_GPCNT_TypeDef;

#define JL_GPCNT0_BASE                  (ls_base + map_adr(0x36, 0x00))
#define JL_GPCNT                        ((JL_GPCNT_TypeDef *)JL_GPCNT0_BASE)

//............. 0x3700 - 0x37ff............ for gpcnt1

//............. 0x3800 - 0x38ff............ for mcpwm
typedef struct {
    ___RW ___u32 TMR0_CON;
    ___RW ___u32 TMR0_CNT;
    ___RW ___u32 TMR0_PR;
    ___RW ___u32 TMR1_CON;
    ___RW ___u32 TMR1_CNT;
    ___RW ___u32 TMR1_PR;
    ___RW ___u32 TMR2_CON;
    ___RW ___u32 TMR2_CNT;
    ___RW ___u32 TMR2_PR;
    ___RW ___u32 TMR3_CON;
    ___RW ___u32 TMR3_CNT;
    ___RW ___u32 TMR3_PR;
    ___RW ___u32 TMR4_CON;
    ___RW ___u32 TMR4_CNT;
    ___RW ___u32 TMR4_PR;
    ___RW ___u32 TMR5_CON;
    ___RW ___u32 TMR5_CNT;
    ___RW ___u32 TMR5_PR;
    ___RW ___u32 TMR6_CON;
    ___RW ___u32 TMR6_CNT;
    ___RW ___u32 TMR6_PR;
    ___RW ___u32 TMR7_CON;
    ___RW ___u32 TMR7_CNT;
    ___RW ___u32 TMR7_PR;
    ___RW ___u32 FPIN_CON;
    ___RW ___u32 CH0_CON0;
    ___RW ___u32 CH0_CON1;
    ___RW ___u32 CH0_CMPH;
    ___RW ___u32 CH0_CMPL;
    ___RW ___u32 CH1_CON0;
    ___RW ___u32 CH1_CON1;
    ___RW ___u32 CH1_CMPH;
    ___RW ___u32 CH1_CMPL;
    ___RW ___u32 CH2_CON0;
    ___RW ___u32 CH2_CON1;
    ___RW ___u32 CH2_CMPH;
    ___RW ___u32 CH2_CMPL;
    ___RW ___u32 CH3_CON0;
    ___RW ___u32 CH3_CON1;
    ___RW ___u32 CH3_CMPH;
    ___RW ___u32 CH3_CMPL;
    ___RW ___u32 CH4_CON0;
    ___RW ___u32 CH4_CON1;
    ___RW ___u32 CH4_CMPH;
    ___RW ___u32 CH4_CMPL;
    ___RW ___u32 CH5_CON0;
    ___RW ___u32 CH5_CON1;
    ___RW ___u32 CH5_CMPH;
    ___RW ___u32 CH5_CMPL;
    ___RW ___u32 CH6_CON0;
    ___RW ___u32 CH6_CON1;
    ___RW ___u32 CH6_CMPH;
    ___RW ___u32 CH6_CMPL;
    ___RW ___u32 CH7_CON0;
    ___RW ___u32 CH7_CON1;
    ___RW ___u32 CH7_CMPH;
    ___RW ___u32 CH7_CMPL;
    ___RW ___u32 MCPWM_CON0;
} JL_MCPWM_TypeDef;

#define JL_MCPWM_BASE                   (ls_base + map_adr(0x38, 0x00))
#define JL_MCPWM                        ((JL_MCPWM_TypeDef *)JL_MCPWM_BASE)

//............. 0x3900 - 0x39ff............ for ledc
typedef struct {
    ___RW ___u32 CLK;
} JL_LEDCK_TypeDef;
#define JL_LEDCK_BASE                   (ls_base + map_adr(0x39, 0x00))
#define JL_LEDCK                        ((JL_LEDCK_TypeDef *)JL_LEDCK_BASE)

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 FD;
    ___RW ___u32 LP;
    ___RW ___u32 TIX;
    ___RW ___u32 RSTX;
    ___WO ___u32 ADR;
} JL_LEDC_TypeDef;

#define JL_LEDC0_BASE                   (ls_base + map_adr(0x39, 0x01))
#define JL_LEDC1_BASE                   (ls_base + map_adr(0x39, 0x09))
#define JL_LEDC2_BASE                   (ls_base + map_adr(0x39, 0x11))

#define JL_LEDC0                        ((JL_LEDC_TypeDef *)JL_LEDC0_BASE)
#define JL_LEDC1                        ((JL_LEDC_TypeDef *)JL_LEDC1_BASE)
#define JL_LEDC2                        ((JL_LEDC_TypeDef *)JL_LEDC2_BASE)

//............. 0x3a00 - 0x3aff............ for pap
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 DAT0;
    ___RW ___u32 DAT1;
    ___RW ___u32 BUF;
    ___RW ___u32 ADR;
    ___RW ___u32 CNT;
} JL_PAP_TypeDef;

#define JL_PAP_BASE                     (ls_base + map_adr(0x3a, 0x00))
#define JL_PAP                          ((JL_PAP_TypeDef *)JL_PAP_BASE)

//............. 0x3c00 - 0x3cff............ for pps
//typedef struct {
//    __RW __u32 CON;
//    __RW __u32 CNT;
//    __RW __u32 CFG;
//    __RW __u32 VTRIM;
//    __RW __u32 TX_BASE;
//    __RW __u32 RX_BASE;
//} JL_PPS_TypeDef;
//
//#define JL_PPS_BASE                     (ls_base + map_adr(0x3c, 0x00))
//#define JL_PPS                          ((JL_PPS_TypeDef       *)JL_PPS_BASE)

//............. 0x3d00 - 0x3dff............ for ppm
//typedef struct {
//    __RW __u32 CON;
//    __RW __u32 STRAN_DAT;
//    __RW __u32 STRAN_SET;
//    __RW __u32 DMA_ADR;
//    __RW __u32 PPM_RD;
// } JL_PPM_TypeDef;

// #define JL_PPM_BASE                    (ls_base + map_adr(0x3d, 0x00))
// #define JL_PPM                         ((JL_PPM_TypeDef       *)JL_PPM_BASE)

//............. 0x3e00 - 0x3eff............ for

//............. 0x3f00 - 0x3fff............ for wl uart (only FPGA)
typedef struct {
    ___RW ___u16 CON0;
    ___RW ___u16 CON1;
    ___RW ___u16 CON2;
    ___RW ___u16 BAUD;
    ___RW ___u8  BUF;
} JL_WL_UART_LITE_TypeDef;

#define JL_WL_UART_BASE                 (ls_base + map_adr(0x3f, 0x00))
#define JL_WL_UART                      ((JL_WL_UART_LITE_TypeDef *)JL_WL_UART_BASE)

//............. 0x4000 - 0x40ff............ for gpadc
typedef struct {
    ___RW ___u32 CON;
    ___RO ___u32 RES;
    ___RW ___u32 DMA_CON0;
    ___RW ___u32 DMA_CON1;
    ___RW ___u32 DMA_CON2;
    ___RW ___u32 DMA_BADR;
} JL_ADC_TypeDef;

#define JL_ADC_BASE                     (ls_base + map_adr(0x40, 0x00))
#define JL_ADC                          ((JL_ADC_TypeDef *)JL_ADC_BASE)

//............. 0x4100 - 0x41ff............ for

//............. 0x4200 - 0x42ff............ for ctmu
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RO ___u32 RES;
} JL_CTMU_TypeDef;

#define JL_CTMU_BASE                    (ls_base + map_adr(0x42, 0x00))
#define JL_CTMU                         ((JL_CTMU_BASE *)JL_CTMU_BASE)

//............. 0x4300 - 0x43ff............ for

//............. 0x4400 - 0x44ff............ for led
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 BRI_PRDL;
    ___RW ___u32 BRI_PRDH;
    ___RW ___u32 BRI_DUTY0L;
    ___RW ___u32 BRI_DUTY0H;
    ___RW ___u32 BRI_DUTY1L;
    ___RW ___u32 BRI_DUTY1H;
    ___RW ___u32 PRD_DIVL;
    ___RW ___u32 DUTY0;
    ___RW ___u32 DUTY1;
    ___RW ___u32 DUTY2;
    ___RW ___u32 DUTY3;
    ___RO ___u32 CNT_RD;
    ___RW ___u32 CON4;
    ___WO ___u32 CNT_SYNC;
    ___RW ___u32 CNT_DEC;
} JL_PLED_TypeDef;

#define JL_PLED_BASE                    (ls_base + map_adr(0x44, 0x00))
#define JL_PLED                         ((JL_PLED_TypeDef *)JL_PLED_BASE)

//............. 0x4500 - 0x45ff............ for lcd
//typedef struct {
//    __RW __u32 CON0;
//    __RW __u32 CON1;
//    __RW __u32 SEG_IO_EN0;
//    __RW __u32 SEG_IO_EN1;
//    __RW __u32 SEG0_DAT;
//    __RW __u32 SEG1_DAT;
//    __RW __u32 SEG2_DAT;
//    __RW __u32 SEG3_DAT;
//    __RW __u32 SEG4_DAT;
//    __RW __u32 SEG5_DAT;
//} JL_LCDC_TypeDef;

//#define JL_LCDC_BASE                    (ls_base + map_adr(0x45, 0x00))
//#define JL_LCDC                         ((JL_LCDC_TypeDef      *)JL_LCDC_BASE)

//............. 0x4600 - 0x47ff............ for can
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CMD0;
    ___RW ___u32 STATUS;
    ___RW ___u32 PND;
    ___RW ___u32 ACF;
    ___RW ___u32 AMF;
    ___RW ___u32 BTIME0;
    ___RW ___u32 BTIME1;
    ___RW ___u32 RESERVED0;
    ___RW ___u32 RESERVED1;
    ___RW ___u32 TIDH;
    ___RW ___u32 TIDL;
    ___RW ___u32 TXB1;
    ___RW ___u32 TXB2;
    ___RW ___u32 TXB3;
    ___RW ___u32 TXB4;
    ___RW ___u32 TXB5;
    ___RW ___u32 TXB6;
    ___RW ___u32 TXB7;
    ___RW ___u32 TXB8;
    ___RW ___u32 RIDH;
    ___RW ___u32 RIDL;
    ___RW ___u32 RXB1;
    ___RW ___u32 RXB2;
    ___RW ___u32 RXB3;
    ___RW ___u32 RXB4;
    ___RW ___u32 RXB5;
    ___RW ___u32 RXB6;
    ___RW ___u32 RXB7;
    ___RW ___u32 RXB8;
    ___RW ___u32 BAUDH;
    ___RW ___u32 CLKDIV;
} JL_CAN_TypeDef;

typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CMD0;
    ___RW ___u32 STATUS;
    ___RW ___u32 PND;
    ___RW ___u32 IE;
    ___RW ___u32 RESERVED0;
    ___RW ___u32 BTIME0;
    ___RW ___u32 BTIME1;
    ___RW ___u32 RESERVED1;
    ___RW ___u32 RESERVED2;
    ___RW ___u32 RESERVED3;
    ___RW ___u32 ALC;
    ___RW ___u32 ECC;
    ___RW ___u32 EWLR;
    ___RW ___u32 RXERR;
    ___RW ___u32 TXERR;
    ___RW ___u32 FF;
    ___RW ___u32 BUF0;
    ___RW ___u32 BUF1;
    ___RW ___u32 BUF2;
    ___RW ___u32 BUF3;
    ___RW ___u32 BUF4;
    ___RW ___u32 BUF5;
    ___RW ___u32 BUF6;
    ___RW ___u32 BUF7;
    ___RW ___u32 BUF8;
    ___RW ___u32 BUF9;
    ___RW ___u32 BUF10;
    ___RW ___u32 BUF11;
    ___RW ___u32 RMC;
    ___RW ___u32 BAUDH;
    ___RW ___u32 CLKDIV;
} JL_ECAN_TypeDef;

#define JL_CAN_BASE                     (ls_base + map_adr(0x46,0x00))
#define JL_CAN                          ((JL_CAN_TypeDef *)JL_CAN_BASE)
#define JL_ECAN_BASE                    (ls_base + map_adr(0x46,0x00))
#define JL_ECAN                         ((JL_ECAN_TypeDef *)JL_ECAN_BASE)

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 ADR;
    ___RW ___u32 RXCNT;
    ___RO ___u32 HRCNT;
} JL_CANDMA_TypeDef;

#define JL_CANDMA_BASE                  (ls_base + map_adr(0x47,0x00))
#define JL_CANDMA                       ((JL_CANDMA_TypeDef *)JL_CANDMA_BASE)

//............. 0x4800 - 0x48ff............ for rand64
typedef struct {
    ___RO ___u32 R64L;
    ___RO ___u32 R64H;
} JL_RAND_TypeDef;

#define JL_RAND_BASE                    (ls_base + map_adr(0x48, 0x00))
#define JL_RAND                         ((JL_RAND_TypeDef *)JL_RAND_BASE)

//............. 0x4900 - 0x49ff............ for crc
typedef struct {
    ___WO ___u32 FIFO;
    ___RW ___u32 REG;
} JL_CRC_TypeDef;

#define JL_CRC_BASE                     (ls_base + map_adr(0x49, 0x00))
#define JL_CRC                          ((JL_CRC_TypeDef *)JL_CRC_BASE)

//............. 0x4a00 - 0x4aff............ for sha
typedef struct {
    ___RW ___u32 CON;
    ___WO ___u32 ADR;
    ___RW ___u32 RESULT0;
    ___RW ___u32 RESULT1;
    ___RW ___u32 RESULT2;
    ___RW ___u32 RESULT3;
    ___RW ___u32 RESULT4;
    ___RW ___u32 RESULT5;
    ___RW ___u32 RESULT6;
    ___RW ___u32 RESULT7;
} JL_SHA_TypeDef;

#define JL_SHA_BASE                     (ls_base + map_adr(0x4a, 0x00))
#define JL_SHA                          ((JL_SHA_TypeDef *)JL_SHA_BASE)

//............. 0x5800 - 0x58ff............ for fusb0
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___WO ___u32 EP0_CNT;
    ___WO ___u32 EP1_CNT;
    ___WO ___u32 EP2_CNT;
    ___WO ___u32 EP3_CNT;
    ___WO ___u32 EP4_CNT;
    ___WO ___u32 EP0_ADR;
    ___WO ___u32 EP1_TADR;
    ___WO ___u32 EP1_RADR;
    ___WO ___u32 EP2_TADR;
    ___WO ___u32 EP2_RADR;
    ___WO ___u32 EP3_TADR;
    ___WO ___u32 EP3_RADR;
    ___WO ___u32 EP4_TADR;
    ___WO ___u32 EP4_RADR;
    ___RW ___u32 TXDLY_CON;
    ___RW ___u32 EP1_RLEN;
    ___RW ___u32 EP2_RLEN;
    ___RW ___u32 EP3_RLEN;
    ___RW ___u32 EP4_RLEN;
    ___RW ___u32 EP1_MTX_PRD;
    ___RW ___u32 EP1_MRX_PRD;
    ___RO ___u32 EP1_MTX_NUM;
    ___RO ___u32 EP1_MRX_NUM;
    ___RW ___u32 SOF_STA_CON;
} JL_USB_TypeDef;

#define JL_USB_BASE                     (ls_base + map_adr(0x58, 0x00))
#define JL_USB                          ((JL_USB_TypeDef *)JL_USB_BASE)

//............. 0x5900 - 0x5dff............ for sie
typedef struct {
    ___RW ___u32 FADDR;
    ___RW ___u32 POWER;
    ___RO ___u32 INTRTX1;
    ___RO ___u32 INTRTX2;
    ___RO ___u32 INTRRX1;
    ___RO ___u32 INTRRX2;
    ___RO ___u32 INTRUSB;
    ___RW ___u32 INTRTX1E;
    ___RW ___u32 INTRTX2E;
    ___RW ___u32 INTRRX1E;
    ___RW ___u32 INTRRX2E;
    ___RW ___u32 INTRUSBE;
    ___RO ___u32 FRAME1;
    ___RO ___u32 FRAME2;
    ___RO ___u32 RESERVED14;
    ___RW ___u32 DEVCTL;
    ___RO ___u32 RESERVED10_0x16[0x16 - 0x10 + 1];
} JL_USB_SIE_TypeDef;

#define JL_USB_SIE_BASE                 (ls_base + map_adr(0x59, 0x00))
#define JL_USB_SIE                      ((JL_USB_SIE_TypeDef *)JL_USB_SIE_BASE)

typedef struct {
    ___RO ___u32 RESERVED0;
    ___RW ___u32 CSR0;
    ___RO ___u32 RESERVED2_5[5 - 1];
    ___RO ___u32 COUNT0;
} JL_USB_EP0_TypeDef;

#define JL_USB_EP0_BASE                 (ls_base + map_adr(0x59, 0x10))
#define JL_USB_EP0                      ((JL_USB_EP0_TypeDef *)JL_USB_EP0_BASE)

typedef struct {
    ___RW ___u32 TXMAXP;
    ___RW ___u32 TXCSR1;
    ___RW ___u32 TXCSR2;
    ___RW ___u32 RXMAXP;
    ___RW ___u32 RXCSR1;
    ___RW ___u32 RXCSR2;
    ___RO ___u32 RXCOUNT1;
    ___RO ___u32 RXCOUNT2;
    ___RW ___u32 TXTYPE;
    ___RO ___u32 TXINTERVAL;
    ___RW ___u32 RXTYPE;
    ___RO ___u32 RXINTERVAL;
} JL_USB_EP_TypeDef;

#define JL_USB_EP1_BASE                 (ls_base + map_adr(0x5a, 0x10))
#define JL_USB_EP1                      ((JL_USB_EP_TypeDef *)JL_USB_EP1_BASE)

#define JL_USB_EP2_BASE                 (ls_base + map_adr(0x5b, 0x10))
#define JL_USB_EP2                      ((JL_USB_EP_TypeDef *)JL_USB_EP2_BASE)

//............. 0x7000 - 0x70ff............ for port others
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
} JL_WAKEUP_TypeDef;

#define JL_WAKEUP_BASE                  (ls_base + map_adr(0x70, 0x00))
#define JL_WAKEUP                       ((JL_WAKEUP_TypeDef *)JL_WAKEUP_BASE)

typedef struct {
    ___RW ___u32 SPI_IOMC0;
    ___RW ___u32 SDC_IOMC0;
    ___RW ___u32 SYS_IOMC0;
    ___RW ___u32 SYS_IOMC1;
    ___RW ___u32 ASS_IOMC0;
    ___RW ___u32 WL_IOMC0;
    ___RW ___u32 ICH_IOMC0;
    ___RW ___u32 ICH_IOMC1;
    ___RW ___u32 ICH_IOMC2;
    ___RW ___u32 ICH_IOMC3;
    ___RW ___u32 ICH_IOMC4;
    ___RW ___u32 ICH_IOMC5;
    ___RW ___u32 OCH_IOMC0;
    ___RW ___u32 OCH_IOMC1;
    ___RW ___u32 OCH_IOMC2;
    ___RW ___u32 OCH_IOMC3;
    ___RW ___u32 OCH_IOMC4;
    ___RW ___u32 OCH_IOMC5;
} JL_IOMC_TypeDef;

#define JL_IOMC_BASE                    (ls_base + map_adr(0x70, 0x05))
#define JL_IOMC                         ((JL_IOMC_TypeDef *)JL_IOMC_BASE)

//............. 0x7100 - 0x7fff............ for port
typedef struct {
    ___RO ___u32 IN;
    ___RW ___u32 OUT;
    ___RW ___u32 DIR;
    ___RW ___u32 DIE;
    ___RW ___u32 DIEH;
    ___RW ___u32 PU0;
    ___RW ___u32 PU1;
    ___RW ___u32 PD0;
    ___RW ___u32 PD1;
    ___RW ___u32 HD0;
    ___RW ___u32 HD1;
    ___RW ___u32 SPL;
    ___RW ___u32 CON;       // usb phy only
    ___RW ___u32 OUT_BSR;
    ___RW ___u32 DIR_BSR;
    ___RW ___u32 DIE_BSR;
    ___RW ___u32 DIEH_BSR;
    ___RW ___u32 PU0_BSR;
    ___RW ___u32 PU1_BSR;
    ___RW ___u32 PD0_BSR;
    ___RW ___u32 PD1_BSR;
    ___RW ___u32 HD0_BSR;
    ___RW ___u32 HD1_BSR;
    ___RW ___u32 SPL_BSR;
    ___RW ___u32 CON_BSR;   // usb phy only
} JL_PORT_TypeDef;

#define JL_PORTA_BASE                   (ls_base + map_adr(0x71, 0x00))
#define JL_PORTB_BASE                   (ls_base + map_adr(0x72, 0x00))
#define JL_PORTC_BASE                   (ls_base + map_adr(0x73, 0x00))
#define JL_PORTD_BASE                   (ls_base + map_adr(0x74, 0x00))
#define JL_PORTE_BASE                   (ls_base + map_adr(0x75, 0x00))
#define JL_PORTF_BASE                   (ls_base + map_adr(0x76, 0x00))
#define JL_PORTG_BASE                   (ls_base + map_adr(0x77, 0x00))
#define JL_PORTH_BASE                   (ls_base + map_adr(0x78, 0x00))
#define JL_PORTI_BASE                   (ls_base + map_adr(0x79, 0x00))

#define JL_PORTP_BASE                   (ls_base + map_adr(0x7a, 0x00))

#define JL_PORTUSB0_BASE                (ls_base + map_adr(0x7c, 0x00))
#define JL_PORTUSB1_BASE                (ls_base + map_adr(0x7d, 0x00))
#define JL_PORTUSB2_BASE                (ls_base + map_adr(0x7e, 0x00))
#define JL_PORTUSB3_BASE                (ls_base + map_adr(0x7f, 0x00))

#define JL_PORTA                        ((JL_PORT_TypeDef *)JL_PORTA_BASE)
#define JL_PORTB                        ((JL_PORT_TypeDef *)JL_PORTB_BASE)
#define JL_PORTC                        ((JL_PORT_TypeDef *)JL_PORTC_BASE)
#define JL_PORTD                        ((JL_PORT_TypeDef *)JL_PORTD_BASE)
#define JL_PORTE                        ((JL_PORT_TypeDef *)JL_PORTE_BASE)
//#define JL_PORTF                        ((JL_PORT_TypeDef *)JL_PORTF_BASE)

#define JL_PORTP                        ((JL_PORT_TypeDef *)JL_PORTP_BASE)

#define JL_PORTFUSB                     ((JL_PORT_TypeDef *)JL_PORTUSB0_BASE)
#define JL_PORTHUSB                     ((JL_PORT_TypeDef *)JL_PORTUSB1_BASE)

//............. 0x8000 - 0x8fff............ for ethernet mac
typedef struct {
    ___RW ___u32 MODE_CON;
    ___RW ___u32 INT_SOURCE;
    ___RW ___u32 INT_MASK;
    ___RW ___u32 IPGT;
    ___RW ___u32 IPGR1;
    ___RW ___u32 IPGR2;
    ___RW ___u32 PACKET_LEN;
    ___RW ___u32 COLL_CONF;
    ___RW ___u32 TX_BD_NUM;
    ___RW ___u32 CTL_MODE;
    ___RW ___u32 MII_MODE;
    ___RW ___u32 MII_CMD;
    ___RW ___u32 MII_ADR;
    ___RW ___u32 MII_TXDATA;
    ___RW ___u32 MII_RXDATA;
    ___RW ___u32 MII_STATUS;
    ___RW ___u32 MAC_ADR0;
    ___RW ___u32 MAC_ADR1;
    ___RW ___u32 HASH0;
    ___RW ___u32 HASH1;
    ___RW ___u32 TX_CTL;
    ___RW ___u32 RX_CTL;
    ___RW ___u32 ETH_DBG;
} JL_ETHMAC_TypeDef;

#define JL_ETHMAC_BASE                  (ls_base + map_adr(0x80, 0x00))
#define JL_ETHMAC                       ((JL_ETHMAC_TypeDef *)JL_ETHMAC_BASE)

//............. 0x9000 - 0x90ff............ for ethernet ctl
typedef struct {
    ___RW ___u32 ETH_CON;
} JL_ETHCTL_TypeDef;

#define JL_ETHCTL_BASE                  (ls_base + map_adr(0x90, 0x00))
#define JL_ETHCTL                       ((JL_ETHCTL_TypeDef *)JL_ETHCTL_BASE)


//===============================================================================//
//
//      ass_sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for ass_mbist
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 SEL;
    ___RW ___u32 BEG;
    ___RW ___u32 END;
    ___RW ___u32 DAT_VLD0;
    ___RW ___u32 DAT_VLD1;
    ___RW ___u32 DAT_VLD2;
    ___RW ___u32 DAT_VLD3;
} JL_ASS_MBIST_TypeDef;

#define JL_ASS_MBIST_BASE               (as_base + map_adr(0x00, 0x00))
#define JL_ASS_MBIST                    ((JL_ASS_MBIST_TypeDef *)JL_ASS_MBIST_BASE)

//............. 0x0100 - 0x01ff............ for aud
typedef struct {
    ___RW ___u32 AUD_CON0;
    ___RW ___u32 AUD_CON1;
    ___RW ___u32 AUD_CON2;
    ___WO ___u32 AUD_COP;
} JL_AUD_TypeDef;

#define JL_AUD_BASE                     (as_base + map_adr(0x01, 0x00))
#define JL_AUD                          ((JL_AUD_TypeDef *)JL_AUD_BASE)

//............. 0x0200 - 0x02ff............ for dac
typedef struct {
    ___RW ___u32(DAC_CON0);   /* 00 */
    ___RW ___u32(DAC_CON1);   /* 01 */
    ___RW ___u32(DAC_VOL);    /* 02 */
    ___RW ___u32(DAC_DIT);    /* 03 */
    ___RW ___u32(DAC_TRM);    /* 04 */
    ___WO ___u32(DAC_DCW);    /* 05 */
    ___RO ___u32(DAC_DCR);    /* 06 */
    ___RW ___u32(DAC_ADR);    /* 07 */
    ___RW ___u16(DAC_LEN);    /* 08 */
    ___RW ___u16(DAC_PNS);    /* 09 */
    ___RW ___u16(DAC_HRP);    /* 0A */
    ___RW ___u16(DAC_SWP);    /* 0B */
    ___RW ___u16(DAC_SWN);    /* 0C */
} JL_AUDDAC_TypeDef;

#define JL_AUDDAC_BASE                  (as_base + map_adr(0x02, 0x00))
#define JL_AUDDAC                       ((JL_AUDDAC_TypeDef *)JL_AUDDAC_BASE)

//............. 0x0300 - 0x03ff............ for adc
typedef struct {
    ___RW ___u32(ADC_CON0);
    ___RW ___u32(ADC_CON1);
    ___WO ___u32(ADC_DCW);
    ___RO ___u32(ADC_DCR);
    ___RW ___u32(ADC_ADR);
    ___RW ___u16(ADC_LEN);
    ___RW ___u16(ADC_PNS);
    ___RW ___u16(ADC_HWP);
    ___RW ___u16(ADC_SRP);
    ___RW ___u16(ADC_SRN);
} JL_AUDADC_TypeDef;

#define JL_AUDADC_BASE                  (as_base + map_adr(0x03, 0x00))
#define JL_AUDADC                       ((JL_AUDADC_TypeDef *)JL_AUDADC_BASE)

//............. 0x0400 - 0x04ff............ for alnk
typedef struct {
    ___RW ___u16 CON0;
    ___RW ___u16 CON1;
    ___RW ___u16 CON2;
    ___RW ___u16 CON3;
    ___RW ___u32 ADR0;
    ___RW ___u32 ADR1;
    ___RW ___u32 ADR2;
    ___RW ___u32 ADR3;
    ___RW ___u16 LEN;
    ___RW ___u32 PNS;
    ___RW ___u32 HWPTR0;
    ___RW ___u32 HWPTR1;
    ___RW ___u32 HWPTR2;
    ___RW ___u32 HWPTR3;
    ___RW ___u32 SWPTR0;
    ___RW ___u32 SWPTR1;
    ___RW ___u32 SWPTR2;
    ___RW ___u32 SWPTR3;
    ___RW ___u32 SHN0;
    ___RW ___u32 SHN1;
    ___RW ___u32 SHN2;
    ___RW ___u32 SHN3;
    ___RW ___u32 BLOCK;
} JL_ALNK_TypeDef;

#define JL_ALNK_BASE                    (as_base + map_adr(0x04, 0x00))
#define JL_ALNK                         ((JL_ALNK_TypeDef *)JL_ALNK_BASE)

//............. 0x0500 - 0x05ff............ for plnk
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 ADR;
    ___RW ___u32 LEN;
    ___RW ___u16 DOR;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___WO ___u32 DCW0;
    ___WO ___u32 DCW1;
    ___RO ___u32 DCR0;
    ___RO ___u32 DCR1;
} JL_PLNK_TypeDef;

#define JL_PLNK_BASE                    (as_base + map_adr(0x05, 0x00))
#define JL_PLNK                         ((JL_PLNK_TypeDef *)JL_PLNK_BASE)

//............. 0x0900 - 0x09ff............ for ass_clkgen
//typedef struct {
//    __RW __u32 CLK_CON;
//} JL_ASS_TypeDef;
//
//#define JL_ASS_BASE                (hs_base + map_adr(0x09, 0x00))
//#define JL_ASS                     ((JL_ASS_TypeDef  *)JL_ASS_BASE)

//............. 0xa00 - 0xaff............ for aud_ana
typedef struct {
    ___RW ___u32 DAA_CON0;
    ___RW ___u32 ADA_CON0;
    ___RW ___u32 ADA_CON1;
    ___RW ___u32 ADA_CON2;
    ___RW ___u32 ADA_CON3;
    ___RW ___u32 ADA_CON4;
    ___RW ___u32 ADA_CON5;
    ___RW ___u32 ADDA_CON0;
    ___RW ___u32 ADDA_CON1;
    ___RO ___u32 ADDA_CON2;
} JL_ADDA_TypeDef;

#define JL_ADDA_BASE                    (as_base + map_adr(0xa, 0x00))
#define JL_ADDA                         ((JL_ADDA_TypeDef *)JL_ADDA_BASE)

//............. 0x0c00 - 0x0cff............  for EQ
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 DATAI_ADR;
    ___RW ___u32 DATAO_ADR;
    ___RW ___u32 DATA_LEN;
    ___RW ___u32 FLT_ADR;
    ___RW ___u32 BUF_MCTL;
    ___RW float BUF_MDAT;    //涉及类型转换，请勿改成u32
} JL_EQ_TypeDef;

#define JL_EQ_BASE                       (hs_base + map_adr(0x0c, 0x00))
#define JL_EQ                            ((JL_EQ_TypeDef *)JL_EQ_BASE)

//............. 0x0d00 - 0x0dff............ for src(src_v2)
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 IDAT_ADR;
    ___RW ___u32 IDAT_LEN;
    ___RW ___u32 ODAT_ADR;
    ___RW ___u32 ODAT_LEN;
    ___RW ___u32 FLTB_ADR;
    ___WO ___u32 ODAT_ADR_START;
    ___WO ___u32 ODAT_ADR_END;
    ___WO ___u32 STOP_FLAG;
    ___RW ___u32 INSR;
    ___RW ___u32 OUTSR;
    ___RW ___u32 PHASE;
    ___RW ___u32 FLTB_COEF;
    ___RW ___u32 USF_COEF;
} JL_SRC_TypeDef;

#define JL_SRC_BASE                     (hs_base + map_adr(0x0d, 0x00))
#define JL_SRC                          ((JL_SRC_TypeDef *)JL_SRC_BASE)

//............. 0x1400 - 0x14ff............ for src_sync
//typedef struct {
//    ___RW ___u32 CON0;
//    ___RW ___u32 CON1;
//    ___RW ___u32 CON2;
//    ___RW ___u32 CON3;
//    ___RW ___u32 IDAT_ADR;
//    ___RW ___u32 IDAT_LEN;
//    ___RW ___u32 ODAT_ADR;
//    ___RW ___u32 ODAT_LEN;
//    ___RW ___u32 FLTB_ADR;
//    ___WO ___u32 ODAT_ADR_START;
//    ___WO ___u32 ODAT_ADR_END;
//    ___RW ___u32 STOP_FLAG;
//    ___RW ___u32 INSR;
//    ___RW ___u32 OUTSR;
//    ___RW ___u32 PHASE;
//    ___RW ___u32 COEF;
//} JL_SRC_SYNC_TypeDef;
//
//#define JL_SRC_SYNC_BASE                     (hs_base + map_adr(0x14, 0x00))
//#define JL_SRC_SYNC                          ((JL_SRC_SYNC_TypeDef *)JL_SRC_SYNC_BASE)

//............. 0x1500 - 0x15ff............ for aes
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 SADR;
    ___RW ___u32 DADR;
    ___RW ___u32 CNT;
    ___RW ___u32 KADR;
    ___RW ___u32 KDAT;
    ___RW ___u32 IV0;
    ___RW ___u32 IV1;
    ___RW ___u32 IV2;
    ___RW ___u32 IV3;
    ___RW ___u32 GM0;
    ___RW ___u32 GM1;
    ___RW ___u32 GM2;
    ___RW ___u32 GM3;
    ___RW ___u32 KEY0;
    ___RW ___u32 KEY1;
    ___RW ___u32 KEY2;
    ___RW ___u32 KEY3;
    ___RW ___u32 KEY4;
    ___RW ___u32 KEY5;
    ___RW ___u32 KEY6;
    ___RW ___u32 KEY7;
    ___RW ___u32 DATI0;
    ___RW ___u32 DATI1;
    ___RW ___u32 DATI2;
    ___RW ___u32 DATI3;
    ___RW ___u32 DATO0;
    ___RW ___u32 DATO1;
    ___RW ___u32 DATO2;
    ___RW ___u32 DATO3;
} JL_AES_ACC_TypeDef;

#define JL_AES_ACC_BASE                 (hs_base + map_adr(0x15, 0x00))
#define JL_AES_ACC                      ((JL_AES_ACC_TypeDef *)JL_AES_ACC_BASE)

////............. 0x1600 - 0x16ff............ for apa_top
//  typedef struct {
//      ___RW ___u32 (APA_LEN);
//      ___RW ___u32 (APA_ADR);
//      ___RW ___u32 (APA_CON0);
//      ___RW ___u32 (APA_CON1);
//      ___RW ___u32 (APA_CON2);
//      ___RW ___u32 (APA_CON3);
//      ___RO ___u32 (APA_CON4);
//      ___RW ___u32 (APA_CON5);
//      ___RW ___u32 (APA_CON6);
//      ___RO ___u32 (APA_CON7);
//  } JL_APA_TypeDef;
//
//  #define JL_APA_BASE                   (hs_base + map_adr(0x16, 0x00))
//  #define JL_APA                        ((JL_APA_TypeDef   *)JL_APA_BASE)
//
//
////............. 0x1700 - 0x17ff............ for hadc_top
//  typedef struct {
//      ___RW ___u32 (HADC_CON0);               /* 00 */
//      ___RW ___u32 (HADC_CON1);               /* 01 */
//      ___RW ___u32 (HADC_COP);                /* 02 */
//      ___RW ___u32 (HADC_LEN);                /* 03 */
//      ___RW ___u32 (HADC_ADR);                /* 04 */
//      ___RO ___u32 (HADC_OSA);                /* 05 */
//  } JL_HADC_TypeDef;
//
//  #define JL_HADC_BASE                  (hs_base + map_adr(0x17, 0x00))
//  #define JL_HADC                       ((JL_HADC_TypeDef   *)JL_HADC_BASE)

//............. 0x3100 - 0x31ff............ as_emu
typedef struct {
    ___RW ___u32 WREN;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 MSG0;
    ___RW ___u32 MSG1;
    ___RW ___u32 MSG2;
    ___RW ___u32 MSG3;
    ___RO ___u32 LOG0;
    ___RO ___u32 LOG1;
    ___RO ___u32 LOG2;
    ___RO ___u32 LOG3;
} JL_AEMU_TypeDef;

#define JL_AEMU_BASE                    (as_base + map_adr(0x31, 0x00))
#define JL_AEMU                         ((JL_AEMU_TypeDef *)JL_AEMU_BASE)


//===============================================================================//
//
//      vdo_sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for emu
typedef struct {
    ___RW ___u32 WREN;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 MSG0;
    ___RW ___u32 MSG1;
    ___RW ___u32 MSG2;
    ___RW ___u32 MSG3;
    ___RO ___u32 LOG0;
    ___RO ___u32 LOG1;
    ___RO ___u32 LOG2;
    ___RO ___u32 LOG3;
} JL_VEMU_TypeDef;

#define JL_VEMU_BASE                    (vd_base + map_adr(0x00, 0x00))
#define JL_VEMU                         ((JL_VEMU_TypeDef *)JL_VEMU_BASE)

//............. 0x0100 - 0x01ff............ for dma2d
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 TASK_BASE_ADR;
    ___RW ___u32 XBUS_DELAY_PRD;
} JL_DMA2D_TypeDef;

#define JL_DMA2D_BASE                   (vd_base + map_adr(0x01, 0x00))
#define JL_DMA2D                        ((JL_DMA2D_TypeDef *)JL_DMA2D_BASE)

//............. 0x2000 - 0x23ff............ for jpg
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 YDCVAL;
    ___RW ___u32 UDCVAL;
    ___RW ___u32 VDCVAL;
    ___RW ___u32 YPTR0;
    ___RW ___u32 UPTR0;
    ___RW ___u32 VPTR0;
    ___RW ___u32 YPTR1;
    ___RW ___u32 UPTR1;
    ___RW ___u32 VPTR1;
    ___RW ___u32 BADDR;
    ___RW ___u32 BCNT;
    ___RW ___u32 MCUCNT;
    ___RW ___u32 PRECNT;
    ___RW ___u32 YUVLINE;
    ___RW ___u32 CFGRAMADDR;
    ___RW ___u32 CFGRAMVAL;
    ___RW ___u32 PTR_NUM;
    ___RW ___u32 HAS_PTR;
    ___RW ___u32 MCU_BUF;
} JL_JPG_TypeDef;

#define JL_JPG0_BASE                    (vd_base + map_adr(0x20, 0x00))
#define JL_JPG0                         ((JL_JPG_TypeDef *)JL_JPG0_BASE)

#define JL_JPG1_BASE                    (vd_base + map_adr(0x22, 0x00))
#define JL_JPG1                         ((JL_JPG_TypeDef *)JL_JPG1_BASE)

//............. 0x0300 - 0x03ff............ for gpu
typedef struct {
    ___RO ___u32 RESERVED_00;
    ___RO ___u32 PND_CON;
    ___WO ___u32 PND_CLR;
    ___RW ___u32 PND_IE;
    ___RW ___u32 XBUS_CON0;
    ___RW ___u32 XBUS_CON1;
    ___RO ___u32 RESERVED_06;
    ___RO ___u32 RESERVED_07;
    ___RW ___u32 STA_CON;
    ___RW ___u32 WIN_CON;
    ___RW ___u32 WIN_BASE;
    ___RW ___u32 WIN_STRIDE;
    ___RW ___u32 WIN_X;
    ___RW ___u32 WIN_Y;
    ___RW ___u32 INST_BASE;
    ___RW ___u32 PAD_COLOR;
    ___RW ___u32 PLUT_PTR;
    ___RW ___u32 PLUT_DAT;
} JL_GPU_TypeDef;

#define JL_GPU_BASE                     (vd_base + map_adr(0x03, 0x00))
#define JL_GPU                          ((JL_GPU_TypeDef *)JL_GPU_BASE)


#include "eva_sfr.h"

//===============================================================================//
//
//      wl_sfr address define
//
//===============================================================================//

//............. 0x0900 - 0x09ff............ for wl rfd
typedef struct {
    ___RW ___u32 WLA_CON0;
    ___RW ___u32 WLA_CON1;
    ___RW ___u32 WLA_CON2;
    ___RW ___u32 WLA_CON3;
    ___RW ___u32 WLA_CON4;
    ___RW ___u32 WLA_CON5;
    ___RW ___u32 WLA_CON6;
    ___RW ___u32 WLA_CON7;
    ___RW ___u32 WLA_CON8;
    ___RW ___u32 WLA_CON9;
    ___RW ___u32 WLA_CON10;
    ___RW ___u32 WLA_CON11;
    ___RW ___u32 WLA_CON12;
    ___RW ___u32 WLA_CON13;
    ___RW ___u32 WLA_CON14;
    ___RW ___u32 WLA_CON15;
    ___RW ___u32 WLA_CON16;
    ___RW ___u32 WLA_CON17;
    ___RW ___u32 WLA_CON20;
    ___RW ___u32 WLA_CON21;
    ___RW ___u32 WLA_CON22;
    ___RW ___u32 WLA_CON23;
    ___RW ___u32 WLA_CON24;
    ___RW ___u32 WLA_CON25;
    ___RW ___u32 WLA_CON26;
    ___RW ___u32 WLA_CON27;
    ___RW ___u32 WLA_CON28;
    ___RW ___u32 WLA_CON30;
    ___RW ___u32 PLL_CON0;
    ___RW ___u32 PLL_CON1;
} JL_WL_RFD_TypeDef;

#define JL_WL_RFD_BASE                  (hs_base + map_adr(0x09, 0x00))
#define JL_WL_RFD                       ((JL_WL_RFD_TypeDef *)JL_WL_RFD_BASE)

//............. 0x0000 - 0x00ff............ for wl_mbist
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 SEL;
    ___RW ___u32 BEG;
    ___RW ___u32 END;
    ___RW ___u32 DAT_VLD0;
    ___RW ___u32 DAT_VLD1;
    ___RW ___u32 DAT_VLD2;
    ___RW ___u32 DAT_VLD3;
} JL_WL_MBIST_TypeDef;

#define JL_WL_MBIST_BASE                (wl_base + map_adr(0x00, 0x00))
#define JL_WL_MBIST                     ((JL_WL_MBIST_TypeDef *)JL_WL_MBIST_BASE)

//............. 0x0100 - 0x01ff............ for wl_bt
typedef struct {
    ___RW ___u32 AUD_CLKCON;
    ___RW ___u32 CON0;
    ___RW ___u32 CON3;
    ___RW ___u32 LOFC_CON;
    ___RW ___u32 LOFC_RES;
    ___RW ___u32 DBG_CON;
    ___RW ___u32 CP_ACCI_RES;
    ___RW ___u32 CP_ACCQ_RES;
} JL_WL_BT_TypeDef;

#define JL_WL_BT_BASE                   (wl_base + map_adr(0x01, 0x00))
#define JL_WL_BT                        ((JL_WL_BT_TypeDef *)JL_WL_BT_BASE)

//............. 0x0200 - 0x05ff............
//Note: BTCORE define in bt_sfr.h

//............. 0x0600 - 0x09ff............
//Note: BT EDR define in jlbt_bredr.h

//............. 0x0a00 - 0x0dff............
//Note: BT BLE define in bt_ble.h

//............. 0x0e00 - 0x0eff............ for bt sync
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 DABUF;
    ___RO ___u32 DASPACE;
    ___RO ___u32 OFFEND;
    ___RO ___u32 OFFSUM;
    ___RO ___u32 PCMNUM;
    ___RO ___u32 BTTIME;
    ___WO ___u32 CON1;
    ___RW ___u32 CON2;
    ___WO ___u32 BT_OFFSET;
    ___WO ___u32 MIN_SLOT;
    ___RW ___u32 OUTSR;
    ___WO ___u32 INSR;
    ___RW ___u32 CON3;
} JL_SYNC_TypeDef;

#define JL_SYNC_BASE                    (wl_base + map_adr(0x0e, 0x00))
#define JL_SYNC                         ((JL_SYNC_TypeDef *)JL_SYNC_BASE)

//............. 0x0f00 - 0x0fff............
//Note: reserved

//............. 0x1000 - 0x10ff............ for wemu
typedef struct {
    ___RW ___u32 WREN;
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 MSG0;
    ___RW ___u32 MSG1;
    ___RW ___u32 MSG2;
    ___RW ___u32 MSG3;
    ___RO ___u32 LOG0;
    ___RO ___u32 LOG1;
    ___RO ___u32 LOG2;
    ___RO ___u32 LOG3;
} JL_WEMU_TypeDef;

#define JL_WEMU_BASE                    (wl_base + map_adr(0x10, 0x00))
#define JL_WEMU                         ((JL_WEMU_TypeDef *)JL_WEMU_BASE)

//............. 0x1100 - 0x11ff............ for wl
typedef struct {
    ___RW ___u32 WL_CON0;
    ___RW ___u32 WL_CON1;
    ___RW ___u32 WL_CON2;
    ___RW ___u32 WL_CON3;
    ___RW ___u32 WL_ANL_CON0;
    ___RW ___u32 WL_ANL_CON1;
    ___RW ___u32 WL_ANL_CON2;
    ___RW ___u32 WL_ANL_CON3;
} JL_WL_TypeDef;

#define JL_WL_BASE                      (wl_base + map_adr(0x11, 0x00))
#define JL_WL                           ((JL_WL_TypeDef *)JL_WL_BASE)

//............. 0x1200 - 0x12ff............ for wl_wf
typedef struct {
    ___RW ___u32 WF_CON0;
    ___RW ___u32 WF_CON1;
    ___RW ___u32 WF_CON2;
    ___RW ___u32 WF_CON3;
    ___RW ___u32 WF_CON4;
    ___RW ___u32 WF_CON5;
    ___RW ___u32 WF_CON6;
    ___RW ___u32 WF_CON7;
    ___RW ___u32 WF_CON8;
    ___RW ___u32 WF_CON9;
    ___RW ___u32 WF_WLC_CON;
    ___RW ___u32 SD_CON0;
    ___RW ___u32 SD_CON1;
    ___RW ___u32 SD_CON2;
    ___RW ___u32 SD_CTU_CON;
    ___WO ___u32 SD_CTU_CNT;
    ___WO ___u32 SD_CPTR;
    ___WO ___u32 SD_DPTR;
} JL_WL_WF_TypeDef;

#define JL_WL_WF_BASE                   (wl_base + map_adr(0x12, 0x00))
#define JL_WL_WF                        ((JL_WL_WF_TypeDef *)JL_WL_WF_BASE)

//............. 0x1300 - 0x112ff............
//Note: WFCORE define in wf_mac.h


//===============================================================================//
//
//      np_sfr address define
//
//===============================================================================//

#include "wl83_nvdla.h"

//===============================================================================//
//
//
//
//===============================================================================//

#include "io_omap.h"
#include "io_imap.h"
#include "husb_sfr.h"

#endif

