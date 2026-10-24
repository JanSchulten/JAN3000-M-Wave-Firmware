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

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#include "apple_mfi.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[mount]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static struct usb_host_device host_devices[USB_MAX_HW_NUM];// SEC(.usb_h_bss);
static u8 *h_ep0_dmabuf[USB_MAX_HW_NUM];
static OS_MUTEX usb_host_mutex;
static char uvc_map[8] = {0};

extern const int config_uvc_host_acceleration;

static int usb_host_mutex_init(void)
{
    return os_mutex_create(&usb_host_mutex);
}
early_initcall(usb_host_mutex_init);

int host_dev_status(const struct usb_host_device *host_dev)
{
    return ((host_dev)->private_data.status);
}

u32 host_device2id(const struct usb_host_device *host_dev)
{
#if USB_MAX_HW_NUM > 1
    return ((host_dev)->private_data.usb_id);
#else
    return 0;
#endif
}
const struct usb_host_device *host_id2device(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    return &host_devices[id];
#else
    return &host_devices[0];
#endif
}
int usb_sem_init(struct usb_host_device  *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

#if USB_HUB
    /* if (host_dev->parent == NULL) { */
    if (host_dev->father == NULL) {
        usb_host_config(usb_id);
    }
#else
    usb_host_config(usb_id);
#endif

    OS_SEM *sem = zalloc(sizeof(OS_SEM));
    ASSERT(sem, "usb alloc sem error");
    host_dev->sem = sem;
    log_info("%s %x %x ", __func__, host_dev, sem);
    os_sem_create(host_dev->sem, 0);
    return 0;
}
int usb_sem_pend(struct usb_host_device  *host_dev, u32 timeout)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    return os_sem_pend(host_dev->sem, timeout);;
}
int usb_sem_post(struct usb_host_device  *host_dev)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_post(host_dev->sem);
    if (ret) {
        log_error("%s %d ", __func__, ret);
    }
    return 0;
}
int usb_sem_del(struct usb_host_device *host_dev)
{
    /* r_printf("1"); */
    if (!host_dev || host_dev->sem == NULL) {
        return 0;
    }
    usb_dev usb_id = host_device2id(host_dev);

    /* r_printf("2"); */
    /* r_printf("3"); */
#if USB_HUB
    if (host_dev->sem && host_dev->father == NULL) {
        //if (host_dev->parent == NULL) {
        os_sem_del(host_dev->sem, OS_DEL_ALWAYS);
    }
#else
    if (host_dev->sem) {
        os_sem_del(host_dev->sem, OS_DEL_ALWAYS);
    }
#endif
    /* r_printf("4"); */
    /* g_printf("%s %x %x ", __func__, host_dev, host_dev->sem); */
    free(host_dev->sem);
    /* r_printf("5"); */
    host_dev->sem = NULL;
#if USB_HUB
    /* if (host_dev->parent == NULL) { */
    if (host_dev->father == NULL) {
        usb_host_free(usb_id);
    }
#else
    usb_host_free(usb_id);
#endif
    /* r_printf("7"); */
    return 0;
}

int usb_mutex_init(struct usb_host_device *host_dev)
{
    OS_MUTEX *mutex = zalloc(sizeof(OS_MUTEX));
    ASSERT(mutex, "usb alloc mutex error");
    host_dev->mutex = mutex;
    os_mutex_create(host_dev->mutex);
    return 0;
}

int usb_mutex_pend(struct usb_host_device *host_dev, u32 timeout)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_pend(host_dev->mutex, timeout);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return ret;
}
int usb_mutex_post(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_post(host_dev->mutex);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return 0;
}
int usb_mutex_del(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 0;
    }
    os_mutex_del(host_dev->mutex, 1);
    free(host_dev->mutex);
    host_dev->mutex = NULL;
    return 0;
}

static int _usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find udisk @ interface %d", interface_num);
#if TCFG_UDISK_ENABLE
    return   usb_msd_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_apple_mfi_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find apple mfi @ interface %d", interface_num);
#if TCFG_USB_APPLE_DOCK_EN
    return   usb_apple_mfi_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adb @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    return usb_adb_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_aoa_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find aoa @ interface %d", interface_num);
