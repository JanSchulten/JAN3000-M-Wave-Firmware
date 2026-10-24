#ifndef __ASM_PLCNT_H__
#define __ASM_PLCNT_H__

#include "generic/typedef.h"

/* =========== pclcnt API ============= */
//plcnt 初始化
int plcnt_init(void *_data);

//获取plcnt按键状态
u8 get_plcnt_value(void);

#endif

