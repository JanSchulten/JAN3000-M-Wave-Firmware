//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : q32DSP core sfr define                                            //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __Q32DSP_CSFR__
#define __Q32DSP_CSFR__

#define ___RW         volatile       // read write
#define ___RO         volatile const // only read
#define ___WO         volatile       // only write

#define ___u8         unsigned int   // u8  to u32 special for struct
#define ___u16        unsigned int   // u16 to u32 special for struct
#define ___u32        unsigned int

#define csfr_base    0xff0000

//*********************************************************************************
//
// hcore_sfr
//
//*********************************************************************************

//............. 0x0000 - 0x00ff............
typedef struct {
    ___RW ___u32 CON0;
    ___RW ___u32 CON1;
    ___RW ___u32 CON2;
    ___RW ___u32 CON3;
    ___RW ___u32 CON4;
    ___RW ___u32 FTMAX;
} JL_CMNG_TypeDef;

#define JL_CMNG_BASE                   (csfr_base + map_adr(0x00, 0x00))
#define JL_CMNG                        ((JL_CMNG_TypeDef *)JL_CMNG_BASE)

//............. 0x0100 - 0x01ff............
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 KEY;
} JL_SDTAP_TypeDef;

#define JL_SDTAP_BASE                  (csfr_base + map_adr(0x01, 0x00))
#define JL_SDTAP                       ((JL_SDTAP_TypeDef *)JL_SDTAP_BASE)

//............. 0x0200 - 0x02ff............
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
} JL_CEMU_TypeDef;

#define JL_CEMU_BASE                   (csfr_base + map_adr(0x02, 0x00))
#define JL_CEMU                        ((JL_CEMU_TypeDef *)JL_CEMU_BASE)

//............. 0x0300 - 0x03ff............
#define TZMPU_U_PWEN                   (1<<17)
#define TZMPU_U_PREN                   (1<<16)
#define TZMPU_U_XEN                    (1<<15)
#define TZMPU_U_WEN                    (1<<14)
#define TZMPU_U_REN                    (1<<13)
#define TZMPU_U_PEN                    (TZMPU_U_PWEN | TZMPU_U_PREN)

#define TZMPU_S_PWEN                   (1<<4)
#define TZMPU_S_PREN                   (1<<3)
#define TZMPU_S_XEN                    (1<<2)
#define TZMPU_S_WEN                    (1<<1)
#define TZMPU_S_REN                    (1<<0)
#define TZMPU_S_PEN                    (TZMPU_S_PWEN | TZMPU_S_PREN)

#define TZMPU_IDx_cfg(n, id)           (id<<(n*8))
#define TZMPU_SID_pen(n, pr, pw)       ((pr<<(5+n)) | (pw<<(9+n)))
#define TZMPU_UID_pen(n, pr, pw)       ((pr<<(18+n)) | (pw<<(22+n)))

typedef struct {
    ___RW ___u32 CON[16]; // 0-7 used in wl83
    ___RW ___u32 CID[16]; // 0-7 used in wl83
    ___RW ___u32 BEG[16]; // 0-7 used in wl83
    ___RW ___u32 END[16]; // 0-7 used in wl83
} JL_TZMPU_TypeDef;

#define JL_TZMPU_BASE                  (csfr_base + map_adr(0x03, 0x00))
#define JL_TZMPU                       ((JL_TZMPU_TypeDef *)JL_TZMPU_BASE)

//............. 0x0400 - 0x04ff............

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 TLB1_BEG;
    ___RW ___u32 TLB1_END;
} JL_MMU_TypeDef;

#define JL_MMU_BASE                   (csfr_base + map_adr(0x04, 0x00))
#define JL_MMU                        ((JL_MMU_TypeDef *)JL_MMU_BASE)

typedef struct {
    short page: 15;
    short vld: 1;
} JL_MMU_TLB1_TypeDef;

#define JL_MMU_TLB1                   ((JL_MMU_TLB1_TypeDef *)(JL_MMU->TLB1_BEG))

//............. 0x0500 - 0x05ff............
//#define JL_TypeDef_L1P                JL_TypeDef_q32DSP_ICU
#define JL_TypeDef_L1P                JL_TypeDef_q32DSP_DCU
#define JL_L1P_BASE                   (csfr_base + map_adr(0x05, 0x00))
#define JL_L1P                        ((JL_TypeDef_L1P *)JL_L1P_BASE)

//............. 0x0600 - 0x06ff............
#define JL_TypeDef_L2I                JL_TypeDef_q32DSP_ICU
#define JL_L2I_BASE                   (csfr_base + map_adr(0x06, 0x00))
#define JL_L2I                        ((JL_TypeDef_L2I *)JL_L2I_BASE)