#if TCFG_AOA_ENABLE
    return usb_aoa_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif

}
static int _usb_hid_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find hid @ interface %d", interface_num);
#if TCFG_HID_HOST_ENABLE
    return usb_hid_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find audio @ interface %d", interface_num);
#if TCFG_HOST_AUDIO_ENABLE
    return usb_audio_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_interface_ptp_mtp_parse(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adbmtp @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    int ret = usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
    return ret == 0 ? USB_DT_INTERFACE_SIZE : ret;
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_hub_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find hub @ interface %d", interface_num);
#if TCFG_HOST_HUB_ENABLE
    return usb_hub_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_uvc_parse(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find uvc @ interface %d", interface_num);
#if TCFG_HOST_UVC_ENABLE
    return usb_uvc_parser(host_dev, interface_num, (u8 *)pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_wireless_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find wireless @ interface %d", interface_num);
#if TCFG_HOST_WIRELESS_ENABLE || TCFG_HOST_RNDIS_ENABLE
    return usbnet_generic_cdc_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_wireless_at_port_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf, struct usb_device_descriptor *device_desc)
{
    log_info("find wireless at_port @ interface %d", interface_num);
#if TCFG_HOST_WIRELESS_ENABLE
    return usbnet_at_port_parser(host_dev, interface_num, pBuf, device_desc);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_cdc_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find cdc @ interface %d", interface_num);
#if TCFG_HOST_CDC_ENABLE
    return usb_cdc_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}

static bool is_valid_interface_configuration(uint16_t idVendor, uint8_t bInterfaceClass, uint8_t bInterfaceSubClass)
{
    if (idVendor == 0x2c7c) {
        return (bInterfaceClass == 0xff && bInterfaceSubClass == 0x00);
    }

    if (idVendor == 0x2c91) {
        return (bInterfaceClass == 0x02 && bInterfaceSubClass == 0x02) ||
               (bInterfaceClass == 0x0A && bInterfaceSubClass == 0x00);
    }

    if (idVendor == 0x2ecc || idVendor == 0x3361 || idVendor == 0x1e0e) {
        return (bInterfaceClass == 0xff && bInterfaceSubClass == 0x00);
    }

    return false;
}

static int usb_descriptor_parser(struct usb_host_device *host_dev, const u8 *pBuf, u32 total_len, struct usb_device_descriptor *device_desc)
{
    int len = 0;
    u8 interface_num = 0;
    struct usb_config_descriptor *cfg_desc = (struct usb_config_descriptor *)pBuf;

    if (cfg_desc->bDescriptorType != USB_DT_CONFIG ||
        cfg_desc->bLength < USB_DT_CONFIG_SIZE) {
        log_error("invalid descriptor for config bDescriptorType = %d bLength= %d",
                  cfg_desc->bDescriptorType, cfg_desc->bLength);
        return -USB_DT_CONFIG;
    }

    log_info("idVendor %x idProduct %x", device_desc->idVendor, device_desc->idProduct);

    len += USB_DT_CONFIG_SIZE;
    pBuf += USB_DT_CONFIG_SIZE;
    int i = 0;
    u32 have_find_valid_class = 0;
    while (len < total_len) {
        if (interface_num > MAX_HOST_INTERFACE) {
            log_error("interface_num too much");
            break;
        }

        struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
        if (interface->bDescriptorType == USB_DT_INTERFACE) {

            log_info("inf class %x subclass %x ep %d",
                     interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bNumEndpoints);

            if (interface->bInterfaceClass == USB_CLASS_MASS_STORAGE) {
                i = _usb_msd_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x05AC) &&
                       ((device_desc->idProduct & 0xff00) == 0x1200)) {
                i = _usb_apple_mfi_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (is_valid_interface_configuration(device_desc->idVendor,
                       interface->bInterfaceClass, interface->bInterfaceSubClass)) {
                i = _usb_wireless_at_port_parser(host_dev, interface_num, pBuf, device_desc);
                if (i < 0) {
                    log_error("---%s %d---, i = %d", __func__, __LINE__, i);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_AUDIO) {
                i = _usb_audio_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bInterfaceClass == 0xff)  &&
                       (interface->bInterfaceSubClass == USB_CLASS_ADB)) {
                i = _usb_adb_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x18d1) &&
                       ((device_desc->idProduct & 0x2d00) == 0x2d00)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (interface->bInterfaceClass == USB_CLASS_HID) {
                i = _usb_hid_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bNumEndpoints == 3) &&
                       (interface->bInterfaceClass == 0xff || interface->bInterfaceClass == 0x06)) {
                i = _usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if ((interface->bInterfaceClass == 0xff) &&
                       (interface->bInterfaceSubClass == 0xff)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_HUB) {
                i = _usb_hub_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (interface->bInterfaceClass == USB_CLASS_VIDEO) {
                i = _usb_uvc_parse(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num += 2;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER
                       || (interface->bInterfaceClass == USB_CLASS_CDC_DATA && interface->bInterfaceSubClass == 0x00)) {
                i = _usb_wireless_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---, i = %d", __func__, __LINE__, i);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_COMM) {
                i = _usb_cdc_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---, i = %d", __func__, __LINE__, i);
                    len = total_len;
                } else {
                    interface_num += 2;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else {
                log_info("find unsupport [class %x subClass %x] @ interface %d",
                         interface->bInterfaceClass,
                         interface->bInterfaceSubClass,
                         interface_num);

                len += USB_DT_INTERFACE_SIZE;
                pBuf += USB_DT_INTERFACE_SIZE;
            }
        } else {
            /* log_error("unknown section %d %d", len, pBuf[0]); */
            if (interface->bDescriptorType == 0x0B) {
                log_info("%s %d IAD", __func__, __LINE__);
            }
            if (pBuf[0]) {
                len += pBuf[0];
                pBuf += pBuf[0];
            } else {
                len = total_len;
            }
        }
    }

    log_debug("len %d total_len %d", len, total_len);

    return !have_find_valid_class;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_suspend
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void usb_host_suspend(const usb_dev usb_id)
{
    usb_h_entry_suspend(usb_id);
}

void usb_host_resume(const usb_dev usb_id)
{
    usb_h_resume(usb_id);
}

static u32 _usb_host_mount(const usb_dev usb_id, u32 port, u32 retry, u32 reset_delay, u32 mount_timeout)
{
    u32 ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
    }
    struct usb_private_data *private_data = &host_dev->private_data;
    u32 speed = USB_SPEED_FULL;

#if defined(FUSB_MODE) && FUSB_MODE
    usb_write_power(usb_id, 0x40);
#elif defined(FUSB_MODE) && (FUSB_MODE==0)
    usb_write_power(usb_id, 0x60);
#else
#error "USB_SPEED_MODE not defined"
#endif
    for (int i = 0; i < retry; i++) {
        if (port == 0) {
            usb_h_sie_init(usb_id);
            ret = usb_host_init(usb_id, reset_delay, mount_timeout);
            if (ret) {
                reset_delay += 10;
                continue;
            }

            if (!h_ep0_dmabuf[usb_id]) {
                h_ep0_dmabuf[usb_id] = usb_h_alloc_ep_buffer(usb_id, 0, 64);
            }
            usb_set_dma_taddr(usb_id, 0, h_ep0_dmabuf[usb_id]);
            usb_set_dma_raddr(usb_id, 0, h_ep0_dmabuf[usb_id]);
            usb_set_dma_raddr(usb_id, 1, h_ep0_dmabuf[usb_id]);
            usb_set_dma_taddr(usb_id, 1, h_ep0_dmabuf[usb_id]);
            usb_set_dma_raddr(usb_id, 2, h_ep0_dmabuf[usb_id]);
            usb_set_dma_taddr(usb_id, 2, h_ep0_dmabuf[usb_id]);
            usb_set_dma_raddr(usb_id, 3, h_ep0_dmabuf[usb_id]);
            usb_set_dma_taddr(usb_id, 3, h_ep0_dmabuf[usb_id]);
            usb_set_dma_raddr(usb_id, 4, h_ep0_dmabuf[usb_id]);
            usb_set_dma_taddr(usb_id, 4, h_ep0_dmabuf[usb_id]);
            if (usb_id == 1) {
                usb_set_dma_raddr(usb_id, 5, h_ep0_dmabuf[usb_id]);
                usb_set_dma_taddr(usb_id, 5, h_ep0_dmabuf[usb_id]);
                usb_set_dma_raddr(usb_id, 6, h_ep0_dmabuf[usb_id]);
                usb_set_dma_taddr(usb_id, 6, h_ep0_dmabuf[usb_id]);
            }

            usb_sie_enable(usb_id);//enable sie intr
            usb_mdelay(reset_delay);
#if USB_HUB
            speed = usb_h_get_ep_speed(usb_id, 0);
            private_data->hub_info.speed = speed;
#endif
        }
        /**********get device descriptor*********/
        struct usb_device_descriptor device_desc;
        private_data->usb_id = usb_id;
        private_data->status = 0;
        private_data->devnum = 0;
        private_data->ep0_max_packet_size = 8;
        /* usb_get_device_descriptor(host_dev, &device_desc); */
        int usb_get_device_descriptor_64(struct usb_host_device * host_dev, struct usb_device_descriptor * desc);
        usb_get_device_descriptor_64(host_dev, &device_desc);

        /**********set address*********/
        usb_mdelay(20);
        u8 devnum = rand32() % 16 + 1;
        if (port == 0) {
            devnum = devnum;
        } else {
            devnum = port;
        }
        ret = set_address(host_dev, devnum);
        check_usb_mount(ret);
        private_data->devnum = devnum ;
#if USB_HUB
        private_data->hub_info.child_devnum = devnum;
#endif

        /**********get device descriptor*********/
        usb_mdelay(20);
        ret = usb_get_device_descriptor(host_dev, &device_desc);
        check_usb_mount(ret);
        private_data->ep0_max_packet_size = device_desc.bMaxPacketSize0;
        private_data->vendor_id = device_desc.idVendor;
        private_data->product_id = device_desc.idProduct;

        if (config_uvc_host_acceleration == 0) {
            /**********get string language*********/
            u8 buf[16];
            memset(buf, 0x0, sizeof(buf));
            ret = usb_recv_descriptor(host_dev, USB_DT_STRING << 8, buf, sizeof(buf));
            if (ret == 0) {
                memcpy(&private_data->language, buf + 2, sizeof(private_data->language));
            }

            /**********get manufacturer string**********/
            memset(private_data->manufacturer, 0x0, sizeof(private_data->manufacturer));
            if (device_desc.iManufacturer) {
                ret = usb_recv_descriptor(host_dev, (USB_DT_STRING << 8) | device_desc.iManufacturer,
                                          private_data->manufacturer, sizeof(private_data->manufacturer));
            }

            /**********get product string**********/
            memset(private_data->product, 0x0, sizeof(private_data->product));
            if (device_desc.iProduct) {
                ret = usb_recv_descriptor(host_dev, (USB_DT_STRING << 8) | device_desc.iProduct,
                                          private_data->product, sizeof(private_data->product));
            }

        }

        /**********get config descriptor*********/
        struct usb_config_descriptor cfg_desc;
        ret = get_config_descriptor(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE);
        check_usb_mount(ret);

#if USB_H_MALLOC_ENABLE
        u8 *desc_buf = zalloc(cfg_desc.wTotalLength + 16);
        ASSERT(desc_buf, "desc_buf");
#else
        u8 desc_buf[128] = {0};
        cfg_desc.wTotalLength = min(sizeof(desc_buf), cfg_desc.wTotalLength);
#endif

        ret = get_config_descriptor(host_dev, desc_buf, cfg_desc.wTotalLength);
        check_usb_mount(ret);

        /**********set configuration*********/
        ret = set_configuration(host_dev);
        /* printf_buf(desc_buf, cfg_desc.wTotalLength); */

        ret |= usb_descriptor_parser(host_dev, desc_buf, cfg_desc.wTotalLength, &device_desc);

#if USB_H_MALLOC_ENABLE
        log_info("free:desc_buf= %x\n", desc_buf);
        free(desc_buf);
#endif
        check_usb_mount(ret);

        for (int itf = 0; itf < MAX_HOST_INTERFACE; itf++) {
            if (host_dev->interface_info[itf]) {
                if (host_dev->interface_info[itf]->ctrl->set_power) {
                    host_dev->interface_info[itf]->ctrl->set_power(host_dev, 1);
                }
            }
        }

        break;//succ
    }

    if (ret) {
        goto __exit_fail;
    }
    private_data->status = 1;
    return DEV_ERR_NONE;

__exit_fail:
    log_error("usb_probe fail");
    private_data->status = 0;
    usb_sie_close(usb_id);
    return ret;
}

void otg_device_event_notify_app(u32 type, struct device_event *event);

static int usb_event_notify(const struct usb_host_device *host_dev, u32 ev)
{
    const usb_dev id = host_device2id(host_dev);
    struct device_event event = {0};

    typedef struct {
        char subdev[MAX_HOST_INTERFACE][8];
    } subdev_t;
    static subdev_t itf_set[USB_MAX_HW_NUM];
    static u32 bmUsbEvent[USB_MAX_HW_NUM];
    u16 have_post_event = 0;
    u8 no_send_event = 0;
    if (ev == 0) {
        event.event = DEVICE_EVENT_IN;
    } else if (ev == 1) {
        event.event = DEVICE_EVENT_CHANGE;
    } else {
        event.event = DEVICE_EVENT_OUT;
        goto __usb_event_out;
    }
    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        no_send_event = 0;
        event.value = 0;
        memset(itf_set[id].subdev[i], 0, sizeof(itf_set[id].subdev[i]));
        if (host_dev->interface_info[i]) {
            switch (host_dev->interface_info[i]->ctrl->interface_class) {
#if TCFG_UDISK_ENABLE
            case USB_CLASS_MASS_STORAGE:
                if (have_post_event & BIT(0)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(0);
                }
                if (id == 0) {
                    event.value = (int)"udisk0";
                } else {
                    event.value = (int)"udisk1";
                }
                bmUsbEvent[id] |= BIT(0);
                break;
#endif
#if TCFG_ADB_ENABLE
            case USB_CLASS_ADB:
                if (have_post_event & BIT(1)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(1);
                }
                if (id == 0) {
                    event.value = (int)"adb0";
                } else {
                    event.value = (int)"adb1";
                }
                bmUsbEvent[id] |= BIT(1);
                break;
#endif
#if TCFG_AOA_ENABLE
            case USB_CLASS_AOA:
                if (have_post_event & BIT(2)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(2);
                }
                if (id == 0) {
                    event.value = (int)"aoa0";
                } else {
                    event.value = (int)"aoa1";
                }
                bmUsbEvent[id] |= BIT(2);
                break;
#endif
#if TCFG_HID_HOST_ENABLE
            case USB_CLASS_HID:
                if (have_post_event & BIT(3)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(3);
                }
                if (id == 0) {
                    event.value = (int)"hid0";
                } else {
                    event.value = (int)"hid1";
                }
                bmUsbEvent[id] |= BIT(3);
                break;
#endif
#if TCFG_HOST_AUDIO_ENABLE
            case USB_CLASS_AUDIO:
                if (have_post_event & BIT(4)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(4);
                }
                if (id == 0) {
                    event.value = (int)"audio0";
                } else {
                    event.value = (int)"audio1";
                }
                bmUsbEvent[id] |= BIT(4);
                break;
#endif
#if TCFG_HOST_UVC_ENABLE
            case USB_CLASS_VIDEO:
                if (have_post_event & BIT(5)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(5);
                }
                if (host_dev->father) {
                    u8 uvc_id = host_dev->private_data.hub_info.port_map;
                    memset(uvc_map, 0, sizeof(host_dev));
                    snprintf(uvc_map, sizeof(uvc_map), "uvc%d", uvc_id + 1);
                    event.value = (int)uvc_map;
                } else {
                    if (id == 0) {
                        event.value = (int)"uvc0";
                    } else {
                        event.value = (int)"uvc1";
                    }
                }
                bmUsbEvent[id] |= BIT(5);
                break;
#endif
#if TCFG_HOST_WIRELESS_ENABLE
            case USB_CLASS_WIRELESS_CONTROLLER:
                if (have_post_event & BIT(6)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(6);
                }
                if (id == 0) {
                    event.value = (int)"wireless0";
                } else {
                    event.value = (int)"wireless1";
                }
                bmUsbEvent[id] |= BIT(6);
                break;

            case USB_CLASS_VENDOR_SPEC:
                if (have_post_event & BIT(7)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(7);
                }
                if (id == 0) {
                    event.value = (int)"at_port0";
                } else {
                    event.value = (int)"at_port1";
                }
                bmUsbEvent[id] |= BIT(7);
                break;

            case USB_CLASS_STILL_IMAGE:
                if (have_post_event & BIT(8)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(8);
                }
                if (id == 0) {
                    event.value = (int)"adbmtp0";
                } else {
                    event.value = (int)"adbmtp1";
                }
                bmUsbEvent[id] |= BIT(8);
                break;
#endif
#if TCFG_HOST_CDC_ENABLE
            case USB_CLASS_COMM:
                if (have_post_event & BIT(9)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(9);
                }
                if (id == 0) {
                    event.value = (int)"cdc0";
                } else {
                    event.value = (int)"cdc1";
                }
                bmUsbEvent[id] |= BIT(9);
                break;
#endif
#if TCFG_HOST_RNDIS_ENABLE
            case USB_CLASS_WIRELESS_CONTROLLER:
                if (have_post_event & BIT(10)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(10);
                }
                if (id == 0) {
                    event.value = (int)"rndis0";
                } else {
                    event.value = (int)"rndis1";
                }
                bmUsbEvent[id] |= BIT(10);
                break;
#endif

#if TCFG_HOST_HUB_ENABLE
            case USB_CLASS_HUB:
                if (have_post_event & BIT(15)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(15);
                }
                sprintf(itf_set[id].subdev[i], "hub%d", id);
                event.value = (int)itf_set[id].subdev[i];
                bmUsbEvent[id] |= BIT(15);
                break;
#endif
            }

            //cppcheck-suppress knownConditionTrueFalse
            if (!no_send_event && event.value) {
                log_info("event %x interface %x class %x %s",
                         event.event, i,
                         host_dev->interface_info[i]->ctrl->interface_class,
                         (const char *)event.value);

                /* printf("usb_host_mount notify >>>>>>>>>>>\n"); */
                event.arg = id == 0 ? (void *)"usb_host0" : (void *)"usb_host1";

                if (config_uvc_host_acceleration == 1 || !(strncmp((const char *)event.value, "hub", 3))) {
                    otg_device_event_notify_app(DEVICE_EVENT_FROM_USB_HOST, &event);
                } else {
                    device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event);
                }
            }
        }
    }

__usb_event_out:
    if (event.event == DEVICE_EVENT_OUT) {
        for (int i = 15; i >= 0; i--) {
            if (bmUsbEvent[id] & BIT(i)) {
                switch (i) {
#if TCFG_UDISK_ENABLE
                case 0:
                    if (id == 0) {
                        event.value = (int)"udisk0";
                    } else {
                        event.value = (int)"udisk1";
                    }
                    break;
#endif
#if TCFG_ADB_ENABLE
                case 1:
                    if (id == 0) {
                        event.value = (int)"adb0";
                    } else {
                        event.value = (int)"adb1";
                    }
                    break;
#endif
#if TCFG_AOA_ENABLE
                case 2:
                    if (id == 0) {
                        event.value = (int)"aoa0";
                    } else {
                        event.value = (int)"aoa1";
                    }
                    break;
#endif
#if TCFG_HID_HOST_ENABLE
                case 3:
                    if (id == 0) {
                        event.value = (int)"hid0";
                    } else {
                        event.value = (int)"hid1";
                    }
                    break;
#endif
#if TCFG_HOST_AUDIO_ENABLE
                case 4:
                    if (id == 0) {
                        event.value = (int)"audio0";
                    } else {
                        event.value = (int)"audio1";
                    }
                    break;
#endif
#if TCFG_HOST_UVC_ENABLE
                case 5:
                    if (host_dev->father) {
                        u8 uvc_id = host_dev->private_data.hub_info.port_map;
                        memset(uvc_map, 0, sizeof(host_dev));
                        snprintf(uvc_map, sizeof(uvc_map), "uvc%d", uvc_id + 1);
                        event.value = (int)uvc_map;
                        break; //hub 情况下只拔uvc */
                    } else {
                        if (id == 0) {
                            event.value = (int)"uvc0";
                        } else {
                            event.value = (int)"uvc1";
                        }
                        break;
                    }
#endif
#if TCFG_HOST_WIRELESS_ENABLE
                case 6:
                    if (id == 0) {
                        event.value = (int)"wireless0";
                    } else {
                        event.value = (int)"wireless1";
                    }
                    break;
                case 7:
                    if (id == 0) {
                        event.value = (int)"at_port0";
                    } else {
                        event.value = (int)"at_port1";
                    }
                    break;
                case 8:
                    if (id == 0) {
                        event.value = (int)"adbmtp0";
                    } else {
                        event.value = (int)"adbmtp1";
                    }
                    break;
#endif
#if TCFG_HOST_CDC_ENABLE
                case 9:
                    if (id == 0) {
                        event.value = (int)"cdc0";
                    } else {
                        event.value = (int)"cdc1";
                    }
                    break;
#endif
#if TCFG_HOST_RNDIS_ENABLE
                case 10:
                    if (id == 0) {
                        event.value = (int)"rndis0";
                    } else {
                        event.value = (int)"rndis1";
                    }
                    break;
#endif
#if TCFG_HOST_HUB_ENABLE
                case 15:
                    if (!host_dev->father) {
                        if (id == 0) {
                            event.value = (int)"hub0";
                        } else {
                            event.value = (int)"hub1";
                        }
                        sprintf(itf_set[id].subdev[i], "hub%d", id);
                        event.value = (int)itf_set[id].subdev[i];
                    }
                    break;
#endif
                default:
                    event.value = 0;
                    break;
                }
                bmUsbEvent[id] &= ~BIT(i);
                if (event.value) {
                    event.arg = (id == 0) ? (void *)"usb_host0" : (void *)"usb_host1";
                    have_post_event = 1;
                    if (config_uvc_host_acceleration == 1 || !(strncmp((const char *)event.value, "hub", 3))) {
                        otg_device_event_notify_app(DEVICE_EVENT_FROM_USB_HOST, &event);
                    } else {
                        device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event);
                    }
                }
            }
        }
    }

    if (have_post_event) {
        return DEV_ERR_NONE;
    } else {
        return DEV_ERR_UNKNOW_CLASS;
    }
}

const char *usb_host_valid_class_to_dev(const usb_dev id, u32 usbclass)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    struct usb_host_device *host_dev = &host_devices[usb_id];
    u32 itf_class;

    for (int i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i] &&
            host_dev->interface_info[i]->ctrl) {
            itf_class = host_dev->interface_info[i]->ctrl->interface_class;
            if (itf_class == usbclass) {
                switch (itf_class) {
                case USB_CLASS_MASS_STORAGE:
                    if (usb_id == 0) {
                        return "udisk0";
                    } else if (usb_id == 1) {
                        return "udisk1";
                    }
                    break;
                case USB_CLASS_ADB:
                    if (usb_id == 0) {
                        return "adb0";
                    } else if (usb_id == 1) {
                        return "adb1";
                    }
                    break;
                case USB_CLASS_AOA:
                    if (usb_id == 0) {
                        return "aoa0";
                    } else if (usb_id == 1) {
                        return "aoa1";
                    }
                    break;
                case USB_CLASS_HID:
                    if (usb_id == 0) {
                        return "hid0";
                    } else if (usb_id == 1) {
                        return "hid1";
                    }
                    break;
                case USB_CLASS_VIDEO:
                    if (usb_id == 0) {
                        return "uvc0";
                    } else if (usb_id == 1) {
                        return "uvc1";
                    }
                    break;
                }
            }
        }
    }
    return NULL;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_mount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
