#include "device/usb_stack.h"
#include "usb_config.h"
#include "os/os_api.h"
#include "cdc_defs.h"
#include "cdc_rndis.h"
#include "rndis_protocol.h"
#include "app_config.h"

#if TCFG_USB_SLAVE_RNDIS_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define RNDIS_RX_TEST_ENABLE    1
#define RNDIS_TX_TEST_ENABLE    1

#if RNDIS_RX_TEST_ENABLE
static OS_SEM rx_test_psem;
#endif

#if RNDIS_TX_TEST_ENABLE
static OS_SEM tx_test_psem;
#endif

#define RNDIS_RESPONSE_AVAILABLE_ENABLE     1   /* windows 下必须打开这个功能 */

struct usb_rndis_gadget {
    u8 id;
    u8 exit_flag;
    u8 mac[6];
    u8 *cdc_buffer;
    u8 *bulk_ep_out_buffer;
    u8 *bulk_ep_in_buffer;
    u8 *intr_ep_in_buffer;

    OS_MUTEX mutex_data;
    OS_MUTEX mutex_intr;
    OS_SEM psem;

    u8 *rndis_rx_buffer;
    u8 *rndis_tx_buffer;
    u8 *rndis_encapsulated_response_buffer;
    u8 *rndis_encapsulated_command_buffer;
    u8 *rndis_notify_response_buffer;

    u32 link_status;
    u32 speed;
    u32 max_size_cdc_bulkout;
    u32 max_size_cdc_bulkin;
    u32 net_filter;
    usb_eth_stat_t eth_state;
    rndis_state_t init_state;
#if RNDIS_RESPONSE_AVAILABLE_ENABLE
    int thread_id;
#endif
#if RNDIS_RX_TEST_ENABLE
    int rx_test_thread_id;
#endif
#if RNDIS_TX_TEST_ENABLE
    int tx_test_thread_id;
#endif
};

static struct usb_rndis_gadget *cdc_rndis_hdl[USB_MAX_HW_NUM];

#if USB_MALLOC_ENABLE
//
#else
/* fs = 64; hs = 512 */
static u8 _cdc_buffer[USB_MAX_HW_NUM][MAXP_SIZE_CDC_BULKOUT_HS] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
/* CDC_RNDIS_ETH_MAX_FRAME_SIZE + sizeof(rndis_data_packet_t) ; sizeof(rndis_data_packet_t) = 44 */
static u8 _rndis_rx_buffer[USB_MAX_HW_NUM][CDC_RNDIS_ETH_MAX_FRAME_SIZE + 44] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
static u8 _rndis_tx_buffer[USB_MAX_HW_NUM][CDC_RNDIS_ETH_MAX_FRAME_SIZE + 44] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
static u8 _rndis_encapsulated_response_buffer[USB_MAX_HW_NUM][CDC_RNDIS_RESP_BUFFER_SIZE] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
static u8 _rndis_encapsulated_command_buffer[USB_MAX_HW_NUM][CDC_RNDIS_RESP_BUFFER_SIZE] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
static u8 _rndis_notify_response_buffer[USB_MAX_HW_NUM][8] SEC(.cdc_rndis_var) __attribute__((aligned(4)));
static struct usb_rndis_gadget _cdc_rndis_hdl[USB_MAX_HW_NUM] SEC(.cdc_rndis_var);
#endif

/* RNDIS options list */
static const uint32_t oid_supported_list[] = {
    /* General OIDs */
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MEDIA_CONNECT_STATUS,

    OID_GEN_PHYSICAL_MEDIUM,

    /* General Statistic OIDs */
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    /* Please configure us */
    OID_GEN_RNDIS_CONFIG_PARAMETER,

    /* 802.3 OIDs */
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,

    /* 802.3 Statistic OIDs */
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

    OID_802_3_MAC_OPTIONS,
};