//............. 0x0700 - 0x07ff............
#define JL_TypeDef_L2D                JL_TypeDef_q32DSP_DCU
#define JL_L2D_BASE                   (csfr_base + map_adr(0x07, 0x00))
#define JL_L2D                        ((JL_TypeDef_L2D *)JL_L2D_BASE)

//............. 0x0800 - 0x08ff............
typedef struct {
    ___RO ___u32 CHIP_ID;
    ___RO ___u32 CHIP_VER;
} JL_SYSTEM_TypeDef;

#define JL_SYSTEM_BASE                (csfr_base + map_adr(0x08, 0x00))
#define JL_SYSTEM                     ((JL_SYSTEM_TypeDef *)JL_SYSTEM_BASE)

//............. 0x0900 - 0x09ff............
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 SEL;
    ___RW ___u32 BEG;
    ___RW ___u32 END;
    ___RW ___u32 DAT_VLD0;
    ___RW ___u32 DAT_VLD1;
    ___RW ___u32 DAT_VLD2;
    ___RW ___u32 DAT_VLD3;
    ___RO ___u32 ROM_CRC;
} JL_MBIST_TypeDef;

#define JL_MBIST_BASE                 (csfr_base + map_adr(0x09, 0x00))
#define JL_MBIST                      ((JL_MBIST_TypeDef *)JL_MBIST_BASE)

//............. 0x0a00 - 0x0aff............
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 CADR;
    ___RW ___u32 ACC0L;
    ___RW ___u32 ACC0H;
    ___RW ___u32 ACC1L;
    ___RW ___u32 ACC1H;
    ___RW ___u32 CONST;
    ___RW ___u32 TEST1;
} JL_FFT_TypeDef;

#define JL_FFT_BASE                   (csfr_base + map_adr(0x0a, 0x00))
#define JL_FFT                        ((JL_FFT_TypeDef *)JL_FFT_BASE)

//............. 0x0b00 - 0x0bff............
typedef struct {
    ___RO ___u32 UID0;
    ___RO ___u32 UID1;
    ___RO ___u32 UID2;
    ___RO ___u32 UID3;
    ___RO ___u32 UID4;
} JL_UUID_TypeDef;

#define JL_UUID_BASE                  (csfr_base + map_adr(0x0b, 0x00))
#define JL_UUID                       ((JL_UUID_TypeDef *)JL_UUID_BASE)

//............. 0x0c00 - 0x0cff............
typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 POL;
    ___WO ___u32 INIT;
    ___RW ___u32 REG;
} JL_GPCRC_TypeDef;

#define JL_GPCRC_BASE                 (csfr_base + map_adr(0x0c, 0x00))
#define JL_GPCRC                      ((JL_GPCRC_TypeDef *)JL_GPCRC_BASE)

//*********************************************************************************
//
// q32DSP_sfr
//
//*********************************************************************************

//---------------------------------------------//
// q32DSP define
//---------------------------------------------//

#define q32DSP_sfr_offset   0x000800
#define q32DSP_sfr_base    (csfr_base + 0xe000)

#define q32DSP_cpu_base    (q32DSP_sfr_base + 0x0000)
#define q32DSP_icu_base    (q32DSP_sfr_base + 0x0400)

#define q32DSP(n)          ((JL_TypeDef_q32DSP     *)(q32DSP_sfr_base + q32DSP_sfr_offset*n))
#define q32DSP_icu(n)      ((JL_TypeDef_q32DSP_ICU *)(q32DSP_icu_base + q32DSP_sfr_offset*n))

//---------------------------------------------//
// q32DSP core sfr
//---------------------------------------------//