u32 usb_host_mount(const usb_dev id, u32 port, u32 retry, u32 reset_delay, u32 mount_timeout)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif

    u32 ret = DEV_ERR_NONE;

    os_mutex_pend(&usb_host_mutex, 0);

    struct usb_host_device *host_dev = &host_devices[usb_id];
    struct usb_private_data *private_data;

    if (port == 0) {
        private_data = &host_dev->private_data;
        if (private_data->status) {
            goto __exit_fail;
        }
        memset(host_dev, 0, sizeof(*host_dev));
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
        private_data = &host_dev->private_data;
    }

    host_dev->private_data.usb_id = id;

    usb_sem_init(host_dev);
    usb_mutex_init(host_dev);
    usb_h_isr_reg(usb_id, 5, 1);

    ret = _usb_host_mount(usb_id, port, retry, reset_delay, mount_timeout);

    usb_otg_resume(usb_id);  //打开usb host之后恢复otg检测，需要在host_mount之后
    if (ret) {
        goto __exit_fail;
    }

    ret = usb_event_notify(host_dev, 0);

    os_mutex_post(&usb_host_mutex);

    return ret;

__exit_fail:
    usb_sie_disable(usb_id);
    usb_sem_del(host_dev);
    usb_mutex_del(host_dev);
    os_mutex_post(&usb_host_mutex);

    return ret;
}

