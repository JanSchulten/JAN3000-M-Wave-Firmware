#ifndef __SFC_NORFLASH_API_H__
#define __SFC_NORFLASH_API_H__

#include "generic/typedef.h"
#include "device/device.h"

typedef enum sfc_read_mode {
    FAST_READ_OUTPUT_MODE = 0,
    FAST_READ_IO_MODE,
    FAST_READ_IO_CONTINUOUS_READ_MODE,
} SFC_READ_MODE;

typedef enum sfc_clk_src {
    SFC_CLK_SRC_HSB_CLK,
    SFC_CLK_SRC_RC_16M,
    SFC_CLK_SRC_PLL_160M,
    SFC_CLK_SRC_PLL_120M,
    SFC_CLK_SRC_PLL_96M,
    SFC_CLK_SRC_STD_48M,
    SFC_CLK_SRC_STD_24M,
    SFC_CLK_SRC_PAT_CLK = 7,
} SFC_CLK_SRC;

struct sfc_spi_platform_data {
    SFC_READ_MODE sfc_read_mode;        //工作模式
    u8 sfc_data_width;                  //数据宽带
    u8 sfc_encry;                       //是否加密
    u8 sfc_clk_div;                     //sfc时钟分频系数
    u8 spi_clk_div;                     //spi时钟分频系数
    u8 sfc_clk_sel;                     //时钟源选择
    u32 unencry_start_addr;             //不加密起始地址
    u32 unencry_size;                   //不加密大小
};

#define SFC_SPI_PLATFORM_DATA_BEGIN(data) \
		static const struct sfc_spi_platform_data data = {

#define SFC_SPI_PLATFORM_DATA_END()  \
};

struct sfc_no_enc_wr {
    u8 *buf;
    u32 addr;
    u32 len;
};

struct norflash_sfc_dev_platform_data {
    const struct sfc_spi_platform_data *sfc_spi_pdata;
    const char *path;       //分区路径,内置flash 需要在ini分配好空间
    u32 start_addr;         //分区起始地址
    u32 size;               //分区大小，若只有1个分区，则这个参数可以忽略
};

#define NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(data) \
		static const struct norflash_sfc_dev_platform_data data = {

#define NORFLASH_SFC_DEV_PLATFORM_DATA_END()  \
};

extern const struct device_operations norflash_sfc_dev_ops;
extern const struct device_operations norflash_sfc_fs_dev_ops;


//sfc1 API:
int sfc_spi_init(const struct sfc_spi_platform_data *sfc_spi_data);
int sfc_spi_open(void);
int sfc_spi_close(void);
u32 sfc_spi_read_id(void);
int sfc_spi_read(u32 addr, void *buf, u32 len);
int sfc_spi_write_pages(u32 addr, void *buf, u32 len);
int sfc_spi_eraser(u32 cmd, u32 addr);
int sfc_spi_read_no_enc(u32 addr, void *buf, u32 len);
void sfc_spi_write_enc_set(u8 enc);
void sfc_spi_reinit_4byte_mode(void);

int norflash_init(const struct dev_node *node, void *arg);
int norflash_open(const char *name, struct device **device, void *arg);
int norflash_read(struct device *device, void *buf, u32 len, u32 offset);
int norflash_origin_read(u8 *buf, u32 offset, u32 len);
int norflash_write(struct device *device, void *buf, u32 len, u32 offset);
int norflash_erase_zone(struct device *device, u32 addr, u32 len);
int norflash_ioctl(struct device *device, u32 cmd, u32 arg);
int norflash_write_protect(u8 id, u32 cmd);
u32 flash_addr2cpu_addr(u32 offset);
u32 cpu_addr2flash_addr(u32 offset);
u32 ex_flash_addr2cpu_addr(u32 offset);
u32 ex_cpu_addr2flash_addr(u32 offset);
u8 *get_norflash_uuid(u8 id);
void norflash_set_spi_con(u32 arg);
int norflash_protect_resume(u8 id);
int norflash_protect_suspend(u8 id);
void norflash_enter_spi_code(u8 id);
void norflash_exit_spi_code(u8 id);
void norflash_spi_cs(u8 id, char cs);
void norflash_spi_write_byte(u8 id, unsigned char data);
u8 norflash_spi_read_byte(u8 id);
int norflash_wait_busy(u8 id);
void MEMCPY_FLASH(void *buf, void *read_addr, u32 size);

/*
 * 用于flash otp擦除
 */
int norflash_eraser_otp(u8 id);

/*
 * 用于flash otp写
 * 参数buf 写入数据buffer
 * 参数len 写入长度，必须以zone_len字节对齐
 * 参数zone_len 为otp单次写入长度
 * 返回值小于0 写入失败, 等于0 写入成功
 */
int norflash_write_otp(u8 id, u8 *buf, const int len, const int zone_len);

/*
 * 用于flash otp读
 * 参数buf 读取数据buffer
 * 参数len 读取长度
 * 参数zone_len 为otp单次读取长度
 * 返回值小于0 读取数据失败, 等于0 读取数据成功
 */
int norflash_read_otp(u8 id, u8 *buf, const int len, const int zone_len);


#endif
