#ifndef __ISP_LIBAWB_H__
#define __ISP_LIBAWB_H__

#include "typedef.h"
#include "hal/statistic.h"
/*#define ISP0_STAT_WEIGHT_WIN_Y 	8
#define ISP0_STAT_WEIGHT_WIN_X 	8

#define ISP0_STAT_AWB_WIN_Y 	16
#define ISP0_STAT_AWB_WIN_X 	16*/



#define HYBRID_AWB_INPUT_WIDTH		16
#define HYBRID_AWB_INPUT_HEIGHT		16

#define HYBRID_AWB_CT_POINTS_MAX	256

#define HYBRID_AWB_CT_MIN		2000
#define HYBRID_AWB_CT_MAX		10000
#define HYBRID_AWB_CT_LUT_LEN		((HYBRID_AWB_CT_MAX-HYBRID_AWB_CT_MIN)/50)

#define NN_HIST_DIM			16


typedef struct {
    int color_temperatrue;
    int rg;
    int bg;
    int radius;
} awb_ct_point_desc_t;

typedef struct {
    unsigned int rg;
    unsigned int bg;
    int color_temperatrue;
    int confidence;
    int type;
    int ctidx;
} awb_result_t;

typedef struct {
    int u0;
    int v0;
    int diagonal_eps;
    int temporal_noise;
    int bin_step;
    int re[NN_HIST_DIM][NN_HIST_DIM];
    int im[NN_HIST_DIM][NN_HIST_DIM];
    int bias[NN_HIST_DIM][NN_HIST_DIM];
    int en;
    int temporal_smooth_en;
} nn_parameter_t;

typedef struct {
    isp0_wb_stat_param_t hw_stat_param;
    nn_parameter_t nn_param;
    awb_ct_point_desc_t ref_points[HYBRID_AWB_CT_POINTS_MAX];
    int ref_points_num;

    s8  indoor_weight[HYBRID_AWB_CT_LUT_LEN]; // indoor
    s8  outdoor_weight[HYBRID_AWB_CT_LUT_LEN]; // outdoor
    s8  sunlight_weight[HYBRID_AWB_CT_LUT_LEN]; // sunlight

    u16  adjust_ratio_gr[HYBRID_AWB_CT_LUT_LEN];
    u16  adjust_ratio_gb[HYBRID_AWB_CT_LUT_LEN];

    int indoor_exp_th;
    int outdoor0_exp_th;
    int outdoor1_exp_th;
    int sunlight_exp_th;

    int lowlight_exp_th;

    int gw_fit_dist;
    int gw_influence;

    int nn_influence;
    int indoor_influence;

    int valid_point_th;
    int dominant_ratio;

    int dec_nn_th;
    int dec_ct_th;
    int dec_gw_th;

    int stab_level;
    int blc_r;
    int blc_g;
    int blc_b;

} hybrid_awb_parameter_t;

void *hybrid_awb_init1();
void *hybrid_awb_init2(char *awb_bin_path, void *hybrid_);
int hybrid_awb_reinit(void *hybrid);
int hybrid_awb_deinit(void *hybrid);
isp0_wb_stat_param_t hybrid_awb_get_hw_param(void *h);
int hybrid_awb_set_parameters(void *hybrid, hybrid_awb_parameter_t *p);
int hybrid_awb_process(void *hybrid, void *stat_input, int exposure);
int hybrid_awb_get_result(void *hybrid, isp_wb_gain_t *awb_result);
int get_awb_status(void *h);

#endif