static u32 _usb_host_unmount(const usb_dev usb_id, u32 port)
{
    struct usb_host_device *host_dev;
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
    }

    struct usb_private_data *private_data = &host_dev->private_data;
    private_data->status = 0;

    usb_sem_post(host_dev);//拔掉设备时，让读写线程快速释放

    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i]) {
            if (host_dev->interface_info[i]->ctrl->set_power) {
                host_dev->interface_info[i]->ctrl->set_power(host_dev, 0);
            }
            /* if (host_dev->interface_info[i]->ctrl->release) { */
            /* host_dev->interface_info[i]->ctrl->release(host_dev); */
            /* } */

            if (!host_dev->father) {
                if (host_dev->interface_info[0] && host_dev->interface_info[0]->dev.hub) {
                    for (int k = 0; k < 7; k++) {
                        if (host_dev->interface_info[0]->dev.hub->child_dev[k] != NULL) {
                            break;
                        }
                    }

                }  else {
                    host_dev->interface_info[i] = NULL;
                }
            } else {
                host_dev->interface_info[i] = NULL;
            }

        }
    }

    if (port == 0) {
        usb_sie_close(usb_id);
    }
    return DEV_ERR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_unmount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
/* u32 usb_host_unmount(const usb_dev usb_id, char *device_name) */
u32 usb_host_unmount(const usb_dev id, u32 port)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
        if (!host_dev) {
            return 0;
        }
    }

    /* struct device_event event = {0}; */
    struct usb_private_data *private_data = &host_dev->private_data;

    os_mutex_pend(&usb_host_mutex, 0);

    if (private_data->status == 0) {
        goto __exit_fail;
    }

