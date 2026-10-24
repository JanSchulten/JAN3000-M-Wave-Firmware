#ifndef _PIPELINE_NEW_H_
#define _PIPELINE_NEW_H_

#include "video/camera.h"
#include "pipeline_filter.h"

//跟硬件挂钩,禁止修改
#define VSID_ISC_OFFSET 0
#define VSID_REP_OFFSET 2
#define VSID_ISP_OFFSET 4

#define VTID_IMC_OFFSET 0
#define VTID_ISP_OFFSET 4

#define PIPELINE_NAME_MAX    (32)

typedef struct pipeline_core {
    struct list_head filters_head;
    OS_MUTEX mutex;
    char name[PIPELINE_NAME_MAX];
    on_event_t on_event;
    void *on_event_arg;
    int channel;
} pipe_core_t;

pipe_core_t *pipeline_init(on_event_t on_event, void *on_event_arg);

int pipeline_uninit(pipe_core_t *pipe_core);

pipe_filter_t *pipeline_filter_add(pipe_core_t *pipe_core, const char *name);

int pipeline_filter_link(pipe_filter_t *prev_filter, pipe_filter_t *filter);

int pipeline_filter_stop(pipe_core_t *pipe_core, pipe_filter_t *filter);

int pipeline_filter_remove(pipe_core_t *pipe_core, pipe_filter_t *filter);

int pipeline_filter_start(pipe_core_t *pipe_core, pipe_filter_t *filter);

int pipeline_filter_stop(pipe_core_t *pipe_core, pipe_filter_t *filter);

int pipeline_filter_reset(pipe_core_t *pipe_core, pipe_filter_t *filter);

pipe_filter_t *pipeline_filter_find(pipe_core_t *pipe_core, const char *name);

pipe_filter_t *pipeline_filter_prev(pipe_core_t *pipe_core, pipe_filter_t *filter);

pipe_filter_t *pipeline_filter_next(pipe_core_t *pipe_core, pipe_filter_t *filter);

// int pipeline_linkports(pipe_core_t *pipe_core, port *port1, port *port2);   //单控件多输入多输出这接口才有用

int pipeline_start(pipe_core_t *pipe_core);

int pipeline_prepare(pipe_core_t *pipe_core);

int pipeline_pause(pipe_core_t *pipe_core);

int pipeline_resume(pipe_core_t *pipe_core);

int pipeline_stop(pipe_core_t *pipe_core);

int pipeline_param_set(pipe_core_t *pipe_core, pipe_filter_t *dest_filter, int cmd, int arg);

int pipeline_param_get(pipe_core_t *pipe_core, pipe_filter_t *dest_filter, int cmd, int arg);

int pipeline_reset(pipe_core_t *pipe_core);

enum {
    PIPELINE_SET_EXTERN_BUFFER = 0x1000,
    PIPELINE_SET_OUTPUT_WIDTH,
    PIPELINE_SET_OUTPUT_HEIGHT,
    PIPELINE_SET_IMAGE_PHOTO,
    PIPELINE_SET_1080P,
    PIPELINE_SET_MCV_TOTAL_CH,
    PIPELINE_EXTERN_CONNECT,
    PIPELINE_SCR_CLI_ADR,
    PIPELINE_SCR_SOCK_TYPE,
    PIPELINE_SCR_ACK_CALLBACK,
    PIPELINE_SET_FORMAT,
    PIPELINE_SET_UVC_FORMAT,
    PIPELINE_GET_FORMAT,
    PIPELINE_SET_CUR_CHANNEL,
    PIPELINE_SET_BUFFER_LINE,
    PIPELINE_INSERT_EMPTY_FRAME,
    PIPELINE_SET_REP_MIN_PERIOD,
    PIPELINE_SET_SINK_OUT_FORMAT,
    PIPELINE_SET_YUV_BUF,
    PIPELINE_GET_YUV_BUF,
    PIPELINE_SET_OUT_FPS,
    PIPELINE_SAVE_FILE,
    PIPELINE_SET_CYC_TIME,
    PIPELINE_SET_DEC_FILE,
};


#endif
