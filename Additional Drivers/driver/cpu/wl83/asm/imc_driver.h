#ifndef IMC_DRIVER_H
#define IMC_DRIVER_H



#include "typedef.h"
#include "system/includes.h"
#include "video/video.h"


//#define IMC_CHANNEL_NUM  	3
#define IMC_ENC_CH_NUM  	2
#define IMC_DISP_CH_NUM  	2

enum imc_src_type {
    IMC_SRC0 = 1,
    IMC_SRC1 = 2,
    IMC_SRC2,
    IMC_SRC_MCV0,
    IMC_SRC_MCV1,
    IMC_SRC_MCV2,
    IMC_SRC_MCV3,
    IMC_SRC_REP,
    IMC_SRC_REP2,
};



// enum imc_isp_sel {
//     IMC_SRC_SEL_ISP0 = 1,
//     IMC_SRC_SEL_ISP1,
//     IMC_SRC_SEL_MANUAL,
//
// };

enum imc_yuv_mode {
    IMC_YUV420 = 0,
    IMC_YUV422,
};

enum imc_axi_max_len {
    IMC_512B = 0,
    IMC_256B,
};

enum imc_event {
    IMC_EVENT_ERR_IRQ,
    IMC_EVENT_FRAME_IRQ,
    IMC_EVENT_LINE_IRQ,
    IMC_EVENT_PAUSE,
    IMC_EVENT_RESUME,
    IMC_EVENT_RESET,
};

enum imc_irq_mode_e {
    IMC_IRQ_M_DISABLE,
    IMC_IRQ_M_16LINE = 0x1,
    IMC_IRQ_M_FRAME = 0x2,
    /*
     * 按位增加
     * 0x4,
     * 0x8,
     * ...
     */
};

enum output_mode {
    IMC_ONE_FRAME_MODE = 0,
    IMC_DOUBLE_BUF_MODE,
    IMC_CYCLE_BUF_MODE,
};

enum output_module {
    IMC_OUT_MODULE_NONE = 0,
    IMC_OUT_MODULE_JPEG0,
    IMC_OUT_MODULE_JPEG1,
    // IMC_OUT_MODULE_IMR0,
    // IMC_OUT_MODULE_IMR1,
};


struct video_source_crop {
    int x_offset;
    int y_offset;
    int width;
    int height;
};

struct imc_dis_s_attr {
    u8 down_smp;
    u8 src;
    // u8 src_isp_sel;
    u16 src_w;
    u16 src_h;
    u16 out_width;
    u16 out_height;
    u16 max_i_width;
    u16 max_o_width;
    enum imc_irq_mode_e irq_mode;
};

struct imc_enc_s_attr {
    u8  mode;
    u8  camera_type;
    u8  fps;
    u16 src_w;
    u16 src_h;
    u16 width;
    u16 height;
    enum imc_src_type src;
    // enum imc_isp_sel src_isp_sel;
};

struct imc_s_attr {
    enum imc_src_type src;
    // enum imc_isp_sel src_isp_sel;
    u8 down_smp;
    u8 mode;
    u8 camera_type;
    u8 fps;
    u32 real_fps;/*camera 实际帧率(浮点)q16) !!!*/
    u16 src_w;
    u16 src_h;
    u16 width;
    u16 height;
    u16 pixformat;
    u16 max_i_width;
    u16 max_o_width;
    u16 out_array;
    enum imc_irq_mode_e irq_mode;
    int manu_rep;
};


struct imc_iattr {
    enum imc_src_type src;
    u32 src_w;
    u32 src_h;
    u32 width;
    u32 height;
    u8 enc_num;//联动的编码器
    u8 *buf;
    u8 *inbuf;
    u8 format;
    u8 rep_yuvformat;
    u8 imc_buf_mode;
    u16 crop_x;
    u16 crop_y;
    u16 crop_width;
    u16 crop_height;
    u8 test_crop;
    u8 mirror;

    u8 in_fps;
    u8 out_fps;

    u8 osd_en;

    // u8 tpz_en;
    // u8 tpz_dir;
    // u32 img_dummy;


};

#if 0

int imc_handl2ch(void *_hdl);

void *imc_enc_ch_open(void *);

int imc_enc_set_output_buf(void *fh, void *);
int imc_enc_switch_output_buf(void *_fh, void *);

int imc_enc_set_output_module(void *_fh, int ch);
int imc_enc_clr_hareware_output_module(void *_fh, int ch);
int imc_enc_set_hareware_output_module(void *_fh, int ch);

int imc_set_event_handler(void *_fh, void *priv,
                          int (*handler)(void *, enum imc_event, void *));

int imc_enc_reset_buffer(void *_fh);
int imc_enc_reset_down_buffer(void *_fh);
int imc_enc_reset_up_buffer(void *_fh);

int imc_enc_ch_start(void *_fh);
int imc_enc_ch_stop(void *_fh);

int imc_enc_ch_close(void *_fh);

struct imc_d_attr {
    u8 output_fps;
};

struct video_text_osd;
struct video_osd_attr;

int imc_set_d_attr(void *_fh, struct imc_d_attr *attr);

int imc_get_s_attr(void *_fh, struct imc_s_attr *attr);
int imc_set_s_attr(void *_fh, struct imc_s_attr *attr);

int imc_enc_set_output_fps(void *_fh, int fps, int tlp_time);
int imc_get_mirror(void *_fh);
int imc_set_mirror(void *_fh, u32 mirror);
int imc_set_crop(void *_fh, void *arg);
int imc_get_jaggy(void *_fh);
int imc_set_jaggy(void *_fh, u32 jaggy);
int imc_set_output_module(void *_fh, int module);

