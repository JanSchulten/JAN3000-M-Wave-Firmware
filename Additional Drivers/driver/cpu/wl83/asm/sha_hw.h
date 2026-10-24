#ifndef __ASM_SHA_H__
#define __ASM_SHA_H__

#include "generic/typedef.h"

/**@brief   处理SHA-1计算的单个数据块
* @param is_start  是否是第一次启动。0:yes  非0:no
* @param state[5]  摘要状态
* @param data[64]  数据块
* @warning  该接口仅供特殊功能使用。
*/
void jl_sha1_process(u32 *is_start, u32 state[5], const u8 data[64]);


/**@brief  SHA-1加密算法(硬件)
* @param input       输入数据的地址
* @param ilen        输入数据的长度
* @param output[20]  计算结果(摘要)
*/
void jl_sha1(const u8 *input, size_t ilen, u8 output[20]);


/**@brief   处理SHA-256计算的单个数据块
* @param is_start  是否是第一次启动。0:yes  非0:no
* @param state[8]  摘要状态
* @param data[64]  数据块
* @warning  该接口仅供特殊功能使用。
*/
void jl_sha256_process(u32 *is_start, u32 state[8], const u8 data[64]);


/**@brief  SHA-256加密算法(硬件)
* @param input       输入数据的地址
* @param ilen        输入数据的长度
* @param output[32]  计算结果(摘要)
* @param is224       决定是否使用SHA-224计算。0:使用SHA-256, 非0:使用SHA-224
*/
void jl_sha256(const u8 *input, size_t ilen, u8 output[32], int is224);


#endif

