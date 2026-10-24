/*****************************************************************
>file name : lib/system/cpu/dv16/video-dev/jpeg/jpeg_codec.h
>author : lichao
>create time : Thu 08 Jun 2017 04:13:07 PM HKT
*****************************************************************/

#ifndef _JPEG_CODEC_H_
#define _JPEG_CODEC_H_
#include "typedef.h"
#include "video/video.h"

struct jpg_dec_attr {
    u16 max_o_width;
    u16 max_o_height;
};

enum {
    JPEG_DEV_ID_0 = 0,
    JPEG_DEV_ID_1,
};

enum {
    SOURCE_FROM_IMC,
    SOURCE_FROM_DDR,
};

enum jpeg_enc_mode {
    JPEG_ENC_TYPE_NONE = 0x0,
    JPEG_ENC_TYPE_STREAM,
    JPEG_ENC_TYPE_IMAGE,
    JPEG_ENC_AUTO_STREAM,
    JPEG_ENC_MANU_STREAM,
    JPEG_ENC_AUTO_IMAGE,
    JPEG_ENC_MANU_IMAGE,
};

enum codec_for_pipe_state {
    JPEG_CODEC_FOR_ONLINE = 0x0100,
    JPEG_CODEC_FOR_DEC    = 0x0011,
    JPEG_CODEC_FOR_ENC    = 0x0010,
    JPEG_CODEC_FOR_IDEAL  = 0X0000,
};



#define AUTO_STREAM_CAPTURE     0x0
#define MANU_STREAM_CAPTURE     0x1


struct jpeg_reg {
    volatile u32 CON0            ;
    volatile u32 CON1            ;
    volatile u32 CON2            ;
    volatile u32 YDCVAL          ;        //write only
    volatile u32 UDCVAL          ;        //write only
    volatile u32 VDCVAL          ;        //write only
    volatile u32 YPTR0           ;        //write only
    volatile u32 UPTR0           ;        //write only
    volatile u32 VPTR0           ;        //write only
    volatile u32 YPTR1           ;        //write only
    volatile u32 UPTR1           ;        //write only
    volatile u32 VPTR1           ;        //write only
    volatile u32 BADDR           ;        //write only
    volatile u32 BCNT            ;
    volatile u32 MCUCNT          ;
    volatile u32 PRECNT          ;        //write only
    volatile u32 YUVLINE         ;        //write only
    volatile u32 CFGRAMADDR      ;        //write only
//u32 rev[0x1c00-0x12] ;
    volatile u32 CFGRAMVAL       ;  //0x1c00*4
    volatile u32 PTR_NUM         ;
    volatile u32 HAS_BUF;
    volatile u32 MCU_BUF;
};

enum mjpg_irq_code {
    MJPG_IRQ_PRECNT,
    MJPG_IRQ_BUFFER_FULL,
    MJPG_IRQ_SPEED_INTEN,
    MJPG_IRQ_NO_BUFF,
    MJPG_IRQ_MCU_PEND,
};

enum mjpg_frame_type {
    MJPG_STREAM_FRAME,
    MJPG_SINGLE_FRAME,
};

struct mjpg_user_ops {
    void   *(*malloc)(void *_video, u32 size);
    void   *(*realloc)(void *_video, void *fb, int newsize);
    void (*free)(void *_video, void *fb);
    int (*size)(void *fb);
    int (*free_size)(void *_video);
    void (*output_frame_end)(void *_video, void *buf);
};

struct jpeg_yuv {
    u8 *y;
    u8 *u;
    u8 *v;
};

#define JPG_SAMP_FMT_INVALID     0x0 //
#define JPG_SAMP_FMT_YUV444      0x1 //
#define JPG_SAMP_FMT_YUV422      0x2 //
#define JPG_SAMP_FMT_YUV420      0x3 //

#define BITS_MODE_CACHE    0
#define BITS_MODE_UNCACHE  1

#define SINGLE_BUF_MODE    0x0
#define DOUBLE_BUF_MODE    0x1
#define CIRCLE_BUF_MODE    0x2
#define WHOLE_BUF_MODE     0x3

#define INSERT_EMPTY_FRAME      0x1
#define INSERT_SPECAIL_FRAME    0x2
#define INVALID_Q_VAL           0xff

struct jpeg_decoder_param {
    u8  bits_mode;
    u8  out_mode;
    u8  output_format;
    u8  yuv_type;
    void *priv;
    int (*yuv_out_dest)(void *priv, void *arg);
    int (*reset_output)(void *priv);
    u8  *cbuf;
    int line_cnt;
};

struct jpeg_basic_info {
    int width;
    int height;
    int align_width;
    int align_height;
    int format;
};

enum {
    JPEG_INPUT_TYPE_FILE,
    JPEG_INPUT_TYPE_DATA,
};

enum {
    JPEG_DECODE_TYPE_YUV420 = 0,
    JPEG_DECODE_TYPE_DEFAULT,
};

enum {
    JPEG_DECODE_OUTPUT_PLANAR_YUV = 0,
    JPEG_DECODE_OUTPUT_PACKED_LINEBASE_YUYV,
    JPEG_DECODE_OUTPUT_PACKED_LINEBASE_UYVY,
    JPEG_DECODE_OUTPUT_PACKED_TILEBASE_YUYV,
    JPEG_DECODE_OUTPUT_PACKED_TILEBASE_UYVY,
};

#define DEC_YUV_ALIGN_SIZE(size)  (((size) + 32 - 1) & ~(32 - 1))
struct decoder_yuv_out {
    int line;
    int width;
    int total_line;
    struct jpeg_yuv *yuv;
    u8 frame_begin;
    u8 mode;//
    u8 yuv_type;
    int file_pos;
};