#if (TCFG_UDISK_ENABLE && UDISK_READ_512_ASYNC_ENABLE)
    _usb_stor_async_wait_sem(host_dev);
#endif
    ret = _usb_host_unmount(usb_id, port);
    if (ret) {
        goto __exit_fail;
    }
    usb_sem_del(host_dev);
    usb_mutex_del(host_dev);

    usb_event_notify(host_dev, 2);

    os_mutex_post(&usb_host_mutex);

    return DEV_ERR_NONE;

__exit_fail:
    os_mutex_post(&usb_host_mutex);

    return ret;
}

u32 usb_host_remount(const usb_dev id, u32 port, u32 retry, u32 delay, u32 ot, u8 notify)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;

    os_mutex_pend(&usb_host_mutex, 0);

    ret = _usb_host_unmount(usb_id, port);
    if (ret) {
        goto __exit_fail;
    }

    struct usb_host_device *host_dev = &host_devices[usb_id];
    os_sem_set(host_dev->sem, 0);

    ret = _usb_host_mount(usb_id, port, retry, delay, ot);
    if (ret) {
        goto __exit_fail;
    }

    if (notify) {
        usb_event_notify(host_dev, 1);
    }

    os_mutex_post(&usb_host_mutex);

    return DEV_ERR_NONE;

