#ifndef __DEVICE_IRKEY_H__
#define __DEVICE_IRKEY_H__

#include "generic/typedef.h"

/**
 * \name irkey_platform_data
 * \{
 */
struct irkey_platform_data {
    u8 timer_index;     ///<  占用的硬件定时器序号 0~5
    int port;           ///<  输入检测IO
};
/* \} name */

#endif

