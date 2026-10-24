#ifndef __DEVICE_SDMMC_H__
#define __DEVICE_SDMMC_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"
#include "generic/list.h"


/**
 * \name SDIO online detect mode
 * \{
 */
#define SD_CMD_DECT     0       ///< 命令检测模式
#define SD_CLK_DECT     1       ///< 时钟检测模式
#define SD_IO_DECT      2       ///< IO检测模式
/* \} name */

/**
 * \name SDIO class type
 * \{
 */
#define SD_CLASS_0      0
#define SD_CLASS_2      1
#define SD_CLASS_4      2
#define SD_CLASS_6      3
#define SD_CLASS_10     4
/* \} name */

/**
 * \name SDIO command cid info
 * \{
 */
typedef struct {
    char MID;                   ///< 制造商ID
    char OID[2];                ///< OEM/应用ID
    char PNM[5];                ///< 产品名称
    char PRV;                   ///< 产品版本
    u8  month;                  ///< 生产日期月
    u16  year;                  ///< 生产日期年
    u32  PSN;                   ///< 产品序列号
} sd_cmd_cid_t;
/* \} name */

sd_cmd_cid_t *get_sdcard_cid_info(u8 index);

#endif