__exit_fail:
    os_mutex_post(&usb_host_mutex);

    return ret;
}

int usb_host_force_reset(const usb_dev usb_id)
{
    //预留以后加上hub的处理
    usb_h_force_reset(usb_id);
    return 0;
}

int host_device_ioctl(struct usb_host_device *host_dev, u32 cmd, void *arg)
{
    int err = 0;
    struct usb_private_data *private_data = &host_dev->private_data;

    if (!host_dev) {
        return -EINVAL;
    }
    switch (cmd) {
    case USBIOC_GET_DEVICE_ID: {
        struct usb_device_id *id = (struct usb_device_id *)arg;
        id->vendor = private_data->vendor_id;
        id->product = private_data->product_id;
    }
    break;
    case USBIOC_GET_MANUFACTURER: {
        struct usb_device_id *id = (struct usb_device_id *)arg;
        id->vendor = private_data->vendor_id;
        id->product = private_data->product_id;
    }
    break;
    case USBIOC_GET_PRODUCT_NAME: {
        struct usb_string *us = (struct usb_string *)arg;
        us->code_type = private_data->product[1];
        us->language = private_data->language;
        us->len = private_data->product[0] - 2;
        us->string = private_data->product + 2;
    }
    break;
    default:
        break;
    }

    return err;
}
#endif
