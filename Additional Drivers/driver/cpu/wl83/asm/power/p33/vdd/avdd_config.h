#ifndef __AVDD_CONFIG_H__
#define __AVDD_CONFIG_H__

/**
 * @brief 设置PV口的方向
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param dir 1: 输入       0: 输出
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_dir(int port, u8 dir);

/**
 * @brief 获取PV口输入电平
 * @param gpio 参考宏IO_PORT_PV_0X
 * @return 0: 低电平  1: 高电平
 */
int avdd_port_pv_read(int port);

/**
 * @brief 设置PV口的输出电平
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param on 1: 高电平       0: 低电平
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_out(int port, u8 on);

/**
 * @brief 设置PV口的强驱
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_hd(int port, u8 on);

/**
 * @brief 设置PV口的上拉电阻
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_pu(int port, u8 on);

/**
 * @brief 设置PV口的下拉电阻
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_pd(int port, u8 on);

/**
 * @brief 设置PV口的数字输入功能
 * @param gpio 参考宏IO_PORT_PV_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int avdd_port_pv_die(int port, u8 on);

/**
 * @brief AVDD18使能。使能流程需要分两步，第一步和第二步之间需延时100us
 * @Params level 电压档位 0-1.8V、1-1.5V、2-1.6V、3-1.7V、4-1.8V、
 *                                5-1.9V、6-2.0V、7-2.1V、8-2.2V
 * @Params step 1:第一步 2:第二步
 */
void avdd18_enable(u8 level, u8 step);

/**
 * @brief AVDD28使能。使能流程需要分两步，第一步和第二步之间需延时150us
 * @Params level 电压档位 0-2.8V、1-2.5V、2-2.6V、3-2.7V、4-2.8V、
 *                                5-2.9V、6-3.0V、7-3.1V、8-3.2V
 * @Params step 1:第一步 2:第二步
 */
void avdd28_enable(u8 level, u8 step);

/**
 * @brief 关闭AVDD18
 */
void avdd18_disable(void);

/**
 * @brief 关闭AVDD28
 */
void avdd28_disable(void);

#endif
