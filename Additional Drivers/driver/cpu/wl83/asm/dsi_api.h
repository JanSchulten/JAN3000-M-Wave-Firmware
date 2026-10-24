#ifndef __DSI_API_H__
#define __DSI_API_H__

#include "asm/dsi.h"
#include "asm/dsi_packet.h"

int dsi_send_init_code(struct mipi_dev *dev);
void dsi_dev_init(struct mipi_dev *dev);
void dsi_dev_deinit(void);
extern void dsi_video_kick_start();
void mipi_wait_fb_swap_finish(u32 arg);
void mipi_wait_line_finish(u32 arg);
void mipi_start_display(u32 arg);
void mipi_set_isr_cb(u32 arg);
void mipi_set_line_isr_cb(u32 arg);

#endif

