#ifndef _EVA_SFR_H_
#define _EVA_SFR_H_
//===============================================================================//
//
//      sfr base
//
//===============================================================================//
#include "mipi_sfr.h"

//#define eva_base(i)     (dip_prp0_base + 0x800*i)
#define eva_base(i)     (vd_base + 0x8000 + 0x800*i)
#define eva_msfr(i,j,k) ((k *)(u32)       (eva_base(i) + j*4))
#define eva_wsfr(i,j)   (*(volatile u32 *)(eva_base(i) + j*4))

//  mount device
//******************************
enum {
    cid_xbus
    , cid_vbus
    , cid_isc
    , cid_rep
    , cid_imc
    , cid_isp
    , cid_dmm
    , cid_dpi
    , cid_csi
    , cid_dsi
    , cid_mcv
    , cid_gp_sca
} ;

//  mount write bus
//******************************
enum {
    wid_imc_ch0
    , wid_imc_ch1
    , wid_imc_ch2
    , wid_imc_ch3
    , wid_isp_stc
    , wid_gp_sca
} ;

//  mount read bus
//******************************
enum {
    rid_imc_ch0
    , rid_imc_ch1
    , rid_rep_ch0
    , rid_rep_ch1
    , rid_dmm
    , rid_gp_sca
} ;

//  video source
//******************************
enum {
    vsid_sen0
    , vsid_sen1
    , vsid_rep_ch0
    , vsid_rep_ch1
    , vsid_isp0
    , vsid_csi_vc0
    , vsid_csi_vc1
    , vsid_mcv_ch0
    , vsid_mcv_ch1
    , vsid_mcv_ch2
    , vsid_mcv_ch3
} ;

//  video target
//******************************
enum {
    vtid_imc_ch0
    , vtid_imc_ch1
    , vtid_imc_ch2
    , vtid_imc_ch3
    , vtid_isp0
} ;

//===============================================================================//
//
//      public
//
//===============================================================================//
#define eva_xbus_seg    eva_wsfr(cid_xbus, 0x0)
#define eva_xbus_wsel   eva_wsfr(cid_xbus, 0x1)
#define eva_xbus_rsel   eva_wsfr(cid_xbus, 0x2)

#define vbus_set(tid)   eva_wsfr(cid_vbus, tid)

//===============================================================================//
//
//      isc
//
//===============================================================================//
#define isc_pnd_con     eva_wsfr(cid_isc, 0x00)

#define isc_sen0_con    eva_wsfr(cid_isc, 0x08)
#define isc_sen0_vtt    eva_wsfr(cid_isc, 0x09)
#define isc_sen0_vblk   eva_wsfr(cid_isc, 0x0a)
#define isc_sen0_vact   eva_wsfr(cid_isc, 0x0b)
#define isc_sen0_htt    eva_wsfr(cid_isc, 0x0c)
#define isc_sen0_hblk   eva_wsfr(cid_isc, 0x0d)
#define isc_sen0_hact   eva_wsfr(cid_isc, 0x0e)

#define isc_sen1_con    eva_wsfr(cid_isc, 0x10)
#define isc_sen1_vtt    eva_wsfr(cid_isc, 0x11)
#define isc_sen1_vblk   eva_wsfr(cid_isc, 0x12)
#define isc_sen1_vact   eva_wsfr(cid_isc, 0x13)
#define isc_sen1_htt    eva_wsfr(cid_isc, 0x14)
#define isc_sen1_hblk   eva_wsfr(cid_isc, 0x15)
#define isc_sen1_hact   eva_wsfr(cid_isc, 0x16)

//===============================================================================//
//
//      rep
//
//===============================================================================//
#define rep_pnd_con      eva_wsfr(cid_rep, 0x00)

#define rep_ch0_con      eva_wsfr(cid_rep, 0x10)
#define rep_ch0_prd      eva_wsfr(cid_rep, 0x11)
#define rep_ch0_cnt      eva_wsfr(cid_rep, 0x12)
#define rep_ch0_w_cfg    eva_wsfr(cid_rep, 0x13)
#define rep_ch0_h_cfg    eva_wsfr(cid_rep, 0x14)
#define rep_ch0_s_cfg    eva_wsfr(cid_rep, 0x15)
#define rep_ch0_y_base   eva_wsfr(cid_rep, 0x16)
#define rep_ch0_u_base   eva_wsfr(cid_rep, 0x17)
#define rep_ch0_v_base   eva_wsfr(cid_rep, 0x18)

