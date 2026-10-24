#include "usb/device/usb_stack.h"
#include "usb/usb_config.h"
#include "usb/device/cdc.h"
#include "app_config.h"
#include "os/os_api.h"
#include "printer.h"  //need redefine __u8, __u16, __u32

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[Printer]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_PRINTER_ENABLE

/* #define PRINTER_TEST_ENABLE */

static const u8 printer_desc[] = {
///Interface
    USB_DT_INTERFACE_SIZE,   //Length
    USB_DT_INTERFACE,   //DescriptorType:Inerface
    0x00,   //InterfaceNum:0
    0x00,   //AlternateSetting:0
    0x02,   //NumEndpoint:0
    USB_CLASS_PRINTER,   //InterfaceClass:Printer
    0x01,   //Printers
    0x02,   //InterfaceProtocol
    0x04,   //Interface String
///Endpoint IN
    USB_DT_ENDPOINT_SIZE,
    USB_DT_ENDPOINT,
    USB_DIR_IN | PTR_BULK_EP_IN,
    USB_ENDPOINT_XFER_BULK,
    LOBYTE(MAXP_SIZE_BULKIN), HIBYTE(MAXP_SIZE_BULKIN),
    0x01,
///Endpoint OUT
    USB_DT_ENDPOINT_SIZE,
    USB_DT_ENDPOINT,
    PTR_BULK_EP_OUT,
    USB_ENDPOINT_XFER_BULK,
    LOBYTE(MAXP_SIZE_BULKOUT), HIBYTE(MAXP_SIZE_BULKOUT),
    0x01,
};


struct usb_printer_handle {
    u8 *ep_out_dmabuffer;
    u8 *ep_in_dmabuffer;
    void (*wakeup_handle)(struct usb_device_t *usb_device);
    void (*reset_wakeup_handle)(struct usb_device_t *usb_device, u32 itf_num);
#ifdef PRINTER_TEST_ENABLE
    OS_SEM wake_sem;
    int thread_id;
    u8 thread_exit;
    u8 usb_id;
#endif
};

static struct usb_printer_handle *printer_handle[USB_MAX_HW_NUM];


static void printer_wakeup_handler(struct usb_device_t *usb_device)
{
#ifdef PRINTER_TEST_ENABLE
    const usb_dev usb_id = usb_device2id(usb_device);

    os_sem_post(&printer_handle[usb_id]->wake_sem);
#endif
}


static void printer_wakeup(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);

#if 0 //for test
    u32 len = 0;
    u8 read_ep[64];

    ep &= 0x0f;
    len = usb_g_intr_read(usb_id, ep, read_ep, sizeof(read_ep), 0);
    log_info("recv_len = %d, ep = %d\n", len, ep);
    put_buf(read_ep, len);
#else
    if ((printer_handle[usb_id]) && (printer_handle[usb_id]->wakeup_handle)) {
        printer_handle[usb_id]->wakeup_handle(usb_device);
    }
#endif
}


static void printer_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    usb_g_ep_config(usb_id, PTR_BULK_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 0, printer_handle[usb_id]->ep_in_dmabuffer, usb_device->bSpeed == USB_SPEED_HIGH ? MAXP_SIZE_BULKIN_HS : MAXP_SIZE_BULKIN_FS);
    usb_g_ep_config(usb_id, PTR_BULK_EP_OUT, USB_ENDPOINT_XFER_BULK, 1, printer_handle[usb_id]->ep_out_dmabuffer, usb_device->bSpeed == USB_SPEED_HIGH ? MAXP_SIZE_BULKOUT_HS : MAXP_SIZE_BULKOUT_FS);
    usb_g_set_intr_hander(usb_id, PTR_BULK_EP_OUT, printer_wakeup);

    usb_enable_ep(usb_id, PTR_BULK_EP_IN);
    usb_enable_ep(usb_id, PTR_BULK_EP_OUT);
}


static void reset_wakeup(struct usb_device_t *usb_device, u32 itf_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    if (printer_handle[usb_id] && printer_handle[usb_id]->reset_wakeup_handle) {
        printer_handle[usb_id]->reset_wakeup_handle(usb_device, itf_num);
    }

    printer_endpoint_init(usb_device, -1);
}


