#ifndef VIDEO_IOCTL_H
#define VIDEO_IOCTL_H



#include "typedef.h"
#include "generic/list.h"
#include "generic/ioctl.h"
#include "generic/lbuf.h"
#include "system/task.h"
#include "asm/imc_driver.h"
#include "asm/imc.h"
#include "video/isp.h"
#include "video/fb.h"
#include "video/camera.h"
#include "video/usb_cam_dev.h"

struct video_capability {
    char name[16];
    u32 capabilities;
};

/* Values for 'capabilities' field */
#define VIDEO_CAP_VIDEO_CAPTURE		0x00000001  /* Is a video capture device */
#define VIDEO_CAP_VIDEO_OUTPUT		0x00000002  /* Is a video output device */
#define VIDEO_CAP_VIDEO_OVERLAY		0x00000004  /* Can do video overlay */

#define VIDEO_CAP_CAMERA_FPS        "camera_fps"


#define VIDEO_CAP_TUNER			0x00010000  /* has a tuner */
#define VIDEO_CAP_AUDIO			0x00020000  /* has audio support */
#define VIDEO_CAP_RADIO			0x00040000  /* is a radio device */
#define VIDEO_CAP_MODULATOR		0x00080000  /* has a modulator */

#define VIDEO_CAP_READWRITE              0x01000000  /* read/write systemcalls */
#define VIDEO_CAP_ASYNCIO                0x02000000  /* async I/O */
#define VIDEO_CAP_STREAMING              0x04000000  /* streaming I/O ioctls */


/*#define VIDEO_FPS(a) 	((a)*100)*/



#define VIDEO_REC_SET_FPS	4
#define VIDEO_REC_SET_FILL_FRAME	5
#define VIDEO_REC_SET_CAMERA_FPS    6
#define VIDEO_REC_SET_CYC_TIME      8
#define VIDEO_REC_SET_DELAY_LIMIT   9
#define VIDEO_REC_RESET_BITS_RATE   10
#define VIDEO_REC_SET_TIME_STAMP    11


#define VIDEO_PIX_FMT_RAW  			0x00000001
#define VIDEO_PIX_FMT_JPEG 			0x00000002
#define VIDEO_PIX_FMT_H264 			0x00000004

#define VIDEO_PIX_FMT_YUV420 		0x00000100
#define VIDEO_PIX_FMT_YUYV          0x00000200
#define VIDEO_PIX_FMT_YV12          0x00000400
#define VIDEO_PIX_FMT_NV12          0x00000800
#define VIDEO_PIX_FMT_YUV422 		0x00001000
#define VIDEO_PIX_FMT_YUV444 		0x00001100
#define VIDEO_PIX_FMT_YUV 			0x0000ff00



