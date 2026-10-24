
#ifndef __ISP_CUSTOMIZE_H__
#define __ISP_CUSTOMIZE_H__



#include "typedef.h"
#include "asm/isp_alg.h"

#define ISPT_DEBUG_LEVEL_AE           0x01
#define ISPT_DEBUG_LEVEL_AWB          0x02
#define ISPT_DEBUG_LEVEL_MD           0x04
#define ISPT_DEBUG_LEVEL_IMP          0x08
#define ISPT_DEBUG_LEVEL_INFO         0x10
#define ISPT_DEBUG_LEVEL_CUST         0x20

int ispt_get_isp_drv_status(u8 id);

int ispt_set_blc(isp_blc_t *p, u8 id);
int ispt_get_blc(isp_blc_t *p, u8 id);

int ispt_set_dpc(isp_dpc_t *p, u8 id);
int ispt_get_dpc(isp_dpc_t *p, u8 id);

int ispt_set_lsc(isp_lsc_t *p, u8 id);
int ispt_get_lsc(isp_lsc_t *p, u8 id);

int ispt_set_tnr(isp_tnr_t *p, u8 id);
int ispt_get_tnr(isp_tnr_t *p, u8 id);

int ispt_set_nr(isp_nr_t *p, u8 id);
int ispt_get_nr(isp_nr_t *p, u8 id);

int ispt_set_cnr(isp_cnr_t *p, u8 id);
int ispt_get_cnr(isp_cnr_t *p, u8 id);

int ispt_set_shp(isp_shp_t *p, u8 id);
int ispt_get_shp(isp_shp_t *p, u8 id);

int ispt_set_ccm(isp_ccm_t p, u8 id);
int ispt_get_ccm(isp_ccm_t p, u8 id);

int ispt_set_adj(isp_adj_t *p, u8 id);
int ispt_get_adj(isp_adj_t *p, u8 id);

int ispt_set_brightness(s32 p, u8 id);
int ispt_get_brightness(s32 *p, u8 id);

int ispt_set_contrast(u32 p, u8 id);
int ispt_get_contrast(u32 *p, u8 id);

int ispt_set_saturation_u(u32 p, u8 id);
int ispt_get_saturation_u(u32 *p, u8 id);

int ispt_set_saturation_v(u32 p, u8 id);
int ispt_get_saturation_v(u32 *p, u8 id);

int ispt_set_gamma(u8 *p, u8 id);
int ispt_get_gamma(u8 *p, u8 id);

int ispt_set_three_gamma(u8 *r, u8 *g, u8 *b, u8 id);
int ispt_get_three_gamma(u8 *r, u8 *g, u8 *b, u8 id);

int ispt_set_wb_type(isp_wb_type_t p, u8 id);
int ispt_get_wb_type(isp_wb_type_t *p, u8 id);

int ispt_set_awb_ww_type(isp_awb_ww_type_t p, u8 id);
int ispt_get_awb_ww_type(isp_awb_ww_type_t *p, u8 id);

int ispt_set_awb_cust_ww(u8 *p, u8 id);
int ispt_get_awb_cust_ww(u8 *p, u8 id);

int ispt_set_awb_gain(u8 type, u16 rgain, u16 ggain, u16 bgain, u8 id);
int ispt_get_awb_gain(u8 type, u16 *rgain, u16 *ggain, u16 *bgain, u8 id);

int ispt_set_ae_ww_type(isp_ae_ww_type_t p, u8 id);
int ispt_get_ae_ww_type(isp_ae_ww_type_t *p, u8 id);

int ispt_set_ae_cust_ww(u8 *p, u8 id);
int ispt_get_ae_cust_ww(u8 *p, u8 id);

int ispt_set_ev(s8  ev, u8 id);
int ispt_get_ev(s8 *ev, u8 id);

int ispt_set_ae_target(s32  ev, u8 id);
int ispt_get_ae_target(s32 *ev, u8 id);


int ispt_set_md_enable(u8  en, u8 id);
int ispt_get_md_enable(u8 *en, u8 id);

int ispt_set_md_level(u8 level, u8 id);
int ispt_get_md_level(u8 *level, u8 id);


int ispt_set_debug_level(u8 level);
int ispt_get_debug_level(u8 *level);

int ispt_set_drc(u8 enable, u32 scale, u8 id);
int ispt_get_drc(u8 *enable, u32 *scale, u8 id);

int ispt_set_drc_lav_limit(u32 lav_limit, u8 id);
int ispt_get_drc_lav_limit(u32 *lav_limit, u8 id);

int ispt_set_contrast_enhance(u8 mode, u8 low, u8 high, u8 id);
int ispt_get_contrast_enhance(u8 *mode, u8 *low, u8 *high, u8 id);


int ispt_get_ae_lv(s32 *lv, u8 id);
int ispt_get_ae_gain(u32 *gain, u8 id);
int ispt_get_ae_luma(u32 *luma, u8 id);
int ispt_get_md_status(u8 *status, u8 id);

int ispt_sensor_write_reg(u16 addr, u16 val, u8 id);
int ispt_sensor_read_reg(u16 addr, u16 *val, u8 id);

/*(-1,0,1)*/
int ispt_set_shp_level(s8 level, u8 id);
int ispt_get_shp_level(u8 *level, u8 id);

int ispt_set_ae_type(int type, u8 id);
int ispt_get_ae_type(int *type, u8 id);

int ispt_set_notify_task_name(char *name);

void ispt_set_block_frame_count(u32 c);
void ispt_fps_meter_enable(u32 en, u8 id);
void ispt_set_ae_update_in_isr(u8 en, u8 id);

int ispt_params_smooth_en(u8 en, u8 id);
int ispt_params_flush(u8 id);
int ispt_params_set_smooth_step(int step, u8 id);

int ispt_get_rawdata(int *width, int *height, u8 *data, u8 id);

int ispt_awb_get_cur_gain(int *rgain, int *ggain, int *bgain, u8 id);
int ispt_get_cur_fps(int *fps, u8 id);
#endif