static const u8 cdc_rndis_desc[] = {
    //IAD Descriptor
    0x08,                                                   /* bLength */
    0x0b,                                                   /* bDescriptorType */
    0x00,                                                   /* bFirstInterface */
    0x02,                                                   /* bInterfaceCount */
    0xe0,                                                   /* bFunctionClass, Wireless Controller Class */
    0x01,                                                   /* bFunctionSubClass, DIRECT LINE CONTROL MODEL */
    0x03,                                                   /* bFunctionProtocol, RNDIS */
    0x00,                                                   /* iFunction */
    //Interface 0, Alt 0
    0x09,                                                   /* bLength */
    0x04,                                                   /* bDescriptorType */
    0x00,                                                   /* bInterfaceNumber */
    0x00,                                                   /* bAlternateSetting */
    0x01,                                                   /* bNumEndpoints */
    0xe0,                                                   /* bInterfaceClass, Wireless Controller Class */
    0x01,                                                   /* bInterfaceSubClass, DIRECT LINE CONTROL MODEL */
    0x03,                                                   /* bInterfaceProtocol, RNDIS */
    0x00,                                                   /* iInterface */
    //CDC Interface Descriptor
    0x05,                                                   /* bLength */
    0x24,                                                   /* bDescriptorType */
    0x00,                                                   /* bDescriptorSubType, Header Functional Desc */
    0x10, 0x01,                                             /* bcdCDC, version 1.10 */
    //CDC Interface Descriptor
    0x05,                                                   /* bLength */
    0x24,                                                   /* bDescriptorType */
    0x01,                                                   /* bDescriptorSubType, Call Management Functional Descriptor */
    0x00,                                                   /* bmCapabilities */
    0x01,                                                   /* bDataInterface */
    //CDC Interface Descriptor
    0x04,                                                   /* bLength */
    0x24,                                                   /* bDescriptorType */
    0x02,                                                   /* bDescriptorSubType, Abstract Control Management Functional Descriptor */
    0x00,                                                   /* bmCapabilities, D7..D2 reversed */
    //  D7..D4 reversed
    //  D3 supports the notification Network_Connection
    //  D2 not supports the request Send_Break
    //  D1 supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State
    //  D0 supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature
    //CDC Interface Descriptor
    0x05,                                                   /* bLength */
    0x24,                                                   /* bDescriptorType */
    0x06,                                                   /* bDescriptorSubType, Union Functional Descriptor */
    0x00,                                                   /* bMasterInterface */
    0x01,                                                   /* bSlaveInterface0 */
    //Endpoint In
    0x07,                                                   /* bLength */
    0x05,                                                   /* bDescriptorType */
    0x83,                                                   /* bEndpointAddress */
    0x03,                                                   /* bmAttributes, interrupt */
    0x08, 0x00,                                             /* wMaxPacketSize */
    0x10,                                                   /* bInterval */
    //Interface 1, Alt 0
    0x09,                                                   /* bLength */
    0x04,                                                   /* bDescriptorType */
    0x01,                                                   /* bInterfaceNumber */
    0x00,                                                   /* bAlternateSetting */
    0x02,                                                   /* bNumEndpoints */
    0x0a,                                                   /* InterfaceClass, CDC Data */
    0x00,                                                   /* bInterfaceSubClass */
    0x00,                                                   /* bInterfaceProtocol */
    0x00,                                                   /* iInterface */
    //Endpoint Out
    0x07,                                                   /* bLength */
    0x05,                                                   /* bDescriptorType */
    0x02,                                                   /* bEndpointAddress */
    0x02,                                                   /* bmAttributes, bulk */
    0x40, 0x00,                                             /* wMaxPacketSize */
    0x00,                                                   /* bInterval */
    //Endpoint In
    0x07,                                                   /* bLength */
    0x05,                                                   /* bDescriptorType */
    0x81,                                                   /* bEndpointAddress */
    0x02,                                                   /* bmAttributes, bulk */
    0x40, 0x00,                                             /* wMaxPacketSize */
    0x00                                                    /* bInterval */
};

static void rndis_endpoint_init(struct usb_device_t *usb_device, u32 itf);
static u32 rndis_setup_rx(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req);
static int rndis_encapsulated_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_init_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_halt_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_query_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_set_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_reset_command_handler(const usb_dev usb_id, u8 *data, u32 len);
static int rndis_keepalive_command_handler(const usb_dev usb_id, u8 *data, u32 len);
#if RNDIS_RX_TEST_ENABLE
static int cdc_rndis_rx_test_main(void *parm);
#endif
#if RNDIS_TX_TEST_ENABLE
static int cdc_rndis_tx_test_main(void *parm);
#endif

static void rndis_notify_response_available(const usb_dev usb_id)
{
    log_debug("%s()", __func__);
#if RNDIS_RESPONSE_AVAILABLE_ENABLE
    memset(cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer, 0x00, 8);
    cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer[0] = 0x01;

    os_sem_post(&cdc_rndis_hdl[usb_id]->psem);
#endif
}

static u32 rndis_setup(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    int recip_type;
    u32 len = 0;

    recip_type = ctrl_req->bRequestType & USB_TYPE_MASK;

    switch (recip_type) {
    case USB_TYPE_CLASS:
        switch (ctrl_req->bRequest) {
        case USB_CDC_SEND_ENCAPSULATED_COMMAND:
            log_info("encapsulated cmd.");
            usb_device->wDataLength = ctrl_req->wLength;    /* setup out data len */
            usb_device->setup_ptr = cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer;
            usb_set_setup_recv(usb_device, rndis_setup_rx);
            break;
        case USB_CDC_GET_ENCAPSULATED_RESPONSE:
            log_info("encapsulated response.");
            if (cdc_rndis_hdl[usb_id] == NULL) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                break;
            }

            len = ((rndis_generic_msg_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer)->MessageLength;
            usb_set_data_payload(usb_device, ctrl_req, cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer, len);
            break;
        default:
            log_error("unsupported class req");
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            break;
        }
        break;
    default:
        log_error("unsupported req type");
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    }
    return 0;
}

