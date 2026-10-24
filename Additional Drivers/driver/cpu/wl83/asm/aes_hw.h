#ifndef __ASM_AES_H__
#define __ASM_AES_H__

#include "asm/WL83.h"
#include "asm/cpu.h"

/**
 * \name AES输入数据单位和密钥/IV配置操作类型
 * \{
 */
#define CRYP_DATAWIDTHUNIT_WORD  0x00000000U    /*!< By default, size unit is word */
#define CRYP_DATAWIDTHUNIT_BYTE  0x00000001U    /*!< Size unit is byte, but all input will be loaded in HW CRYPT IP by block of 4 words */

#define CRYP_HEADERWIDTHUNIT_WORD  0x00000000U  /*!< By default, header size unit is word */
#define CRYP_HEADERWIDTHUNIT_BYTE  0x00000001U  /*!< Size unit is byte, but all input will be loaded in HW CRYPT IP by block of 4 words */

#define CRYP_KEYIVCONFIG_ALWAYS  0x00000000U    /*!< Peripheral Key and IV configuration to do systematically */
#define CRYP_KEYIVCONFIG_ONCE    0x00000001U    /*!< Peripheral Key and IV configuration to do only once      */
/* \} name */

/** @defgroup CRYP_IS_CRYP_Definitions CRYP Private macros to check input parameters
  * @{
  */
#define IS_CRYP_ALGORITHM(ALGORITHM) (((ALGORITHM)  == CRYP_AES_ECB)   || \
		((ALGORITHM)  == CRYP_AES_CBC)   || \
		((ALGORITHM)  == CRYP_AES_CTR)   || \
		((ALGORITHM)  == CRYP_AES_CFB)   || \
		((ALGORITHM)  == CRYP_AES_OFB)   || \
		((ALGORITHM)  == CRYP_AES_GCM)   || \
		((ALGORITHM)  == CRYP_AES_CCM)   || \
		((ALGORITHM)  == CRYP_AES_XTS))

#define IS_CRYP_KEYSIZE(KEYSIZE)(((KEYSIZE) == CRYP_KEYSIZE_128B)   || \
		((KEYSIZE) == CRYP_KEYSIZE_192B)   || \
		((KEYSIZE) == CRYP_KEYSIZE_256B))

#define IS_CRYP_DATATYPE(DATATYPE)(((DATATYPE) == CRYP_DATATYPE_32B)   || \
		((DATATYPE) == CRYP_DATATYPE_1B) || \
		((DATATYPE) == CRYP_DATATYPE_8B) || \
		((DATATYPE) == CRYP_DATATYPE_16B))

#define IS_CRYP_INIT(CONFIG)(((CONFIG) == CRYP_KEYIVCONFIG_ALWAYS) || \
		((CONFIG) == CRYP_KEYIVCONFIG_ONCE))


///  \cond DO_NOT_DOCUMENT
//for bt aes
#define HARDWARE_METHOD 1

#define		STORE32H(x, y)                                                                     \
			do { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);   \
			(y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); } while(0)

#define		LOAD32H(x, y)                            \
			do { x = ((unsigned int)((y)[0] & 255)<<24) | \
			((unsigned int)((y)[1] & 255)<<16) | \
			((unsigned int)((y)[2] & 255)<<8)  | \
			((unsigned int)((y)[3] & 255)); } while(0)

#define		byte(x, n) ((unsigned char)((x) >> (8 * (n))))
#define		RORc(x, y) ( ((((unsigned int)(x)&0xFFFFFFFFUL)>>(unsigned int)((y)&31)) | ((unsigned int)(x)<<(unsigned int)(32-((y)&31)))) & 0xFFFFFFFFUL)

#define READ_BT32_YP(p, i)       (p[i]|(p[i+1]<<8)|(p[i+2]<<16)|(p[i+3]<<24))
#define WRITE_BT32_YP(p, i, v) \
       do { \
               p[i] = v; \
               p[i+1] = v>>8; \
               p[i+2] = v>>16; \
               p[i+3] = v>>24; \
       } while(0)

#if (!HARDWARE_METHOD)
extern const unsigned int TE0[256];
extern const unsigned int TE1[256];
extern const unsigned int TE2[256];
extern const unsigned int TE3[256];
extern const unsigned int Te4_0[] ;
extern const unsigned int Te4_1[] ;
extern const unsigned int Te4_2[] ;
extern const unsigned int Te4_3[] ;

#define Te0(x) TE0[x]
#define Te1(x) TE1[x]
#define Te2(x) TE2[x]
#define Te3(x) TE3[x]

extern const unsigned int rcon[];
#endif
/// \endcond

