#ifndef __ASM_RDEC_H__
#define __ASM_RDEC_H__

#include "generic/typedef.h"

#define RDEC_MODULE_NUM 		1

typedef enum rdec_index {
    RDEC0,
    RDEC1,
    RDEC2,
} RDEC_INDEX;

typedef enum rdec_mode {
    RDEC_PHASE_1,
    RDEC_PHASE_2,
    RDEC_PHASE_4,
} RDEC_MODE;

struct rdec_device {
    RDEC_INDEX index;
    RDEC_MODE mode; //工作模式，0:1 phase  1:2 phase  2:4 phase
    int sin_port0; 	//采样信号端口0
    int sin_port1; 	//采样信号端口1
    u8 key_value0; 	//键值1
    u8 key_value1; 	//键值2
    u8 clk_spnd;    //采样时钟设置，取值范围0~15
};

struct rdec_platform_data {
    u8 num; 	//rdec数量
    const struct rdec_device *rdec;
};

int rdec_init(const struct rdec_platform_data *user_data);

int rdec_close(void);

char get_rdec_rdat(int index);

#endif

