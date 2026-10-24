#ifndef __ASM_AUDIO_H__
#define __ASM_AUDIO_H__

#include "generic/typedef.h"
#include "asm/ladc.h"
#include "asm/dac.h"
#include "asm/alnk.h"
#include "asm/plnk.h"
#include "asm/src.h"
// #include "asm/eq.h"

struct audio_pf_data {
    const struct adc_platform_data *adc_pf_data;
    const struct dac_platform_data *dac_pf_data;
    const struct alnk_platform_data *alnk_pf_data;
    const struct plnk_platform_data *plnk_pf_data;
};

#endif
