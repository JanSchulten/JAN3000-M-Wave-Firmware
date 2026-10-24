#ifndef __POWER_WAKEUP_H__
#define __POWER_WAKEUP_H__

/**
 * \name 唤醒原因
 * \{
 */
typedef enum {
    PWR_WK_REASON_PLUSE_CNT_OVERFLOW,           ///< pcnt唤醒复位
    PWR_WK_REASON_P11,                          ///< P11唤醒复位
    PWR_WK_REASON_LPCTMU,                       ///< 触摸唤醒复位
    PWR_WK_REASON_PORT_EDGE,                    ///< 数字io输入边沿唤醒复位
    PWR_WK_REASON_ANA_EDGE,                     ///< 模拟io输入边沿唤醒复位
    PWR_WK_REASON_VDDIO_LVD,                    ///< vddio lvd唤醒复位
    PWR_WK_REASON_WDT,                          ///< vddio lvd看门狗唤醒复位

    PWR_WK_REASON_FALLING_EDGE_0,               ///< p33 index0  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_1,               ///< p33 index1  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_2,               ///< p33 index2  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_3,               ///< p33 index3  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_4,               ///< p33 index4  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_5,               ///< p33 index5  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_6,               ///< p33 index6  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_7,               ///< p33 index7  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_8,               ///< p33 index8  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_9,               ///< p33 index9  io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_10,              ///< p33 index10 io下降沿唤醒复位
    PWR_WK_REASON_FALLING_EDGE_11,              ///< p33 index11 io下降沿唤醒复位

    PWR_WK_REASON_RISING_EDGE_0,                ///< p33 index0  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_1,                ///< p33 index1  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_2,                ///< p33 index2  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_3,                ///< p33 index3  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_4,                ///< p33 index4  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_5,                ///< p33 index5  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_6,                ///< p33 index6  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_7,                ///< p33 index7  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_8,                ///< p33 index8  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_9,                ///< p33 index9  io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_10,               ///< p33 index10 io上升沿唤醒复位
    PWR_WK_REASON_RISING_EDGE_11,               ///< p33 index11 io上升沿唤醒复位

    PWR_ANA_WK_REASON_FALLING_EDGE_LDOIN,       ///< LDO5V下降沿唤醒复位
    PWR_ANA_WK_REASON_FALLING_EDGE_VBATCH,      ///< VBATCH下降沿唤醒复位
    PWR_ANA_WK_REASON_FALLINIG_EDGE_VBATDT,     ///< vbatdt下降沿唤醒复位
    PWR_ANA_WK_REASON_FALLINIG_EDGE_CHARGEFULL, ///< charge full下降沿唤醒复位

    PWR_ANA_WK_REASON_RISING_EDGE_LDOIN,        ///< LDO5V上降沿唤醒复位
    PWR_ANA_WK_REASON_RISING_EDGE_VBATCH,       ///< VBATCH上降沿唤醒复位
    PWR_ANA_WK_REASON_RISING_EDGE_VBATDT,       ///< vbatdet上升沿唤醒复位
    PWR_ANA_WK_REASON_RISING_EDGE_CHARGEFULL,   ///< chargefull上升沿唤醒复位

    PINR_PND1_WKUP,

    PWR_RTC_WK_REASON_ALM,                      ///< RTC闹钟唤醒复位
    PWR_RTC_WK_REASON_256HZ,                    ///< RTC 256Hz时基唤醒复位
    PWR_RTC_WK_REASON_64HZ,                     ///< RTC 64Hz时基唤醒复位
    PWR_RTC_WK_REASON_2HZ,                      ///< RTC 2Hz时基唤醒复位
    PWR_RTC_WK_REASON_1HZ,                      ///< RTC 1Hz时基唤醒复位
    PWR_RTC_WK_REASON_PORT,                     ///< RTC PR口边沿唤醒

    PWR_WKUP_REASON_RESERVE = 63,
} wakeup_reason_t;
/* \} name */

