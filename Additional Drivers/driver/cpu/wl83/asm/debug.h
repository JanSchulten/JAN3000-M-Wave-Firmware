#ifndef __ASM_DEBUG_H__
#define __ASM_DEBUG_H__

#include "generic/typedef.h"

typedef enum etm_detect_mode {
    CPU_SFR_DETECT_MODE = 1,
    CPU_RD_BUS_DETECT_MODE,
    CPU_WR_BUS_DETECT_MODE,
    CPU_RD_WR_BUS_DETECT_MODE,
} ETM_DETECT_MODE;

typedef enum axi_master_id {
    AXI_MASTER_DCACHE = 0x8000,
    AXI_MASTER_DCACHE_SID_RD = 0x0000,
    AXI_MASTER_DCACHE_SID_WRITE_TRIGGER = 0x0001,
    AXI_MASTER_ICACHE = 0x8100,
    AXI_MASTER_ICACHE_SID_CPU0 = 0x0100,
    AXI_MASTER_ICACHE_SID_CPU1 = 0x0101,
    AXI_MASTER_DMA_COPY = 0x8200,
    AXI_MASTER_JPG0 = 0x8300,
    AXI_MASTER_JPG1 = 0x8400,
    AXI_MASTER_EVA0 = 0x8500,
    AXI_MASTER_EVA1 = 0x8600,
    AXI_MASTER_GPU0 = 0x8800,
    AXI_MASTER_GPU1 = 0x8900,
    AXI_MASTER_DMA2D = 0x8a00,
    AXI_MASTER_NONE = 0x8f00,
} AXI_MASTER_ID;

const char *get_debug_dev_name(u32 id);
u32 get_debug_dev_id(const char *name);
void pc_rang_limit(void *low_addr0, void *high_addr0, void *low_addr1, void *high_addr1, void *low_addr2, void *high_addr2);
void cpu_etm_range_value_limit_detect(void *low_addr, void *high_addr, u32 low_limit_value, u32 high_limit_value, ETM_DETECT_MODE mode, int limit_range_out);
void mpu_enable_by_index(int idx, u32 begin, u32 end, u32 inv, const char *format, ...);
void mpu_disable_by_index(u8 idx);
void mpu_diasble_all(void);
void tzasc_enable_by_index(int idx, u32 begin, u32 end, const char *format, ...);
void tzasc_disable_by_index(u8 idx);
void tzasc_diasble_all(void);
void debug_init(void);
void debug_clear(void);
void debug_msg_clear(void);
void cpu_efficiency_calculate_init(void);
void cpu_efficiency_calculate_uninit(void);
void cpu_efficiency_calculate_show(void);

extern u32 rom_text_code_begin;
extern u32 rom_text_code_end;
extern u32 ram_text_code_begin;
extern u32 ram_text_code_end;
extern u32 sdram_text_code_begin;
extern u32 sdram_text_code_end;

#endif