struct jpeg_file {
    const char *name;
};

struct jpeg_data {
    u8 *buf;
    u32 len;
};


struct jpeg_decode_req {
    u8 input_type;
    u8 output_type; //0 -- yuv420, 1 -- original mode
    u8 bits_mode;
    u8 output_format;
    union {
        struct jpeg_file file;
        struct jpeg_data data;
    } input;
    u8 *buf_y;
    u8 *buf_u;
    u8 *buf_v;
    u16 buf_width;
    u16 buf_height;
    u16 buf_xoffset;
    u16 buf_yoffset;
    u16 out_width;
    u16 out_height;
    u16 out_xoffset;
    u16 out_yoffset;
    void *priv;
    void (*stream_end)(void *priv);
};

struct jpeg_image_info {
    union {
        struct jpeg_data data;
    } input;
    int sample_fmt;
    u16 width;
    u16 height;
};

struct jpeg_encode_req {
    u8 format;
    u8 q;
    struct jpeg_data data;
    u8 *y;
    u8 *u;
    u8 *v;
    u16 width;
    u16 height;
};

struct jpeg_codec_handle {
    int id;
    volatile int state;
    u32 pipe_state;
    int timer;
    struct jpeg_reg *reg;
    struct list_head encoder;
    struct list_head decoder;
    int (*frame_end)(void *priv, void *fb);
    OS_SEM sem;
    OS_SEM sync_sem;
};

#define jpg_be16_to_cpu(x) (((x)[0] << 8) | (x)[1])


int jpeg_codec_init(void);
/*
 * jpeg encode
 * JPEG编码使用函数
 */
int jpeg_encode_one_image(struct jpeg_encode_req *req);
int jpeg_manual_encode_frame(void *fd, struct jpeg_encode_req *req);
int jpeg_get_codec_state(u8 id);

int mjpg_handl2ch(void *_fh);
void *mjpg_enc_open(void *_info, enum jpeg_enc_mode mode);
void *mjpg_enc_force_open(void *_info, enum jpeg_enc_mode mode, u8 id);
int mjpg_enc_get_s_attr(void *_fh, struct mjpg_s_attr *attr);
int mjpg_enc_set_s_attr(void *_fh, struct mjpg_s_attr *attr);

int mjpg_enc_get_d_attr(void *_fh, struct mjpg_d_attr *attr);
int mjpg_enc_set_d_attr(void *_fh, struct mjpg_d_attr *attr);
void mjpg_enc_set_imc(void *_fh, void *imc);
void *mjpg_enc_get_imc(void *_fh);

int mjpg_enc_set_output_fps(void *_fh, int fps);

int mjpg_enc_set_input_buf(void *_fh, struct video_cap_buffer *b);

void mjpg_enc_set_output_buf_ops(void *_fh, void *, const struct mjpg_user_ops *ops);

int mjpg_enc_set_irq_handler(void *_fh, void *priv,
                             int (*handler)(void *, enum mjpg_irq_code, enum mjpg_frame_type));
int mjpg_enc_start(void *_fh);

int mjpg_enc_pause(void *fd);

int mjpg_enc_resume(void *fd);

int mjpg_enc_kstart(void *_fh);
int mjpg_enc_mline_kick(void *_fh);

int mjpg_enc_wait_stop(void *_fh);

int mjpg_enc_stop(void *_fh);
int mjpg_start_next_frame(void *_fh);
u8 mjpg_image_check_channel_free();
int mjpg_enc_close(void *_fh);

void *mjpg_image_enc_open(void *_info, enum jpeg_enc_mode mode);
void *mjpg_image_enc_force_open(void *_info, enum jpeg_enc_mode mode, int id);
int mjpg_image_enc_start(void *_fh, struct YUV_frame_data *input_frame, u8 *bits_buf, int buf_len, u8 q_val);
int mjpg_enc_set_thumbnails(void *_fh, void *thumbnail, int size);
void *mjpg_get_auto_stream_encoder(void *_fh);
int mjpg_stream_reset_bits_rate(void *fd, u32 bits_rate);
int mjpg_manu_enc_check_space(void *fd, u32 size);
int mjpg_dri_enc_start(void *_fh, struct YUV_frame_data *input_frame, u8 *bits_buf, int buf_len, u8 q_val);
/*
 * jpeg decode
 * JPEG解码使用函数
 */
u8 *find_jpg_frame(u8 *buf, int limit);
u8 *find_jpg_frame_detail(u8 *buf, int limit, struct jpeg_basic_info *info);
void *jpeg_decode_open(void *arg);
int jpeg_decode_reset_param(void *fd, void *arg);
int jpeg_decode_close(void *fd);
int jpeg_decode_reset(void *fd, void *arg);
int jpeg_decode_start(void *fd, u8 *buf, int len);
int jpeg_decode_release(void *fd);
int jpeg_dec_change_omode(void *fd, void *buf, u8 mode);
int jpeg_dec_manual_start(void *fd, u8 *buf, int len);
int jpeg_decode_reset_bits(void *fd, u8 *buf, int len);

int jpeg_yuv_to_yuv420(struct jpeg_yuv *src_yuv, struct jpeg_yuv *dst_yuv, u16 stride, u16 image_w, u16 out_w, u8 yuv_type, u16 lines);
int jpeg_decode_image_info(struct jpeg_image_info *info);
int jpeg_decode_one_image(struct jpeg_decode_req *req, u8 id);
int jpeg_yuv_to_yuv420(struct jpeg_yuv *src_yuv, struct jpeg_yuv *dst_yuv, u16 stride, u16 image_w, u16 out_w, u8 yuv_type, u16 lines);
void *jpeg_decode_force_open(void *arg, int id);

#endif