int imc_enc_get_osd(void *_fh, struct video_text_osd *osd);
int imc_enc_set_osd(void *_fh, struct video_text_osd *osd);
int imc_enc_set_osd_enable(void *_fh);
int imc_enc_set_osd_disable(void *_fh);
int imc_enc_ch_pause(void *_fh);
int imc_enc_ch_resume(void *_fh, void *);
int imc_enc_get_osd_attr(void *_fh, struct video_osd_attr *attr);
int imc_enc_set_osd_attr(void *_fh, struct video_osd_attr *attr);

int imc_enc_ch_get_capability(void *_fh);
int imc_enc_ch_stop_image_cap(void *_fh);
int imc_enc_image_scale(void *_fh, struct image_scale_data *scale_data);
int imc_get_image_text_osd(void *_fh, struct video_text_osd *text_osd);
int imc_enc_pause_image_osd(void *_fh);
int imc_enc_resume_image_osd(void *_fh);

#endif


/*
struct imc_manu_i_data {
    u8 proc_wait;
    u8 post_wait;
    struct YUV_frame_data *frame;
};
*/

struct imc_manu_i_data {
    u8 restart_ch;
    u8 proc_wait;
    u8 post_wait;
    struct YUV_frame_data *frame;
};

#if 0
void *imc_dis_ch_open(void *);

int imc_get_s_attr(void *, struct imc_s_attr *attr);

int imc_set_s_attr(void *, struct imc_s_attr *attr);

int imc_dis_set_output_buf(void *_fh, u8 *yaddr, u8 *uaddr, u8 *vaddr);

int imc_dis_set_pingpong_buf(void *_fh, struct fb_map_user *map0, struct fb_map_user *map1);

int imc_dis_set_output_module(void *_fh, int ch);

int imc_dis_ch_input_data(void *_fh, struct imc_manu_i_data *i_data);

int imc_set_irq_handler(void *_fh, void *priv, int (*handler)(void *));

int imc_dis_ch_start(void *_fh);

int imc_dis_ch_stop(void *_fh);

int imc_dis_ch_close(void *_ch);

int imc_manu_src_input_data(void *_fh, struct imc_manu_i_data *i_data);
int imc_dis_ch_input_data(void *_fh, struct imc_manu_i_data *i_data);
int imc_dis_ch_max_width(void *_fh);

int imc_set_rep_used(u8 src_rep, u8 status);
int imc_get_free_rep();



int imc_mch_init(const char *name, const struct video_platform_data *data);
int imc_mch_get_ch(void *_fh);
void *imc_mch_get_fh(void *_fh);
void *imc_mch_open(struct video_format *f, int ch_num, u8 type);
int imc_mch_max_width(void *_fh);
int imc_mch_get_s_attr(void *_fh, struct imc_s_attr *attr);
int imc_mch_set_s_attr(void *_fh, struct imc_s_attr *attr);
int imc_mch_set_irq_handler(void *_fh, void *priv, int (*handler)(void *));
int imc_mch_set_output_buf(void *_fh, u8 *yaddr, u8 *uaddr, u8 *vaddr);
int imc_mch_set_event_handler(void *_fh, void *priv,
                              int (*handler)(void *, enum imc_event, void *));
int imc_mch_handl2ch(void *_fh);
int imc_mch_set_output_module(void *_fh, int ch);
int imc_mch_set_output_fps(void *_fh, int fps, int tlp_time);
int imc_mch_set_mirror(void *_fh, u32 mirror);
int imc_mch_set_jaggy(void *_fh, u32 jaggy);
int imc_mch_get_osd_attr(void *_fh, struct video_osd_attr *attr);
int imc_mch_set_osd_attr(void *_fh, struct video_osd_attr *attr, void *soft_osd_cfg, int num);
int imc_mch_start(void *_fh);
int imc_mch_stop(void *_fh, int mode);
int imc_mch_close(void *_fh);
int imc_mch_image_scale(void *_fh, struct image_scale_data *scale_data);
int imc_mch_ch_pause(void *_fh);
int imc_mch_ch_resume(void *_fh, void *);
int imc_mch_ch_capture_one_frame(void *_fh, struct yuv_image *image);
int imc_mch_get_image_text_osd(void *_fh, struct video_text_osd *text_osd);
int imc_mch_pause_image_osd(void *_fh);
int imc_mch_resume_image_osd(void *_fh);
int imc_mch_ch_image_capture(void *_fh, struct video_image_capture *icap);
int imc_mch_ch_stop_image_cap(void *_fh);
int imc_mch_rep_one_frame(void *_fh, struct imc_manu_i_data *i_data);
int imc_mch_osd_start(void *_fh);
int imc_mch_osd_stop(void *_fh);
void imc_mch_insert_osd(void *soft_osd_cfg, u8 *buf, struct sys_time *ftime);
int imc_mch_get_sosd_size();
int imc_mch_get_osd_status(void *soft_osd_cfg);
int imc_mch_set_ftype(void *_fh, int type);
int imc_mch_get_rtc_time(void *soft_osd_cfg, struct sys_time *rtime);
int imc_mch_count_time(struct sys_time *time, struct sys_time *base_time, int secs);






int imc_manual_scale(struct image_scale_data *data); //手动缩放接口
int imc_enc_dou_start(void *imc, void *subimc, u8 osd_on_right);
void imc_set_xbus_lvl(void *_fh, u8 lvl);



#endif










#endif

