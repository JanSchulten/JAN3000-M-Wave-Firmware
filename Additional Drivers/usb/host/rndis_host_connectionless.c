#include "event/event.h"
#include "event/device_event.h"
#include "os/os_api.h"
#include "usb_host.h"
#include "usbnet.h"
#include "cdc_defs.h"
#include "device_drive.h"
#include "host/usb_ctrl_transfer.h"
#include "system/init.h"
#include "app_config.h"

#if TCFG_HOST_RNDIS_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[net]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/* test 只针对 usb1，注意 usb_id */
#define RNDIS_RX_TEST_ENABLE    1
#define RNDIS_TX_TEST_ENABLE    1

#if RNDIS_RX_TEST_ENABLE
static OS_SEM rx_test_psem;
#endif

#define USB_MAX_PACKET_SIZE     (512)
#define LTE_RX_BUFF_SIZE	    (1536)    //(1518)align(32)
#define LTE_TX_BUFF_SIZE	    (1536)    //(1518)align(32)
#define USB_RNDIS_HEAD_SIZE     (44)      // (44) align(32)

static bool is_online;
static u16 frame_len;
static u16 recv_len;
/* Let the ethernet packets use a space beginning here for buffering */
static u8 tx_buf_addr[(LTE_TX_BUFF_SIZE + USB_RNDIS_HEAD_SIZE)] ALIGNE(4);
static u8 rx_buf_addr[(LTE_RX_BUFF_SIZE + USB_RNDIS_HEAD_SIZE + USB_MAX_PACKET_SIZE)] ALIGNE(4);

static void rndis_rx_isr(usb_dev usb_id, u8 *buf, u32 len)
{
#if RNDIS_RX_TEST_ENABLE
    os_sem_post(&rx_test_psem);
    log_debug("rndis rx len = %d.", len);
    //TODO，解析收包
#else
    log_debug("rndis rx len = %d.", len);
#endif

    usbnet_host_bulk_only_receive_int(usb_id);
}

static int rndis_host_connectionless_event_handler(struct sys_event *event)
{
    struct usb_host_device *host_dev = NULL;
    struct device_event *dev_event = (struct device_event *)event->payload;
    const char *name = (const char *)dev_event->value;

    if (event->from != DEVICE_EVENT_FROM_USB_HOST) {
        return 0;
    }

    int usb_id = ((const char *)dev_event->arg)[8] - '0';

    if (!strncmp(name, "rndis", strlen("rndis"))) {
        switch (dev_event->event) {
        case DEVICE_EVENT_IN:
            host_dev = (struct usb_host_device *)host_id2device(usb_id);
            log_info("rndis device in.");
            extern s32 generic_rndis_bind(struct usb_host_device * device, s32 flags);
            if (0 != generic_rndis_bind(host_dev, 0x01)) {
                break;
            }

            frame_len = 0;
            recv_len = 0;
            memset(rx_buf_addr, 0, sizeof(rx_buf_addr));
            usbnet_set_rx_complete_cb(rndis_rx_isr);
            usbnet_host_bulk_only_receive_int(usb_id);
            is_online = usb_id + 1;
            break;
        case DEVICE_EVENT_OUT:
            log_info("rndis device out.");

            is_online = 0;
            break;

        default:
            break;
        }
    }

    return 0;
}

SYS_EVENT_STATIC_HANDLER_REGISTER(rndis_host_connectionless_event, 0) = {
    .event_type     = SYS_DEVICE_EVENT,
    .prob_handler   = rndis_host_connectionless_event_handler,
    .post_handler   = NULL,
};

#if RNDIS_RX_TEST_ENABLE
static void host_rndis_rx_test_task(void *parm)
{
    os_sem_create(&rx_test_psem, 0);
    while (1) {
        os_sem_pend(&rx_test_psem, 0);
        log_debug("rndis data in.");
    }
}

static int host_rndis_rx_test_main(void)
{
    return thread_fork("host_rndis_rx_test_task", 11, 512, 0, NULL, host_rndis_rx_test_task, NULL);
}
late_initcall(host_rndis_rx_test_main);
#endif

#if RNDIS_TX_TEST_ENABLE
void host_rndis_tx_test_task(void *parm)
{
    while (1) {
        if (is_online) {
            //TODO，发包格式
            usbnet_host_bulk_only_send(is_online - 1, tx_buf_addr, 100);
            log_debug("rndis data out.");
        }
        os_time_dly(100);
    }
}

static int host_rndis_tx_test_main(void)
{
    return thread_fork("host_rndis_tx_test_task", 11, 512, 0, NULL, host_rndis_tx_test_task, NULL);
}
late_initcall(host_rndis_tx_test_main);
#endif

#endif

