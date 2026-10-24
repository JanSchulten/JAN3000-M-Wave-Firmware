#ifndef __DSI_PACKET_H__
#define __DSI_PACKET_H__

#include "generic/typedef.h"
#include "asm/dsi.h"

//dcs command
#define null_packet									0x09
#define set_maximum_return_packet_size 				0x37
#define dcs_short_write_p0 							0x05
#define dcs_short_write_p1 							0x15
#define dcs_long_write    							0x39
#define dcs_short_read_p0   						0x06
#define generic_short_write_p0 						0x03
#define generic_short_write_p1 						0x13
#define generic_short_write_p2 						0x23
#define generic_short_read_p0  						0x04
#define generic_short_read_p1  						0x14
#define generic_short_read_p2  						0x24
#define generic_long_write    						0x29
#define packed_pixel_format_rgb565_16bit			0x0e
#define packed_pixel_format_rgb666_18bit			0x1e
#define packed_pixel_format_rgb666_18bit_loosely	0x2e
#define packed_pixel_format_rgb888_24bit			0x3e

void dsi_rd_short(u8 *buf, u8 len);
void dsi_rd_long(u8 *buf, u8 len);

void dcs_set_max_packet_size(u8 size);
void dcs_send_short_p0(u8 cmd);
u8   dcs_send_short_p0_bta(u8 cmd);
void dcs_send_short_p1(u8 cmd, u8 val);
u8   dcs_send_short_p1_bta(u8 cmd, u8 val);
void dcs_send_long_packet(u8 *buf, u8 len);
u8   dcs_send_long_packet_bta(u8 *buf, u8 len);
u8   dcs_recv_short_p0_bta(u8 cmd);
u8   dcs_read_parm(u8 cmd, u8 *buf, u8 len);

void generic_set_max_packet_size(u8 size);
void generic_send_short_p1(u8 cmd);
u8   generic_send_short_p1_bta(u8 cmd);
void generic_send_short_p2(u8 cmd, u8 val);
u8   generic_send_short_p2_bta(u8 cmd, u8 val);
void generic_send_long_packet(u8 *buf, u8 len);
u8   generic_send_long_packet_bta(u8 *buf, u8 len);
u8   generic_recv_short_p1_bta(u8 cmd);
u8   generic_read_parm(u8 cmd, u8 *buf, u8 len);

#endif

