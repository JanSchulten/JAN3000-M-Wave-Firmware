#ifndef __DSI_H__
#define __DSI_H__

#include "generic/typedef.h"
#include "asm/lcd_common.h"
#include "generic/ioctl.h"
#include "dpi.h"

// #define DSI_DEBUG

//lane config
#define MIPI_LANE_DIS 	0x00
#define MIPI_LANE_EN  	0x10
#define MIPI_LANE_EX    0x20	// DP/DN exchange enable

#define MIPI_LANE_D0  	0x00
#define MIPI_LANE_D1  	0x01
#define MIPI_LANE_D2  	0x02
#define MIPI_LANE_D3  	0x03
#define MIPI_LANE_CLK 	0x04

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                  MIPI屏相关                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @Synopsis  dsi时序设置
 */
struct dsi_timing {
    u16 freq;			// mipi pll总线频率
    u16 tval_lpx;		// LowPower state period 传输时间
    u32 tval_wkup;      // 总线从ulps模式唤醒的时间
    u16 tval_c_pre;		// clk lane HS transition prepare time
    u16 tval_c_sot;     // clk lane HS transition sot time:LP-00
    u16 tval_c_eot;     // clk lane HS transition eot time
    u16 tval_c_brk;		// clk lane总线离开回到LP-11的时间
    u16 tval_d_pre;		// data lane HS transition prepare time
    u16 tval_d_sot;		// data lane HS transition sot time:LP-00
    u16 tval_d_eot;		// data lane HS transition eot time
    u16 tval_d_brk;		// data lane总线离开回到LP-11的时间
    u16 tval_c_rdy;
};

/**
 * @Synopsis  lane重映射
 */
struct dsi_lane_mapping {
    u8 x0_lane;
    u8 x1_lane;
    u8 x2_lane;
    u8 x3_lane;
    u8 x4_lane;
};
/*
 * 视频/命令模式
 */
enum VIDEO_STREAM_MODE {
    VIDEO_STREAM_COMMAND,   //命令模式
    VIDEO_STREAM_VIDEO,		//视频模式
};
/*
 * 同步事件
 */
enum SYNC_MODE {
    SYNC_PULSE_MODE,
    SYNC_EVENT_MODE,
    BURST_MODE,
};


enum COLOR_FORMAT {
    COLOR_FORMAT_RGB565,
    COLOR_FORMAT_RGB666,
    COLOR_FORMAT_RGB666_LOOSELY,
    COLOR_FORMAT_RGB888,
};
/*
 *  数据包类型
 */
enum PIXEL_TYPE {
    PIXEL_RGB565_COMMAND,
    PIXEL_RGB565_VIDEO,
    PIXEL_RGB666,
    PIXEL_RGB666_LOOSELY,
    PIXEL_RGB888,
};


/**
 * @Synopsis  时序
 */
struct dsi_video_timing {
    u8 video_mode;			//command/video mode
    u8 sync_mode;			//sync pulse/event mode
    u8 color_mode;
    u8 virtual_ch;		    //virtual channel(video mode)
    u8 hs_eotp_en;			//enable EoT packet
    u8 pixel_type;

    u16 dsi_vdo_vsa_v;		//vertical sync pluse
    u16 dsi_vdo_vbp_v;		//vertical back porch
    u16 dsi_vdo_vact_v; 	//vertical active line
    u16 dsi_vdo_vfp_v;		//vertical front porch

    u16 dsi_vdo_hsa_v;		//horizontal sync pulse
    u16 dsi_vdo_hbp_v;		//horizontal back porch
    u16 dsi_vdo_hact_v;		//horizontal active pixel
    u16 dsi_vdo_hfp_v;		//horizontal front porch

    u16 dsi_vdo_bllp0_v;	//BLLP period width
    u16 dsi_vdo_bllp1_v;	//horizontal null dummy
};
/*
 * MIPI PLL时钟分频
 */
enum MIPI_PLL_DIV {
    MIPI_PLL_DIV1,
    MIPI_PLL_DIV2,
    MIPI_PLL_DIV4,
    MIPI_PLL_DIV8,
};

/*
 * 包类型
 */
enum PACKET_TYPE {
    PACKET_DCS,		 			/* DCS包 */
    PACKET_DCS_WITHOUT_BTA,		/* DCS包不带响应 */
    PACKET_GENERIC,  			/* 通用包 */
    PACKET_GENERIC_WITHOUT_BTA, /* 通用包不带响应 */
};

