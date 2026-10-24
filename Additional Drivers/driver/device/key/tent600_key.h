#ifndef __DEVICE_TENT600_KEY_H__
#define __DEVICE_TENT600_KEY_H__

#include "generic/typedef.h"

/**
 * \name tent600_platform_data
 * \{
 */
struct tent600_key_platform_data {
    u8 tent600_key_pin;
    u8 extern_up_en;
    u32 ad_channel;
};
/* \} name */

#endif