#define VIDIOC_QUERYCAP 		        _IOR('V', 0, sizeof(struct video_capability))
#define VIDIOC_GET_FMT 			        _IOR('V', 1, sizeof(struct video_format))
#define VIDIOC_SET_FMT 			        _IOW('V', 1, sizeof(struct video_format))
#define VIDIOC_OVERLAY 			        _IOW('V', 2, sizeof(int))
#define VIDIOC_STREAM_ON 		        _IOW('V', 3, sizeof(enum video_buf_type))
#define VIDIOC_STREAM_OFF 		        _IOW('V', 4, sizeof(enum video_buf_type))
#define VIDIOC_REQBUFS 			        _IOW('V', 5, sizeof(struct video_reqbufs))
#define VIDIOC_QUERYBUF 		        _IOR('V', 6, sizeof(struct videobuf_buffer))
#define VIDIOC_QBUF 			        _IOR('V', 7, sizeof(struct videobuf_buffer))
#define VIDIOC_DQBUF 			        _IOW('V', 8, sizeof(struct videobuf_buffer))
/*#define VIDIOC_GET_AUDIO 		        _IOR('V', 9, sizeof(struct video_audio))
#define VIDIOC_SET_AUDIO 	 	        _IOW('V', 9, sizeof(struct video_audio))*/
#define VIDIOC_IMAGE_CAPTURE 	        _IOR('V', 10, sizeof(struct video_image_capture))
#define VIDIOC_DEL_IMAGE 		        _IOW('V', 11, sizeof(struct video_image_capture))
#define VIDIOC_PLAY   			        _IOW('V', 12, sizeof(int))
#define VIDIOC_GET_S_ATTR               _IOR('V', 13, sizeof(struct video_enc_attr))
#define VIDIOC_SET_S_ATTR               _IOW('V', 13, sizeof(struct video_enc_attr))
#define VIDIOC_GET_D_ATTR               _IOR('V', 14, sizeof(struct video_enc_attr))
#define VIDIOC_SET_D_ATTR               _IOW('V', 14, sizeof(struct video_enc_attr))
#define VIDIOC_GET_FPS                  _IOR('V', 15, sizeof(struct video_dev_fps))
#define VIDIOC_SET_FPS                  _IOW('V', 15, sizeof(struct video_dev_fps))
#define VIDIOC_GET_OSD                  _IOR('V', 16, sizeof(struct video_osd))
#define VIDIOC_SET_OSD                  _IOW('V', 16, sizeof(struct video_osd))
#define VIDIOC_OSD_ENABLE               _IOW('V', 17, sizeof(bool))
#define VIDIOC_OSD_DISABLE              _IOW('V', 18, sizeof(bool))
#define VIDIOC_GET_FRAME_CNT            _IOW('V', 19, sizeof(int))
#define VIDIOC_TRY_STOP_FRAME_INTERVAL  _IOW('V', 20, sizeof(int))
#define VIDIOC_SET_STOP_FRAME_INTERVAL  _IOW('V', 21, sizeof(int))
#define VIDIOC_GET_OSD_ATTR             _IOR('V', 22, sizeof(struct video_osd_attr))
#define VIDIOC_SET_OSD_ATTR             _IOW('V', 23, sizeof(struct video_osd_attr))
#define VIDIOC_START_IFREME             _IOW('V', 24, sizeof(int))
#define VIDIOC_APPEND_USER_DATA         _IOW('V', 25, sizeof(struct h264_user_data))
#define VIDIOC_V4_PAUSE_RUN   			_IOW('V', 26, sizeof(int))
#define VIDIOC_SET_AVC                  _IOW('V', 27, sizeof(struct app_enc_info))
#define VIDIOC_SET_OVERLAY				_IOW('V', 26, sizeof(int))
#define VIDIOC_GET_OVERLAY				_IOR('V', 26, sizeof(int))
#define VIDIOC_CLEAR_OVERLAY			_IOW('V', 27, sizeof(int))
#define VIDIOC_BUF_MOVE                 _IOW('V', 28, sizeof(int))
#define VIDIOC_SET_ENC_CROP             _IOW('V', 29, sizeof(struct video_source_crop))
#define VIDIOC_SET_DIS_CROP             _IOW('V', 30, sizeof(struct video_source_crop))
#define VIDIOC_GET_MIRROR               _IOR('V', 31, sizeof(struct video_mirror))
#define VIDIOC_SET_MIRROR               _IOW('V', 31, sizeof(struct video_mirror))
#define VIDIOC_GET_JAGGY                _IOR('V', 32, sizeof(struct video_jaggy))
#define VIDIOC_SET_JAGGY                _IOW('V', 32, sizeof(struct video_jaggy))
#define VIDIOC_REQDBUF 			        _IOW('V', 33, sizeof(struct video_reqbufs))
#define VIDIOC_RDBUF 			        _IOR('V', 34, sizeof(struct videobuf_buffer))
#define VIDIOC_IDBUF 			        _IOW('V', 35, sizeof(struct videobuf_buffer))
#define VIDIOC_YUV_CAPTURE 	        	_IOR('V', 36, sizeof(struct video_image_capture))
#define VIDIOC_CLOSE_CAMERA             _IOW('V', 37, sizeof(int))
#define VIDIOC_SET_CAMERA_INFO          _IOW('V', 38, sizeof(struct camera_device_info))
#define VIDIOC_PAUSE                    _IOW('V', 39, sizeof(int))
#define VIDIOC_RESUME                   _IOW('V', 40, sizeof(int))
#define VIDIOC_GET_SOURCE_CH            _IOW('V', 41, sizeof(int))
#define VIDIOC_SAVE_FILE                _IOW('V', 42, sizeof(int))
#define VIDIOC_SET_CYC_TIME             _IOW('V', 43, sizeof(int))
#define VIDIOC_SET_FILE                 _IOW('V', 44, sizeof(int))



