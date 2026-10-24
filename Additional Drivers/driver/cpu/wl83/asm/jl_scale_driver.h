#ifndef __JLSCALE_DRIVER_H__
#define __JLSCALE_DRIVER_H__


#define SCALE_PLATFORM_PC	0


#if SCALE_PLATFORM_PC
#include "jlscaler.h"
extern JLSCALER_TypeDef *JLSCALER;

void free_all_tasks();

void reset_all_regs();
#else
#include <stdint.h>

#include "system/includes.h"
#endif


#define SCALE_DONE_PAND()     	(scale_con & BIT(4))
#define SCALE_DONE_PAND_CLR() 	scale_con |= BIT(3)
#define SCALE_KICK_START() 		scale_con |= BIT(0)
#define SCALE_DOWN_ISR_EN() 	scale_con |= BIT(2)
#define SCALE_CON_CLR() 		scale_con = 0

enum JLSCALER_INTERP {
    JLSCALER_INTERP_NEAREST,
    JLSCALER_INTERP_BILINEAR,
};

enum SCARE_DATA_TYPE {
    YUV420P,//通道1
    YUV422P,//通道1
    RGB, //通道3//rgb888
    ARGB,   //通道4
};



void scale_run();
int scale_wait_done();
int scale_init();
int scale_free();

void set_scale_param(uint8_t *src_img_adr, uint32_t src_w, uint32_t src_h, uint32_t src_stride,
                     uint8_t *dst_img_adr, uint32_t dst_w, uint32_t dst_h, uint32_t dst_stride,
                     uint32_t channel, uint32_t x_step, uint32_t y_step,
                     int32_t x_offset, int32_t y_offset, uint32_t interp);


#endif

