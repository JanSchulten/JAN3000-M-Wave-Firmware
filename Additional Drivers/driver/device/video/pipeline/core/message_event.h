#ifndef __MESSAGE_EVENT_H
#define __MESSAGE_EVENT_H

#include "generic/list.h"
#include "spinlock.h"

#define MESSAGE_PREV    BIT(31)
#define MESSAGE_NEXT    BIT(30)
#define MESSAGE_ALL     ( BIT(30) | BIT(31) )

typedef void (*on_event_t)(const char *plugin_name, int event, void *arg);

typedef struct pipeline_common {
    struct list_head entry;
    void *data;
    u8 channel;
    u8 format;
    u16 width;
    u16 height;
    u32 fps;
    int priv;
} pipe_common_t;

typedef struct pipeline_message_group {
    spinlock_t lock;
    struct list_head list;
    int group_num;
} pipe_msg_group_t;

struct info_handle {
    struct list_head entry;
    void *data;
    u8 channel;

    u8 dst_format;
    u16 dst_width;
    u16 dst_height;
    u32 dst_fps;

    u8 src_format;
    u16 src_width;
    u16 src_height;
    u32 src_fps;

    u8 refs;
};

typedef enum {
    FORMAT_YUV420P = 0,
    FORMAT_YUV422P,
    FORMAT_YUV422_YUYV,
    FORMAT_YUV422_UYVY,
    FORMAT_RAW8,
    FORMAT_NONE = 0xff,
} pipe_video_format_t;

enum {
    PIPE_MESSAGE_OK = 0,
    PIPE_MESSAGE_CONTINUE,
    PIPE_MESSAGE_NONE,
};

//内部通信使用
enum {
    IMC_UP_BUFFER,
    IMC_DOWN_BUFFER,
    IMC_GET_OUTPUT_HEIGHT,
    IMC_GET_OUTPUT_WIDTH,
    IMC_SET_SCALE,

    JPEG_DEC_ONE_FRAME,
    JPEG_DEC_ONE_FILE,
    JPEG_ENC_ONE_FRAME,

    H264_ENC_ONE_FRAME,
    SOFT_H264_DEC_ONE_FRAME,

    ENCODER_NEED_KSTART,
    ENCODER_KSTART_SUCC,
    ENCODER_KSTART_FAIL,
    IMC_SEM_POST,
    ENCODER_USED_JPEG,

    GET_JPEG_NEED_KSTART,

    GET_CAMERA_SENSOR_DEV,
    GET_CAMERA_SENSOR_DEV_ALL,

    GET_RAW_MODE,
    GET_DATA_INFO,
    GET_DATA_INFO_ALL,

    SET_FRAME_SENSOR_START,
    SET_REP_GET_DATA_RATE,
    PIPELINE_SET_SOURCE_CHANNEL,

    REP_KSTART_NOTIFY,
    REP_BUF_NOTIFY,
    REP_COMPOSITE_BUF_NOTIFY,
    REP_ERR_NOTIFY,
    NEXT_MODULE_NOTIFY,
    REP_MODE_NOTIFY,
    REP_SEM_ADD,
    REP_SEM_DEC,
    REP_SEM_NUM,
    REP_SEM_POST,
    CHECK_DROP_FRAME,

    PIPELINE_SET_IMC_RAW,
    PIPELINE_SET_FIRST_RAW_IMAGE,
    GET_ISP_SEM_HANDLE,
    GET_DROP_CHANNEL,
    INC_IMC_FRAME_RATE,
    DEC_IMC_FRAME_RATE,
    SFT_FREE_BUF,
    DISP_FB_PUT_BUF,
    GET_OUTPUT_LINE,
    VIDEOSINK_ONE_FRAME,
    VIDEOSINK_ONE_H264_FRAME,
    SFT_IS_STOP,
    IMC_FRAME_DONE,
    COMPOSITE_ONE_FRAME,
    IMC_BUF_RELOAD,
    //....
    //....
    //....


};


//回调事件消息

#define   EVENT_PREPARE_DONE                  0x1
#define   EVENT_START_DONE                  0x2
#define   EVENT_STOP_DONE                  0x3
#define   EVENT_RESET_DONE                  0x4



#define   EVENT_ERROR                  0x5
#define   EVENT_WARNNING               0x6


#define   EVENT_FRAME_DOING             0x8
#define   EVENT_FRAME_DONE             0x9
#define   EVENT_DATA_DONE             0x10
#define   EVENT_BUFFER_FULL            0x11
#define   EVENT_BW_FULL                0x12
#define   EVENT_SPEED_FULL             0x13
#define   EVENT_LINE_ERR             0x14
#define   EVENT_OSD_ERR             0x15
#define   EVENT_BUFFER_EMPTY        0x16
#define   EVENT_JPEG_DEC_RESET        0x17


#endif