static u32 rndis_setup_rx(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    ASSERT(cdc_rndis_hdl[usb_id], "rndis not register");

    int recip_type;
    u32 len = min(usb_device->wDataLength, EP0_SETUP_LEN);
    //log_debug("rx len = %d", len);

    usb_read_ep0(usb_id, usb_device->setup_ptr, len);
    /* printf_buf(usb_device->setup_ptr, len); */
    usb_device->setup_ptr += len;
    usb_device->wDataLength -= len;
    if (usb_device->wDataLength == 0) {
        recip_type = ctrl_req->bRequestType & USB_TYPE_MASK;
        switch (recip_type) {
        case USB_TYPE_CLASS:
            switch (ctrl_req->bRequest) {
            case USB_CDC_SEND_ENCAPSULATED_COMMAND:
                log_info("USB_CDC_SEND_ENCAPSULATED_COMMAND");

                rndis_encapsulated_command_handler(usb_id, cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer, len);
                break;
            }
            break;
        }
        return USB_EP0_STAGE_SETUP;
    } else {
        return USB_EP0_STAGE_OUT;   /* 控制传输数据部分未结束 */
    }
}

static int rndis_encapsulated_command_handler(const usb_dev usb_id, u8 *data, u32 len)
{
    switch (((rndis_generic_msg_t *)data)->MessageType) {
    case REMOTE_NDIS_INITIALIZE_MSG:
        return rndis_init_command_handler(usb_id, data, len);
    case REMOTE_NDIS_HALT_MSG:
        return rndis_halt_command_handler(usb_id, data, len);
    case REMOTE_NDIS_QUERY_MSG:
        return rndis_query_command_handler(usb_id, data, len);
    case REMOTE_NDIS_SET_MSG:
        return rndis_set_command_handler(usb_id, data, len);
    case REMOTE_NDIS_RESET_MSG:
        return rndis_reset_command_handler(usb_id, data, len);
    case REMOTE_NDIS_KEEPALIVE_MSG:
        return rndis_keepalive_command_handler(usb_id, data, len);

    default:
        break;
    }
    return -1;
}

