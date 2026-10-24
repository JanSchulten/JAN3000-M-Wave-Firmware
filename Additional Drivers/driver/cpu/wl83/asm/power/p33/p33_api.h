#ifndef __P33_API_H__
#define __P33_API_H__


//
//
//					vol
//
//
//
/****************************************************************/
typedef enum {
    DVDD_VOL_093V = 0,
    DVDD_VOL_096V,
    DVDD_VOL_099V,
    DVDD_VOL_102V,
    DVDD_VOL_105V,
    DVDD_VOL_108V,
    DVDD_VOL_111V,
    DVDD_VOL_114V,
    DVDD_VOL_117V,
    DVDD_VOL_120V,
    DVDD_VOL_123V,
    DVDD_VOL_126V,
    DVDD_VOL_129V,
    DVDD_VOL_132V,
    DVDD_VOL_135V,
    DVDD_VOL_138V,
} dvdd_vol_t;

typedef enum {
    DCVDD_VOL_125V = 0,
    DCVDD_VOL_130V,
    DCVDD_VOL_135V,
    DCVDD_VOL_140V,
    DCVDD_VOL_145V,
    DCVDD_VOL_150V,
    DCVDD_VOL_155V,
    DCVDD_VOL_160V,
} dcvdd_vol_t;

typedef enum {
    VDDIOM_VOL_240V = 0,
    VDDIOM_VOL_280V,
    VDDIOM_VOL_290V,
    VDDIOM_VOL_300V,
    VDDIOM_VOL_310V,
    VDDIOM_VOL_320V,
    VDDIOM_VOL_330V,
    VDDIOM_VOL_340V,
} vddiom_vol_t;

typedef enum {
    VDDIOW_VOL_200V = 0,
    VDDIOW_VOL_240V,
    VDDIOW_VOL_280V,
    VDDIOW_VOL_320V,
} vddiow_vol_t;

typedef enum {
    WVDD_VOL_060V = 0,
    WVDD_VOL_065V,
    WVDD_VOL_070V,
    WVDD_VOL_075V,
    WVDD_VOL_080V,
    WVDD_VOL_085V,
    WVDD_VOL_090V,
    WVDD_VOL_138V,
} wvdd_vol_t;

typedef enum {
    AVDD18_VOL_DEFAULT = 0,
    AVDD18_VOL_150V,
    AVDD18_VOL_160V,
    AVDD18_VOL_170V,
    AVDD18_VOL_180V,
    AVDD18_VOL_190V,
    AVDD18_VOL_200V,
    AVDD18_VOL_210V,
    AVDD18_VOL_220V,
} avdd18_vol_t;

typedef enum {
    AVDD28_VOL_DEFAULT = 0,
    AVDD28_VOL_250V,
    AVDD28_VOL_260V,
    AVDD28_VOL_270V,
    AVDD28_VOL_280V,
    AVDD28_VOL_290V,
    AVDD28_VOL_300V,
    AVDD28_VOL_310V,
    AVDD28_VOL_320V,
} avdd28_vol_t;

vddiom_vol_t get_vddiom_vol_sel(void);
vddiow_vol_t get_vddiow_vol_sel(void);

//
//
//					lvd
//
//
//
/****************************************************************/
typedef enum {
    LVD_RESET_MODE,		//复位模式
    LVD_EXCEPTION_MODE, //异常模式，进入异常中断
    LVD_WAKEUP_MODE,    //唤醒模式，进入唤醒中断，callback参数为回调函数
} lvd_mode_t;

typedef enum {
    VLVD_SEL_190V = 0,
    VLVD_SEL_200V,
    VLVD_SEL_210V,
    VLVD_SEL_220V,
    VLVD_SEL_230V,
    VLVD_SEL_240V,
    VLVD_SEL_250V,
    VLVD_SEL_260V,
} lvd_vol_t;

void lvd_en(u8 en);

void lvd_config(lvd_vol_t vol, u8 expin_en, lvd_mode_t mode, void (*callback));

//
//
//                    pinr
//
//
//
//******************************************************************
void gpio_longpress_pin0_reset_config(u32 pin, u32 level, u32 time, u32 release, u32 pull_enable);

//
//
//                    dcdc
//
//
//
//******************************************************************
typedef enum {
    PWR_NO_CHANGE = 0,
    PWR_LDO33,
    PWR_LDO15,
    PWR_DCDC15,
} power_mode_t;

typedef enum {
    PWR_DCDC12,
    PWR_DCDC18_DCDC12,
    PWR_DCDC18_DCDC12_DCDC09,
} power_dcdc_t;

void power_set_dcdc_type(power_dcdc_t type);

void power_set_mode(power_mode_t mode);

//
//
//                    adc_test
//
//
//
//******************************************************************
void adc_pmu_detect_en(u32 ch);

#endif
