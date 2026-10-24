#ifndef __ASM_DPI_H__
#define __ASM_DPI_H__

#include "generic/typedef.h"
#include "asm/lcd_common.h"

#define DPI_EN()        	    dpi_con |= BIT(0)//推屏使能
#define DPI_DIS()       	    dpi_con &=~BIT(0)//关闭推屏
#define DPI_KICK_START()	    dpi_con |= BIT(1)//推屏开始
#define DPI_KICK_END()	        dpi_con &=~BIT(1)//推屏暂停
#define DPI_SINGAL_FRAME_EN()   dpi_con |= BIT(2)//单帧模式
#define DPI_SINGAL_FRAME_DIS()  dpi_con &=~BIT(2)
#define DPI_ISR_EN()		    dpi_con |= BIT(3)//帧中断使能
#define DPI_ISR_DIS()           dpi_con &=~BIT(3)
#define DPI_PND_CLR()   	    dpi_con |= BIT(4)
#define DPI_PND()			    dpi_con &  BIT(5)//帧pend

#define DMM_LINE_PEND()         dmm_con  & BIT(30)
#define DMM_LINE_PEND_CLR()     dmm_con |= BIT(29)
#define DMM_LINE_IE()           dmm_con |= BIT(28)
#define DMM_LINE_IE_DIS()       dmm_con &=~BIT(28)
#define DMM_LINE_IE_IS_EN()     dmm_con  & BIT(28)
#define DMM_FIFO_BUSY()         dmm_con  & BIT(27)
#define DMM_GET_ERR_PND()       dmm_con  & BIT(15)
#define DMM_FREND_PND()         dmm_con  & BIT(14)
#define DMM_FIFO_PND()          dmm_con  & BIT(13)
#define DMM_SFR_PND()           dmm_con  & BIT(12)
#define DMM_FST_PND()           dmm_con  & BIT(11)
#define DMM_GET_ERR_PND_CLR()   dmm_con |= BIT(10)
#define DMM_FREND_PND_CLR()     dmm_con |= BIT(9 )
#define DMM_FIFO_PND_CLR()      dmm_con |= BIT(8 )
#define DMM_SFR_PND_CLR()       dmm_con |= BIT(7 )
#define DMM_FST_PND_CLR()       dmm_con |= BIT(6 )
#define DMM_GET_ERR_IE()        dmm_con |= BIT(5 )
#define DMM_FREND_IE()          dmm_con |= BIT(4 )
#define DMM_FIFO_IE()           dmm_con |= BIT(3 )
#define DMM_SFR_IE()            dmm_con |= BIT(2 )
#define DMM_FST_IE()            dmm_con |= BIT(1 )
#define DMM_DMM_DIS()           dmm_con &=~BIT(0 )
#define DMM_DMM_EN()            dmm_con |= BIT(0 )


#define R_GAMMA BIT(SET_R_GMA)
#define G_GAMMA BIT(SET_G_GMA)
#define B_GAMMA BIT(SET_B_GMA)

extern const struct device_operations imd_dev_ops;

//<时钟数>
enum NCYCLE {
    CYCLE_ONE,      //RGB-24BIT、MCU-16BIT
    CYCLE_TWO,      //MCU-8BIT
    CYCLE_THREE,    //RGB-8BIT
    CYCLE_FOUR,     //RGB-8BIT+dummy
};

#define CLK_NEGATIVE    BIT(0)
#define CLK_EN          BIT(1)
#define CLK_DIS         0

#define SIGNAL_DEN      (0<<2)
#define SIGNAL_HSYNC    (1<<2)
#define SIGNAL_VSYNC    (2<<2)
#define SIGNAL_FIELD    (3<<2)


//<位宽>
enum LCD_PORT {
    PORT_3BITS  = 0x01,
    PORT_6BITS  = 0x03,
    PORT_8BITS  = 0x07,
    PORT_9BITS  = 0x0F,
    PORT_16BITS = 0x1F,
    PORT_18BITS = 0x3F,
    PORT_24BITS = 0x7F,
};
//<lcd port select>
/*
 * PORT_GROUP_AA:
 *    DAT0~DAT23 : PA04~PA11,PB00~PB15
 *    PA13 : DCLK
 *    PA12 : DE(Default,configurable)
 *    PA14 : HSYNC(Default,configurable)
 *    PA15 : VSYNC(Default,configurable)
 *
 * PORT_GROUP_AB:
 *    DAT0~DAT18 : PB10~PB15,PC00~PC01,PB00~PB09
 *    PA12 : DE(Default,configurable)
 *    PA14 : HSYNC(Default,configurable)
 *    PA15 : VSYNC(Default,configurable)
 *
 */