typedef struct {
    /* 00 */  ___RO ___u32 DR00;
    /* 01 */  ___RO ___u32 DR01;
    /* 02 */  ___RO ___u32 DR02;
    /* 03 */  ___RO ___u32 DR03;
    /* 04 */  ___RO ___u32 DR04;
    /* 05 */  ___RO ___u32 DR05;
    /* 06 */  ___RO ___u32 DR06;
    /* 07 */  ___RO ___u32 DR07;
    /* 08 */  ___RO ___u32 DR08;
    /* 09 */  ___RO ___u32 DR09;
    /* 0a */  ___RO ___u32 DR10;
    /* 0b */  ___RO ___u32 DR11;
    /* 0c */  ___RO ___u32 DR12;
    /* 0d */  ___RO ___u32 DR13;
    /* 0e */  ___RO ___u32 DR14;
    /* 0f */  ___RO ___u32 DR15;

    /* 10 */  ___RO ___u32 RETI;
    /* 11 */  ___RO ___u32 RETE;
    /* 12 */  ___RO ___u32 RETX;
    /* 13 */  ___RO ___u32 RETS;
    /* 14 */  ___RO ___u32 SR04;
    /* 15 */  ___RO ___u32 PSR;
    /* 16 */  ___RO ___u32 CNUM;
    /* 17 */  ___RO ___u32 SR07;
    /* 18 */  ___RO ___u32 SR08;
    /* 19 */  ___RO ___u32 SR09;
    /* 1a */  ___RO ___u32 SR10;
    /* 1b */  ___RO ___u32 ICFG;
    /* 1c */  ___RO ___u32 USP;
    /* 1d */  ___RO ___u32 SSP;
    /* 1e */  ___RO ___u32 SP;
    /* 1f */  ___RO ___u32 PCRS;

    /* 20 */  ___RW ___u32 BPCON;
    /* 21 */  ___RW ___u32 BSP;
    /* 22 */  ___RW ___u32 BP0;
    /* 23 */  ___RW ___u32 BP1;
    /* 24 */  ___RW ___u32 BP2;
    /* 25 */  ___RW ___u32 BP3;
    /* 26 */  ___WO ___u32 CMD_PAUSE;
    /* 27 */  ___RW ___u32 BP4;
    /* 28 */  ___RW ___u32 BP5;
    /* 29 */  ___RW ___u32 BP6;
    /* 2a */  ___RW ___u32 BP7;
    /*    */  ___RO ___u32 REV2a[0x30 - 0x2a - 1];

    /* 30 */  ___RW ___u32 PMU_CON0;
    /* 31 */  ___RW ___u32 PMU_CON1;
    /* 32 */  ___RW ___u32 RST_ADDR;
    /* 33 */  ___RW ___u32 TSEVT;
    /*    */  ___RO ___u32 REV30[0x37 - 0x33 - 1];
    /* 37 */  ___RW ___u32 NSC_CON0;
    /* 38 */  ___RW ___u32 NSC_ADRH;
    /* 39 */  ___RW ___u32 NSC_ADRL;
    /* 3a */  ___RW ___u32 NSC_NS;
    /* 3b */  ___RW ___u8  TTMR_CON;
    /* 3c */  ___RW ___u32 TTMR_CNT;
    /* 3d */  ___RW ___u32 TTMR_PRD;
    /* 3e */  ___RW ___u32 BANK_CON;
    /* 3f */  ___RW ___u32 BANK_NUM;

    /* 40 */  ___RW ___u32 ICFG00;
    /* 41 */  ___RW ___u32 ICFG01;
    /* 42 */  ___RW ___u32 ICFG02;
    /* 43 */  ___RW ___u32 ICFG03;
    /* 44 */  ___RW ___u32 ICFG04;
    /* 45 */  ___RW ___u32 ICFG05;
    /* 46 */  ___RW ___u32 ICFG06;
    /* 47 */  ___RW ___u32 ICFG07;
    /* 48 */  ___RW ___u32 ICFG08;
    /* 49 */  ___RW ___u32 ICFG09;
    /* 4a */  ___RW ___u32 ICFG10;
    /* 4b */  ___RW ___u32 ICFG11;
    /* 4c */  ___RW ___u32 ICFG12;
    /* 4d */  ___RW ___u32 ICFG13;
    /* 4e */  ___RW ___u32 ICFG14;
    /* 4f */  ___RW ___u32 ICFG15;

    /* 50 */  ___RW ___u32 ICFG16;
    /* 51 */  ___RW ___u32 ICFG17;
    /* 52 */  ___RW ___u32 ICFG18;
    /* 53 */  ___RW ___u32 ICFG19;
    /* 54 */  ___RW ___u32 ICFG20;
    /* 55 */  ___RW ___u32 ICFG21;
    /* 56 */  ___RW ___u32 ICFG22;
    /* 57 */  ___RW ___u32 ICFG23;
    /* 58 */  ___RW ___u32 ICFG24;
    /* 59 */  ___RW ___u32 ICFG25;
    /* 5a */  ___RW ___u32 ICFG26;
    /* 5b */  ___RW ___u32 ICFG27;
    /* 5c */  ___RW ___u32 ICFG28;
    /* 5d */  ___RW ___u32 ICFG29;
    /* 5e */  ___RW ___u32 ICFG30;
    /* 5f */  ___RW ___u32 ICFG31;

    /* 60 */  ___RO ___u32 IPND0;
    /* 61 */  ___RO ___u32 IPND1;
    /* 62 */  ___RO ___u32 IPND2;
    /* 63 */  ___RO ___u32 IPND3;
    /* 64 */  ___RO ___u32 IPND4;
    /* 65 */  ___RO ___u32 IPND5;
    /* 66 */  ___RO ___u32 IPND6;
    /* 67 */  ___RO ___u32 IPND7;
    /* 68 */  ___WO ___u32 ILAT_SET;
    /* 69 */  ___WO ___u32 ILAT_CLR;
    /* 6a */  ___RW ___u32 IPMASK;
    /* 6b */  ___RW ___u32 GIEMASK;
    /* 6c */  ___RW ___u32 IWKUP_NUM;
    /*    */  ___RO ___u32 REV6a[0x70 - 0x6c - 1];

    /* 70 */  ___RW ___u32 ETM_CON;
    /* 71 */  ___RO ___u32 ETM_PC0;
    /* 72 */  ___RO ___u32 ETM_PC1;
    /* 73 */  ___RO ___u32 ETM_PC2;
    /* 74 */  ___RO ___u32 ETM_PC3;
    /* 75 */  ___RW ___u32 WP0_ADRH;
    /* 76 */  ___RW ___u32 WP0_ADRL;
    /* 77 */  ___RW ___u32 WP0_DATH;
    /* 78 */  ___RW ___u32 WP0_DATL;
    /* 79 */  ___RW ___u32 WP0_PC;
    /* 7a */  ___RO ___u32 ETM_PC4;
    /* 7b */  ___RO ___u32 ETM_PC5;
    /*    */  ___RO ___u32 REV7b[0x80 - 0x7b - 1];

    /* 80 */  ___RW ___u32 EMU_CON;
    /* 81 */  ___RW ___u32 EMU_MSG;
    /* 82 */  ___RW ___u32 EMU_SSP_H;
    /* 83 */  ___RW ___u32 EMU_SSP_L;
    /* 84 */  ___RW ___u32 EMU_USP_H;
    /* 85 */  ___RW ___u32 EMU_USP_L;
    /* 86 */  ___RW ___u32 LIM_PC0_H;
    /* 87 */  ___RW ___u32 LIM_PC0_L;
    /* 88 */  ___RW ___u32 LIM_PC1_H;
    /* 89 */  ___RW ___u32 LIM_PC1_L;
    /* 8a */  ___RW ___u32 LIM_PC2_H;
    /* 8b */  ___RW ___u32 LIM_PC2_L;
    /*    */  ___RO ___u32 REV8b[0x90 - 0x8b - 1];

    /* 90 */  ___RW ___u32 ESU_CON;
    /* 91 */  ___RO ___u32 CNT_CHIT;
    /* 92 */  ___RO ___u32 CNT_CMIS;
    /* 93 */  ___RO ___u32 CNT_FILL;
    /* 94 */  ___RO ___u32 CNT_IHIT;
    /* 95 */  ___RO ___u32 CNT_IMIS;
    /* 96 */  ___RO ___u32 CNT_RHIT;
    /* 97 */  ___RO ___u32 CNT_RMIS;
    /* 98 */  ___RO ___u32 CNT_WHIT;
    /* 99 */  ___RO ___u32 CNT_WMIS;
} JL_TypeDef_q32DSP;