enum video_buf_type {
    VIDEO_BUF_TYPE_VIDEO_CAPTURE,
    VIDEO_BUF_TYPE_IMAGE_CAPTURE,
    VIDEO_BUF_TYPE_VIDEO_OVERLAY,
    VIDEO_BUF_TYPE_VIDEO_PLAY,
    VIDEO_BUF_TYPE_YUV_CAPTURE,
    VIDEO_BUF_TYPE_UVC_CAPTURE,
    VIDEO_BUF_TYPE_ISP_TRAIN,
    VIDEO_BUF_TYPE_QUICK_IMAGE,
    VIDEO_BUF_TYPE_STICKER_IMAGE,
    VIDEO_PIPE_PRE_CREATE,
};

enum video_three_way_type {
    VIDEO_THREE_WAY_NONE,
    VIDEO_THREE_WAY_JPEG,
    VIDEO_THREE_WAY_H264,
    VIDEO_THREE_WAY_DOU_RAW,

};

struct video_mirror {
    u32 enc_dis;
    u32 mirror;
};

struct video_jaggy {
    u32 enc_dis;
    u32 jaggy;
};

struct video_window {
    u16 left;
    u16 top;
    u16 width;
    u16 height;
    u16 border_left;
    u16 border_top;
    u16 border_right;
    u16 border_bottom;
    u16 mirror;
    u8 win_type;
    u8 combine;
};


struct video_format {
    enum video_buf_type type;
    u8 camera_type;
    u8 online;//0:是手动编码，1是联动
    u8 enable_dri;
    u8 three_way_type;
    u8 disp_id;
    u8 disp_yuv_type;
    u8 disp_fb_buf_num;
    u8 uvc_id;
    u8 fps;
    u8 osd_en;
    u16 enc_rotate;
    u16 dis_rotate;
    u8 stop_wait;
    u8 three_in_one_open;
    u16 src_width;
    u16 src_height;
    int blk_width;
    int blk_height;
    u32 pixelformat;
    u32 gap_fps;
    struct video_window win;
    void *private_data;
    u8 double_raw;
    void *sticker;
};

struct jpg_thumbnail {
    u8 enable;
    u8 quality;
    u16 width;
    u16 height;
    u8 *buf;
    int len;
};

struct image_sticker {
    u8 enable;
    u8 format;//YUV420/YUV422
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    u8 *addr;
    u32 size;
};


struct video_image_capture {
    u16 width;
    u16 height;
    u8  *baddr;
    u32 size;
    int format;
    u32 mirror;
    struct jpg_thumbnail *thumbnail;
    struct image_sticker *sticker;
    u8 linear_scale;	// 使用软件线性插值标志
};

struct video_reqbufs {
    void *buf;
    int size;
    struct buffer_dev {
        void *fd;
        u32 head_size;
        u32 align_size;
        u32 addr;
        u32 size;
        int (*read)(void *fd, void *buf, u32 addr, int len);
        int (*write)(void *fd, void *buf, u32 addr, int len);
    } dev;

};