enum LCD_GROUP {//datasheet
    PORT_GROUP_AA,
    PORT_GROUP_AB,
};

//<color type>

//<奇/偶行相位>
enum PHASE {
    PHASE_R,
    PHASE_G,
    PHASE_B,
};

//<RAW模式>
enum RAW_MODE {
    RAW_STR_MODE,//B->R->G(default)
    RAW_DLT_MODE,//R->G->B
};

//<模式>
/*enum LCD_MODE {//单帧mode 和 rgb+mipi */
/*MODE_PAP_RGB_MCU,//使用PAP接口加RGB屏接口推MCU 有调色模块*/
/*MODE_PAP_MCU,//使用PAP接口推屏mcu 无调色模块*/
/*MODE_RGB,//推RGB屏*/
/*};//多加一个mipi*/

/*enum LCD_MODE {//单帧mode 和 rgb+mipi */
/*MODE_SINGLE_FRAME,//使用PAP接口加RGB屏接口推MCU 有调色模块*/
/*MODE_MCU,//使用PAP接口推屏mcu 无调色模块*/
/*MODE_RGB,//推RGB屏*/
/*MODE_MIPI,//推MIPI*/
/*};*/
//ti统一用之歌on=

/*
 * 屏接口类型
 */



//<RGB输出大小端配置只有IMD模块支持>
enum LCD_ENDIAN {//小端模式原始数据输出 大端模式交换两个字节的位置 仅在RGB565模式生效 //取余数据格式通过调节RB生效
    MODE_BE,//大端
    MODE_LE,//小端
};

//<时钟选择以及分频>
#define IMD_INTERNAL    	0x8000 //使用单独的PLL，时钟源可配置
#define IMD_EXTERNAL    	0x0000 //[默认,可省略] 共享外部PLL时钟,目标频率为480MHz,通过分频得到所需的频率

/*
 * 注: IMD_EXTERNAL可省略
 * IMD_EXTERNAL|PLL2_CLK|DIVA_1|DIVB_2         等同于 PLL2_CLK|DIVA_1|DIVB_2
 */


#define PLL_SRC_SEL   		2
#define PLL_DIV1  			6
#define PLL_DIV2  			8

/*
 * @brief DPI模块内部时钟源选择
 */
#define PLL2_CLK_480M       (3<<PLL_SRC_SEL)
#define PLL2_CLK_192M       (4<<PLL_SRC_SEL)
#define PLL3_CLK        	(5<<PLL_SRC_SEL)
#define PLL4_CLK        	(6<<PLL_SRC_SEL)
#define PLL5_CLK        	(7<<PLL_SRC_SEL)

/*
 * @brief DCLK时钟源选择
 */
#define DPI_CLK_HSB          (1<<PLL_SRC_SEL)
#define DPI_CLK_PAT          (2<<PLL_SRC_SEL)
#define DPI_CLK_PLL_160M     (3<<PLL_SRC_SEL)
#define DPI_CLK_PLL_192M     (4<<PLL_SRC_SEL)
#define DPI_CLK_PLL_480M     (5<<PLL_SRC_SEL)
#define DPI_CLK_SYSPLL_D1P0  (6<<PLL_SRC_SEL)
#define DPI_CLK_DDRPLL_D1P0  (7<<PLL_SRC_SEL)
#define DPI_CLK_USBPLL_D1P0  (8<<PLL_SRC_SEL)
#define DPI_CLK_MIPIPLL_D1P0 (9<<PLL_SRC_SEL)
#define DPI_CLK_WIFIPLL_D1P0 (10<<PLL_SRC_SEL)
#define DPI_CLK_STD_48M      (11<<PLL_SRC_SEL)

/**
 * @brief  时钟分频系统
 */
#define DIVA_1          	(0<<PLL_DIV1)
#define DIVA_3         		(1<<PLL_DIV1)
#define DIVA_5          	(2<<PLL_DIV1)
#define DIVA_7          	(3<<PLL_DIV1)

#define DIVB_1          	(0<<PLL_DIV2)
#define DIVB_2          	(1<<PLL_DIV2)
#define DIVB_4          	(2<<PLL_DIV2)
#define DIVB_8          	(3<<PLL_DIV2)

/**
 * @brief  颜色校正
 */

//二级命令									DMM_PARAM_GET	参数说明
#define ADJUST_EN       0x31//使能调试          不带参数
#define ADJUST_DIS      0x32//禁止调试			不带参数
#define GET_ALL         0x33//					struct color_correct
#define SET_ALL         0x34//					struct color_correct