static const u8 device_id[] = {
    /* 0x00, 0x0D, 0x50, 0x72, 0x69, 0x6E, 0x74, 0x65, 0x72, 0x20, 0x56, 0x30, 0x2E, 0x31, 0x00 */
    0x00, 0x50, 0x4d, 0x46, 0x47, 0x3a, 0x50, 0x6f, 0x73, 0x74, 0x65, 0x4b, 0x50, 0x72, 0x69, 0x6e,
    0x74, 0x65, 0x72, 0x73, 0x3b, 0x4d, 0x44, 0x4c, 0x3a, 0x50, 0x4f, 0x53, 0x54, 0x45, 0x4b, 0x20,
    0x51, 0x38, 0x2f, 0x32, 0x30, 0x30, 0x3b, 0x43, 0x4d, 0x44, 0x3a, 0x50, 0x50, 0x4c, 0x49, 0x3b,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


static u32 printer_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;

    log_info("%s() %x %x", __func__, bRequestType, setup->bRequest);

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (setup->bRequest) {
        case USB_REQ_GET_STATUS:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_GET_INTERFACE:
            tx_len = 1;
            u8 i = 0;
            usb_set_data_payload(usb_device, setup, &i, tx_len);
            break;
        case USB_REQ_SET_INTERFACE:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        default:
            break;
        }
        break;
    case USB_TYPE_CLASS:
        switch (setup->bRequest) {
        case 0:
            /* printer_endpoint_init(usb_device, -1); */
            tx_len = sizeof(device_id);
            memcpy(tx_payload, device_id, sizeof(device_id));
            usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return 0;
}


u32 printer_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    log_info("%s() %d", __func__, *cur_itf_num);

    memcpy(ptr, printer_desc, sizeof(printer_desc));
    ptr[2] = *cur_itf_num;

    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        ptr[13] = LOBYTE(MAXP_SIZE_BULKIN_HS);
        ptr[14] = HIBYTE(MAXP_SIZE_BULKIN_HS);
        ptr[20] = LOBYTE(MAXP_SIZE_BULKOUT_HS);
        ptr[21] = HIBYTE(MAXP_SIZE_BULKOUT_HS);
    }

    if (usb_set_interface_hander(usb_id, *cur_itf_num, printer_itf_hander) != *cur_itf_num) {

    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, reset_wakeup) != *cur_itf_num) {

    }
    *cur_itf_num += 1;

    return sizeof(printer_desc);
}


#ifdef PRINTER_TEST_ENABLE
static void printer_data_handler_task(void *priv)
{
    struct usb_printer_handle *handle = (struct usb_printer_handle *)priv;

    u32 len;
    u8 buffer[64];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        os_sem_pend(&handle->wake_sem, 0);
        if (handle->thread_exit) {
            return;
        }
        len = usb_g_bulk_read(handle->usb_id, PTR_BULK_EP_OUT, buffer, sizeof(buffer), 0);
        log_debug("%s, recv_len = %d", __func__, len);

        put_buf(buffer, len);
        usb_g_bulk_write(handle->usb_id, PTR_BULK_EP_IN, buffer, len);
    }
}
#endif


void printer_register(const usb_dev usb_id)
{
    if (printer_handle[usb_id] == NULL) {
        printer_handle[usb_id] = (struct usb_printer_handle *)zalloc(sizeof(struct usb_printer_handle));
        if (printer_handle[usb_id] == NULL) {
            log_error("printer_register err");
            return;
        }
        log_info("printer_handle[%d] = %x", usb_id, printer_handle[usb_id]);

#if defined(FUSB_MODE) && FUSB_MODE
        printer_handle[usb_id]->ep_out_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_OUT | USB_DIR_OUT, MAXP_SIZE_BULKOUT_FS * 2);
        printer_handle[usb_id]->ep_in_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_IN | USB_DIR_IN, MAXP_SIZE_BULKIN_FS);
#else
        printer_handle[usb_id]->ep_out_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_OUT | USB_DIR_OUT, (usb_id ? MAXP_SIZE_BULKOUT_HS : MAXP_SIZE_BULKOUT_FS) * 2);
        printer_handle[usb_id]->ep_in_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_IN | USB_DIR_IN, usb_id ? MAXP_SIZE_BULKIN_HS : MAXP_SIZE_BULKIN_FS);
#endif
        printer_handle[usb_id]->wakeup_handle = printer_wakeup_handler;

#ifdef PRINTER_TEST_ENABLE
        os_sem_create(&printer_handle[usb_id]->wake_sem, 0);
        printer_handle[usb_id]->usb_id = usb_id;
        thread_fork("printer_data_handler_task", 20, 512, 0, &printer_handle[usb_id]->thread_id, printer_data_handler_task, printer_handle[usb_id]);
#endif
    }
}


void printer_release(const usb_dev usb_id)
{
    if (printer_handle[usb_id]) {
#ifdef PRINTER_TEST_ENABLE
        printer_handle[usb_id]->thread_exit = 1;
        os_sem_post(&printer_handle[usb_id]->wake_sem);
        thread_kill(&printer_handle[usb_id]->thread_id, KILL_WAIT);
        os_sem_del(&printer_handle[usb_id]->wake_sem, OS_DEL_ALWAYS);
#endif
        if (printer_handle[usb_id]->ep_out_dmabuffer) {
            usb_free_ep_dmabuffer(usb_id, printer_handle[usb_id]->ep_out_dmabuffer);
        }
        if (printer_handle[usb_id]->ep_in_dmabuffer) {
            usb_free_ep_dmabuffer(usb_id, printer_handle[usb_id]->ep_in_dmabuffer);
        }
        free(printer_handle[usb_id]);
        printer_handle[usb_id] = NULL;
    }
}


#endif

