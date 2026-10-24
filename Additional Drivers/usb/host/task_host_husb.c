#include "includes.h"
#include "init.h"
#include "app_config.h"
#include "device_drive.h"
#include "event/device_event.h"
#if TCFG_USB_HOST_ENABLE
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "usb_ctrl_transfer.h"
#include "usb_storage.h"
#include "adb.h"
#include "aoa.h"
#include "host_uvc.h"
#include "hid.h"
#include "audio.h"
#include "hub.h"
#include "usbnet.h"
#include "usb_host_cdc.h"

extern const int config_uvc_host_acceleration;

void fb_lcd_buf_clear(void);
void otg_event_handler(struct device_event *event);
void usb_host_event_handler(struct device_event *event);
int start_quick_uvc_disp(u8 id, u32 x, u32 y, u32 width, u32 height);
int stop_quick_uvc_disp(u8 id);

/**
 * @brief  usb host 上线事件处理线程
 *         独立的事件线程可以加速usb上下线处理速度,避免被其他事件处理阻挡
 * @param: none
 *
 * @return:
 **/
static void khubd_task(void *p)
{
    int err;
    int msg[8];
    struct device_event *event;
    int usb_id;

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ) {
            continue;
        }
        switch (msg[1]) {
        case DEVICE_EVENT_FROM_OTG:
            printf("\n khubd_task otg online: %lums\n", jiffies_to_msecs(jiffies));
            event = (struct device_event *)&msg[2];
            otg_event_handler(event);
            break;
        case DEVICE_EVENT_FROM_USB_HOST:
            printf("\nkhubd_task usb host online: %lums\n", jiffies_to_msecs(jiffies));
            event = (struct device_event *)&msg[2];
            usb_host_event_handler(event);
            usb_id = ((const char *)event->arg)[8] - '0';

            if (event->event == DEVICE_EVENT_IN) {
                if (config_uvc_host_acceleration) {
                    start_quick_uvc_disp(usb_id, 0, 0, 800, 480);
                }
            } else if (event->event == DEVICE_EVENT_OUT) {
                if (config_uvc_host_acceleration) {
                    stop_quick_uvc_disp(usb_id);
                    fb_lcd_buf_clear();
                }
            }
            break;
#if TCFG_HOST_HUB_ENABLE
        case USBSTACK_HUB_MSG:
            int evt = msg[2];
            u32 value = msg[3];
            struct usb_host_device *host_dev = (struct usb_host_device *)msg[4];
            if (evt == DEVICE_EVENT_CHANGE) {
                usb_hub_event(host_dev, value >> 16, value & 0xff);
            } else if (evt == DEVICE_EVENT_ONLINE) {
                usb_hub_port_event(host_dev, value, USB_HUB_PORT_RESET);
            } else if (evt == DEVICE_EVENT_IN) {
                usb_hub_port_event(host_dev, value, USB_HUB_PORT_IN);
            } else if (evt == DEVICE_EVENT_OUT) {
                usb_hub_port_event(host_dev, value, USB_HUB_PORT_OUT);
            }
            break;
#endif
        default:
            break;
        }
    }
}

static int khubd_host_init(void)
{
    return thread_fork("khubd", 25, 1024, 512, 0, khubd_task, NULL);
}
early_initcall(khubd_host_init);

#endif /* CONFIG_VIDEO_ENABLE */

