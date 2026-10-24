#ifndef  __RMII_MII_IO_CONFIG_H__
#define  __RMII_MII_IO_CONFIG_H__

#include "generic/typedef.h"

struct software_rmii {
    u8 phy_addr; //0-32
    u8 hd_mode;
    int clk_pin;
    int dat_pin;
    int reset_pin;
};

void eth_mii_write(const struct software_rmii *bus, u16 regnum, u16 data);

u16 eth_mii_read(const struct software_rmii *rmii_bus, u16 regnum);

#endif  /*RMII_MII_IO_CONFIG_H*/