#define MAX_WAKEUP_PORT     12  //最大同时支持数字io输入个数
#define MAX_WAKEUP_ANA_PORT 0   //最大同时支持模拟io输入个数

/**
 * \name 唤醒边沿
 * \{
 */
typedef enum {
    RISING_EDGE = 1,
    FALLING_EDGE,
    BOTH_EDGE,
} p33_io_wkup_edge_t;
/* \} name */

/**
 * \name 唤醒边沿滤波时间
 * \{
 */
typedef enum {
    PORT_FLT_DISABLE,
    PORT_FLT_16us,
    PORT_FLT_128us,
    PORT_FLT_1ms,
    PORT_FLT_4ms,
} p33_io_wkup_flt_t;
/* \} name */

/**
 * \name 唤醒IO配置
 * \{
 */
typedef struct {
    u32 gpio;                                   ///< 唤醒io
    u32 pullup_down_enable;                     ///< 内部上下拉是否使能
    p33_io_wkup_flt_t filter;                   ///< 滤波参数
    p33_io_wkup_edge_t edge;                    ///< 唤醒边沿条件
    void (*callback)(p33_io_wkup_edge_t edge);  ///< 唤醒回调
} p33_io_wakeup_config_t;
/* \} name */

/**
* @brief  判断是否为指定唤醒源
* @param  index: 唤醒源
* @retval 1: 是     0: 否
*/
bool is_wakeup_source(wakeup_reason_t index);

/*
 *@brief  判断是否为指定gpio唤醒
 *@param  gpio序号
 *@retval 0: 不是该唤醒
 *@retval RISING_EDGE: 上升沿唤醒
 *@retval RISING_EDGE: 下降沿沿唤醒
 *@retval BOTH_EDGE: 双边沿沿唤醒
 */
u32 is_gpio_wakeup(u32 gpio);

/*
 *@brief  初始化一个唤醒口
 *@note   优先和长按复位复用
 *@note   所有通道都用完之后，若长按复位没有开启，则占用长按复位通道
 */
void p33_io_wakeup_port_init(const p33_io_wakeup_config_t *config);

/**
 * @brief 关闭IO唤醒功能
 */
void p33_io_wakeup_port_uninit(u32 gpio);

/**
 * @brief 设置IO唤醒的滤波时间
 */
void p33_io_wakeup_filter(u32 gpio, p33_io_wkup_flt_t filter);

/**
 * @brief 获取gpio的wakeup pending状态
 */
int p33_io_wakeup_get_pending(u32 gpio);

/**
 * @brief 使能IO唤醒功能
 */
void __p33_io_wakeup_enable(u32 gpio, u32 enable);

/**
 * @brief 使能IO唤醒功能(加了POWER_CRITICAL)
 */
void p33_io_wakeup_enable(u32 gpio, u32 enable);

/**
 * @brief 设置IO唤醒边沿
 */
void p33_io_wakeup_edge(u32 gpio, p33_io_wkup_edge_t edge);

/**
 * @brief 设置IO唤醒回调
 */
void p33_io_wakeup_set_callback(u32 gpio, void (*callback)(p33_io_wkup_edge_t edge));

/**
 * @brief IO是否设置了唤醒
 * @retval index 设置成功的index
 * @retval -1 没有设置唤醒
 */
int p33_get_io_wakeup_port_used(u32 gpio);

/**
 * @brief 配置soff流程中wakeup io在active是否复位
 *
 * @Params gpio
 * @Params enable 0-disable 1-enable
 *
 * @return  0-success -1-fail
 */
int p33_io_wakeup_active_soff_rst_config(u32 gpio, u8 enable);

/**
 * @brief 获取wake io的active状态(FALLING_EDGE的active状态为低电平,
 *        RISING_EDGE的active状态为高电平)
 * @return wake io 是否在active状态的状态
 */
u32 p33_get_io_wakeup_active_status(void);

/**
 * @brief 查询io wakeup active时是否需要复位
 *
 * @return 0-无需复位 1-需要复位
 */
u8 is_wakeup_io_active_need_rst(void);

#endif