struct videobuf_buffer {
    u32 len;
    u32 magic;
    u32 msec;
    u8 data[0];
};

struct videobuf_state {
    u32 available_len;
    u32 max_continue_len;
};

// struct videobuf_queue {
// u8 ref;
// u8 streaming;
// u16 align;
// const char *name;
// OS_SEM sem;
// void *buf;
// struct lbuff_head *lbuf;
// void *private_data;
// };

struct video_buffer {
    u8  index;
    u8 	noblock;
    u16 timeout;
    u32 time_msec;
    u32 len;
    u32 baddr;
    void *priv;
    u32 test_num;
};

//enum videobuf_state {
//VIDEOBUF_NEEDS_INIT = 0,
//VIDEOBUF_PREPARED   = 1,
//VIDEOBUF_QUEUED     = 2,
//VIDEOBUF_ACTIVE     = 3,
//VIDEOBUF_DONE       = 4,
//VIDEOBUF_ERROR      = 5,
//VIDEOBUF_IDLE       = 6,
//};

//struct videobuf_queue;



//#define VIDEO_MAX_FRAME   16

//struct videobuf_buffer {
//struct list_head stream;
//OS_SEM done;
//u32 baddr;
//u8 state;
//u8 index;
//void *priv_data;
//};

//struct videobuf_queue {
//struct list_head stream;
//struct videobuf_buffer *bufs[VIDEO_MAX_FRAME];
//struct videobuf_buffer *curr;
//struct videobuf_queue_ops *ops;
//OS_SEM sem;
//int msize;
//void *pirv_data;
//u8 streaming;
//};


/*struct video_audio {
    u32 capabilities;
};*/

//struct videobuf_queue_ops {
//int (*buf_setup)(struct videobuf_queue *q, int *count);
//int (*buf_prepare)(struct videobuf_queue *q, struct videobuf_buffer *);
//};




/*struct video_ioctl_ops {
    int (*vidioc_querycap)(void *fh, struct video_capability *cap);

    int (*vidioc_get_fmt_vid_cap)(void *fh, struct video_format *f);
    int (*vidioc_get_fmt_vid_out)(void *fh, struct video_format *f);
    int (*vidioc_get_fmt_vid_overlay)(void *fh, struct video_format *f);


    int (*vidioc_set_fmt_vid_cap)(void *fh, struct video_format *f);
    int (*vidioc_set_fmt_vid_out)(void *fh, struct video_format *f);
    int (*vidioc_set_fmt_vid_overlay)(void *fh, struct video_format *f);

    int (*vidioc_try_fmt_vid_cap)(void *fh, struct video_format *f);
    int (*vidioc_try_fmt_vid_out)(void *fh, struct video_format *f);
    int (*vidioc_try_fmt_vid_overlay)(void *fh, struct video_format *f);

    int (*vidioc_reqbufs)(void *fh, struct video_reqbufs *b);
    int (*vidioc_querybuf)(void *fh, struct video_buffer *b);
    int (*vidioc_qbuf)(void *fh, struct video_buffer *b);
    int (*vidioc_dqbuf)(void *fh, struct video_buffer *b);

    int (*vidioc_overlay)(void *fh, unsigned int i);

    int (*vidioc_streamon)(void *fh, int i);
    int (*vidioc_streamoff)(void *fh, int i);


    int (*vidioc_get_audio)(void *fh, struct video_audio *a);
    int (*vidioc_set_audio)(void *fh, struct video_audio *a);
};*/




/*struct video_device {
    const char *name;
    const struct video_ioctl_ops *ioctl;
    const struct video_file_operations *fops;
    void *priv;
};*/




/*struct video_file_operations {
    int (*open)(struct video_device *device);
    int (*write)(struct video_device *device, void *buf, u32 len);
    int (*close)(struct video_device *device);
};*/



/*struct video_dec_format {
    int pixelformat;
};*/













































#endif

