#ifndef __CDC_RNDIS_H__
#define __CDC_RNDIS_H__

#include "usb/device/usb_stack.h"
// #include "host/usbnet.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CDC_RNDIS_RESP_BUFFER_SIZE
#define CDC_RNDIS_RESP_BUFFER_SIZE 156
#endif

#ifndef CDC_RNDIS_ETH_MAX_FRAME_SIZE
#define CDC_RNDIS_ETH_MAX_FRAME_SIZE 1536
#endif

#ifndef CDC_RNDIS_VENDOR_ID
#define CDC_RNDIS_VENDOR_ID 0x00000000
#endif

#ifndef CDC_RNDIS_VENDOR_DESC
#define CDC_RNDIS_VENDOR_DESC "JieLi-USB"
#endif

#define RNDIS_INQUIRY_PUT(src, len)   (memcpy(infomation_buffer, src, len))
#define RNDIS_INQUIRY_PUT_LE32(value) (*(u32 *)infomation_buffer = (value))

u32 rndis_desc_config(const usb_dev usb_id, u8 *ptr, u32 *itf_num);
void rndis_register(const usb_dev usb_id);
void rndis_release(const usb_dev usb_id);

#ifdef __cplusplus
}
#endif

#endif /* USBD_RNDIS_H */


