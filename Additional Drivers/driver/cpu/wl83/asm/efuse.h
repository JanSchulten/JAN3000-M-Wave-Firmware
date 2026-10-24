#ifndef __EFUSE_H__
#define __EFUSE_H__

#include "generic/typedef.h"

u16 get_chip_id(void);

u8 get_vbg_trim(void);

u32 get_chip_version(void);

u8 efuse_get_en_act(void);

u8 efuse_get_lvd_level(void);

u8 efuse_get_vddio_level(void);

u8 efuse_get_mclr_en(void);

u8 efuse_get_fast_up_en(void);

u8 efuse_get_pin_reset_en(void);

u16 efuse_get_vtemp(void);

// u8 efuse_get_boot_decrypt(void);
u8 efuse_get_adc_trim(void);

u8 efuse_get_avdd18_en(void);

u8 efuse_get_avdd28_en(void);

u8 efuse_get_mvbg_level(void);

u8 efuse_get_trim_act(void);

u8 efuse_get_sfc_fast_boot_dis(void);

u8 efuse_get_cp_pass(void);

u8 efuse_get_ft_pass(void);

u8 efuse_get_audio_vbg_trim(void);

u8 efuse_get_wvdd_level_trim(void);

u32 efuse_get_chip_id(void);

#endif  /*EFUSE_H*/
