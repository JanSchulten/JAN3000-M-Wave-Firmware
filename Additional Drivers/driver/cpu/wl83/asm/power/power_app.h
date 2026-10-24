#ifndef __POWER_APP_H__
#define __POWER_APP_H__

/**
 * @brief 系统电源开机初始化
 */
void power_early_flowing(void);

/**
 * @brief 系统电源板级初始化
 */
void board_power_init(void);

#endif
