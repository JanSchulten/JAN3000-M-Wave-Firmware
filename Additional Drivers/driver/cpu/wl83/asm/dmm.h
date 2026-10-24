#ifndef __DMM_H__
#define __DMM_H__

#include "typedef.h"
#include "asm/cpu.h"
#include "generic/ioctl.h"
#include "asm/lcd_common.h"


typedef struct {	//ccm //最后给到寄存器的值
    int matrix[12];
    //matrix[0];  //(-128.00-127.00, 步进0.01)
    //matrix[1];  //(-128.00-127.00, 步进0.01)
    //matrix[2];  //(-128.00-127.00, 步进0.01)
    //matrix[3];  //(-8192.00-8191.00, 步进0.01)
    //matrix[4];  //(-128.00-127.00, 步进0.01)
    //matrix[5];  //(-128.00-127.00, 步进0.01)
    //matrix[6];  //(-128.00-127.00, 步进0.01)
    //matrix[7];  //(-8192.00-8191.00, 步进0.01)
    //matrix[8];  //(-128.00-127.00, 步进0.01)
    //matrix[9];  //(-128.00-127.00, 步进0.01)
    //matrix[10]; //(-128.00-127.00, 步进0.01)
    //matrix[11]; //(-8192.00-8191.00, 步进0.01)

} dmm_ccm_param_t;

typedef struct {
    float matrix[12];
} dmm_ccmf_param_t;


typedef struct {	//gamma
    uint8_t lut[256];
} dmm_gamma_param_t;

typedef struct {	//blend param
    float r_gain; 		//r分量增益 (0.00-2.00, 步进0.01)
    float g_gain; 		//g分量增益 (0.00-2.00, 步进0.01)
    float b_gain; 		//b分量增益 (0.00-2.00, 步进0.01)
    float bright_gain;	//亮度 (0.00-4.00, 步进0.01)
    float contrast_gain;//对比度 (0.00-4.00, 步进0.01)
    float mean;			//对比度均值 (0.0-255.00, 步进1)
    float saturation_gain;//饱和度 (0.00-4.00, 步进0.01)
    float angle;		//色相 (0.00-360.00, 步进1)
    int mode;  			//(0,1,2,3 , 步进1)
} dmm_blend_param_t;

typedef struct {//ccm+gamma+rbs+blend
    dmm_gamma_param_t rgb_gamma_param; // rgb gamma参数
    dmm_ccmf_param_t ccmf_param;  	   // ccm矩阵
    float rbs_con; 				       // in_swp:r分量和b分量需要对调
    dmm_blend_param_t blend_param;     // blend
} dmm_param_t;


//debug
typedef enum {
    DMM_GET_ERR, 			   //模块运行出错:当FIFO Empty时，如果dpi拿数
    DMM_INTERRUPT_BLOCKED_ERR, //fifo_busy 中断被挡 引起报错的原因：1.有优先级比dmm高 2.外部关中断 3.内存带宽被占据，导致DMM延时进中断
    DMM_WAITING_VSYNC,
    ERROR_TYPE_COUNT//报错类型的个数，用于定义数组大小
} ErrorType;

typedef struct { //报错信息结构体
    ErrorType error_type;
    u32 error_type_count[ERROR_TYPE_COUNT];
    int max_frame_period_us;
    int max_wait_vsync_period_us;
} dmm_error_info;

void dmm_log_error_statistics(void);
void dmm_get_error_statistics(dmm_error_info *info);
void dmm_debug_max_Vsync(void);

//dmm 控制接口
void dmm_init(void);
void dmm_deinit(void);
void dmm_config(struct basic_info *dmm_info);
void dmm_buf_sem_post(void);
int dmm_buf_sem_query(void);
int dmm_buf_sem_pend(void);
void dmm_line_sem_post(void);
int dmm_line_sem_query(void);
int dmm_line_sem_pend(void);
void dmm_set_isr_cb(u32 arg);
void dmm_set_line_isr_cb(u32 arg);
void dmm_write_sem_post(void);
void dmm_line_pend_init(int line);
u32 dmm_get_frame_period_us(void);
u8 dmm_get_line_period_us(void);

//读写设置接口
void dmm_param_set(dmm_param_t *p);
void dmm_param_write(void);
void dmm_param_get(dmm_param_t *p);
uint8_t dmm_get_write_flag(void);
void dmm_clr_write_flag(void);
void set_dmm_default_param(void);
void set_dmm_rgb_gamma_param(dmm_gamma_param_t *p);
void set_a_gamma_param(dmm_gamma_param_t *gamma_test);//test
void set_a_gamma_default_param(dmm_gamma_param_t *gamma_test);//test
void dmm_default_blend_param_get(dmm_blend_param_t *p);//获取默认blend参数
/*
以下是调效果的接口
*/
void set_rgb_gain_ccmf_param(float r_gain, float g_gain, float b_gain);
void set_brightenss_ccmf_param(float brightenss_gain);
void set_contrast_ccmf_param(float contrast_gain, float mean);
void set_saturation_ccmf_param(float saturation_gain);
void set_hue_ccmf_param(float angle);
/*
set_blend_ccm_param
设置多种图像效果的接口
r_gain，g_gain，b_gain：R,G,B三通道增益
bright_gain，contrast_gain，saturation_gain：亮度，对比度，饱和度增益
mean：对比度均值，不会设置就设置为128
angle：色相角度
mode：代表不同的顺序
mode=0的顺序为：RGB三通道增益，亮度，对比度，饱和度，色相
mode=1的顺序为：RGB三通道增益，饱和度，对比度，亮度，色相
mode=2的顺序为：RGB三通道增益，色相，亮度，对比度，饱和度
mode=3的顺序为：RGB三通道增益，色相，饱和度，对比度，亮111度
*/
void set_blend_ccmf_param(float r_gain, float g_gain, float b_gain, float bright_gain, float contrast_gain, float saturation_gain, float mean, float angle, int mode);

//普通模式和专业模式叠加接口
void set_final_ccm_param(float r_gain, float g_gain, float b_gain, float bright_gain, float contrast_gain, float saturation_gain, float mean, float angle, int mode, dmm_ccmf_param_t *special_ccmf_param);

#endif