struct mipi_dev {
    struct basic_info info;
    struct pll4_info pll4;
    struct dsi_lane_mapping lane_mapping;
    struct dsi_video_timing video_timing;
    struct dsi_timing timing;

    unsigned int reset_gpio;
    void (*lcd_reset)(unsigned int reset_gpio);
    void (*lcd_reset_release)(unsigned int reset_gpio);

    const u8 *cmd_list;
    u16 cmd_list_item;

    u8 debug_mode;								//调试使能
    /* 配置PLL的频率范围为600MHz~1.2GHz,少于600MHz的频率通过分频获得 */
};


//control
#define vdo_done_isr_en   dsi_sys_con |= BIT(2)
#define vdo_done_isr_dis  dsi_sys_con &=~BIT(2)
#define cmd_done_isr_en   dsi_sys_con |= BIT(3)
#define cmd_done_isr_dis  dsi_sys_con &=~BIT(3)
#define cmd_isr_status    (dsi_sys_con&BIT(3))
#define bta_done_isr_en   dsi_sys_con |= BIT(4)
#define bta_fail_isr_en   dsi_sys_con |= BIT(5)
#define cdet_isr_en       dsi_sys_con |= BIT(6)
#define vdo_done_pnd     (dsi_sys_con &  BIT(9))
#define vdo_done_pnd_clr  dsi_sys_con |= BIT(8)
#define cmd_done_pnd     (dsi_sys_con &  BIT(11))
#define cmd_done_pnd_clr  dsi_sys_con |= BIT(10)
#define bta_done_pnd     (dsi_sys_con &  BIT(13))
#define bta_done_pnd_clr  dsi_sys_con |= BIT(12)
#define bta_fail_pnd     (dsi_sys_con &  BIT(15))
#define bta_fail_pnd_clr  dsi_sys_con |= BIT(14)
#define cdet_pnd         (dsi_sys_con &  BIT(17))
#define cdet_pnd_clr      dsi_sys_con |= BIT(16)

#define dsi_tri_kick  	  dsi_task_con |= BIT(4)
#define dsi_cmd_kick  	  dsi_task_con |= BIT(5)
#define dsi_vdo_kick  	  dsi_task_con |= BIT(6)
#define dsi_vdo_stop  	  dsi_task_con |= BIT(7)
#define clock_ready   	 (dsi_bus_con & BIT(3))

#define rx_te   		 (dsi_cmd_con1&BIT(16))
#define rx_ack  		 (dsi_cmd_con1&BIT(17))
#define rx_err  		 (dsi_cmd_con1&BIT(18))
#define rx_eot  		 (dsi_cmd_con1&BIT(19))
#define rx_gsr1 		 (dsi_cmd_con1&BIT(20))
#define rx_gsr2 		 (dsi_cmd_con1&BIT(21))
#define rx_glr  		 (dsi_cmd_con1&BIT(22))
#define rx_dsr1 		 (dsi_cmd_con1&BIT(23))
#define rx_dsr2 		 (dsi_cmd_con1&BIT(24))
#define rx_dlr  		 (dsi_cmd_con1&BIT(25))
#define cmd_err 		 (dsi_cmd_con1&BIT(26))

#define BTA_TIMEOUT		0x00
#define BTA_FAIL		0x01
#define BTA_RX_TE		0x02
#define BTA_RX_ACK		0x03
#define BTA_RX_EOT		0x04
#define BTA_RX_ERR		0x05
#define BTA_CMD_ERR		0x06
#define BTA_RX_GSR1		0x07
#define BTA_RX_GSR2		0x08
#define BTA_RX_GLR		0x09
#define BTA_RX_DSR1		0x0a
#define BTA_RX_DSR2		0x0b
#define BTA_RX_DLR		0x0c
#define BTA_DONE        0x0d

#define _W 0x00
#define _R 0x01
#define DELAY(t) t
#define SIZE(s)  s

struct init_cmd {
    u8 rw;
    u8 delay;
    u8 packet_type;
    u8 size;
    u8 data[0];
};

void dcs_set_debug(u8 onoff);
void dsi_video_kick_start();
void dsi_dev_init(struct mipi_dev *dev_t);
void dsi_port_init(struct mipi_dev *dev);
void dsi_video_stop();
u8 dsi_cmd_busy_status(void);
void dsi_disable(void);

#define delay_10us(t) //未定义
#define delay_10ms(t) delay_ms(10*t)

#endif