static int rndis_init_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    log_debug("%s()", __func__);

    rndis_initialize_msg_t *cmd = (rndis_initialize_msg_t *)data;
    rndis_initialize_cmplt_t *resp;

    resp = ((rndis_initialize_cmplt_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->RequestId = cmd->RequestId;
    resp->MessageType = REMOTE_NDIS_INITIALIZE_CMPLT;
    resp->MessageLength = sizeof(rndis_initialize_cmplt_t);
    resp->MajorVersion = RNDIS_MAJOR_VERSION;
    resp->MinorVersion = RNDIS_MINOR_VERSION;
    resp->Status = RNDIS_STATUS_SUCCESS;
    resp->DeviceFlags = RNDIS_DF_CONNECTIONLESS;
    resp->Medium = RNDIS_MEDIUM_802_3;
    resp->MaxPacketsPerTransfer = 1;
    resp->MaxTransferSize = CDC_RNDIS_ETH_MAX_FRAME_SIZE + sizeof(rndis_data_packet_t);
    //log_debug("[%s] max eth len = %d; rndis data packet len = %d", __func__, CDC_RNDIS_ETH_MAX_FRAME_SIZE, sizeof(rndis_data_packet_t));
    resp->PacketAlignmentFactor = 0;
    resp->AfListOffset = 0;
    resp->AfListSize = 0;

    cdc_rndis_hdl[usb_id]->init_state = rndis_initialized;

    rndis_notify_response_available(usb_id);
    return 0;
}

static int rndis_halt_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    log_debug("%s()", __func__);

    rndis_halt_msg_t *resp;

    resp = ((rndis_halt_msg_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->MessageLength = 0;

    cdc_rndis_hdl[usb_id]->init_state = rndis_uninitialized;

    return 0;
}

static int rndis_query_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    rndis_query_msg_t *cmd = (rndis_query_msg_t *)data;
    rndis_query_cmplt_t *resp;
    uint8_t *infomation_buffer;
    uint32_t infomation_len = 0;

    resp = ((rndis_query_cmplt_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->MessageType = REMOTE_NDIS_QUERY_CMPLT;
    resp->RequestId = cmd->RequestId;
    resp->InformationBufferOffset = sizeof(rndis_query_cmplt_t) - sizeof(rndis_generic_msg_t);
    resp->Status = RNDIS_STATUS_SUCCESS;

    infomation_buffer = (uint8_t *)resp + sizeof(rndis_query_cmplt_t);

    log_debug("%s() OID : 0x%02x", __func__, cmd->Oid);

    switch (cmd->Oid) {
    case OID_GEN_SUPPORTED_LIST:
        RNDIS_INQUIRY_PUT(oid_supported_list, sizeof(oid_supported_list));
        infomation_len = sizeof(oid_supported_list);
        break;
    case OID_GEN_HARDWARE_STATUS:
        RNDIS_INQUIRY_PUT_LE32(NDIS_HW_STS_READY);
        infomation_len = 4;
        break;
    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        RNDIS_INQUIRY_PUT_LE32(NDIS_MEDIUM_802_3);
        infomation_len = 4;
        break;
    case OID_GEN_MAXIMUM_FRAME_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        RNDIS_INQUIRY_PUT_LE32(CDC_RNDIS_ETH_MAX_FRAME_SIZE);
        infomation_len = 4;
        break;
    case OID_GEN_VENDOR_ID:
        RNDIS_INQUIRY_PUT_LE32(CDC_RNDIS_VENDOR_ID);
        infomation_len = 4;
        break;
    case OID_GEN_VENDOR_DRIVER_VERSION:
        RNDIS_INQUIRY_PUT_LE32(0x0001);
        infomation_len = 4;
        break;
    case OID_GEN_VENDOR_DESCRIPTION:
        RNDIS_INQUIRY_PUT(CDC_RNDIS_VENDOR_DESC, strlen(CDC_RNDIS_VENDOR_DESC));
        infomation_len = (strlen(CDC_RNDIS_VENDOR_DESC) + 1);
        break;
    case OID_802_3_CURRENT_ADDRESS:
    case OID_802_3_PERMANENT_ADDRESS:
        RNDIS_INQUIRY_PUT(cdc_rndis_hdl[usb_id]->mac, 6);
        infomation_len = 6;
        break;
    case OID_GEN_PHYSICAL_MEDIUM:
        RNDIS_INQUIRY_PUT_LE32(NDIS_MEDIUM_802_3);
        infomation_len = 4;
        break;
    case OID_GEN_LINK_SPEED:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->speed / 100);
        infomation_len = 4;
        break;
    case OID_GEN_CURRENT_PACKET_FILTER:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->net_filter);
        infomation_len = 4;
        break;
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        RNDIS_INQUIRY_PUT_LE32(CDC_RNDIS_ETH_MAX_FRAME_SIZE + CDC_RNDIS_RESP_BUFFER_SIZE);
        infomation_len = 4;
        break;
    case OID_GEN_MEDIA_CONNECT_STATUS:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->link_status);
        infomation_len = 4;
        break;
    case OID_GEN_RNDIS_CONFIG_PARAMETER:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_802_3_MAXIMUM_LIST_SIZE:
        RNDIS_INQUIRY_PUT_LE32(1); /* one address */
        infomation_len = 4;
        break;
    case OID_802_3_MULTICAST_LIST:
        //RNDIS_INQUIRY_PUT_LE32(0xE0000000); /* 224.0.0.0 */
        resp->Status = RNDIS_STATUS_NOT_SUPPORTED;
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_802_3_MAC_OPTIONS:
        // infomation_len = 0;
        resp->Status = RNDIS_STATUS_NOT_SUPPORTED;
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_GEN_MAC_OPTIONS:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_802_3_RCV_ERROR_ALIGNMENT:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_802_3_XMIT_ONE_COLLISION:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_802_3_XMIT_MORE_COLLISIONS:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    case OID_GEN_XMIT_OK:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->eth_state.txok);
        infomation_len = 4;
        break;
    case OID_GEN_RCV_OK:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->eth_state.rxok);
        infomation_len = 4;
        break;
    case OID_GEN_RCV_ERROR:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->eth_state.rxbad);
        infomation_len = 4;
        break;
    case OID_GEN_XMIT_ERROR:
        RNDIS_INQUIRY_PUT_LE32(cdc_rndis_hdl[usb_id]->eth_state.txbad);
        infomation_len = 4;
        break;
    case OID_GEN_RCV_NO_BUFFER:
        RNDIS_INQUIRY_PUT_LE32(0);
        infomation_len = 4;
        break;
    default:
        resp->Status = RNDIS_STATUS_FAILURE;
        infomation_len = 0;
        log_error("Unhandled query for Object ID 0x%x", cmd->Oid);
        break;
    }

    resp->MessageLength = sizeof(rndis_query_cmplt_t) + infomation_len;
    resp->InformationBufferLength = infomation_len;

    rndis_notify_response_available(usb_id);
    return 0;
}