#define SET_Y_GAIN      0x01//					u16
#define SET_U_GAIN      0x02//					u16
#define SET_V_GAIN      0x03//					u16
#define SET_Y_OFFS      0x04//					s16
#define SET_U_OFFS      0x05//					s16
#define SET_V_OFFS      0x06//					s16
#define SET_R_GAIN      0x07//					u16
#define SET_G_GAIN      0x08//					u16
#define SET_B_GAIN      0x09//					u16
#define SET_R_OFFS      0x0a//					s16
#define SET_G_OFFS      0x0b//					s16
#define SET_B_OFFS      0x0c//					s16
#define SET_R_COE0      0x0d//					u16
#define SET_R_COE1      0x0e//					s16
#define SET_R_COE2      0x0f//					s16
#define SET_G_COE0      0x10//					s16
#define SET_G_COE1      0x11//					u16
#define SET_G_COE2      0x12//					s16
#define SET_B_COE0      0x13//					s16
#define SET_B_COE1      0x14//					s16
#define SET_B_COE2      0x15//					u16
#define SET_R_GMA       0x16//					u16
#define SET_G_GMA       0x17//					u16
#define SET_B_GMA       0x18//					u16
#define SET_R_GMA_TBL   0x19//					256 bytes
#define SET_G_GMA_TBL   0x1a//					256 bytes
#define SET_B_GMA_TBL   0x1b//					256 bytes
#define SET_ISP_SCENE   0x1c//					s8
#define SET_CAMERA   	0x1d//					u8

/**
 * 硬件时序
 */
struct imd_dpi_timing {
    u16 hori_total; 		    /*水平时钟总数(Horizontal Line/HSD period)*/
    u16 hori_sync;		        /*水平同步时钟(HSYNC pulse width/HSD pulse width)*/
    u16 hori_back_porth;		/*水平起始时钟(HSYNC blanking/HSD back porth)*/
    u16 hori_pixel;	            /*水平像素(Horizontal display area/HSD display period)*/

    u16 vert_total;		        /*垂直同步总数(Vertical display area/VSD period time)*/
    u16 vert_sync;		        /*垂直同步时钟(VSYNC pulse width)*/
    u16 vert_back_porth_odd;	/*垂直起始时钟(VSYNC Blanking Odd field/VSD back porch Odd field)*/
    u16 vert_back_porth_even;	/*垂直起始时钟(隔行模式)(VSYNC Blanking Even field/VSD back porch Even field)*/
    u16 vert_pixel;	            /*垂直像素(Vertical display area)*/
};

//<输入数据格式>
#define LCD_IN_YUV422   0
#define LCD_IN_RGB565   1
#define LCD_IN_RGB888   2
#define LCD_IN_ARGB888  3

//<输出数据格式>
#define LCD_OUT_YUV422  0
#define LCD_OUT_RGB565  1
#define LCD_OUT_RGB666  2
#define LCD_OUT_RGB888  3

////显示相关////
struct imd_dev {
    struct basic_info info;

    enum LCD_ENDIAN data_out_endian;//输出数据大小端仅输出RGB565支持
    enum NCYCLE ncycle;             //每像素时钟数
    enum PHASE raw_odd_phase;	    //奇行相位
    enum PHASE raw_even_phase;      //偶行相位
    enum RAW_MODE raw_mode;         //RAW模式选择
    enum LCD_PORT data_width;       //数据位宽
    enum LCD_GROUP lcd_group;       //输出端口选择
    u8 avout_mode;				    //AVOUT制式(PAL/NTSC/TESTMODE)
    u8 show_buf_mode;               //显示buf模式

    u8 set_io_hd;                   //gpio_hd
    u8 dclk_set;                    //dclk使能以及极性配置
    u8 sync0_set;                   //DE/HSYNC/VSYNC
    u8 sync1_set;                   //DE/HSYNC/VSYNC
    u8 sync2_set;                   //DE/HSYNC/VSYNC
    u8 sync3_set;                   //Reversed
    u32 clk_cfg;                    //dpi clk时钟配置
    u32 dclk_cfg;                   //dclk时钟配置
    struct imd_dpi_timing timing;   //时序参数
};

struct imd_dev_info {
    struct basic_info *info;
    struct color_correct *adjust;
    u8 opened;
};


int dpi_open(struct imd_dev *dev);
int dpi_close(void);
void dpi_start_display(u32 arg);
void dpi_set_isr_cb(u32 arg);
void dpi_set_line_isr_cb(u32 arg);
void dpi_wait_fb_swap_finish(u32 arg);
void dpi_wait_line_finish(u32 arg);
#endif
