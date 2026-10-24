#ifndef __DEVICE_SLIDEKEY_H__
#define __DEVICE_SLIDEKEY_H__

#include "generic/typedef.h"

struct slidekey_port {
    u8 io;
    u8 io_up_en;                //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u32 level;
    u32 ad_channel;
    int msg;
};

/**
 * \name slidekey_platform_data
 * \{
 */
struct slidekey_platform_data {
    u8 num;
    const struct slidekey_port *port;
};
/* \} name */

#endif