/**
 * @brief  CRYP_Algorithm_Mode enum definition
 */
typedef enum {
    CRYP_AES_ECB, /*!< AES_ECB_MODE */
    CRYP_AES_CBC, /*!< AES_CBC_MODE */
    CRYP_AES_CTR, /*!< AES_CTR_MODE */
    CRYP_AES_CFB, /*!< AES_CFB_MODE */
    CRYP_AES_OFB, /*!< AES_OFB_MODE */
    CRYP_AES_GCM, /*!< AES_GCM_MODE */
    CRYP_AES_CCM, /*!< AES_CCM_MODE */
    CRYP_AES_XTS  /*!< AES_XTS_MODE */
} AES_ACC_MODE;

/**
 * @brief  CRYP_KEY_SIZE enum definition
 */
typedef enum {
    CRYP_KEYSIZE_128B, /*!< KEY_SIZE_128BIT */
    CRYP_KEYSIZE_192B, /*!< KEY_SIZE_192BIT */
    CRYP_KEYSIZE_256B, /*!< KEY_SIZE_256BIT */
} AES_ACC_KEYSIZE;

/**
 * @brief  CRYP_DATA_MODE enum definition
 */
typedef enum {
    AES_CPU_MODE, /*!< DATA_MOED_CPU */
    AES_DMA_MODE  /*!< DATA_MODE_DMA */
} MST_MODE;

/**
 * @brief HAL Status enum definition
 */
typedef enum {
    HAL_OK       = 0x00, /*!< succ */
    HAL_ERROR    = 0x01, /*!< error */
    HAL_BUSY     = 0x02, /*!< no use */
    HAL_TIMEOUT  = 0x03  /*!< no use */
} HAL_StatusTypeDef;

/**
 * @brief Data processing type enum definition
 */
typedef enum {
    CRYP_DATATYPE_32B,  /*!< CRYP_NO_SWAP */
    CRYP_DATATYPE_1B,   /*!< CRYP_BIT_SWAP */
    CRYP_DATATYPE_8B,   /*!< CRYP_BYTE_SWAP */
    CRYP_DATATYPE_16B   /*!< CRYP_HALFWORD_SWAP */
} AES_DATATYPE;

/*
 * @brief CRYP Init Structure definition
 */
typedef struct {
    uint8_t alg_rundir : 1;              /*!< CRYP GCM/CCM/XTS algorithm rundir */
    uint8_t Xts_Ct_Steal_En : 1;         /*!< CRYP XTS ct_steal en */
    uint8_t Xts_Bp_En : 1;               /*!< CRYP XTS_BP ENABLE xts模乘旁路使能 */
    uint8_t Gcm_Iv_Not96_En : 1;         /*!< CRYP GCM USE IV != 96BIT ENABLE */
    uint8_t Aes_Int_EN : 1;              /*!< AES INTERRUPT ENABLE */
    uint32_t Gcm_Ivlen;                  /*!< CRYP GCM USE IV != 96BIT, ivlen, size unit byte */
    uint32_t *gcm_mbed_iv;               /*!< CRYP GCM USE IV != 96BIT, iv */
    uint32_t gcm_tmp_iv[4];              /*!< CRYP GCM USE IV != 96BIT, save tmp iv for calculate tag */
    uint32_t *pKey2;                     /*!< CRYP XTS twk_ph 调整阶段使用的KEY */
    uint32_t *Tweak;                     /*!< CRYP XTS twk_ph 调整阶段输入的tweak */
    uint32_t DataType;                   /*!< no swap(32-bit data), halfword swap(16-bit data), byte swap(8-bit data) or bit swap(1-bit data).this parameter can be a value of @ref CRYP_Data_Type */
    uint32_t KeySize;                    /*!< Used only in AES mode : 128, 192 or 256 bit key length in CRYP1. This parameter can be a value of @ref CRYP_Key_Size */
    uint32_t Algorithm;                  /*!<  DES/ TDES Algorithm ECB/CBC AES Algorithm ECB/CBC/CTR/GCM or CCM This parameter can be a value of @ref CRYP_Algorithm_Mode */
    uint32_t HeaderSize;                 /*!< The size of header buffer */
    uint32_t DataWidthUnit;              /*!< Payload data Width Unit, this parameter can be value of @ref CRYP_Data_Width_Unit*/
    uint32_t HeaderWidthUnit;            /*!< Header Width Unit, this parameter can be value of @ref CRYP_Header_Width_Unit*/
    uint32_t KeyIVConfigSkip;            /*!< CRYP peripheral Key and IV configuration skip, to configure Key and Initialization Vector only once and to skip configuration for consecutive processing. This parameter can be a value of @ref CRYP_Configuration_Skip */
    uint32_t *pKey;                      /*!< The key used for encryption/decryption */
    uint32_t *pInitVect;                 /*!< The initialization vector used also as initialization counter in CTR mode */
    uint32_t *Header;                    /*!< used only in AES GCM and CCM Algorithm for authentication, GCM : also known as Additional Authentication Data CCM : named B1 composed of the associated data length and Associated Data. */
    uint32_t *B0;                        /*!< B0 is first authentication block used only  in AES CCM mode */
} CRYP_ConfigTypeDef;


