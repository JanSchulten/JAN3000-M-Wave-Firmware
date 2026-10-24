#ifndef __POWER_TRIM_H__
#define __POWER_TRIM_H__

/**
 * @brief 系统电源校准
 */
void pmu_trim(u32 force_trim, u32 vddio_tieup_vbat);

#endif
