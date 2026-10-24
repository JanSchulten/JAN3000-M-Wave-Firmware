#ifndef __PMU_FLAG_H__
#define __PMU_FLAG_H__

struct efuse_page0_config_t {
    /* 0    */  u32 en_act: 1;
    /* 1    */  u32 lvd_rst_en: 1;
    /* 4-2  */  u32 lvd_level: 3;
    /* 8-5  */  u32 iovdd_level: 4;
    /* 9    */  u32 mclr_en : 1;
    /* 10   */  u32 fast_up: 1;
    /* 11   */  u32 pinr_io_enable: 1;
    /* 18-12*/  u32 vtemp: 7;
    /* 19   */  //u32 boot_decrypt : 1;
    /* 23-20*/  //u32 res: 4;
    /* 23-19*/  u32 adc_trim: 5;
    /* 24   */  u32 avdd18_dis: 1;
    /* 25   */  u32 avdd28_dis: 1;
    /* 16   */  u32 res_sfc_fast_boot_dis: 1;
    /* 27   */  u32 trim_act: 1;
    /* 27-24*/  u32 mvbg_lel: 4;
};

struct efuse_page0_t {
    u8 efuse_read_finish;
    union {
        struct efuse_page0_config_t cfg;
        u32 value;
    } u;
};

struct efuse_page1_config_t {
    /* 23-0 */  u32 chip_key: 24;
    /* 16   */  u32 cp_pass: 1;
    /* 17   */  u32 ft_pass: 1;
    /* 27:26*/  u32 wvdd_level_trim: 2;
    /* 31-28*/  u32 audio_vbg: 4;
};

struct efuse_page1_t {
    union {
        struct efuse_page1_config_t cfg;
        u32 value;
    } u;
};

struct soft_flag0_t {
    u8 wdt_dis: 1;
    u8 lvd_en: 1;
    u8 sfc_fast_boot: 1;
    u8 flash_power_keep: 1;
    u8 skip_flash_reset: 1;
    u8 sfc_flash_stable_delay_sel: 1;   //0: 0.5mS; 1: 1mS
    u8 flash_stable_delay_sel: 2;       //0: 0mS;  1: 2mS;  2:4mS; 3:6mS
};

struct soft_flag1_t {
    u8 fusbdp: 4;
    u8 fusbdm: 4;
};

struct soft_flag2_t {
    u8 pp0: 4;
    u8 disable_uart_upgrade: 1;
    u8 uart_key_port_pull_down: 1;
    //for avddr.  0:0.2mS;  1:0.3mS;  2:0.4mS;  3:0.5mS;
    u8 flash_spi_baud: 2;
};

struct soft_flag3_t {
    u8 husbdp: 4;
    u8 husbdm: 4;
};

struct soft_flag7_4_t {
    u32 chip_key : 32;
};

struct soft_flag11_8_t {
    u32 pll_lrc_nr : 12;
    u32 res : 20;
};

struct boot_soft_flag_t {
    u8 soff_wkup;
    union {
        struct soft_flag0_t boot_ctrl;
        u8 value;
    } flag0;
    union {
        struct soft_flag1_t boot_ctrl;
        u8 value;
    } flag1;
    union {
        struct soft_flag2_t boot_ctrl;
        u8 value;
    } flag2;
    union {
        struct soft_flag3_t boot_ctrl;
        u8 value;
    } flag3;
    union {
        struct soft_flag7_4_t boot_ctrl;
        u32 value;
    } flag7_4;
    union {
        struct soft_flag11_8_t boot_ctrl;
        u32 value;
    } flag11_8;
};

extern struct boot_soft_flag_t g_boot_soft_flag;

typedef enum {
    SOFTFLAG_HIGH_RESISTANCE,
    SOFTFLAG_PU,
    SOFTFLAG_PD,

    SOFTFLAG_OUT0,
    SOFTFLAG_OUT0_HD0,
    SOFTFLAG_OUT0_HD,
    SOFTFLAG_OUT0_HD0_HD,

    SOFTFLAG_OUT1,
    SOFTFLAG_OUT1_HD0,
    SOFTFLAG_OUT1_HD,
    SOFTFLAG_OUT1_HD0_HD,
} soft_flag_io_stage_t;

#endif
