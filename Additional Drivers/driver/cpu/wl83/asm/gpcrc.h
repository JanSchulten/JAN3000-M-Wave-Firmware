#ifndef __ASM_GPCRC_H__
#define __ASM_GPCRC_H__

#include "generic/typedef.h"

/**
 * \name GPCRC revin config parameters
 * \{
 */
typedef enum {
    GPCRC_REV_IN,         ///< 输入数据按位反转
    GPCRC_NO_REV_IN,      ///< 输入数据不按位反转
} gpcrc_rin_t;
/* \} name */


/**
 * \name GPCRC revout config parameters
 * \{
 */
typedef enum {
    GPCRC_NO_REV_OUT,     ///< 输出数据不按位反转
    GPCRC_REV_OUT,        ///< 输出数据按位反转
} gpcrc_rout_t;
/* \} name */


/**
 * \name GPCRC poly size config parameters
 * \{
 */
typedef enum {
    GPCRC_POL32,          ///< 32位多项式
    GPCRC_POL16,          ///< 16位多项式
    GPCRC_POL8,           ///< 8位多项式
    GPCRC_POL7,           ///< 7位多项式
} gpcrc_pol_size_t;
/* \} name */


/**
 * \name GPCRC data size config parameters
 * \{
 */
typedef enum {
    GPCRC_DAT32,          ///< 输入数据格式-32位
    GPCRC_DAT16,          ///< 输入数据格式-16位
    GPCRC_DAT8,           ///< 输入数据格式-8位
} gpcrc_dat_size_t;
/* \} name */


/**
 * \name GPCRC swap config parameters
 * \{
 */
typedef enum {
    GPCRC_NO_SWP_IN,      ///< 输入数据端序不翻转
    GPCRC_HWORD_SWP_IN,   ///< 输入数据端序按半字翻转
    GPCRC_WORD_SWP_IN,    ///< 输入数据端序按字翻转
} gpcrc_swp_t;
/* \} name */


/**
 * \name GPCRC algorithm config item
 * \{
 */
typedef struct {
    char *crc_name;              ///< 算法名
    u32 init;                    ///< 初始值
    u32 pol;                     ///< 多项式
    gpcrc_rin_t rev_in;          ///< 输入数据是否按位反转
    gpcrc_rout_t rev_out;        ///< 输出数据是否按位反转
    gpcrc_pol_size_t pol_size;   ///< 多项式位宽
    u32 xorout;                  ///< 结果异或值
} gpcrc_algorithm_cfg_t;
/* \} name */


/**@brief     使用指定算法计算u8类型buf数据的crc值
* @param cfg  算法配置
* @param data buf地址(buf空间4字节对齐)
* @param cfg  buf长度
* @return     crc计算结果
* @warning    同样计算u8类型buf的crc时，该函数会比gpcrc_calc_buf_crc_flex()效率更高。
*/
u32 gpcrc_calc_u8_buf(gpcrc_algorithm_cfg_t *cfg, u8 *data, u32 len);


/**@brief     使用指定算法计算指定数据格式的buf数据的crc值
* @param cfg  算法配置
* @param dat_size buf数据宽度
* @param data buf地址
* @param cfg  buf长度
* @return     crc计算结果
*/
u32 gpcrc_calc_buf_crc_flex(gpcrc_algorithm_cfg_t *cfg, gpcrc_dat_size_t dat_size, void *data, u32 len);


#endif


