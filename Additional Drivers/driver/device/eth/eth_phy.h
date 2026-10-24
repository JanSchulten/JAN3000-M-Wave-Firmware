#ifndef __ETH_PHY_H__
#define __ETH_PHY_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"
#include "eth/mdio_bus.h"

/**
 * \name ETH dev_ioctl funciton selest
 * \{
 */
#define ETH_MAGIC                               'E'
#define IOCTL_ETH_NETWORK_START                 _IO(ETH_MAGIC,1)                                        ///< 启动以太网网络
#define IOCTL_ETH_NETWORK_STOP                  _IO(ETH_MAGIC,2)                                        ///< 关闭以太网网络
#define IOCTL_ETH_SET_LAN_SETTING               _IOW(ETH_MAGIC,3,struct lan_setting *)                  ///< 设置以太网的局域网配置
#define IOCTL_ETH_EVENT_CB_FUNC                 _IOW(ETH_MAGIC,4,int (*)(void *, eth_event_t, void *))  ///< 设置以太网事件回调
#define IOCTL_ETH_GET_MAC_ADDR                  _IOR(ETH_MAGIC,5,u8 (*)[6])                             ///< 获取以太网网卡的mac地址
#define IOCTL_ETH_SET_MAC_ADDR                  _IOW(ETH_MAGIC,6,u8 (*)[6])                             ///< 设置以太网网卡的mac地址
/* \} name */

/**
 * \name eth phy state
 * \{
 */
typedef enum {
    PHY_CHECK,                  ///< 检测中
    PHY_RUNNING,                ///< 已连接
    PHY_UNLINK,                 ///< 已断开
} eth_phy_state_t;
/* \} name */

/**
 * \name eth phy link speed mode
 * \{
 */
typedef enum {
    PHY_FULLDUPLEX_100M,        ///< 全双工100M
    PHY_HALFDUPLEX_100M,        ///< 半双工100M
    PHY_FULLDUPLEX_10M,         ///< 全双工10M
    PHY_HALFDUPLEX_10M,         ///< 全双工10M
    PHY_AUTONEGOTINATION,       ///< 自动协商
} eth_link_speed_t;
/* \} name */

/**
 * \name eth phy mode
 * \{
 */
typedef enum {
    MII_MODE,
    RMII_MODE,
} eth_phy_mode_t;
/* \} name */

/**
 * \name eth event callback
 * \{
 */
typedef enum {
    ETH_EVENT_LINK_ON,          ///< 以太网连接打开
    ETH_EVENT_LINK_OFF,         ///< 以太网连接关闭
} eth_event_t;
/* \} name */

/**
 * \name eth platform data
 * \{
 */
struct eth_platform_data {
    const char *name;               ///< eth phy model name
    eth_link_speed_t speed;         ///< eth phy link speed mode
    eth_phy_mode_t mode;            ///< eth phy mode
    u8 hd_mode;                     ///< io high drive level
    u8 task_prio;                   ///< eth rx task priority
    char irq;                       ///< eth irq priority, -1 disabled irq
    u32 check_link_time;            ///< timer check link internal ms
    u8 *mac_addr;                   ///< eth mac address
    struct software_rmii rmii_bus;  ///< rmii_bus config
};
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define NET_PLATFORM_DATA_BEGIN(data) \
    static u8 eth_mac_##data[6]; \
	static const struct eth_platform_data data = {\
        .mac_addr = eth_mac_##data, \

#define NET_PLATFORM_DATA_END() \
};

struct eth_phy_device {
    const char *name;
    int (*init)(const struct eth_platform_data *);
    int (*is_connect)(const struct eth_platform_data *);
    void (*get_link_speed)(const struct eth_platform_data *);
    void (*set_link_speed)(const struct eth_platform_data *);
};

#define REGISTER_NET_PHY_DEVICE(dev) \
	static const struct eth_phy_device dev SEC_USED(.eth_phy_device)

extern const struct eth_phy_device eth_phy_device_begin[];
extern const struct eth_phy_device eth_phy_device_end[];

extern const struct device_operations eth_phy_dev_ops;
/// \endcond

/**
 * @brief 获取以太网网卡的mac地址
 *
 * @return 以太网网卡的mac地址
 */
const u8 *get_mac_address_for_eth(void);

/**
 * @brief 获取以太网设备的板级配置
 *
 * @return 以太网设备的板级配置
 */
const struct eth_platform_data *get_eth_platform_data(void);

#endif  /*PHY_STATE_H*/
