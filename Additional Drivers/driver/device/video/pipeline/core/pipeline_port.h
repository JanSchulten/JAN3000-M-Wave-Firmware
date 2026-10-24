#ifndef __PORT_H
#define __PORT_H


#include "generic/list.h"
#include "spinlock.h"
#include "pipeline_buffer.h"
#include "message_event.h"


typedef struct pipeline_endpoint {
    struct list_head entry;
    long long channel;
    char name[10];
    buffer_t *data_buffer;
    void *priv_data;
} pipe_endpoint_t;


typedef struct pipeline_port {
    spinlock_t lock;
    struct list_head in_endpoint;
    struct list_head out_endpoint;
    int   in_endpoint_num;
    int   out_endpoint_num;

} pipe_port_t;



pipe_port_t *port_alloc();

void endpoint_source_channel_inc(pipe_endpoint_t *endpoint, int source_channel);
void endpoint_source_channel_dec(pipe_endpoint_t *endpoint, int source_channel);
bool endpoint_source_channel_vaild(pipe_endpoint_t *endpoint, int source_channel);

void port_release(pipe_port_t *port);



pipe_endpoint_t *port_add_input_endpoint(pipe_port_t *port, const char *prev_name, int source_channel);
pipe_endpoint_t *port_get_input_endpoint(pipe_port_t *port, int num);
char *port_get_input_endpoint_name(pipe_port_t *port, int num);
int port_input_endpoint_num(pipe_port_t *port);
int port_delete_input_endpoint(pipe_port_t *port, const char *prev_name);





pipe_endpoint_t *port_add_output_endpoint(pipe_port_t *port, const char *next_name, int source_channel);
pipe_endpoint_t *port_get_output_endpoint(pipe_port_t *port, int num);
char *port_get_output_endpoint_name(pipe_port_t *port, int num);
int port_output_endpoint_num(pipe_port_t *port);
int port_delete_output_endpoint(pipe_port_t *port, const char *next_name);


int port_delete_input_endpoint_by_channel(pipe_port_t *port, int source_channel);


int message_request(pipe_port_t *port, int cmd, void *arg);




#endif