static int rndis_set_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    log_debug("%s()", __func__);

    rndis_set_msg_t *cmd = (rndis_set_msg_t *)data;
    rndis_set_cmplt_t *resp;
    rndis_config_parameter_t *param;

    resp = ((rndis_set_cmplt_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->RequestId = cmd->RequestId;
    resp->MessageType = REMOTE_NDIS_SET_CMPLT;
    resp->MessageLength = sizeof(rndis_set_cmplt_t);
    resp->Status = RNDIS_STATUS_SUCCESS;

    switch (cmd->Oid) {
    case OID_GEN_RNDIS_CONFIG_PARAMETER:
        param = (rndis_config_parameter_t *)((uint8_t *) & (cmd->RequestId) + cmd->InformationBufferOffset);
        log_error("RNDIS cfg param: NameOfs=%d, NameLen=%d, ValueOfs=%d, ValueLen=%d",
                  param->ParameterNameOffset, param->ParameterNameLength,
                  param->ParameterValueOffset, param->ParameterValueLength);
        break;
    case OID_GEN_CURRENT_PACKET_FILTER:
        if (cmd->InformationBufferLength < sizeof(cdc_rndis_hdl[usb_id]->net_filter)) {
            log_error("PACKET_FILTER!");
            resp->Status = RNDIS_STATUS_INVALID_DATA;
        } else {
            uint32_t *filter;
            /* Parameter starts at offset buf_offset of the req_id field */
            filter = (uint32_t *)((uint8_t *) & (cmd->RequestId) + cmd->InformationBufferOffset);

            cdc_rndis_hdl[usb_id]->net_filter = *(uint32_t *)filter;
            if (cdc_rndis_hdl[usb_id]->net_filter) {
                cdc_rndis_hdl[usb_id]->init_state = rndis_data_initialized;
            } else {
                cdc_rndis_hdl[usb_id]->init_state = rndis_initialized;
            }
        }
        break;
    case OID_GEN_CURRENT_LOOKAHEAD:
        break;
    case OID_GEN_PROTOCOL_OPTIONS:
        break;
    case OID_802_3_MULTICAST_LIST:
        break;
    case OID_PNP_ADD_WAKE_UP_PATTERN:
    case OID_PNP_REMOVE_WAKE_UP_PATTERN:
    case OID_PNP_ENABLE_WAKE_UP:
    default:
        resp->Status = RNDIS_STATUS_FAILURE;
        log_error("Unhandled query for Object ID 0x%x", cmd->Oid);
        break;
    }

    rndis_notify_response_available(usb_id);

    return 0;
}

static int rndis_reset_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    log_debug("%s()", __func__);
    // rndis_reset_msg_t *cmd = (rndis_reset_msg_t *)data;
    rndis_reset_cmplt_t *resp;

    resp = ((rndis_reset_cmplt_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->MessageType = REMOTE_NDIS_RESET_CMPLT;
    resp->MessageLength = sizeof(rndis_reset_cmplt_t);
    resp->Status = RNDIS_STATUS_SUCCESS;
    resp->AddressingReset = 1;

    cdc_rndis_hdl[usb_id]->init_state = rndis_uninitialized;

    rndis_notify_response_available(usb_id);

    return 0;
}

static int rndis_keepalive_command_handler(const usb_dev usb_id, uint8_t *data, uint32_t len)
{
    log_debug("%s()", __func__);

    rndis_keepalive_msg_t *cmd = (rndis_keepalive_msg_t *)data;
    rndis_keepalive_cmplt_t *resp;

    resp = ((rndis_keepalive_cmplt_t *)cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    resp->RequestId = cmd->RequestId;
    resp->MessageType = REMOTE_NDIS_KEEPALIVE_CMPLT;
    resp->MessageLength = sizeof(rndis_keepalive_cmplt_t);
    resp->Status = RNDIS_STATUS_SUCCESS;

    rndis_notify_response_available(usb_id);

    return 0;
}

static void rndis_reset(struct usb_device_t *usb_device, u32 itf)
{
    log_error("%s()", __func__);

#if USB_ROOT2
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_disable_ep(usb_id, CDC_DATA_EP_IN);
    usb_disable_ep(usb_id, usb_id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN);
#else
    rndis_endpoint_init(usb_device, itf);
#endif
}

// CDC RNDIS DESCRIPTOR LEN : 8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 7
u32 rndis_desc_config(const usb_dev usb_id, u8 *ptr, u32 *itf_num)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    u8 *tptr;
    tptr = ptr;
    memcpy(tptr, cdc_rndis_desc, sizeof(cdc_rndis_desc));
    //iad interface number
    tptr[2] = *itf_num;
    //control interface number
    tptr[8 + 2] = *itf_num;     /* control interface number */
    tptr[8 + 9 + 5 + 4] = *itf_num + 1; /* data interface number */
    tptr[8 + 9 + 5 + 5 + 4 + 3] = *itf_num; /* union : master interface */
    tptr[8 + 9 + 5 + 5 + 4 + 4] = *itf_num + 1; /* union : slave interface */
    //interrupt in ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 2] = USB_DIR_IN | (usb_id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN);  /* bEndpointAddress */
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 4] = MAXP_SIZE_CDC_INTRIN & 0xff;  /* wMaxPacketSize L */
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 5] = (MAXP_SIZE_CDC_INTRIN >> 8) & 0xff;   /* wMaxPacketSize H */
    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        tptr[8 + 9 + 5 + 5 + 4 + 5 + 6] = CDC_INTR_INTERVAL_HS;  //high-speed mode, 125x2^(4-1)=1ms, bInterval
    }
    //data interface number
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 2] = *itf_num + 1; /* data interface number */
    //bulk out ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 2] = USB_DIR_OUT | CDC_DATA_EP_OUT;    /* bEndpointAddress */
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 4] = cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout & 0xff;     /* wMaxPacketSize L */
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 5] = (cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout >> 8) & 0xff;  /* wMaxPacketSize H */
    //bulk in ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 2] = USB_DIR_IN | CDC_DATA_EP_IN;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 4] = cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin & 0xff;  /* wMaxPacketSize L */
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 5] = (cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin >> 8) & 0xff;   /* wMaxPacketSize H */
    tptr += sizeof(cdc_rndis_desc);

    /* 只注册了的控制接口的 setup 回调函数 */
    if (usb_set_interface_hander(usb_id, *itf_num, rndis_setup) != *itf_num) {
        ASSERT(0, "rndis set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *itf_num, rndis_reset) != *itf_num) {

    }
    *itf_num += 2;
    return (u32)(tptr - ptr);
}

