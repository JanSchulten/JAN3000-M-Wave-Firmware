#ifndef _PIPELINE_PLUGINS_H_
#define _PIPELINE_PLUGINS_H_

#include "pipeline_port.h"

#define PLUGIN_SOURCE_ISC_BASE  (0)
#define PLUGIN_SOURCE_CSI_BASE  (2)
#define PLUGIN_SOURCE_UVC_BASE  (3)
#define PLUGIN_SOURCE_MCV_BASE  (5)
#define PLUGIN_SOURCE_VIR_BASE  (9)
#define PLUGIN_SOURCE_FILE_BASE (15)
#define PLUGIN_SOURCE_SCR_BASE (16)



#define PLUGIN_NAME_MAX  (32)

/* ---> UNINIT ---> INITED ----> READY ----- */
/* |                                       |    */
/* ---------- PAUSED <=======> RUNNING <----    */

enum pipeline_plugin_state {
    PLUGIN_UNINIT = 0,
    PLUGIN_INITED,
    PLUGIN_PAUSED,
    PLUGIN_READY,
    PLUGIN_RUNNING,
};

enum pipeline_plugin_type {
    PLUGIN_SOURCE = 0,
    PLUGIN_FILTER,
    PLUGIN_SINK,
};

enum pipeline_plugin_error {
    PLUGIN_NO_ERR            =  0,
    PLUGIN_UNKOWN_ERR        = -1, //不知道返回什么就-1
    PLUGIN_BUFFER_ERR        = -2, //buffer申请问题
    PLUGIN_ARGS_ERR          = -3, //参数问题
    PLUGIN_LINK_ERR          = -4, //连接问题
};

#define PLUGIN_ERR_LOG() do {log_error("PLUGIN UNKOWN ERR AT %s  %d",__func__,__LINE__);} while(0);






#define PLUGIN_USE_DOUBLE_RAW  BIT(31)
#define PLUGIN_USE_DOUBLE_RAW_DISP BIT(30)
#define PLUGIN_USE_DOUBLE_RAW_CAPTURE BIT(29)




char *video_format_str(int format);
char *plugin_state_str(int state);

typedef struct pipeline_plugin pipe_plugin_t;

typedef struct pipeline_plugin_ops pipe_plugin_ops;

struct pipeline_plugin {
    char name[PLUGIN_NAME_MAX];
    on_event_t on_event;
    void *on_event_arg;
    void *private_data;
    pipe_port_t *port;
    const pipe_plugin_ops *ops;
};


struct pipeline_plugin_ops {
    char *name;
    int type;
    int (*init)(pipe_plugin_t *plugin);
    int (*connect)(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin, int source_channel);
    int (*prepare)(pipe_plugin_t *plugin, int source_channel);
    int (*start)(pipe_plugin_t *plugin, int source_channel);
    int (*stop)(pipe_plugin_t *plugin, int source_channel);
    int (*reset)(pipe_plugin_t *plugin, int source_channel);
    int (*pause)(pipe_plugin_t *plugin, int source_channel);
    int (*resume)(pipe_plugin_t *plugin, int source_channel);
    int (*get_parameter)(pipe_plugin_t *plugin, int cmd, void *arg, int source_channel);
    int (*set_parameter)(pipe_plugin_t *plugin, int cmd, void *arg, int source_channel);
    int (*msg_cb)(pipe_plugin_t *plugin, int cmd, void *arg);
};

struct plugin_factory_entry {
    struct list_head entry;
    int channel;
    int extra;
    u8 used;
    void *plugin;
    struct pipeline_plugin_ops *ops;
};



#define REGISTER_PLUGIN(ops) \
    const struct pipeline_plugin_ops ops SEC_USED(.pipeline_plugin)

#define TO_STRING(x)    #x
#define REGISTER_PLUGIN_OPS(plugin_type, plugin_name, plugin_id) \
    const struct pipeline_plugin_ops plugin_name##plugin_id SEC_USED(.pipeline_plugin) = { \
        .name           = TO_STRING(plugin_name##plugin_id),                 \
        .type           = plugin_type,                  \
        .init           = plugin_name##_init,            \
        .connect        = plugin_name##_connect,         \
        .prepare        = plugin_name##_prepare,         \
        .start          = plugin_name##_start,           \
        .stop           = plugin_name##_stop,            \
        .reset           = plugin_name##_reset,          \
        .get_parameter  = plugin_name##_get_parameter,   \
        .set_parameter  = plugin_name##_set_parameter,   \
        .msg_cb         = plugin_name##_message_callback,\
    };

extern const struct pipeline_plugin_ops plugin_begin[];
extern const struct pipeline_plugin_ops plugin_end[];

#define list_for_each_plugin(plugin) \
    for(plugin = plugin_begin; plugin < plugin_end; plugin++)



void port_source_channel_inc(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin, int source_channel);

pipe_plugin_t *plugin_register(const char *name, int channel);

void plugin_unregister(pipe_plugin_t *plugin);

int plugin_get_state(pipe_plugin_t *plugin);
//获取plugin输出的端点数量
int plugin_output_port_number(pipe_plugin_t *plugin);
//获取plugin输入的端点数量
int plugin_input_port_number(pipe_plugin_t *plugin);

char *plugin_get_name(pipe_plugin_t *plugin);
//查看plugin 使用情况
int plugin_factory_plugin_status(void);

char *plugin_factory_find_reverse(const char *find_name);

char *plugin_factory_find(const char *find_name);

void plugin_factory_set_extra(const char *find_name, int _extra);

char *find_use_for_display_plugin(const char *_name);

int plugin_source_to_channel(const char *source_name);

pipe_plugin_t *plugin_factory_find_used(const char *find_name);

char *plugin_factory_find_used_by_channel(const char *find_name, int channel);

char *plugin_factory_find_used_by_extra(const char *find_name, int _extra);

char *plugin_factory_find_used_by_all(const char *find_name, int channel, int _extra);

bool port_check_connected(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin);

void plugin_pre_unregister(pipe_plugin_t *plugin, int channel);

#endif


