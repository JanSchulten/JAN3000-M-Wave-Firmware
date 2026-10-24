#ifndef  __PRINTER_H__
#define  __PRINTER_H__

u32 printer_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);

void printer_register(const usb_dev usb_id);

void printer_release(const usb_dev usb_id);


#endif  /*PRINTER_H*/