static void rndis_intrrx(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (cdc_rndis_hdl[usb_id] == NULL) {
        return;
    }

#if RNDIS_RX_TEST_ENABLE
    os_sem_post(&rx_test_psem);
    //log_debug("rndis rx len = %d.", len);
#else
    u8 *rx_buf = cdc_rndis_hdl[usb_id]->cdc_buffer;
    //由于bulk传输使用双缓冲，无法用usb_get_ep_buffer()知道是哪一个buffer，需要外部buffer接收数据
    u32 len = usb_g_bulk_read(usb_id, CDC_DATA_EP_OUT, rx_buf, cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout, 0);
    log_debug("rndis rx len = %d.", len);
#endif
}

static void rndis_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    ASSERT(cdc_rndis_hdl[usb_id], "rndis not register");

    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_HS;
        cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_HS;
    } else {
        cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_FS;
        cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_FS;
    }

    usb_g_ep_config(usb_id, CDC_DATA_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK,
                    0, cdc_rndis_hdl[usb_id]->bulk_ep_in_buffer, cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin);

    usb_enable_ep(usb_id, CDC_DATA_EP_IN);

    usb_g_ep_config(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, USB_ENDPOINT_XFER_BULK,
                    1, cdc_rndis_hdl[usb_id]->bulk_ep_out_buffer, cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout);

    usb_g_set_intr_hander(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, rndis_intrrx);

    usb_enable_ep(usb_id, CDC_DATA_EP_OUT);

    usb_g_ep_config(usb_id, (usb_id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN) | USB_DIR_IN, USB_ENDPOINT_XFER_INT,
                    0, cdc_rndis_hdl[usb_id]->intr_ep_in_buffer, MAXP_SIZE_CDC_INTRIN);
    usb_enable_ep(usb_id, usb_id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN);
}

#if RNDIS_RESPONSE_AVAILABLE_ENABLE
static void cdc_rndis_notify_task(void *parm)
{
    struct usb_rndis_gadget *rndis_hdl = (struct usb_rndis_gadget *)parm;

    os_sem_create(&rndis_hdl->psem, 0);

    while (1) {
        os_sem_pend(&rndis_hdl->psem, 0);
        if (rndis_hdl->exit_flag) {
            os_sem_del(&rndis_hdl->psem, OS_DEL_ALWAYS);
            return;
        }
        os_mutex_pend(&rndis_hdl->mutex_intr, 0);
        usb_g_intr_write(rndis_hdl->id, rndis_hdl->id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN, rndis_hdl->rndis_notify_response_buffer, 8);
        os_mutex_post(&rndis_hdl->mutex_intr);
        log_debug("rndis intr in.");
    }
}
#endif

