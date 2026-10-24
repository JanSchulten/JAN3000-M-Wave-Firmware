#ifndef _PIPELINE_FILTER_H_
#define _PIPELINE_FILTER_H_

#include "generic/list.h"
#include "pipeline_plugin.h"

typedef struct pipeline_filter pipe_filter_t;
struct pipeline_filter {
    struct list_head entry;

    on_event_t on_event;
    void *on_event_arg;
    void *parent;

    pipe_plugin_t *plugin;
    int (*init)(pipe_filter_t *filter);
    int (*connect)(pipe_filter_t *prev_filter, pipe_filter_t *filter, int source_channel);
    int (*start)(pipe_filter_t *filter, int source_channel);
    int (*stop)(pipe_filter_t *filter, int source_channel);
    int (*reset)(pipe_filter_t *filter, int source_channel);
    int (*prepare)(pipe_filter_t *filter, int source_channel);
    int (*pause)(pipe_filter_t *filter, int source_channel);
    int (*resume)(pipe_filter_t *filter, int source_channel);
    int (*set_parameter)(pipe_filter_t *filter, int cmd, void *arg, int source_channel);
    int (*get_parameter)(pipe_filter_t *filter, int cmd, void *arg, int source_channel);
};

pipe_filter_t *filter_register(const char *name, int channel);

int filter_pre_unregister(pipe_filter_t *filter, int channel);
int filter_unregister(pipe_filter_t *filter);

#endif