#define rep_ch1_con      eva_wsfr(cid_rep, 0x20)
#define rep_ch1_prd      eva_wsfr(cid_rep, 0x21)
#define rep_ch1_cnt      eva_wsfr(cid_rep, 0x22)
#define rep_ch1_w_cfg    eva_wsfr(cid_rep, 0x23)
#define rep_ch1_h_cfg    eva_wsfr(cid_rep, 0x24)
#define rep_ch1_s_cfg    eva_wsfr(cid_rep, 0x25)
#define rep_ch1_y_base   eva_wsfr(cid_rep, 0x26)
#define rep_ch1_u_base   eva_wsfr(cid_rep, 0x27)
#define rep_ch1_v_base   eva_wsfr(cid_rep, 0x28)

//===============================================================================//
//
//      imc
//
//===============================================================================//
#define imc_pnd_con          eva_wsfr(cid_imc, 0x000)

#define imc_rep0_con         eva_wsfr(cid_imc, 0x001)
#define imc_rep0_h_cfg       eva_wsfr(cid_imc, 0x002)
#define imc_rep0_v_cfg       eva_wsfr(cid_imc, 0x003)
#define imc_rep0_y_base      eva_wsfr(cid_imc, 0x004)
#define imc_rep0_u_base      eva_wsfr(cid_imc, 0x005)
#define imc_rep0_v_base      eva_wsfr(cid_imc, 0x006)

#define imc_rep1_con         eva_wsfr(cid_imc, 0x009)
#define imc_rep1_h_cfg       eva_wsfr(cid_imc, 0x00a)
#define imc_rep1_v_cfg       eva_wsfr(cid_imc, 0x00b)
#define imc_rep1_y_base      eva_wsfr(cid_imc, 0x00c)
#define imc_rep1_u_base      eva_wsfr(cid_imc, 0x00d)
#define imc_rep1_v_base      eva_wsfr(cid_imc, 0x00e)

#define imc_ch0_com_con      eva_wsfr(cid_imc, 0x020)
#define imc_ch0_src_con      eva_wsfr(cid_imc, 0x021)
#define imc_ch0_crop_h       eva_wsfr(cid_imc, 0x022)
#define imc_ch0_crop_v       eva_wsfr(cid_imc, 0x023)
#define imc_ch0_h_stp        eva_wsfr(cid_imc, 0x024)
#define imc_ch0_h_wth        eva_wsfr(cid_imc, 0x025)
#define imc_ch0_v_stp        eva_wsfr(cid_imc, 0x026)
#define imc_ch0_v_wth        eva_wsfr(cid_imc, 0x027)
#define imc_ch0_dma_con      eva_wsfr(cid_imc, 0x028)
#define imc_ch0_dma_cnt      eva_wsfr(cid_imc, 0x029)
#define imc_ch0_dma_y_bs     eva_wsfr(cid_imc, 0x02a)
#define imc_ch0_dma_u_bs     eva_wsfr(cid_imc, 0x02b)
#define imc_ch0_dma_v_bs     eva_wsfr(cid_imc, 0x02c)
#define imc_ch0_osd_con      eva_wsfr(cid_imc, 0x02d)
#define imc_ch0_osd_color0   eva_wsfr(cid_imc, 0x02e)
#define imc_ch0_osd_color1   eva_wsfr(cid_imc, 0x02f)
#define imc_ch0_osd_color2   eva_wsfr(cid_imc, 0x030)
#define imc_ch0_osd_color3   eva_wsfr(cid_imc, 0x031)
#define imc_ch0_osd0_h_cfg   eva_wsfr(cid_imc, 0x032)
#define imc_ch0_osd0_v_cfg   eva_wsfr(cid_imc, 0x033)
#define imc_ch0_osd0_base    eva_wsfr(cid_imc, 0x034)
#define imc_ch0_osd1_h_cfg   eva_wsfr(cid_imc, 0x035)
#define imc_ch0_osd1_v_cfg   eva_wsfr(cid_imc, 0x036)
#define imc_ch0_osd1_base    eva_wsfr(cid_imc, 0x037)

#define imc_ch1_com_con      eva_wsfr(cid_imc, 0x040)
#define imc_ch1_src_con      eva_wsfr(cid_imc, 0x041)
#define imc_ch1_crop_h       eva_wsfr(cid_imc, 0x042)
#define imc_ch1_crop_v       eva_wsfr(cid_imc, 0x043)
#define imc_ch1_h_stp        eva_wsfr(cid_imc, 0x044)
#define imc_ch1_h_wth        eva_wsfr(cid_imc, 0x045)
#define imc_ch1_v_stp        eva_wsfr(cid_imc, 0x046)
#define imc_ch1_v_wth        eva_wsfr(cid_imc, 0x047)
#define imc_ch1_dma_con      eva_wsfr(cid_imc, 0x048)
#define imc_ch1_dma_cnt      eva_wsfr(cid_imc, 0x049)
#define imc_ch1_dma_y_bs     eva_wsfr(cid_imc, 0x04a)
#define imc_ch1_dma_u_bs     eva_wsfr(cid_imc, 0x04b)
#define imc_ch1_dma_v_bs     eva_wsfr(cid_imc, 0x04c)
#define imc_ch1_osd_con      eva_wsfr(cid_imc, 0x04d)
#define imc_ch1_osd_color0   eva_wsfr(cid_imc, 0x04e)
#define imc_ch1_osd_color1   eva_wsfr(cid_imc, 0x04f)
#define imc_ch1_osd_color2   eva_wsfr(cid_imc, 0x050)
#define imc_ch1_osd_color3   eva_wsfr(cid_imc, 0x051)
#define imc_ch1_osd0_h_cfg   eva_wsfr(cid_imc, 0x052)
#define imc_ch1_osd0_v_cfg   eva_wsfr(cid_imc, 0x053)
#define imc_ch1_osd0_base    eva_wsfr(cid_imc, 0x054)
#define imc_ch1_osd1_h_cfg   eva_wsfr(cid_imc, 0x055)
#define imc_ch1_osd1_v_cfg   eva_wsfr(cid_imc, 0x056)
#define imc_ch1_osd1_base    eva_wsfr(cid_imc, 0x057)

#define imc_ch2_com_con      eva_wsfr(cid_imc, 0x060)
#define imc_ch2_src_con      eva_wsfr(cid_imc, 0x061)
#define imc_ch2_crop_h       eva_wsfr(cid_imc, 0x062)
#define imc_ch2_crop_v       eva_wsfr(cid_imc, 0x063)
#define imc_ch2_h_stp        eva_wsfr(cid_imc, 0x064)
#define imc_ch2_h_wth        eva_wsfr(cid_imc, 0x065)
#define imc_ch2_v_stp        eva_wsfr(cid_imc, 0x066)
#define imc_ch2_v_wth        eva_wsfr(cid_imc, 0x067)
#define imc_ch2_dma_con      eva_wsfr(cid_imc, 0x068)
#define imc_ch2_dma_cnt      eva_wsfr(cid_imc, 0x069)
#define imc_ch2_dma_y_bs     eva_wsfr(cid_imc, 0x06a)
#define imc_ch2_dma_u_bs     eva_wsfr(cid_imc, 0x06b)
#define imc_ch2_dma_v_bs     eva_wsfr(cid_imc, 0x06c)

#define imc_ch3_com_con      eva_wsfr(cid_imc, 0x070)
#define imc_ch3_src_con      eva_wsfr(cid_imc, 0x071)
#define imc_ch3_crop_h       eva_wsfr(cid_imc, 0x072)
#define imc_ch3_crop_v       eva_wsfr(cid_imc, 0x073)
#define imc_ch3_h_stp        eva_wsfr(cid_imc, 0x074)
#define imc_ch3_h_wth        eva_wsfr(cid_imc, 0x075)
#define imc_ch3_v_stp        eva_wsfr(cid_imc, 0x076)
#define imc_ch3_v_wth        eva_wsfr(cid_imc, 0x077)
#define imc_ch3_dma_con      eva_wsfr(cid_imc, 0x078)
#define imc_ch3_dma_cnt      eva_wsfr(cid_imc, 0x079)
#define imc_ch3_dma_y_bs     eva_wsfr(cid_imc, 0x07a)
#define imc_ch3_dma_u_bs     eva_wsfr(cid_imc, 0x07b)
#define imc_ch3_dma_v_bs     eva_wsfr(cid_imc, 0x07c)

//===============================================================================//
//
//      isp
//
//===============================================================================//
#define isp0_pnd_con         eva_wsfr(cid_isp, 0x000)
#define isp0_scn_con         eva_wsfr(cid_isp, 0x001)

#define isp0_src_con         eva_wsfr(cid_isp, 0x004)
#define isp0_src_haw         eva_wsfr(cid_isp, 0x005)
#define isp0_src_vaw         eva_wsfr(cid_isp, 0x006)

#define isp0_blc_off_r       eva_wsfr(cid_isp, 0x008)
#define isp0_blc_off_gr      eva_wsfr(cid_isp, 0x009)
#define isp0_blc_off_gb      eva_wsfr(cid_isp, 0x00a)
#define isp0_blc_off_b       eva_wsfr(cid_isp, 0x00b)

#define isp0_dpc_th0         eva_wsfr(cid_isp, 0x00c)
#define isp0_dpc_th1         eva_wsfr(cid_isp, 0x00d)
#define isp0_dpc_th2         eva_wsfr(cid_isp, 0x00e)

#define isp0_lsc_cen_x       eva_wsfr(cid_isp, 0x010)
#define isp0_lsc_cen_y       eva_wsfr(cid_isp, 0x011)
#define isp0_lsc_dth_th      eva_wsfr(cid_isp, 0x012)
#define isp0_lsc_dth_prm0    eva_wsfr(cid_isp, 0x013)
#define isp0_lsc_dth_prm1    eva_wsfr(cid_isp, 0x014)
#define isp0_lsc_lut_r       eva_wsfr(cid_isp, 0x015)
#define isp0_lsc_lut_g       eva_wsfr(cid_isp, 0x016)
#define isp0_lsc_lut_b       eva_wsfr(cid_isp, 0x017)

#define isp0_awb_r_gain      eva_wsfr(cid_isp, 0x018)
#define isp0_awb_g_gain      eva_wsfr(cid_isp, 0x019)
#define isp0_awb_b_gain      eva_wsfr(cid_isp, 0x01a)

#define isp0_drc_lut         eva_wsfr(cid_isp, 0x01b)

#define isp0_tnr_con         eva_wsfr(cid_isp, 0x01c)
#define isp0_tnr_base        eva_wsfr(cid_isp, 0x01d)
#define isp0_tnr_size        eva_wsfr(cid_isp, 0x01e)
#define isp0_tnr_2d_str      eva_wsfr(cid_isp, 0x01f)
#define isp0_tnr_3d_th0      eva_wsfr(cid_isp, 0x020)
#define isp0_tnr_3d_th1      eva_wsfr(cid_isp, 0x021)
#define isp0_tnr_mt_th       eva_wsfr(cid_isp, 0x022)
#define isp0_tnr_wmax        eva_wsfr(cid_isp, 0x023)
#define isp0_tnr_wmin        eva_wsfr(cid_isp, 0x024)
#define isp0_tnr_wslope      eva_wsfr(cid_isp, 0x025)
#define isp0_tnr_break       eva_wsfr(cid_isp, 0x026)
#define isp0_tnr_scale0      eva_wsfr(cid_isp, 0x027)
#define isp0_tnr_scale1      eva_wsfr(cid_isp, 0x028)
#define isp0_tnr_scale2      eva_wsfr(cid_isp, 0x029)
#define isp0_tnr_scale3      eva_wsfr(cid_isp, 0x02a)
#define isp0_tnr_scale4      eva_wsfr(cid_isp, 0x02b)
#define isp0_tnr_scale5      eva_wsfr(cid_isp, 0x02c)
#define isp0_tnr_scale6      eva_wsfr(cid_isp, 0x02d)
#define isp0_tnr_scale7      eva_wsfr(cid_isp, 0x02e)

#define isp0_ccm_r_coe0      eva_wsfr(cid_isp, 0x030)
#define isp0_ccm_r_coe1      eva_wsfr(cid_isp, 0x031)
#define isp0_ccm_r_coe2      eva_wsfr(cid_isp, 0x032)
#define isp0_ccm_r_off       eva_wsfr(cid_isp, 0x033)
#define isp0_ccm_g_coe0      eva_wsfr(cid_isp, 0x034)
#define isp0_ccm_g_coe1      eva_wsfr(cid_isp, 0x035)
#define isp0_ccm_g_coe2      eva_wsfr(cid_isp, 0x036)
#define isp0_ccm_g_off       eva_wsfr(cid_isp, 0x037)
#define isp0_ccm_b_coe0      eva_wsfr(cid_isp, 0x038)
#define isp0_ccm_b_coe1      eva_wsfr(cid_isp, 0x039)
#define isp0_ccm_b_coe2      eva_wsfr(cid_isp, 0x03a)
#define isp0_ccm_b_off       eva_wsfr(cid_isp, 0x03b)

#define isp0_gma_r_lut       eva_wsfr(cid_isp, 0x03c)
#define isp0_gma_g_lut       eva_wsfr(cid_isp, 0x03d)
#define isp0_gma_b_lut       eva_wsfr(cid_isp, 0x03e)
#define isp0_csc_y_lut       eva_wsfr(cid_isp, 0x03f)

#define isp0_dnr_sim_th      eva_wsfr(cid_isp, 0x040)
#define isp0_dnr_rng_sgm     eva_wsfr(cid_isp, 0x041)
#define isp0_dnr_gaus_c00    eva_wsfr(cid_isp, 0x042)
#define isp0_dnr_gaus_c01    eva_wsfr(cid_isp, 0x043)
#define isp0_dnr_gaus_c02    eva_wsfr(cid_isp, 0x044)
#define isp0_dnr_gaus_c03    eva_wsfr(cid_isp, 0x045)
#define isp0_dnr_gaus_c11    eva_wsfr(cid_isp, 0x046)
#define isp0_dnr_gaus_c12    eva_wsfr(cid_isp, 0x047)
#define isp0_dnr_gaus_c13    eva_wsfr(cid_isp, 0x048)
#define isp0_dnr_gaus_c22    eva_wsfr(cid_isp, 0x049)
#define isp0_dnr_gaus_c23    eva_wsfr(cid_isp, 0x04a)
#define isp0_dnr_gaus_c33    eva_wsfr(cid_isp, 0x04b)
#define isp0_dnr_cmid_en     eva_wsfr(cid_isp, 0x04c)

#define isp0_shp_lone_th     eva_wsfr(cid_isp, 0x04d)
#define isp0_shp_ech_min     eva_wsfr(cid_isp, 0x04e)
#define isp0_shp_ech_max     eva_wsfr(cid_isp, 0x04f)

#define isp0_shp_hf_th0      eva_wsfr(cid_isp, 0x050)
#define isp0_shp_hf_th1      eva_wsfr(cid_isp, 0x051)
#define isp0_shp_hf_th2      eva_wsfr(cid_isp, 0x052)
#define isp0_shp_hf_amt      eva_wsfr(cid_isp, 0x053)
#define isp0_shp_hf_gain     eva_wsfr(cid_isp, 0x054)
#define isp0_shp_hf_c00      eva_wsfr(cid_isp, 0x055)
#define isp0_shp_hf_c01      eva_wsfr(cid_isp, 0x056)
#define isp0_shp_hf_c02      eva_wsfr(cid_isp, 0x057)
#define isp0_shp_hf_c10      eva_wsfr(cid_isp, 0x058)
#define isp0_shp_hf_c11      eva_wsfr(cid_isp, 0x059)
#define isp0_shp_hf_c12      eva_wsfr(cid_isp, 0x05a)
#define isp0_shp_hf_c20      eva_wsfr(cid_isp, 0x05b)
#define isp0_shp_hf_c21      eva_wsfr(cid_isp, 0x05c)
#define isp0_shp_hf_c22      eva_wsfr(cid_isp, 0x05d)
#define isp0_shp_hf_sft      eva_wsfr(cid_isp, 0x05e)

#define isp0_shp_mf_th0      eva_wsfr(cid_isp, 0x060)
#define isp0_shp_mf_th1      eva_wsfr(cid_isp, 0x061)
#define isp0_shp_mf_amt      eva_wsfr(cid_isp, 0x062)
#define isp0_shp_mf_gain     eva_wsfr(cid_isp, 0x063)
#define isp0_shp_mf_c00      eva_wsfr(cid_isp, 0x064)
#define isp0_shp_mf_c01      eva_wsfr(cid_isp, 0x065)
#define isp0_shp_mf_c02      eva_wsfr(cid_isp, 0x066)
#define isp0_shp_mf_c10      eva_wsfr(cid_isp, 0x067)
#define isp0_shp_mf_c11      eva_wsfr(cid_isp, 0x068)
#define isp0_shp_mf_c12      eva_wsfr(cid_isp, 0x069)
#define isp0_shp_mf_c20      eva_wsfr(cid_isp, 0x06a)
#define isp0_shp_mf_c21      eva_wsfr(cid_isp, 0x06b)
#define isp0_shp_mf_c22      eva_wsfr(cid_isp, 0x06c)
#define isp0_shp_mf_sft      eva_wsfr(cid_isp, 0x06d)

#define isp0_shp_cr_smt_th   eva_wsfr(cid_isp, 0x070)
#define isp0_shp_cr_c00      eva_wsfr(cid_isp, 0x071)
#define isp0_shp_cr_c01      eva_wsfr(cid_isp, 0x072)
#define isp0_shp_cr_c02      eva_wsfr(cid_isp, 0x073)
#define isp0_shp_cr_c10      eva_wsfr(cid_isp, 0x074)
#define isp0_shp_cr_c11      eva_wsfr(cid_isp, 0x075)
#define isp0_shp_cr_c12      eva_wsfr(cid_isp, 0x076)
#define isp0_shp_cr_c20      eva_wsfr(cid_isp, 0x077)
#define isp0_shp_cr_c21      eva_wsfr(cid_isp, 0x078)
#define isp0_shp_cr_c22      eva_wsfr(cid_isp, 0x079)
#define isp0_shp_cr_sft      eva_wsfr(cid_isp, 0x07a)

#define isp0_cbs_y_gain      eva_wsfr(cid_isp, 0x080)
#define isp0_cbs_u_gain      eva_wsfr(cid_isp, 0x081)
#define isp0_cbs_v_gain      eva_wsfr(cid_isp, 0x082)
#define isp0_cbs_y_offs      eva_wsfr(cid_isp, 0x083)
#define isp0_cbs_u_offs      eva_wsfr(cid_isp, 0x084)
#define isp0_cbs_v_offs      eva_wsfr(cid_isp, 0x085)

#define isp0_out_hst         eva_wsfr(cid_isp, 0x088)
#define isp0_out_hed         eva_wsfr(cid_isp, 0x089)
#define isp0_out_vst         eva_wsfr(cid_isp, 0x08a)
#define isp0_out_ved         eva_wsfr(cid_isp, 0x08b)

#define isp0_stc_ae_base0    eva_wsfr(cid_isp, 0x090)
#define isp0_stc_ae_base1    eva_wsfr(cid_isp, 0x091)
#define isp0_stc_ae_base2    eva_wsfr(cid_isp, 0x092)
#define isp0_stc_ae_base3    eva_wsfr(cid_isp, 0x093)
#define isp0_stc_ae_basex    eva_wsfr(cid_isp, 0x094)
#define isp0_stc_ae_en       eva_wsfr(cid_isp, 0x095)
#define isp0_stc_ae_lv1      eva_wsfr(cid_isp, 0x096)
#define isp0_stc_ae_lv2      eva_wsfr(cid_isp, 0x097)
#define isp0_stc_ae_lv3      eva_wsfr(cid_isp, 0x098)
#define isp0_stc_ae_lv4      eva_wsfr(cid_isp, 0x099)
#define isp0_stc_ae_lv5      eva_wsfr(cid_isp, 0x09a)
#define isp0_stc_ae_lv6      eva_wsfr(cid_isp, 0x09b)
#define isp0_stc_ae_lv7      eva_wsfr(cid_isp, 0x09c)

#define isp0_stc_wb_base0    eva_wsfr(cid_isp, 0x0a0)
#define isp0_stc_wb_base1    eva_wsfr(cid_isp, 0x0a1)
#define isp0_stc_wb_base2    eva_wsfr(cid_isp, 0x0a2)
#define isp0_stc_wb_base3    eva_wsfr(cid_isp, 0x0a3)
#define isp0_stc_wb_basex    eva_wsfr(cid_isp, 0x0a4)
#define isp0_stc_wb_en       eva_wsfr(cid_isp, 0x0a5)
#define isp0_stc_wb_r_th     eva_wsfr(cid_isp, 0x0a6)
#define isp0_stc_wb_g_th     eva_wsfr(cid_isp, 0x0a7)
#define isp0_stc_wb_b_th     eva_wsfr(cid_isp, 0x0a8)
#define isp0_stc_wb_w_th     eva_wsfr(cid_isp, 0x0a9)
#define isp0_stc_wb_y_min    eva_wsfr(cid_isp, 0x0aa)
#define isp0_stc_wb_y_max    eva_wsfr(cid_isp, 0x0ab)
#define isp0_stc_wb_rg_min   eva_wsfr(cid_isp, 0x0ac)
#define isp0_stc_wb_rg_max   eva_wsfr(cid_isp, 0x0ad)
#define isp0_stc_wb_bg_min   eva_wsfr(cid_isp, 0x0ae)
#define isp0_stc_wb_bg_max   eva_wsfr(cid_isp, 0x0af)

//===============================================================================//
//
//      dmm
//
//===============================================================================//
#define dmm_con             eva_wsfr(cid_dmm, 0x00)
#define dmm_in_con1         eva_wsfr(cid_dmm, 0x01)
#define dmm_in_con2         eva_wsfr(cid_dmm, 0x02)
#define dmm_addr_base       eva_wsfr(cid_dmm, 0x03)
#define dmm_out_con1        eva_wsfr(cid_dmm, 0x04)
#define dmm_out_con2        eva_wsfr(cid_dmm, 0x05)
#define dmm_out_con3        eva_wsfr(cid_dmm, 0x06)
#define dmm_bg_color        eva_wsfr(cid_dmm, 0x07)
#define dmm_ccm0_con        eva_wsfr(cid_dmm, 0x08)
#define dmm_ccm0_con1       eva_wsfr(cid_dmm, 0x09)
#define dmm_ccm0_con2       eva_wsfr(cid_dmm, 0x0a)
#define dmm_ccm0_con3       eva_wsfr(cid_dmm, 0x0b)
#define dmm_ccm1_con        eva_wsfr(cid_dmm, 0x0c)
#define dmm_ccm1_con1       eva_wsfr(cid_dmm, 0x0d)
#define dmm_ccm1_con2       eva_wsfr(cid_dmm, 0x0e)
#define dmm_ccm1_con3       eva_wsfr(cid_dmm, 0x0f)
#define dmm_ccm2_con        eva_wsfr(cid_dmm, 0x10)
#define dmm_ccm2_con1       eva_wsfr(cid_dmm, 0x11)
#define dmm_ccm2_con2       eva_wsfr(cid_dmm, 0x12)
#define dmm_ccm2_con3       eva_wsfr(cid_dmm, 0x13)
#define dmm_lut_r_ptr       eva_wsfr(cid_dmm, 0x14)
#define dmm_lut_r_dat       eva_wsfr(cid_dmm, 0x15)
#define dmm_lut_g_ptr       eva_wsfr(cid_dmm, 0x16)
#define dmm_lut_g_dat       eva_wsfr(cid_dmm, 0x17)
#define dmm_lut_b_ptr       eva_wsfr(cid_dmm, 0x18)
#define dmm_lut_b_dat       eva_wsfr(cid_dmm, 0x19)
#define dmm_test_con        eva_wsfr(cid_dmm, 0x1a)
#define dmm_out_con4        eva_wsfr(cid_dmm, 0x1b)

//===============================================================================//
//
//      dpi
//
//===============================================================================//
#define dpi_clk_con      eva_wsfr(cid_dpi, 0x00)
#define dpi_io_con       eva_wsfr(cid_dpi, 0x01)
#define dpi_con          eva_wsfr(cid_dpi, 0x02)
#define dpi_fmt          eva_wsfr(cid_dpi, 0x03)
#define dpi_emi          eva_wsfr(cid_dpi, 0x04)
#define dpi_htt_cfg      eva_wsfr(cid_dpi, 0x05)
#define dpi_hsw_cfg      eva_wsfr(cid_dpi, 0x06)
#define dpi_hst_cfg      eva_wsfr(cid_dpi, 0x07)
#define dpi_haw_cfg      eva_wsfr(cid_dpi, 0x08)
#define dpi_vtt_cfg      eva_wsfr(cid_dpi, 0x09)
#define dpi_vsw_cfg      eva_wsfr(cid_dpi, 0x0a)
#define dpi_vst_o_cfg    eva_wsfr(cid_dpi, 0x0b)
#define dpi_vst_e_cfg    eva_wsfr(cid_dpi, 0x0c)
#define dpi_vaw_cfg      eva_wsfr(cid_dpi, 0x0d)

//===============================================================================//
//
//      csi
//
//===============================================================================//
#define csi_sys_con     eva_wsfr(cid_csi, 0x0)
#define csi_rmap_con    eva_wsfr(cid_csi, 0x1)
#define csi_lane_con    eva_wsfr(cid_csi, 0x2)
#define csi_tval_con    eva_wsfr(cid_csi, 0x3)
#define csi_task_con    eva_wsfr(cid_csi, 0x4)
#define csi_task_haw    eva_wsfr(cid_csi, 0x5)
#define csi_task_vaw    eva_wsfr(cid_csi, 0x6)
#define csi_phy_con0    eva_msfr(cid_csi, 0x7, t_csi_phy_con0)
#define csi_phy_con1    eva_msfr(cid_csi, 0x8, t_csi_phy_con1)
#define csi_phy_con2    eva_msfr(cid_csi, 0x9, t_csi_phy_con2)

//===============================================================================//
//
//      dsi
//
//===============================================================================//
#define dsi_sys_con     eva_wsfr(cid_dsi, 0x00)

#define dsi_vdo_fmt     eva_wsfr(cid_dsi, 0x10)
#define dsi_vdo_vsa     eva_wsfr(cid_dsi, 0x11)
#define dsi_vdo_vbp     eva_wsfr(cid_dsi, 0x12)
#define dsi_vdo_vact    eva_wsfr(cid_dsi, 0x13)
#define dsi_vdo_vfp     eva_wsfr(cid_dsi, 0x14)
#define dsi_vdo_hsa     eva_wsfr(cid_dsi, 0x15)
#define dsi_vdo_hbp     eva_wsfr(cid_dsi, 0x16)
#define dsi_vdo_hact    eva_wsfr(cid_dsi, 0x17)
#define dsi_vdo_hfp     eva_wsfr(cid_dsi, 0x18)
#define dsi_vdo_bllp0   eva_wsfr(cid_dsi, 0x19)
#define dsi_vdo_bllp1   eva_wsfr(cid_dsi, 0x1a)
#define dsi_frm_tval    eva_wsfr(cid_dsi, 0x1b)

#define dsi_tval_con0   eva_wsfr(cid_dsi, 0x20)
#define dsi_tval_con1   eva_wsfr(cid_dsi, 0x21)
#define dsi_tval_con2   eva_wsfr(cid_dsi, 0x22)
#define dsi_tval_con3   eva_wsfr(cid_dsi, 0x23)
#define dsi_tval_con4   eva_wsfr(cid_dsi, 0x24)
#define dsi_tval_con5   eva_wsfr(cid_dsi, 0x25)
#define dsi_rmap_con    eva_wsfr(cid_dsi, 0x26)

#define dsi_pll_con0    eva_msfr(cid_dsi, 0x30, t_dsi_pll_con0)
#define dsi_pll_con1    eva_msfr(cid_dsi, 0x31, t_dsi_pll_con1)
#define dsi_phy_con0    eva_msfr(cid_dsi, 0x32, t_dsi_phy_con0)
#define dsi_phy_con1    eva_msfr(cid_dsi, 0x33, t_dsi_phy_con1)
#define dsi_phy_con2    eva_msfr(cid_dsi, 0x34, t_dsi_phy_con2)
#define dsi_phy_con3    eva_wsfr(cid_dsi, 0x35)

#define dsi_task_con    eva_wsfr(cid_dsi, 0x40)
#define dsi_bus_con     eva_wsfr(cid_dsi, 0x41)
#define dsi_cmd_con0    eva_wsfr(cid_dsi, 0x42)
#define dsi_cmd_con1    eva_wsfr(cid_dsi, 0x43)
#define dsi_cmd_con2    eva_wsfr(cid_dsi, 0x44)
#define dsi_cmd_fifo    eva_wsfr(cid_dsi, 0x45)
#define dsi_lane_con    eva_wsfr(cid_dsi, 0x46)

//===============================================================================//
//
//      mcv
//
//===============================================================================//
#define mcv_scon        eva_wsfr(cid_mcv, 0x00)
#define mcv_mcon        eva_wsfr(cid_mcv, 0x01)
#define mcv0_dcon       eva_wsfr(cid_mcv, 0x02)
#define mcv0_vact       eva_wsfr(cid_mcv, 0x03)
#define mcv0_hact       eva_wsfr(cid_mcv, 0x04)
#define mcv1_dcon       eva_wsfr(cid_mcv, 0x05)
#define mcv1_vact       eva_wsfr(cid_mcv, 0x06)
#define mcv1_hact       eva_wsfr(cid_mcv, 0x07)
#define mcv2_dcon       eva_wsfr(cid_mcv, 0x08)
#define mcv2_vact       eva_wsfr(cid_mcv, 0x09)
#define mcv2_hact       eva_wsfr(cid_mcv, 0x0a)
#define mcv3_dcon       eva_wsfr(cid_mcv, 0x0b)
#define mcv3_vact       eva_wsfr(cid_mcv, 0x0c)
#define mcv3_hact       eva_wsfr(cid_mcv, 0x0d)

//===============================================================================//
//
//     scale
//
//===============================================================================//
#define scale_con          eva_wsfr(cid_gp_sca, 0x00)
#define scale_in_con       eva_wsfr(cid_gp_sca, 0x01)
#define scale_in_stride    eva_wsfr(cid_gp_sca, 0x02)
#define scale_in_base      eva_wsfr(cid_gp_sca, 0x03)
#define scale_w_step       eva_wsfr(cid_gp_sca, 0x04)
#define scale_h_step       eva_wsfr(cid_gp_sca, 0x05)
#define scale_w_offset     eva_wsfr(cid_gp_sca, 0x06)
#define scale_h_offset     eva_wsfr(cid_gp_sca, 0x07)
#define scale_out_con      eva_wsfr(cid_gp_sca, 0x08)
#define scale_out_stride   eva_wsfr(cid_gp_sca, 0x09)
#define scale_out_base     eva_wsfr(cid_gp_sca, 0x0a)



#endif