void rndis_register(const usb_dev usb_id)
{
    uint8_t template_mac[6] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    /* log_info("%s() %d", __func__, __LINE__); */
    if (!cdc_rndis_hdl[usb_id]) {
#if USB_MALLOC_ENABLE
        cdc_rndis_hdl[usb_id] = zalloc(sizeof(struct usb_rndis_gadget));
        if (!cdc_rndis_hdl[usb_id]) {
            log_error("rndis_register err 1");
            return;
        }

        if (usb_id == 0) {
            cdc_rndis_hdl[usb_id]->speed = 12000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_FS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_FS;
        } else {
#if defined(FUSB_MODE) && FUSB_MODE
            cdc_rndis_hdl[usb_id]->speed = 12000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_FS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_FS;
#elif defined(FUSB_MODE) && (FUSB_MODE==0)
            cdc_rndis_hdl[usb_id]->speed = 480000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_HS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_HS;
#else
#error "USB_SPEED_MODE not defined"
#endif
        }
        cdc_rndis_hdl[usb_id]->cdc_buffer = malloc(cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout);
        if (!cdc_rndis_hdl[usb_id]->cdc_buffer) {
            log_error("rndis_register err 2");
            goto __exit_err;
        }
        cdc_rndis_hdl[usb_id]->rndis_rx_buffer = malloc(CDC_RNDIS_ETH_MAX_FRAME_SIZE + sizeof(rndis_data_packet_t));
        if (!cdc_rndis_hdl[usb_id]->rndis_rx_buffer) {
            log_error("rndis_register err 3");
            goto __exit_err;
        }
        cdc_rndis_hdl[usb_id]->rndis_tx_buffer = malloc(CDC_RNDIS_ETH_MAX_FRAME_SIZE + sizeof(rndis_data_packet_t));
        if (!cdc_rndis_hdl[usb_id]->rndis_tx_buffer) {
            log_error("rndis_register err 4");
            goto __exit_err;
        }
        cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer = malloc(CDC_RNDIS_RESP_BUFFER_SIZE);
        if (!cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer) {
            log_error("rndis_register err 5");
            goto __exit_err;
        }
        cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer = malloc(CDC_RNDIS_RESP_BUFFER_SIZE);
        if (!cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer) {
            log_error("rndis_register err 6");
            goto __exit_err;
        }
        cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer = malloc(8);
        if (!cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer) {
            log_error("rndis_register err 7");
            goto __exit_err;
        }
#else
        memset(&_cdc_hdl[usb_id], 0, sizeof(struct usb_cdc_gadget));
        cdc_rndis_hdl[usb_id] = &_cdc_hdl[usb_id];

        if (usb_id == 0) {
            cdc_rndis_hdl[usb_id]->speed = 12000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_FS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_FS;
        } else {
#if defined(FUSB_MODE) && FUSB_MODE
            cdc_rndis_hdl[usb_id]->speed = 12000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_FS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_FS;
#elif defined(FUSB_MODE) && (FUSB_MODE==0)
            cdc_rndis_hdl[usb_id]->speed = 480000000;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout = MAXP_SIZE_CDC_BULKOUT_HS;
            cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin = MAXP_SIZE_CDC_BULKIN_HS;
#else
#error "USB_SPEED_MODE not defined"
#endif
        }
        cdc_rndis_hdl[usb_id]->cdc_buffer = _cdc_buffer[usb_id];
        cdc_rndis_hdl[usb_id]->rndis_rx_buffer = _rndis_rx_buffer;
        cdc_rndis_hdl[usb_id]->rndis_tx_buffer = _rndis_tx_buffer;
        cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer = _rndis_encapsulated_response_buffer;
        cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer = _rndis_encapsulated_command_buffer;
        cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer = _rndis_notify_response_buffer;
#endif

        os_mutex_create(&cdc_rndis_hdl[usb_id]->mutex_data);

        cdc_rndis_hdl[usb_id]->bulk_ep_in_buffer = usb_alloc_ep_dmabuffer(usb_id, CDC_DATA_EP_IN | USB_DIR_IN, \
                cdc_rndis_hdl[usb_id]->max_size_cdc_bulkin);
        cdc_rndis_hdl[usb_id]->bulk_ep_out_buffer = usb_alloc_ep_dmabuffer(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, \
                cdc_rndis_hdl[usb_id]->max_size_cdc_bulkout * 2);

        os_mutex_create(&cdc_rndis_hdl[usb_id]->mutex_intr);
        cdc_rndis_hdl[usb_id]->intr_ep_in_buffer = usb_alloc_ep_dmabuffer(usb_id, (usb_id ? CDC_INTR_HUSB_EP_IN : CDC_INTR_FUSB_EP_IN) | USB_DIR_IN, MAXP_SIZE_CDC_INTRIN);

        //TODO
        memcpy(cdc_rndis_hdl[usb_id]->mac, template_mac, 6);

        cdc_rndis_hdl[usb_id]->mac[5] -= usb_id;

        cdc_rndis_hdl[usb_id]->id = usb_id;

#if RNDIS_RESPONSE_AVAILABLE_ENABLE
        if (thread_fork("cdc_rndis_notify_task", 10, 512, 0, &cdc_rndis_hdl[usb_id]->thread_id, cdc_rndis_notify_task, cdc_rndis_hdl[usb_id]) != OS_NO_ERR) {
            log_error("cdc_rndis_notify_task thread fork fail");
        }
#endif
#if RNDIS_RX_TEST_ENABLE
        cdc_rndis_rx_test_main(cdc_rndis_hdl[usb_id]);
#endif
#if RNDIS_TX_TEST_ENABLE
        cdc_rndis_tx_test_main(cdc_rndis_hdl[usb_id]);
#endif
    }

    return;
__exit_err:
#if USB_MALLOC_ENABLE
    if (cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer) {
        free(cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer);
    }
    if (cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer) {
        free(cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer);
    }
    if (cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer) {
        free(cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
    }
    if (cdc_rndis_hdl[usb_id]->rndis_tx_buffer) {
        free(cdc_rndis_hdl[usb_id]->rndis_tx_buffer);
    }
    if (cdc_rndis_hdl[usb_id]->rndis_rx_buffer) {
        free(cdc_rndis_hdl[usb_id]->rndis_rx_buffer);
    }
    if (cdc_rndis_hdl[usb_id]->cdc_buffer) {
        free(cdc_rndis_hdl[usb_id]->cdc_buffer);
    }
    if (cdc_rndis_hdl[usb_id]) {
        free(cdc_rndis_hdl[usb_id]);
    }
#endif
    cdc_rndis_hdl[usb_id] = NULL;
}

void rndis_release(const usb_dev usb_id)
{
    /* log_info("%s() %d", __func__, __LINE__); */
    if (cdc_rndis_hdl[usb_id]) {
        cdc_rndis_hdl[usb_id]->exit_flag = 1;

#if RNDIS_RESPONSE_AVAILABLE_ENABLE
        if (cdc_rndis_hdl[usb_id]->thread_id) {
            os_sem_post(&cdc_rndis_hdl[usb_id]->psem);
            thread_kill(&cdc_rndis_hdl[usb_id]->thread_id, KILL_WAIT);
        }
#endif
#if RNDIS_RX_TEST_ENABLE
        if (cdc_rndis_hdl[usb_id]->rx_test_thread_id) {
            os_sem_post(&rx_test_psem);
            thread_kill(&cdc_rndis_hdl[usb_id]->rx_test_thread_id, KILL_WAIT);
        }
#endif
#if RNDIS_TX_TEST_ENABLE
        if (cdc_rndis_hdl[usb_id]->tx_test_thread_id) {
            os_sem_post(&tx_test_psem);
            thread_kill(&cdc_rndis_hdl[usb_id]->tx_test_thread_id, KILL_WAIT);
        }
#endif
        if (cdc_rndis_hdl[usb_id]->bulk_ep_in_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_rndis_hdl[usb_id]->bulk_ep_in_buffer);
            cdc_rndis_hdl[usb_id]->bulk_ep_in_buffer = NULL;
        }
        if (cdc_rndis_hdl[usb_id]->bulk_ep_out_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_rndis_hdl[usb_id]->bulk_ep_out_buffer);
            cdc_rndis_hdl[usb_id]->bulk_ep_out_buffer = NULL;
        }
        if (cdc_rndis_hdl[usb_id]->intr_ep_in_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_rndis_hdl[usb_id]->intr_ep_in_buffer);
            cdc_rndis_hdl[usb_id]->intr_ep_in_buffer = NULL;
        }
        os_mutex_del(&cdc_rndis_hdl[usb_id]->mutex_data, OS_DEL_ALWAYS);
        os_mutex_del(&cdc_rndis_hdl[usb_id]->mutex_intr, OS_DEL_ALWAYS);