//---------------------------------------------//
// q32DSP icache sfr
//---------------------------------------------//

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 EMU_CON;
    ___RW ___u32 EMU_MSG;
    ___RW ___u32 EMU_ID;
    ___RW ___u32 CMD_CON;
    ___RW ___u32 CMD_BEG;
    ___RW ___u32 CMD_END;
    ___RW ___u32 CNT_RACK;
    ___RW ___u32 CNT_RNAK;
    ___RW ___u32 MBIST_CON;
} JL_TypeDef_q32DSP_ICU;

//---------------------------------------------//
// q32DSP dcache sfr
//---------------------------------------------//

typedef struct {
    ___RW ___u32 CON;
    ___RW ___u32 EMU_CON;
    ___RW ___u32 EMU_MSG;
    ___RW ___u32 EMU_ID;
    ___RW ___u32 CNT_WACK;
    ___RW ___u32 CNT_WNAK;
    ___RW ___u32 CNT_RACK;
    ___RW ___u32 CNT_RNAK;
    ___RW ___u32 MBIST_CON;
    ___RO ___u32 REV0[0x10 - 0x8 - 1];
    ___RW ___u32 CMD_CON[4];
    ___RW ___u32 CMD_BEG[4];
    ___RW ___u32 CMD_END[4];
    ___RO ___u32 REV1[0x20 - 0x1b - 1];
    ___WO ___u32 CMO[32];
} JL_TypeDef_q32DSP_DCU;

#undef ___RW
#undef ___RO
#undef ___WO

#undef ___u8
#undef ___u16
#undef ___u32

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif

