#ifndef _MIPI_SFR_H_
#define _MIPI_SFR_H_

#define __RW               volatile       // read write
#define __RO               volatile const // only read
#define __WO               volatile       // only write

//======================================================================
//
//      DSI
//
//======================================================================
typedef struct {
    __RW ___u32 enable               :  1; // [0]        pll enable
    __RW ___u32 reset                :  1; // [1]        pll reset (0 = reset)
    __RW ___u32 drven                :  1; // [2]
    __RW ___u32 ckoe                 :  1; // [3]        pll output enable
    __RW ___u32 ref_div_en           :  1; // [4]        ref_clk devide enable
    __RW ___u32 ref_div              :  6; // [10:5]     nf  = (ref_div + 2)
    __RW ___u32 fdb_div              : 10; // [20:11]    nr  = (fdb_div + 2)*2
    __RW ___u32 out_div              :  2; // [22:21]    out = 2^out_div
    __RW ___u32 /*reserved*/         :  1; // [23]
    __RW ___u32 pfd                  :  2; // [25:24]    default: 0b01
    __RW ___u32 icp                  :  2; // [27:26]    default: 0b01
    __RW ___u32 rlf                  :  2; // [29:28]    default: 0b10
    __RW ___u32 sys_ckoe             :  1; // [30]       output to system enable
} t_dsi_pll_con0;

typedef struct {
    __RW ___u32 vcos                 :  3; // [2:0]      default: 0b100
    __RW ___u32 test_oe              :  1; // [3]
    __RW ___u32 test_sel             :  2; // [5:4]
} t_dsi_pll_con1;

typedef struct {
    __RW ___u32 bandgap_en           :  1; // [0]        bandgap enable
    __RW ___u32 vti_en               :  1; // [1]        vti enable
    __RW ___u32 vref_en              :  1; // [2]        voltage reference enable
    __RW ___u32 mpldo_en             :  1; // [3]        ldo enable
    __RW ___u32 vti_restrim          :  4; // [7:4]      default: 0b0011
    __RW ___u32 vih_sel              :  2; // [9:8]      default: 0b01
    __RW ___u32 vil_sel              :  2; // [11:10]    default: 0b01
    __RW ___u32 ldo_trim             :  3; // [14:12]    default: 0b000
    __RW ___u32 v400m_trim           :  3; // [17:15]    default: 0b100

    __RW ___u32 mp_test_oe           :  1; // [18]
    __RW ___u32 vbg_test_en          :  1; // [19]
    __RW ___u32 phy_vitest_sel       :  3; // [22:20]
    __RW ___u32 phy_hsrx0outtest_en  :  1; // [23]
    __RW ___u32 txphy_lprx0outn_en   :  1; // [24]
    __RW ___u32 txphy_lprx0outp_en   :  1; // [25]
    __RW ___u32 mipi_ana_tsen        :  1; // [26]
} t_dsi_phy_con0;

typedef struct {
    __RW ___u32 hstx_iccrtrim        :  4; // [3:0]      default: 0b0011
    __RW ___u32 hstx_nonoverdlytrim  :  4; // [7:4]      default: 0b0100
    __RW ___u32 hstx_trftrim         :  5; // [12:8]     default: 0b00011
} t_dsi_phy_con1;

typedef struct {
    __RW ___u32 lptx_srtrim          :  4; // [3:0]      default: 0b0100
    __RW ___u32 lptx_nrestrim        :  3; // [6:4]      default: 0b111
    __RW ___u32 lptx_prestrim        :  3; // [9:7]      default: 0b111
    __RW ___u32 lprx_lpfbwtrim       :  4; // [13:10]    default: 0b0000
    __RW ___u32 lprx_trigtrim        :  3; // [16:14]    default: 0b000
    __RW ___u32 lprx_bwtrim          :  3; // [19:17]    default: 0b000
} t_dsi_phy_con2;

typedef struct {
    __RW ___u32 lane0_bias_en        :  1; // [0]
    __RW ___u32 lane0_hs_en          :  1; // [1]
    __RW ___u32 lane0_icc_en         :  1; // [2]
    __RW ___u32 lane0_lp_en          :  1; // [3]

    __RW ___u32 lane1_bias_en        :  1; // [4]
    __RW ___u32 lane1_hs_en          :  1; // [5]
    __RW ___u32 lane1_icc_en         :  1; // [6]
    __RW ___u32 lane1_lp_en          :  1; // [7]

    __RW ___u32 lane2_bias_en        :  1; // [8]
    __RW ___u32 lane2_hs_en          :  1; // [9]
    __RW ___u32 lane2_icc_en         :  1; // [10]
    __RW ___u32 lane2_lp_en          :  1; // [11]

    __RW ___u32 lane3_bias_en        :  1; // [12]
    __RW ___u32 lane3_hs_en          :  1; // [13]
    __RW ___u32 lane3_icc_en         :  1; // [14]
    __RW ___u32 lane3_lp_en          :  1; // [15]

    __RW ___u32 lane4_bias_en        :  1; // [16]
    __RW ___u32 lane4_hs_en          :  1; // [17]
    __RW ___u32 lane4_icc_en         :  1; // [18]
    __RW ___u32 lane4_lp_en          :  1; // [19]

    __RO ___u32 lane0_ld_dpi         :  1; // [20]
    __RO ___u32 lane0_ld_dni         :  1; // [21]
    __RO ___u32 lane1_ld_dpi         :  1; // [22]
    __RO ___u32 lane1_ld_dni         :  1; // [23]
    __RO ___u32 lane2_ld_dpi         :  1; // [24]
    __RO ___u32 lane2_ld_dni         :  1; // [25]
    __RO ___u32 lane3_ld_dpi         :  1; // [26]
    __RO ___u32 lane3_ld_dni         :  1; // [27]
    __RO ___u32 lane4_ld_dpi         :  1; // [28]
    __RO ___u32 lane4_ld_dni         :  1; // [29]
} t_dsi_phy_con3;

//======================================================================
//
//      CSI
//
//======================================================================
typedef struct {
    __RW ___u32 test_oe              :  1; // [0]
    __RW ___u32 test_sel             :  4; // [5:2]
} t_csi_phy_con0;

typedef struct {
    __RW ___u32 hsrx_ctermtrim       :  3; // [2:0]      default: 0b010
    __RW ___u32 hsrx_rtermtrim       :  5; // [7:3]      default: 0b00011
    __RW ___u32 lprx_lpfbwtrm        :  4; // [11:8]     default: 0b0000
    __RW ___u32 lprx_trigtrm         :  3; // [14:12]    default: 0b000
    __RW ___u32 lprx_bwtrm           :  3; // [17:15]    default: 0b000
} t_csi_phy_con1;

typedef struct {
    __RW ___u32 x0_bias_en           :  1; // [0]
    __RW ___u32 x0_lpf_en            :  1; // [1]
    __RW ___u32 x0_lp_die            :  1; // [2]
    __RW ___u32 x0_hs_en             :  1; // [3]

    __RW ___u32 x1_bias_en           :  1; // [4]
    __RW ___u32 x1_lpf_en            :  1; // [5]
    __RW ___u32 x1_lp_die            :  1; // [6]
    __RW ___u32 x1_hs_en             :  1; // [7]

    __RW ___u32 x2_bias_en           :  1; // [8]
    __RW ___u32 x2_lpf_en            :  1; // [9]
    __RW ___u32 x2_lp_die            :  1; // [10]
    __RW ___u32 x2_hs_en             :  1; // [11]

    __RW ___u32 x3_bias_en           :  1; // [12]
    __RW ___u32 x3_lpf_en            :  1; // [13]
    __RW ___u32 x3_lp_die            :  1; // [14]
    __RW ___u32 x3_hs_en             :  1; // [15]

    __RW ___u32 x4_bias_en           :  1; // [16]
    __RW ___u32 x4_lpf_en            :  1; // [17]
    __RW ___u32 x4_lp_die            :  1; // [18]
    __RW ___u32 x4_hs_en             :  1; // [19]
} t_csi_phy_con2;

#endif