#if USB_MALLOC_ENABLE
        free(cdc_rndis_hdl[usb_id]->rndis_notify_response_buffer);
        free(cdc_rndis_hdl[usb_id]->rndis_encapsulated_command_buffer);
        free(cdc_rndis_hdl[usb_id]->rndis_encapsulated_response_buffer);
        free(cdc_rndis_hdl[usb_id]->rndis_tx_buffer);
        free(cdc_rndis_hdl[usb_id]->rndis_rx_buffer);
        free(cdc_rndis_hdl[usb_id]->cdc_buffer);
        free(cdc_rndis_hdl[usb_id]);
#endif
        cdc_rndis_hdl[usb_id] = NULL;
    }
}

#if RNDIS_RX_TEST_ENABLE
static void cdc_rndis_rx_test_task(void *parm)
{
    struct usb_rndis_gadget *rndis_hdl = (struct usb_rndis_gadget *)parm;

    os_sem_create(&rx_test_psem, 0);

    while (1) {
        os_sem_pend(&rx_test_psem, 0);
        if (rndis_hdl->exit_flag) {
            os_sem_del(&rx_test_psem, OS_DEL_ALWAYS);
            return;
        }
        os_mutex_pend(&rndis_hdl->mutex_data, 0);
        //由于bulk传输使用双缓冲，无法用usb_get_ep_buffer()知道是哪一个buffer，需要外部buffer接收数据
        u32 len = usb_g_bulk_read(rndis_hdl->id, CDC_DATA_EP_OUT, rndis_hdl->cdc_buffer, rndis_hdl->max_size_cdc_bulkout, 0);
        os_mutex_post(&rndis_hdl->mutex_data);
        log_debug("rndis rx len = %d.", len);
        log_debug("rndis data in.");
        os_sem_post(&tx_test_psem);
    }
}

static int cdc_rndis_rx_test_main(void *parm)
{
    struct usb_rndis_gadget *rndis_hdl = (struct usb_rndis_gadget *)parm;

    return thread_fork("cdc_rndis_rx_test_task", 11, 512, 0, &rndis_hdl->rx_test_thread_id, cdc_rndis_rx_test_task, parm);
}
#endif

#if RNDIS_TX_TEST_ENABLE
static void cdc_rndis_tx_test_task(void *parm)
{
    struct usb_rndis_gadget *rndis_hdl = (struct usb_rndis_gadget *)parm;

    u8 buff[100];

    os_sem_create(&tx_test_psem, 0);

    while (1) {
        os_sem_pend(&tx_test_psem, 0);
        if (rndis_hdl->exit_flag) {
            os_sem_del(&tx_test_psem, OS_DEL_ALWAYS);
            return;
        }
        os_mutex_pend(&rndis_hdl->mutex_data, 0);
        usb_g_bulk_write(rndis_hdl->id, CDC_DATA_EP_IN, buff, 100);
        os_mutex_post(&rndis_hdl->mutex_data);

        log_debug("rndis data out.");
    }
}

static int cdc_rndis_tx_test_main(void *parm)
{
    struct usb_rndis_gadget *rndis_hdl = (struct usb_rndis_gadget *)parm;

    return thread_fork("cdc_rndis_tx_test_task", 11, 512, 0, &rndis_hdl->tx_test_thread_id, cdc_rndis_tx_test_task, parm);
}
#endif

#endif

