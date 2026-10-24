#ifndef __ASM_RTC_H__
#define __ASM_RTC_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"
#include "system/sys_time.h"

/**
 * \name ioctl cmd for rtc
 * \{
 */
#define RTC_MAGIC                       'R'
#define IOCTL_GET_SYS_TIME              _IOR(RTC_MAGIC, 1, struct sys_time *)
#define IOCTL_SET_SYS_TIME              _IOW(RTC_MAGIC, 2, struct sys_time *)
#define IOCTL_GET_ALARM                 _IOR(RTC_MAGIC, 3, struct sys_time *)
#define IOCTL_SET_ALARM                 _IOW(RTC_MAGIC, 4, struct sys_time *)
#define IOCTL_SET_ALARM_ENABLE          _IOW(RTC_MAGIC, 5, bool)
#define IOCTL_SET_COUNTDOWN_ALARM       _IOW(RTC_MAGIC, 6, u32)
/* \} name */

/**
 * \name RTC时钟源选择
 * \{
 */
typedef enum {
    CLK_SEL_32K = 1,
    CLK_SEL_BTOSC_DIV1,
    CLK_SEL_BTOSC_DIV2,
    CLK_SEL_LRC,
} rtc_clk_sel_t;
/* \} name */

/**
 * \name PR口唤醒边沿
 * \{
 */
typedef enum {
    PR_RISING_EDGE  = BIT(0),
    PR_FALLING_EDGE = BIT(1),
    PR_BOTH_EDGE    = PR_RISING_EDGE | PR_FALLING_EDGE,
} pr_io_wkup_edge_t;
/* \} name */

/**
 * \name PR口唤醒滤波时间
 * \{
 */
typedef enum {
    PR_PORT_FLT_DISABLE,
    PR_PORT_FLT_16us,
    PR_PORT_FLT_128us,
    PR_PORT_FLT_1ms,
    PR_PORT_FLT_4ms,
} pr_io_wkup_flt_t;
/* \} name */

/**
 * \name 振荡器电流选项
 * \{
 */
typedef enum {
    X32XS_LEVEL_0,
    X32XS_LEVEL_1,
    X32XS_LEVEL_2,
    X32XS_LEVEL_3,
} x32xs_t;
/* \} name */

/**
 * \name rtc_platform_data
 * \{
 */
struct rtc_platform_data {
    struct sys_time default_sys_time;
    void (*alarm_wakeup_cb_in_irq)(void);
    void (*pr_wakeup_cb_in_irq)(int);
    rtc_clk_sel_t clk_sel;
    x32xs_t x32xs;
};
/* \} name */

/// \cond DO_NOT_DOCUMENT
#define RTC_PLATFORM_DATA_BEGIN(data) \
    static const struct rtc_platform_data data = {

#define RTC_PLATFORM_DATA_END()  \
};

extern const struct device_operations rtc_dev_ops;
/// \endcond

void set_rtc_platform_data(const struct rtc_platform_data *priv);

/**
 * @brief rtc开机初始化函数，必须先初始化完adc
 */
int rtc_early_init(void);

/**
 * @brief 软关机前保存rtc校准数据
 */
void poweroff_save_rtc_time(void);

/**
 * @brief 设置PR口的方向
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param dir 1: 输入       0: 输出
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_dir(int port, u8 dir);

/**
 * @brief 获取PR口输入电平
 * @param gpio 参考宏IO_PORT_PR_0X
 * @return 0: 低电平  1: 高电平
 */
int rtc_port_pr_read(int port);

/**
 * @brief 设置PR口的输出电平
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param on 1: 高电平       0: 低电平
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_out(int port, u8 on);

/**
 * @brief 设置PR口的强驱
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_hd(int port, u8 on);

/**
 * @brief 设置PR口的上拉电阻
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_pu(int port, u8 on);

/**
 * @brief 设置PR口的下拉电阻
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_pd(int port, u8 on);

/**
 * @brief 设置PR口的数字输入功能
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param on 1: 打开       0: 关闭
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_die(int port, u8 on);

/**
 * @brief 设置PR口的唤醒功能
 * @param gpio 参考宏IO_PORT_PR_0X
 * @param wkup_edge 唤醒边沿
 * @param flt_cfg 唤醒滤波时间
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_wkup_cfg(int port, pr_io_wkup_edge_t wkup_edge, pr_io_wkup_flt_t flt_cfg);

/**
 * @brief 关闭PR口的唤醒功能
 * @param gpio 参考宏IO_PORT_PR_0X
 * @return 0: 成功  非0: 失败
 */
int rtc_port_pr_wkup_disable(int port);

#endif