/**
 * @brief  CRYP handle Structure definition
 */
typedef struct {
    CRYP_ConfigTypeDef Init;      /*!< CRYP required parameters */
    uint16_t CrypHeaderCount;     /*!< Counter of header data */
    uint16_t CrypInCount;         /*!< Counter of input data */
    uint16_t CrypOutCount;        /*!< Counter of output data */
    uint16_t Size;                /*!< length of input data in word or in byte, according to DataWidthUnit */
    uint32_t Phase;               /*!< CRYP peripheral phase */
    uint32_t ErrorCode;           /*!< CRYP peripheral error code */
    uint32_t Version;             /*!< CRYP1 IP version*/
    uint32_t KeyIVConfig;         /*!< CRYP peripheral Key and IV configuration flag, used when configuration can be skipped */
    uint32_t SizesSum;            /*!< Sum of successive payloads lengths (in bytes), stored for a single signature computation after several messages processing */
    uint32_t *pCrypInBuffPtr;     /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */
    uint32_t *pCrypOutBuffPtr;    /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */
    JL_AES_ACC_TypeDef *Instance; /*!< CRYP registers base address */
    uint32_t gcm_gm_save[4];      /*!< CRYP GCM save tmp gm for tag_calculate */
    uint32_t gcm_iv_save[4];      /*!< CRYP GCM save tmp iv for tag_calculate */
    uint32_t ccm_gm_save[4];      /*!< CRYP CCM save tmp gm for tag_calculate */
} CRYP_HandleTypeDef;


/**
 * @brief AES配置参数初始化函数
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_Init(CRYP_HandleTypeDef *hcryp);

/**
 * @brief AES清空结构体配置参数函数
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_DeInit(CRYP_HandleTypeDef *hcryp);

/**
 * @brief 获取hcryp结构体配置参数到pConf参数配置结构体指针中
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param pConf: CRYP_ConfigTypeDef参数配置结构体指针
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_GetConfig(CRYP_HandleTypeDef *hcryp, CRYP_ConfigTypeDef *pConf);

/**
 * @brief 将pConf参数配置设置到hcryp句柄中
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param pConf: CRYP_ConfigTypeDef参数配置结构体指针
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_SetConfig(CRYP_HandleTypeDef *hcryp, CRYP_ConfigTypeDef *pConf);

/**
 * @brief AES加密模式函数(CPU模式处理数据)
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param Input: 明文输入数据,需要32位对齐
 * @param Size: 根据配置参数的DataWidthUnit决定单位的输入数据的长度
 * @param Output: 密文输出数据,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_Encrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output, uint32_t Timeout);

/**
 * @brief AES解密模式函数(CPU模式处理数据)
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param Input: 密文输入数据,需要32位对齐
 * @param Size: 根据配置参数的DataWidthUnit决定单位的输入数据的长度
 * @param Output: 明文输出数据,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_Decrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output, uint32_t Timeout);

/**
 * @brief AES加密模式函数(DMA模式处理数据)
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param Input: 明文输入数据,需要32位对齐
 * @param Size: 根据配置参数的DataWidthUnit决定单位的输入数据的长度
 * @param Output: 密文输出数据,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);

/**
 * @brief AES解密模式函数(DMA模式处理数据)
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param Input: 密文输入数据,需要32位对齐
 * @param Size: 根据配置参数的DataWidthUnit决定单位的输入数据的长度
 * @param Output: 明文输出数据,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYP_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);

/**
 * @brief AES GCM模式获取TAG值
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param AuthTag: tag值, 16bytes,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout);

/**
 * @brief AES CCM模式获取TAG值
 *
 * @param hcryp: CRYP_HandleTypeDef句柄结构体指针
 * @param AuthTag: tag值, 默认16bytes，需要不同长度需获取后自行截取,需要32位对齐
 * @param Timeout: no use
 *
 * @return 0:成功 非0:失败
*/
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout);

///  \cond DO_NOT_DOCUMENT
//for bt aes
#if HARDWARE_METHOD
int aes128_start_enc(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16]);
int aes128_start_dec(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16]);
#endif
/// \endcond

#endif
