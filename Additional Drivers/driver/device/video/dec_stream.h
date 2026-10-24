#ifndef __DEC_STREAM_H__
#define __DEC_STREAM_H__


#include "system/includes.h"


enum vdec_stream_out_type {
    OUT_DISPLAY,
    OUT_YUVDATA,
};

enum {
    VDEC_STREAM_STOP,
    VDEC_STREAM_START,
};
enum {
    ADEC_STREAM_STOP,
    ADEC_STREAM_START,
};

struct vdec_stream_info {
    const char *stream_name; //"h264" or "jpeg"
    enum vdec_stream_out_type out_type;

    //输入端
    void *priv;
    int (*wait_get_frame)(void *priv, struct video_buffer *b); /* 获取一帧数据,但还没释放buf */
    void (*set_free_frame)(void *priv, struct video_buffer *b); /* 释放这一帧数据占据的buf */
    void (*dec_err_end)(void *priv); //解码失败调用的函数

    //输出端(只在 out_type == OUT_YUVDATA时有效)
    // int (*out_yuv)(struct video_buffer *b);[>输出一帧YUV数据<]

    // u8 * video_buf;
    // int video_buf_size;

    u16 disp_left;
    u16 disp_top;
    u16 disp_width;
    u16 disp_height;

    u16 stream_width;
    u16 stream_height;
};

struct adec_stream_info {
    void *priv;
    int (*wait_get_frame)(void *priv, struct video_buffer *b); /* 获取一帧数据,但还没释放buf */
    void (*set_free_frame)(void *priv, struct video_buffer *b); /* 释放这一帧数据占据的buf */

    struct audio_format f;
};


extern const struct device_operations vdec_stream_ops;
extern const struct device_operations adec_stream_ops;








#endif





