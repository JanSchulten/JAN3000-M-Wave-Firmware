#include "system/includes.h"
#include "device/usb_stack.h"
#include "usb_config.h"
#include "os/os_api.h"
#include "app_config.h"

#if TCFG_USB_SLAVE_UVC_ENABLE
#include "uvc_test_data.h"
#include "slave_uvc.h"
#include "video/video_ioctl.h"
#include "video/video.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct usb_uvc_camera {
    u32 itf_base: 8;
    u32 tx_broken: 1;
    u32 is_new_frame: 1;
    u32 reserved: 22;
    u16 width[FMT_MAX_RESO_NUM * 2];
    u16 height[FMT_MAX_RESO_NUM * 2];
    int (*video_open)(int idx, int fmt, int frame_id, int fps);
    int (*video_reqbuf)(int idx, void *buf, u32 len, u32 *frame_end);
    int (*video_close)(int idx);
    int (*processing_unit_response)(struct uvc_unit_ctrl *ctl_req);
    int (*private_cmd_response)(u16 cmd, u16 data);
    u32 uvc_status;
    u32 last_len;
    u8 *stream_buf;
    u8 *ep_dma_buffer;
};
static struct usb_uvc_camera *uvc_handle[USB_MAX_HW_NUM];
#if USB_MALLOC_ENABLE
#else
static struct usb_uvc_camera _uvc_handle[USB_MAX_HW_NUM];
static u8 _stream_buf[UVC_PKT_SPILT * UVC_FIFO_TXMAXP];
#endif

typedef void (*pUVC_SETUP_HOOK)(usb_dev, struct usb_device_t *, struct usb_ctrlrequest *);

static u32 uvc_vc_itf_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
static u32 uvc_vs_itf_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
static u32 uvc_vc_itf_recv_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
static u32 uvc_vs_itf_recv_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
static void usb_video_open_camera(usb_dev usb_id);
static void usb_video_close_camera(usb_dev usb_id);


void uvc_vc_pu_wBacklightCompensation_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wBrightness_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wContrast_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wGain_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bPowerLineFrequency_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wHue_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wSaturation_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wSharpness_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wGamma_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wWhiteBalanceTemperature_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bWhiteBalanceTemperatureAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_white_balance_component_control_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bWhiteBalanceComponentAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wMultiplierStep_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_wMultiplierLimit_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bHueAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bVideoStandard_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vc_pu_bStatus_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);

void uvc_vs_probe_commit_ctl_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);
void uvc_vs_still_probe_commit_ctl_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req);

#define UVC_TEST_ENABLE       0

#define UVC_YUV_DATA_WIDTH 	TCFG_SLAVE_UVC_YUV_DATA_OUT_WIDTH
#define UVC_YUV_DATA_HEIGHT	TCFG_SLAVE_UVC_YUV_DATA_OUT_HEIGHT

/*------------------------------- uvc descriptor -------------------------------*/
/*********************************************************/
/*
			   Video Class
*/
/*********************************************************/
static const u8 uvc_interface_desc_header[] = {
/// Interface Association Descriptor  IAD descriptor
    USB_DT_INTERFACE_ASSOCIATION_SIZE,       //bLength
    USB_DT_INTERFACE_ASSOCIATION,       //bDescriptorType
    0x00,       //bFirstInterface:0
    0x02,       //bInterfaceCount:0x02
    USB_CLASS_VIDEO,   //bFunctionClass:USB_CLASS_VIDEO
    SC_VIDEO_INTERFACE_COLLECTION,  //bFunctionSubClass:SC_VIDEO_INTERFACE_COLLECTION
    UVC_PC_PROTOCOL_UNDEFINED,      //bFunctionProtocol:Not used. Must be set to UVC_PC_PROTOCOL_UNDEFINED
    VIDEO_STR_INDEX, //iFunction !! must match iInterface in VC Interface descriptor. Index of a string descriptor that describes this interface
};
static const u8 uvc_vc_interface_desc[] = {
///standard VC interface descriptor:   /*0x08*/
    USB_DT_INTERFACE_SIZE,       //Length
    USB_DT_INTERFACE,       //DescriptorType:Inerface
    0x00,       //InterfaceNum:0  !!
    0x00,       //AlternateSetting:0
    0x00,       //NumEndpoint:1 非零端点数目(interrupt endpoint)
    USB_CLASS_VIDEO,   //InterfaceClass:USB_CLASS_VIDEO
    SC_VIDEOCONTROL,     //InterfaceSubClass:SC_VIDEOCONTROL
    UVC_PC_PROTOCOL_UNDEFINED,       //bFunctionProtocol:Not used. Must be set to UVC_PC_PROTOCOL_UNDEFINED
    VIDEO_STR_INDEX,       //iInterface
///class-specific VC interface descriptor  /*0x11*/
    /* 0x0e,    	//Length */
    0x0d,
    USB_DT_CS_INTERFACE,   	//DescriptorType:cs_interface
    VC_HEADER, 	//DescriptorSubType:vc_header subtype
    0x00, 0x01, //bcdUVC:VC Device Class v1.00
    0x4d, 0x00, //TotalLength of class-specific VC interface descriptors
    DW1BYTE(UVC_DWCLOCKFREQUENCY),
    DW2BYTE(UVC_DWCLOCKFREQUENCY),
    DW3BYTE(UVC_DWCLOCKFREQUENCY),
    DW4BYTE(UVC_DWCLOCKFREQUENCY), //dwClockFreq
    /* 0x02,		//InCollection: Num of streaming interfaces */
    /* 0x01,      	//InterfaceNr(1) - VS #1 id VideoStreaming interface 1 belongs to this VideoControl interface */
    /* 0x02,      	//InterfaceNr(2) - VS #2 id VideoStreaming interface 1 belongs to this VideoControl interface */
    0x01,		//InCollection: Num of streaming interfaces
    0x01,      	//InterfaceNr(0) - VS #0 id VideoStreaming interface 1 belongs to this VideoControl interface
};
static const u8 uvc_terminal_desc[] = {
//Video Input Terminal Descriptor
    0x12,       //Length
    USB_DT_CS_INTERFACE,	      	 //DescriptorType:cs_interface
    VC_INPUT_TERMINAL,      	 	 //DescriptorSubType:vc_input_terminal subtype
    UVC_IT_TERMINALID,      		 //bTerminalID:0x01
    LOBYTE(ITT_CAMERA), HIBYTE(ITT_CAMERA),  //TerminalType:itt_camera type
    0x00,       //AssocTerminal
    0x00,       //iTerminal
    0x00, 0x00, 	//wObjectiveFocalLengthMin
    0x00, 0x00, //wObjectiveFocalLengthMax
    0x00, 0x00, //wOcularFocalLength
    0x03,      	//bControlSize
    0x00, 0x00, 0x00, //bmControls

///Processing Unit descriptor
    0x0B,       //Length
    USB_DT_CS_INTERFACE,        //DescriptorType:cs_interface
    VC_PROCESSING_UNIT,        //DescriptorSubType:vc_processing_unit subtype
    UVC_PU_TERMINALID,       //UnitID:0x03
    UVC_IT_TERMINALID,		//SourceID:this input pin of this unit is connected to the input pin of unit with ID 0x01
    0x00, 0x00, //MaxMultiplier
    0x02,       //ControlSize
    0x00, 0x00,   //mControls: Brightness control supported
    0x00,       //iProcessing

///Video Extension Unit descriptor(Camera)
    0x1A,       //Length
    USB_DT_CS_INTERFACE,        //DescriptorType:cs_interface
    VC_EXTENSION_UNIT,        //DescriptorSubType:vc_Extension_unit
    UVC_EXT_TERMINALID1,       //TerminalID:0x04
    UVC_GUID_UNIT_EXTENSION1,//guidExtensionCode

    0x08,//0x03,       //NumControl ??
    0x01,       //NrlnPins
    UVC_PU_TERMINALID,       //SourceID
    0x01,       //ControlSize
    0x00,  		//mControls
    0x00,

///Video Extension Unit descriptor(Camera)
    0x1A,       //Length
    USB_DT_CS_INTERFACE,        //DescriptorType:cs_interface
    VC_EXTENSION_UNIT,        //DescriptorSubType:vc_Extension_unit
    UVC_EXT_TERMINALID2,       //TerminalID:0x04
    UVC_GUID_UNIT_EXTENSION2,//guidExtensionCode

    0x08,//0x03,       //NumControl ??
    0x01,       //NrlnPins
    UVC_EXT_TERMINALID1,       //SourceID
    0x01,       //ControlSize
    0x00,  		//mControls
    0x00,

///Output Terminal descriptor
    0x09,       //Length
    USB_DT_CS_INTERFACE,			 //DescriptorType:cs_interface
    VC_OUTPUT_TERMINAL,				 //DescriptorSubType:vc_output_terminal subtype
    UVC_OT_TERMINALID1,  	    	 //bTerminalID:0x02
    LOBYTE(TT_STREAMING), HIBYTE(TT_STREAMING),//wTerminalType:usb streaming terminal
    0x00,       //AssocTerminal
    UVC_EXT_TERMINALID2,       //SourceID: the input pin of this unit is connected to the output pin of unit 3
    0x00,       //Terminal
};
static const u8 uvc_standard_intr_endpoint_desc[] = {
///standard Interrupt Endpoint descriptor:
    USB_DT_ENDPOINT_SIZE,       //Length
    USB_DT_ENDPOINT,        //DescriptorType:enp descriptor
    USB_DIR_IN | VIDEO_STATUS_EP_IN,     //Endp_addr:IN endpoint 3
    USB_ENDPOINT_XFER_INT,       //Attributes :Interrupt transfer type
    LOBYTE(VIDEO_STATUS_TXMAXP), HIBYTE(VIDEO_STATUS_TXMAXP), //MaxPacketSize:8byte status packet
    0x1,       //Interval:

///class-specific Interrupt Endpoint descriptor
    0x05,    	//Length
    USB_DT_CS_ENDPOINT,       	//DescriptorType:cs_endpoint descriptor
    USB_ENDPOINT_XFER_INT,     	//DescriptorSubType:ep_interrupt
    LOBYTE(VIDEO_STATUS_TXMAXP), HIBYTE(VIDEO_STATUS_TXMAXP), //MaxPacketSize:8byte status packet
};
static const u8 uvc_vs_interface_desc[] = {
#if UVC_ISO_MODE
///standard VS interface descriptor:(Interface 2, Alternate Setting 1):
    USB_DT_INTERFACE_SIZE,       //Length
    USB_DT_INTERFACE,      //DescriptorType:Inerface
    0x01,       //InterfaceNum:1  !!
    0x00,       //AlternateSetting
    0x00,       //NumEndpoint:1 非零端点数目
    USB_CLASS_VIDEO, //0x0E,       //InterfaceClass:USB_CLASS_VIDEO
    SC_VIDEOSTREAMING, //0x02,       //InterfaceSubClass:SC_VIDEOSTREAMING
    UVC_PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    0x00,       //iInterface

#else
///standard VS interface descriptor:(Interface 1, Alternate Setting 0):
    USB_DT_INTERFACE_SIZE,       //Length
    USB_DT_INTERFACE,      //DescriptorType:Inerface
    0x01,       //InterfaceNum:1  !!
    0x00,       //AlternateSetting
    0x01,       //NumEndpoint:1 非零端点数目
    USB_CLASS_VIDEO, //0x0E,       //InterfaceClass:USB_CLASS_VIDEO
    SC_VIDEOSTREAMING, //0x02,       //InterfaceSubClass:SC_VIDEOSTREAMING
    UVC_PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    0x00,       //iInterface

///standard VS Iso Video Data Endpoint descriptor:
    USB_DT_ENDPOINT_SIZE,       //Length
    USB_DT_ENDPOINT, //0x05,       //DescriptorType:Endpoint
    USB_DIR_IN | UVC_STREAM_EP_IN,       //EndpointAddr:IN endpoint 2
    USB_ENDPOINT_XFER_BULK,

    LOBYTE(UVC_FIFO_TXMAXP),
    HIBYTE(UVC_FIFO_TXMAXP) | ((UVC_PKT_SPILT - 1) << 3),//MaxPacketSize:MAX packet size

    0,       //Interval:one frame interval
#endif
};
static const u8 uvc_vs_interface_alternate_desc[] = {
///standard VS interface descriptor:(Interface 2, Alternate Setting 1):
    USB_DT_INTERFACE_SIZE,       //Length
    USB_DT_INTERFACE,      //DescriptorType:Inerface
    0x01,       //InterfaceNum:1  !!
    0x01,       //AlternateSetting
    0x01,       //NumEndpoint:1 非零端点数目
    USB_CLASS_VIDEO, //0x0E,       //InterfaceClass:USB_CLASS_VIDEO
    SC_VIDEOSTREAMING, //0x02,       //InterfaceSubClass:SC_VIDEOSTREAMING
    UVC_PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    0x00,       //iInterface

///standard VS Iso Video Data Endpoint descriptor:
    USB_DT_ENDPOINT_SIZE,       //Length
    USB_DT_ENDPOINT, //0x05,       //DescriptorType:Endpoint
    USB_DIR_IN | UVC_STREAM_EP_IN,       //EndpointAddr:IN endpoint 2
    USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,

    LOBYTE(UVC_FIFO_TXMAXP),
    HIBYTE(UVC_FIFO_TXMAXP) | ((UVC_PKT_SPILT - 1) << 3),//MaxPacketSize:MAX packet size

    ISO_EP_INTERVAL,       //Interval:one frame interval
};
static const u8 uvc_vs_inf_input_header_desc[] = {
//VS Input Header descriptor
    0x0f,       //Length
    USB_DT_CS_INTERFACE,      	//DescriptorType:cs_interface
    VS_INPUT_HEADER,    	//DescriptorSubType:VS_INPUT_HEADER
    1,  		//Numformats:One format descriptor follows
    0, 0, //TotalLength of class-specific VS interface descriptors

    USB_DIR_IN | UVC_STREAM_EP_IN,     	//endpoint_addr: addr of iso endpoint used for video data
    0x00,		//info :no dynamic format change supported
    UVC_OT_TERMINALID1,      	//TerminalLink: this videostreaming interface supplies terminal ID 4(Output Terminal)
    0,		//StillCaptureMethod :not support still image capture
    0,  		//TriggerSupport:none
    0,  		//TriggerUsage:none
    0x02,      	//ControlSize
    0x00,      	//Controls:No VideoStreaming specific controls are supported
    0x00,      	//Controls:No VideoStreaming specific controls are supported
};
#if UVC_FORMAT_MJPG
static const u8 uvc_mjpeg_format_desc[] = {
///class-specific VS Format descriptor
    0x0b,    	//Length
    USB_DT_CS_INTERFACE,      	//DescriptorType:cs_interface
    VS_FORMAT_MJPEG,      	//DescriptorSubType:VS_FORMAT_MJPEG
    0x01,  		//FormatIndex:first format descriptor

    0x05,		//NumFrameDescriptors: Three frame descriptor for this format follows

    0x00,     	//flags: NO uses fixed size samples
    0x01,		//defaultFrameIndex :1
    0,      	//AspectRatioX: non interlaced stream 16:9 = 1280 * 720
    0,  		//AspectRatioY :non interlaced stream 16:9 = 1280 * 720
    0x00,  		//InterfaceFlags:non interlaced stream
    0x00,  		//CopyProtect:
};
static const u8 uvc_mjpeg_frame_desc[] = {
///class-specific VS Frame descriptor
    0x1e + 4,    	//Length
    USB_DT_CS_INTERFACE,      	//DescriptorType:cs_interface
    VS_FRAME_MJPEG,      	//DescriptorSubType:VS_FRAME_MJPEG
    0x01,  		//FrameIndex:Num 1 Frame descriptor
    0x00,		//bmCapabilities:
    LOBYTE(MJPG_WWIDTH_0), HIBYTE(MJPG_WWIDTH_0),
    LOBYTE(MJPG_WHEIGHT_0), HIBYTE(MJPG_WHEIGHT_0),

    DW1BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 40),
    DW2BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 40),
    DW3BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 40),
    DW4BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 40),      	//MinBitRate: min bit rate in bits/s

    DW1BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 240),
    DW2BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 240),
    DW3BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 240),
    DW4BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 240), 		//MaxBitRate :max bit rate in bits/s

    DW1BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 2),
    DW2BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 2),
    DW3BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 2),
    DW4BYTE(MJPG_WWIDTH_0 *MJPG_WHEIGHT_0 * 2), //MaxVideoFrameBufSIZE : in bytes一帧数据或者still帧的最大空间

    DW1BYTE(FRAME_FPS),
    DW2BYTE(FRAME_FPS),
    DW3BYTE(FRAME_FPS),
    DW4BYTE(FRAME_FPS), //DefaultFrameInterval : 33333300ns(30fps) 默认的帧间隔

    0x02,		//FrameIntervalType: discrete frame interval,
    DW1BYTE(FRAME_FPS),
    DW2BYTE(FRAME_FPS),
    DW3BYTE(FRAME_FPS),
    DW4BYTE(FRAME_FPS),

    DW1BYTE(FRAME_FPS * 2),
    DW2BYTE(FRAME_FPS * 2),
    DW3BYTE(FRAME_FPS * 2),
    DW4BYTE(FRAME_FPS * 2),
};
#endif
#if UVC_FORMAT_YUY2 || UVC_FORMAT_NV12 || UVC_FORMAT_I420
static const u8 uvc_yuv_format_desc[] = {
    0x1B,//bLength
    USB_DT_CS_INTERFACE,//bDescriptorType
    VS_FORMAT_UNCOMPRESSED,//bDescriptorSubtype
    2, //bFormatIndex
    5,//bNumFrameDescriptors
#if UVC_FORMAT_YUY2
    UVC_GUID_FORMAT_YUY2,
    0x10,//bBitsPerPixel
#elif UVC_FORMAT_NV12
    UVC_GUID_FORMAT_NV12,
    0x0c,//bBitsPerPixel
#elif UVC_FORMAT_I420
    UVC_GUID_FORMAT_I420,
    0x0c,//bBitsPerPixel
#endif

    1,//bDefaultFrameIndex
    0,//bAspectRatioX
    0,//bAspectRatioY
    0,//bmInterlaceFlags
    0,//bCopyProtect
};
static const u8 uvc_yuv_frame_desc[] = {
///class-specific VS Frame descriptor                /*0xE0*/                /*0x8C*/
    0x1E + 4,    	//Length
    USB_DT_CS_INTERFACE, //0x24,      	//DescriptorType:cs_interface
    VS_FRAME_UNCOMPRESSED, //0x05,     	//DescriptorSubType:VS_FRAME_MJPEG
    0x01,  		//FrameIndex:Num 1 Frame descriptor
    0x00,		//bmCapabilities:
    LOBYTE(YUV_WWIDTH_0), HIBYTE(YUV_WWIDTH_0),
    LOBYTE(YUV_WHEIGHT_0), HIBYTE(YUV_WHEIGHT_0),

    DW1BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW2BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW3BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW4BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),      	//MinBitRate: min bit rate in bits/s

    DW1BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW2BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW3BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8),
    DW4BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2 * 30 * 8), 		//MaxBitRate :max bit rate in bits/s

#if UVC_FORMAT_YUY2
    DW1BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2),
    DW2BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2),
    DW3BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2),
    DW4BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 2), //0x00, 0x58, 0x02, 0x00, 		//MaxVideoFrameBufSIZE : in bytes一帧数据或者still帧的最大空间
#elif UVC_FORMAT_NV12
    DW1BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW2BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW3BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW4BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
#elif UVC_FORMAT_I420
    DW1BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW2BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW3BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
    DW4BYTE(YUV_WWIDTH_0 *YUV_WHEIGHT_0 * 3 / 2),
#endif

    DW1BYTE(FRAME_FPS),
    DW2BYTE(FRAME_FPS),
    DW3BYTE(FRAME_FPS),
    DW4BYTE(FRAME_FPS), 		//DefaultFrameInterval : 33333300ns(30fps) 默认的帧间隔
    0x02,		//FrameIntervalType: discrete frame interval,支持下面的四种帧间隔

    DW1BYTE(FRAME_FPS),
    DW2BYTE(FRAME_FPS),
    DW3BYTE(FRAME_FPS),
    DW4BYTE(FRAME_FPS),

    DW1BYTE(FRAME_FPS * 2),
    DW2BYTE(FRAME_FPS * 2),
    DW3BYTE(FRAME_FPS * 2),
    DW4BYTE(FRAME_FPS * 2),
};
#endif
static const u8 uvc_color_matching_desc[] = {
///Video Color Matching descriptor:          /*0x178*/               /*0x11C*/
    0x06,       //Length
    USB_DT_CS_INTERFACE, //0x24,      	//DescriptorType:cs_interface
    VS_COLORFORMAT, //0x0d,       //DescriptorSubType:VS_ColorFormat
    0x01,       //ColorPrimaries 0: Unspecified (Image characteristics unknown) 1: BT.709, sRGB (default) 2: BT.470-2 (M) 3: BT.470-2 (B, G) 4: SMPTE 170M 5: SMPTE 240M 6-255: Reserved
    0x01,       //TransferCharacter 0: Unspecified (Image characteristics unknown) 1: BT.709 (default) 2: BT.470-2 M 3: BT.470-2 B, G
    0x04,       //MatrixCoeffcients
};
static u8 uvc_slave_probe_commit_control[USB_MAX_HW_NUM][26] = {
    {
        0x00, 0x00,                     //bmHint
        0x01,                           //FormatIndex
        0x01,                           //FrameIndex
        DW1BYTE(FRAME_FPS),
        DW2BYTE(FRAME_FPS),
        DW3BYTE(FRAME_FPS),
        DW4BYTE(FRAME_FPS), //dwFrameInterval Frame interval in 100 ns units.
        0x00, 0x00,                     //KeyFrameRate
        0x00, 0x00,                     //PFrameRate
        0x00, 0x00,                     //CompQuanlity
        0x00, 0x00,                     //CompWindowsSize
        0x00, 0x00, //0x32,0x00,                      //Delay
        DW1BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW2BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW3BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW4BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0), //MaxVideoFrameSize
#if UVC_ISO_MODE
        DW1BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW2BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW3BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW4BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT), //MaxPayLoadTransferSize
#else
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2), //MaxPayLoadTransferSize
#endif
    },
    {
        0x00, 0x00,                     //bmHint
        0x01,                           //FormatIndex
        0x01,                           //FrameIndex
        DW1BYTE(FRAME_FPS),
        DW2BYTE(FRAME_FPS),
        DW3BYTE(FRAME_FPS),
        DW4BYTE(FRAME_FPS), //dwFrameInterval Frame interval in 100 ns units.
        0x00, 0x00,                     //KeyFrameRate
        0x00, 0x00,                     //PFrameRate
        0x00, 0x00,                     //CompQuanlity
        0x00, 0x00,                     //CompWindowsSize
        0x00, 0x00, //0x32,0x00,                      //Delay
        DW1BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW2BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW3BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0),
        DW4BYTE(MJPG_WHEIGHT_0 * MJPG_WWIDTH_0), //MaxVideoFrameSize
#if UVC_ISO_MODE
        DW1BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW2BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW3BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW4BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT), //MaxPayLoadTransferSize
#else
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2), //MaxPayLoadTransferSize
#endif
    },
};
static u8 uvc_slave_stillprobe_commit_control[USB_MAX_HW_NUM][11] = {
    {
        0x01,
        0x01,
        0x01,
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
#if UVC_ISO_MODE
        DW1BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW2BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW3BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW4BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT), //MaxPayLoadTransferSize
#else
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2), //MaxPayLoadTransferSize
#endif
    },
    {
        0x01,
        0x01,
        0x01,
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
#if UVC_ISO_MODE
        DW1BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW2BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW3BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT),
        DW4BYTE(UVC_FIFO_TXMAXP * UVC_PKT_SPILT), //MaxPayLoadTransferSize
#else
        DW1BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW2BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW3BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2),
        DW4BYTE(MJPG_WHEIGHT_0 *MJPG_WWIDTH_0 * 2), //MaxPayLoadTransferSize
#endif
    },
};
static const u8 videoStringDescriptor[] = {
    22,         //该描述符的长度为22字节
    0x03,       //字符串描述符的类型编码为0x03
    'U', 0x00, //u
    'S', 0x00, //s
    'B', 0x00, //b
    ' ', 0x00, //
    'C', 0x00, //c
    'A', 0x00, //a
    'M', 0x00, //m
    'E', 0x00, //e
    'R', 0x00, //r
    'A', 0x00, //a
};

static void uvc_reset_handle(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (uvc_handle[usb_id]) {
#if UVC_ISO_MODE
        usb_g_ep_config(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc_handle[usb_id]->ep_dma_buffer, (UVC_FIFO_TXMAXP | ((UVC_PKT_SPILT - 1) << 11)));
        uvc_handle[usb_id]->tx_broken = 1;
        uvc_handle[usb_id]->last_len = 0;
#else
        usb_g_ep_config(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, uvc_handle[usb_id]->ep_dma_buffer, UVC_FIFO_TXMAXP);
#endif
    }
}

u32 uvc_desc_config(usb_dev usb_id, u8 *ptr, u32 *inf)
{
    int i;
    u8 *tptr = ptr;
    u16 width, height;
    u8 *frame_num;
    int yuv_fmt_enable;
    u32 total_len = 0;
    u8 *tptr_vs;
    u8 fmt_idx = 0;

    if (!uvc_handle[usb_id] || !ptr || !inf) {
        return 0;
    }
#if UVC_FORMAT_YUY2 || UVC_FORMAT_NV12 || UVC_FORMAT_I420
    if (uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM] && uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM]) {
        yuv_fmt_enable = 1;
    } else {
        yuv_fmt_enable = 0;
    }
#else
    yuv_fmt_enable = 0;
#endif
    log_info("video interface num: %d", *inf + 1);

    memcpy(tptr, (u8 *)uvc_interface_desc_header, sizeof(uvc_interface_desc_header));
    tptr[2] = *inf;
    tptr += sizeof(uvc_interface_desc_header);

    memcpy(tptr, (u8 *)uvc_vc_interface_desc, sizeof(uvc_vc_interface_desc));
    tptr[2] = *inf;
    tptr[21] = *inf + 1;
    total_len = 0x0d + sizeof(uvc_terminal_desc);
    tptr[14] = LOBYTE(total_len);
    tptr[15] = HIBYTE(total_len);
    tptr += sizeof(uvc_vc_interface_desc);

    memcpy(tptr, (u8 *)uvc_terminal_desc, sizeof(uvc_terminal_desc));
#if VS_IT_CONTROLS_ENABLE
    tptr[15] = 0xff;
    tptr[16] = 0xff;
    tptr[17] = 0x1f;
#endif
#if VS_PU_CONTROLS_ENABLE
    tptr[0x12 + 8] = 0xff;
    tptr[0x12 + 9] = 0xff;
#endif
    tptr += sizeof(uvc_terminal_desc);

    /* memcpy(tptr, (u8 *)uvc_standard_intr_endpoint_desc, sizeof(uvc_standard_intr_endpoint_desc)); */
    /* tptr += sizeof(uvc_standard_intr_endpoint_desc); */

    memcpy(tptr, (u8 *)uvc_vs_interface_desc, sizeof(uvc_vs_interface_desc));
    tptr[2] = *inf + 1;
    tptr += sizeof(uvc_vs_interface_desc);

    memcpy(tptr, (u8 *)uvc_vs_inf_input_header_desc, sizeof(uvc_vs_inf_input_header_desc));
#if (UVC_FORMAT_I420 || UVC_FORMAT_YUY2) && UVC_FORMAT_MJPG
    tptr[12] = 1;
#endif
    tptr_vs = tptr;
    tptr += sizeof(uvc_vs_inf_input_header_desc);
    total_len = sizeof(uvc_vs_inf_input_header_desc);

#if UVC_FORMAT_MJPG
    memcpy(tptr, (u8 *)uvc_mjpeg_format_desc, sizeof(uvc_mjpeg_format_desc));
    tptr[3] = ++fmt_idx;
    frame_num = &tptr[4];
    *frame_num = 0;
    total_len += sizeof(uvc_mjpeg_format_desc);
    tptr += sizeof(uvc_mjpeg_format_desc);
    for (i = 0; i < FMT_MAX_RESO_NUM; i++) {
        if (uvc_handle[usb_id]->width[i] == 0 || uvc_handle[usb_id]->height[i] == 0) {
            break;
        }
        (*frame_num)++;
        memcpy(tptr, (u8 *)uvc_mjpeg_frame_desc, sizeof(uvc_mjpeg_frame_desc));

        tptr[3] = i + 1;
        width = uvc_handle[usb_id]->width[i];
        height = uvc_handle[usb_id]->height[i];

        tptr[5] = LOBYTE(width);
        tptr[6] = HIBYTE(width);
        tptr[7] = LOBYTE(height);
        tptr[8] = HIBYTE(height);

        tptr[9]  = DW1BYTE(width * height * 2 * 15 * 8);
        tptr[10] = DW2BYTE(width * height * 2 * 15 * 8);
        tptr[11] = DW3BYTE(width * height * 2 * 15 * 8);
        tptr[12] = DW4BYTE(width * height * 2 * 15 * 8); //MinBitRate

        tptr[13] = DW1BYTE(width * height * 2 * 30 * 8);
        tptr[14] = DW2BYTE(width * height * 2 * 30 * 8);
        tptr[15] = DW3BYTE(width * height * 2 * 30 * 8);
        tptr[16] = DW4BYTE(width * height * 2 * 30 * 8);  //MaxBitRate

        tptr[17] = DW1BYTE(width * height * 2);
        tptr[18] = DW2BYTE(width * height * 2);
        tptr[19] = DW3BYTE(width * height * 2);
        tptr[20] = DW4BYTE(width * height * 2);  //MaxVideoFrameBufSize

        tptr += sizeof(uvc_mjpeg_frame_desc);
        total_len += sizeof(uvc_mjpeg_frame_desc);
    }
#endif
#if UVC_FORMAT_YUY2 || UVC_FORMAT_NV12 || UVC_FORMAT_I420
    if (yuv_fmt_enable) {
        memcpy(tptr, (u8 *)uvc_yuv_format_desc, sizeof(uvc_yuv_format_desc));
        tptr[3] = ++fmt_idx;
        frame_num = &tptr[4];
        *frame_num = 0;
        tptr += sizeof(uvc_yuv_format_desc);
        total_len += sizeof(uvc_yuv_format_desc);
        for (i = 0; i < FMT_MAX_RESO_NUM; i++) {
            if (uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM + i] == 0 ||
                uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM + i] == 0) {
                break;
            }
            (*frame_num)++;
            memcpy(tptr, (u8 *)uvc_yuv_frame_desc, sizeof(uvc_yuv_frame_desc));
            tptr[3] = i + 1;

            width = uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM + i];
            height = uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM + i];
            tptr[5] = LOBYTE(width);
            tptr[6] = HIBYTE(width);
            tptr[7] = LOBYTE(height);
            tptr[8] = HIBYTE(height);

#if UVC_FORMAT_YUY2
            tptr[9]  = DW1BYTE(width * height * 2 * 15 * 8);
            tptr[10] = DW2BYTE(width * height * 2 * 15 * 8);
            tptr[11] = DW3BYTE(width * height * 2 * 15 * 8);
            tptr[12] = DW4BYTE(width * height * 2 * 15 * 8);  //MinBitRate

            tptr[13] = DW1BYTE(width * height * 2 * 30 * 8);
            tptr[14] = DW2BYTE(width * height * 2 * 30 * 8);
            tptr[15] = DW3BYTE(width * height * 2 * 30 * 8);
            tptr[16] = DW4BYTE(width * height * 2 * 30 * 8);  //MaxBitRate

            tptr[17] = DW1BYTE(width * height * 2);
            tptr[18] = DW2BYTE(width * height * 2);
            tptr[19] = DW3BYTE(width * height * 2);
            tptr[20] = DW4BYTE(width * height * 2);  //MaxVideoFrameBufSize
#elif UVC_FORMAT_NV12
            tptr[9]  = DW1BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[10] = DW2BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[11] = DW3BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[12] = DW4BYTE(width * height * 3 / 2 * 15 * 8);  //MinBitRate

            tptr[13] = DW1BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[14] = DW2BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[15] = DW3BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[16] = DW4BYTE(width * height * 3 / 2 * 30 * 8);  //MaxBitRate

            tptr[17] = DW1BYTE(width * height * 3 / 2);
            tptr[18] = DW2BYTE(width * height * 3 / 2);
            tptr[19] = DW3BYTE(width * height * 3 / 2);
            tptr[20] = DW4BYTE(width * height * 3 / 2);  //MaxVideoFrameBufSize
#elif UVC_FORMAT_I420
            tptr[9]  = DW1BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[10] = DW2BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[11] = DW3BYTE(width * height * 3 / 2 * 15 * 8);
            tptr[12] = DW4BYTE(width * height * 3 / 2 * 15 * 8);  //MinBitRate

            tptr[13] = DW1BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[14] = DW2BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[15] = DW3BYTE(width * height * 3 / 2 * 30 * 8);
            tptr[16] = DW4BYTE(width * height * 3 / 2 * 30 * 8);  //MaxBitRate

            tptr[17] = DW1BYTE(width * height * 3 / 2);
            tptr[18] = DW2BYTE(width * height * 3 / 2);
            tptr[19] = DW3BYTE(width * height * 3 / 2);
            tptr[20] = DW4BYTE(width * height * 3 / 2);  //MaxVideoFrameBufSize
#endif
            tptr += sizeof(uvc_yuv_frame_desc);
            total_len += sizeof(uvc_yuv_frame_desc);
        }
    }
#endif

    memcpy(tptr, (u8 *)uvc_color_matching_desc, sizeof(uvc_color_matching_desc));
    tptr += sizeof(uvc_color_matching_desc);
    total_len += sizeof(uvc_color_matching_desc);

    //fill uvc_vs_inf_input_header_desc
    tptr_vs[3] = fmt_idx;
    tptr_vs[4] = LOBYTE(total_len);
    tptr_vs[5] = HIBYTE(total_len);

#if UVC_ISO_MODE
    memcpy(tptr, (u8 *)uvc_vs_interface_alternate_desc, sizeof(uvc_vs_interface_alternate_desc));
    tptr[2] = *inf + 1;
    tptr += sizeof(uvc_vs_interface_alternate_desc);
#endif

    if (usb_set_interface_hander(usb_id, *inf, uvc_vc_itf_handle) != *inf) {
        ASSERT(0, "uvc set vc interface handle fail");
    }
    if (usb_set_interface_hander(usb_id, *inf + 1, uvc_vs_itf_handle) != *inf + 1) {
        ASSERT(0, "uvc set vs interface handle fail")
    }
    if (usb_set_reset_hander(usb_id, *inf, uvc_reset_handle) != *inf) {
        ASSERT(0, "uvc set reset handle fail")
    }
    uvc_handle[usb_id]->itf_base = *inf;
    *inf += 2;
    return (u32)(tptr - ptr);
}

const u8 *uvc_get_string()
{
    return videoStringDescriptor;
}

/*------------------------------- uvc setup -------------------------------*/
static u32 uvc_vc_itf_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_device->wDataLength = req->wLength;
    usb_device->bDataOverFlag = 0;

    log_debug("%x %x %x %x", req->bRequest,
              req->wIndex,
              req->wLength,
              req->wValue);

    usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);

    switch (req->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        log_info("standard request");
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;

    case USB_TYPE_CLASS:
        if (LOBYTE(req->wIndex) == uvc_handle[usb_id]->itf_base) {  //vc interface class specificed request
            log_debug("vc interface request");
            switch (HIBYTE(req->wIndex)) {
#if VS_IT_CONTROLS_ENABLE
            case UVC_IT_TERMINALID:
                usb_device->wDataLength = req->wLength;
#if 1
                goto __req_default;
#endif
                switch (HIBYTE(req->wValue)) {
                case CT_SCANNING_MODE_CONTROL:
                    break;
                case CT_AE_MODE_CONTROL:
                    break;
                case CT_AE_PRIORITY_CONTROL:
                    break;
                case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                    break;
                case CT_EXPOSURE_TIME_RELATIVE_CONTROL:
                    break;
                case CT_FOCUS_ABSOLUTE_CONTROL:
                    break;
                case CT_FOCUS_RELATIVE_CONTROL:
                    break;
                case CT_FOCUS_AUTO_CONTROL:
                    break;
                case CT_IRIS_ABSOLUTE_CONTROL:
                    break;
                case CT_IRIS_RELATIVE_CONTROL:
                    break;
                case CT_ZOOM_ABSOLUTE_CONTROL:
                    break;
                case CT_ZOOM_RELATIVE_CONTROL:
                    break;
                case CT_PANTILT_ABSOLUTE_CONTROL:
                    break;
                case CT_PANTILT_RELATIVE_CONTROL:
                    break;
                case CT_ROLL_ABSOLUTE_CONTROL:
                    break;
                case CT_ROLL_RELATIVE_CONTROL:
                    break;
                case CT_PRIVACY_CONTROL:
                    break;
                }
                break;
#endif
#if VS_PU_CONTROLS_ENABLE
            case UVC_PU_TERMINALID:
                usb_device->wDataLength = req->wLength;
                if (uvc_handle[usb_id]->processing_unit_response && req->bRequest != UVC_SET_CUR) {
                    struct uvc_unit_ctrl unit_ctrl_req;
                    unit_ctrl_req.request = req->bRequest;
                    unit_ctrl_req.unit = HIBYTE(req->wValue);
                    unit_ctrl_req.data = usb_device->setup_ptr;
                    unit_ctrl_req.len = req->wLength;
                    int err = uvc_handle[usb_id]->processing_unit_response(&unit_ctrl_req);
                    if (!err) {
                        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                        if (unit_ctrl_req.len != req->wLength) {
                            if (unit_ctrl_req.len < req->wLength) {
                                usb_device->wDataLength = unit_ctrl_req.len;
                                usb_device->bDataOverFlag = 1;
                            }
                        }
                        break;
                    }
                }
                switch (HIBYTE(req->wValue)) {
                case PU_BACKLIGHT_COMPENSATION_CONTROL:  //背光对比
                    uvc_vc_pu_wBacklightCompensation_handle(usb_id, usb_device, req);
                    break;
                case PU_BRIGHTNESS_CONTROL:  //亮度
                    uvc_vc_pu_wBrightness_handle(usb_id, usb_device, req);
                    break;
                case PU_CONTRAST_CONTROL: //对比度
                    uvc_vc_pu_wContrast_handle(usb_id, usb_device, req);
                    break;
                case PU_GAIN_CONTROL:
                    uvc_vc_pu_wGain_handle(usb_id, usb_device, req);
                    break;
                case PU_POWER_LINE_FREQUENCY_CONTROL:
                    uvc_vc_pu_bPowerLineFrequency_handle(usb_id, usb_device, req);
                    break;
                case PU_HUE_CONTROL: //色调
                    uvc_vc_pu_wHue_handle(usb_id, usb_device, req);
                    break;
                case PU_SATURATION_CONTROL: //饱和度
                    uvc_vc_pu_wSaturation_handle(usb_id, usb_device, req);
                    break;
                case PU_SHARPNESS_CONTROL: //清晰度，锐度
                    uvc_vc_pu_wSharpness_handle(usb_id, usb_device, req);
                    break;
                case PU_GAMMA_CONTROL: //伽马
                    uvc_vc_pu_wGamma_handle(usb_id, usb_device, req);
                    break;
                case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: //白平衡
                    uvc_vc_pu_wWhiteBalanceTemperature_handle(usb_id, usb_device, req);
                    break;
                case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
                    uvc_vc_pu_bWhiteBalanceTemperatureAuto_handle(usb_id, usb_device, req);
                    break;
                case PU_WHITE_BALANCE_COMPONENT_CONTROL:
                    uvc_vc_pu_white_balance_component_control_handle(usb_id, usb_device, req);
                    break;
                case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
                    uvc_vc_pu_bWhiteBalanceComponentAuto_handle(usb_id, usb_device, req);
                    break;
                case PU_DIGITAL_MULTIPLIER_CONTROL:
                    uvc_vc_pu_wMultiplierStep_handle(usb_id, usb_device, req);
                    break;
                case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
                    uvc_vc_pu_wMultiplierLimit_handle(usb_id, usb_device, req);
                    break;
                case PU_HUE_AUTO_CONTROL:
                    uvc_vc_pu_bHueAuto_handle(usb_id, usb_device, req);
                    break;
                case PU_ANALOG_VIDEO_STANDARD_CONTROL:
                    uvc_vc_pu_bVideoStandard_handle(usb_id, usb_device, req);
                    break;
                case PU_ANALOG_LOCK_STATUS_CONTROL:
                    uvc_vc_pu_bStatus_handle(usb_id, usb_device, req);
                    break;
                default:
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
                break;
#endif
            case UVC_EXT_TERMINALID1:
            case UVC_EXT_TERMINALID2:
                switch (req->bRequest) {
                case UVC_GET_CUR:
                    *(u16 *)usb_device->setup_buffer = 0x000a;
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_GET_MIN:
                    *(u16 *)usb_device->setup_buffer = 0x0008;
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_GET_MAX:
                    *(u16 *)usb_device->setup_buffer = 0x0010;
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_GET_RES:
                    *(u16 *)usb_device->setup_buffer = 0x0001;
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_GET_INFO:
                    usb_device->setup_buffer[0] = 0x03;
                    usb_device->wDataLength = 1;
                    if (usb_device->wDataLength < req->wLength) {
                        usb_device->bDataOverFlag = 1;
                    }
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_GET_LEN:
                    if (HIBYTE(req->wValue) == 2 || HIBYTE(req->wValue) == 3) {
                        *(u16 *)usb_device->setup_buffer = 0x0002;
                    } else {
                        *(u16 *)usb_device->setup_buffer = 0x0004;
                    }
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
                    break;
                case UVC_SET_CUR:
                    goto __req_default;
                    break;
                default:
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
                break;
            default:
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;

    case USB_TYPE_VENDOR:
        goto __req_default;
        break;
    default:
        log_info("requeset type dose not exist");
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }

    if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
        usb_set_setup_recv(usb_device, uvc_vc_itf_recv_handle);
    }
    //log_info("%d", req->wLength);
    return 0;

__req_default:
    if (req->bRequest & 0x80) {  //UVC_GET_XXX
        memset(usb_device->setup_ptr, 0, req->wLength);
        usb_device->wDataLength = 0;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
    } else if (req->bRequest == UVC_SET_CUR) {
        usb_device->wDataLength = req->wLength;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
        uvc_handle[usb_id]->uvc_status = 0;
        usb_set_setup_recv(usb_device, uvc_vc_itf_recv_handle);
    }
    return 0;
}

static u32 uvc_vs_itf_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *probe_commit = uvc_slave_probe_commit_control[usb_id];
    u8 *stillprobe_commit = uvc_slave_stillprobe_commit_control[usb_id];
    u32 frame_size = probe_commit[3] - 1;
    usb_device->wDataLength = req->wLength;
    usb_device->bDataOverFlag = 0;

    usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);

    switch (req->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        log_info("standard request");
        if (req->bRequest == USB_REQ_SET_INTERFACE) {
            if (req->wIndex == uvc_handle[usb_id]->itf_base + 1) {  //mjpeg vs interface number
                log_info("uvc set vs_interface %d alternate setting to %d", req->wIndex, req->wValue);
                if (req->wValue) {
                    usb_video_open_camera(usb_id);
                } else {
                    usb_video_close_camera(usb_id);
                }
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
        }
        break;
    case USB_TYPE_CLASS:
        if (LOBYTE(req->wIndex) == uvc_handle[usb_id]->itf_base + 1) { //vs interface class specificed request
            log_debug("vs interface request");
            switch (HIBYTE(req->wValue)) {
            case UVC_VS_PROBE_CONTROL:
            case UVC_VS_COMMIT_CONTROL:
                uvc_vs_probe_commit_ctl_handle(usb_id, usb_device, req);
                break;
            case UVC_VS_STILL_PROBE_CONTROL:
            case UVC_VS_STILL_COMMIT_CONTROL:
                uvc_vs_still_probe_commit_ctl_handle(usb_id, usb_device, req);
                break;
            }
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    case USB_TYPE_VENDOR:
        goto __req_default;
        break;
    default:
        log_info("requeset type dose not exist");
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
    if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
        usb_set_setup_recv(usb_device, uvc_vs_itf_recv_handle);
    }
    //log_info("%d", req->wLength);
    return 0;

__req_default:
    if (req->bRequest & 0x80) {  //UVC_GET_XXX
        memset(usb_device->setup_ptr, 0, req->wLength);
        usb_device->wDataLength = 0;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
    } else if (req->bRequest == UVC_SET_CUR) {
        usb_device->wDataLength = req->wLength;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
        uvc_handle[usb_id]->uvc_status = 0;
        usb_set_setup_recv(usb_device, uvc_vs_itf_recv_handle);
    }
    return 0;
}

static u32 uvc_vc_itf_recv_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    /* u32 rx_len = req->wLength; */
    int err;
    const usb_dev usb_id = usb_device2id(usb_device);

    usb_device->setup_ptr = usb_device->setup_buffer;
    u32 rx_len = usb_read_count0(usb_id);
    usb_read_ep0(usb_id, usb_device->setup_ptr, rx_len);
    if (LOBYTE(req->wIndex) == uvc_handle[usb_id]->itf_base) {
        err = -1;
        if (HIBYTE(req->wIndex) == UVC_PU_TERMINALID &&
            uvc_handle[usb_id]->processing_unit_response) {
            struct uvc_unit_ctrl unit_ctrl_req;
            unit_ctrl_req.request = req->bRequest;
            unit_ctrl_req.unit = HIBYTE(req->wValue);
            unit_ctrl_req.data = usb_device->setup_ptr;
            unit_ctrl_req.len = req->wLength;
            err = uvc_handle[usb_id]->processing_unit_response(&unit_ctrl_req);
        }
        if (err && uvc_handle[usb_id]->uvc_status) {
            ((pUVC_SETUP_HOOK)uvc_handle[usb_id]->uvc_status)(usb_id, usb_device, req);
        }
    } else {
        log_info("interface invalid");
    }

    uvc_handle[usb_id]->uvc_status = 0;
    return USB_EP0_STAGE_SETUP;
}

static u32 uvc_vs_itf_recv_handle(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    /* u32 rx_len = req->wLength; */
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_device->setup_ptr = usb_device->setup_buffer;
    u32 rx_len = usb_read_count0(usb_id);
    usb_read_ep0(usb_id, usb_device->setup_ptr, rx_len);
    if (LOBYTE(req->wIndex) == uvc_handle[usb_id]->itf_base + 1) {
        if (uvc_handle[usb_id]->uvc_status) {
            ((pUVC_SETUP_HOOK)uvc_handle[usb_id]->uvc_status)(usb_id, usb_device, req);
        }
    } else {
        log_info("interface invalid");
    }

    uvc_handle[usb_id]->uvc_status = 0;
    return USB_EP0_STAGE_SETUP;
}

int uvc_setup_private(usb_dev usb_id, struct usb_device_t *usb_devie, struct usb_ctrlrequest *req)
{
    uvc_handle[usb_id]->private_cmd_response(req->bRequest, req->wValue);
    return 0;
}

/*------------------------------- uvc main -------------------------------*/
volatile u8 g_still_image_flag;
#define UVC_CAMERA_MAGIC    0x5aa555aa


enum {
    UVC_CUR,
    UVC_MIN,
    UVC_MAX,
    UVC_DEF,
};
struct video_arg_info {
    //cur min max default
    u16 wBacklightCompensation[4];//number
    u16 wBrightness[4];//Signed number
    u16 wContrast[4];//Number
    u16 wGain[4];//Number
    u8 bPowerLineFrequency[4];//0: Disabled 1: 50 Hz 2: 60 Hz
    u16 wHue[4];//Signed number
    u16 wSaturation[4];
    u16 wSharpness[4];
    u16 wGamma[4];
    u16 wWhiteBalanceTemperature[4];
    u8 bWhiteBalanceTemperatureAuto[4];
    u32 pu_white_balance_component_control[4];//wWhiteBalanceBlue wWhiteBalanceRed
    u8 bWhiteBalanceComponentAuto[4];
    u16 wMultiplierStep[4];
    u16 wMultiplierLimit[4];
    u8 bHueAuto[4];
    u8 bVideoStandard[4];//0: None 1: NTSC – 525/60 2: PAL – 625/50 4: SECAM – 625/50 4: NTSC – 625/50 5: PAL – 525/60 6-255: Reserved. Do not use
    u8 bStatus[4];//0: Video Decoder is locked. 1: Video Decoder is not locked. 2-255: Reserved. Do not use.
};
static struct video_arg_info video_ctl_value[USB_MAX_HW_NUM];

static void usb_video_feature_init(usb_dev usb_id)
{
    log_info("sizeof(video_ctl_value[%d]) = %lu", usb_id, sizeof(video_ctl_value[usb_id]));
    video_ctl_value[usb_id].wBacklightCompensation[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wBacklightCompensation[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wBacklightCompensation[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wBacklightCompensation[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wBrightness[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wBrightness[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wBrightness[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wBrightness[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wContrast[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wContrast[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wContrast[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wContrast[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wGain[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wGain[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wGain[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wGain[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].bPowerLineFrequency[UVC_MAX] = 0x02;
    video_ctl_value[usb_id].bPowerLineFrequency[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bPowerLineFrequency[UVC_DEF] = 0x00;
    video_ctl_value[usb_id].bPowerLineFrequency[UVC_CUR] = 0x00;

    video_ctl_value[usb_id].wHue[UVC_MAX] = 180;
    video_ctl_value[usb_id].wHue[UVC_MIN] = -180;
    video_ctl_value[usb_id].wHue[UVC_DEF] = 0;
    video_ctl_value[usb_id].wHue[UVC_CUR] = 0;

    video_ctl_value[usb_id].wSaturation[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wSaturation[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wSaturation[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wSaturation[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wSharpness[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wSharpness[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wSharpness[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wSharpness[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wGamma[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wGamma[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wGamma[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wGamma[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_MAX] = 1;
    video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_DEF] = 0;
    video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_CUR] = 0;

    video_ctl_value[usb_id].pu_white_balance_component_control[UVC_MAX] = 0x00ff00ff;
    video_ctl_value[usb_id].pu_white_balance_component_control[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].pu_white_balance_component_control[UVC_DEF] = 0x00800080;
    video_ctl_value[usb_id].pu_white_balance_component_control[UVC_CUR] = 0x00800080;

    video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_MAX] = 1;
    video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_DEF] = 0;
    video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_CUR] = 0;

    video_ctl_value[usb_id].wMultiplierStep[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wMultiplierStep[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wMultiplierStep[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wMultiplierStep[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].wMultiplierLimit[UVC_MAX] = 0x00ff;
    video_ctl_value[usb_id].wMultiplierLimit[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].wMultiplierLimit[UVC_DEF] = 0x80;
    video_ctl_value[usb_id].wMultiplierLimit[UVC_CUR] = 0x80;

    video_ctl_value[usb_id].bHueAuto[UVC_MAX] = 1;
    video_ctl_value[usb_id].bHueAuto[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bHueAuto[UVC_DEF] = 0;
    video_ctl_value[usb_id].bHueAuto[UVC_CUR] = 0;

    video_ctl_value[usb_id].bVideoStandard[UVC_MAX] = 0;
    video_ctl_value[usb_id].bVideoStandard[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bVideoStandard[UVC_DEF] = 0;
    video_ctl_value[usb_id].bVideoStandard[UVC_CUR] = 0;

    video_ctl_value[usb_id].bStatus[UVC_MAX] = 1;
    video_ctl_value[usb_id].bStatus[UVC_MIN] = 0x00;
    video_ctl_value[usb_id].bStatus[UVC_DEF] = 1;
    video_ctl_value[usb_id].bStatus[UVC_CUR] = 1;
}

typedef struct uvc_payload_hdr {
    u8 bfh_val;
    u32 frame_cnt;
    u32 scr;
    u16 sof_cnt;
} UVC_PLD_HDR;

static UVC_PLD_HDR pld_hdr[USB_MAX_HW_NUM];
/* static u32 IsNewFrame; */

static void fill_payload_head(u8 *tx_addr, UVC_PLD_HDR *hdr)
{
    u32 frame_cnt = 0, scr = 0;
    u16 sof_count = 0;
    /// Stream Head
    tx_addr[0x00] = 0x0C;//Head Length
    tx_addr[0x01] = hdr->bfh_val; //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
    /* tx_addr[0x1] &= ~(BIT(2) | BIT(3) | BIT(5) | BIT(6)); */
    frame_cnt = hdr->frame_cnt;
    tx_addr[0x02] = frame_cnt ;//PTS[7:0]
    tx_addr[0x03] = (frame_cnt >> 8); //PTS[15:8]
    tx_addr[0x04] = (frame_cnt >> 16) ; //PTS[23:16]
    tx_addr[0x05] = (frame_cnt >> 24) ; //PTS[31:24]
    scr = hdr->scr;
    tx_addr[0x06] = DW1BYTE(scr);//SCR[7:0]
    tx_addr[0x07] = DW2BYTE(scr);//SCR[15:8]
    tx_addr[0x08] = DW3BYTE(scr);//SCR[23:16]
    tx_addr[0x09] = DW4BYTE(scr);//SCR[31:24]
    sof_count = hdr->sof_cnt;
    tx_addr[0x0A] = LOBYTE(sof_count); //SCR[42:32] 1KHz SOF token counter
    tx_addr[0x0B] = HIBYTE(sof_count); //SCR[47:42] res set to 0.
}

static void uvc_g_bulk_transfer(struct usb_device_t *usb_device, u32 ep)
{
    u32 len = 0;
    u32 IsEOFFrame = 0;
    u8 *tx_addr;
    const usb_dev usb_id = usb_device2id(usb_device);

    /* putchar('i'); */
    if (uvc_handle[usb_id] == NULL) {
        return;
    }
    tx_addr = uvc_handle[usb_id]->stream_buf;
    if (uvc_handle[usb_id]->video_reqbuf) {
        if (uvc_handle[usb_id]->is_new_frame == 0) {
            len = uvc_handle[usb_id]->video_reqbuf(usb_id, tx_addr, UVC_FIFO_TXMAXP, &IsEOFFrame);
            if (IsEOFFrame) {
                uvc_handle[usb_id]->is_new_frame = 1;
            } else {
                uvc_handle[usb_id]->is_new_frame = 0;
            }
        } else {
            len = uvc_handle[usb_id]->video_reqbuf(usb_id, &tx_addr[0x0c], UVC_FIFO_TXMAXP - 0x0c, &IsEOFFrame);
            if (len) {
                pld_hdr[usb_id].sof_cnt = musb_read_sofframe(usb_id);
                pld_hdr[usb_id].frame_cnt++;
                pld_hdr[usb_id].bfh_val &= ~UVC_EOF;
                fill_payload_head(tx_addr, &pld_hdr[usb_id]);
                pld_hdr[usb_id].bfh_val ^= UVC_FID;
                len += 12;
                uvc_handle[usb_id]->is_new_frame = 0;
            }
        }
    }
    usb_g_bulk_write(usb_id, UVC_STREAM_EP_IN, tx_addr, len);
}

static void uvc_g_iso_transfer(struct usb_device_t *usb_device, u32 ep)
{
    u32 len = 0;
    u32 IsEOFFrame = 0;
    u32 len2;
    u8 *tx_addr;
    const usb_dev usb_id = usb_device2id(usb_device);
    /*putchar('i');*/

    if (uvc_handle[usb_id] == NULL) {
        return;
    }
    tx_addr = uvc_handle[usb_id]->stream_buf;
    pld_hdr[usb_id].sof_cnt = musb_read_sofframe(usb_id);
    if (!uvc_handle[usb_id]->tx_broken) {
        if (uvc_handle[usb_id]->video_reqbuf)  {//请求uvc从机的JPEG或YUV数据帧
            len = uvc_handle[usb_id]->video_reqbuf(usb_id, &tx_addr[0x0c], UVC_PKT_SPILT * UVC_FIFO_TXMAXP - 0x0c, &IsEOFFrame);
            if (IsEOFFrame) {
                pld_hdr[usb_id].bfh_val |= UVC_EOF;
                pld_hdr[usb_id].frame_cnt++;
                fill_payload_head(tx_addr, &pld_hdr[usb_id]);
                pld_hdr[usb_id].bfh_val ^= UVC_FID;
            } else {
                pld_hdr[usb_id].bfh_val &= ~UVC_EOF;
                fill_payload_head(tx_addr, &pld_hdr[usb_id]);
            }
        }
        len = len > 0 ? len + 12 : 0;
    } else {
        len = uvc_handle[usb_id]->last_len;
    }
    len2 = usb_g_iso_write(usb_id, UVC_STREAM_EP_IN, tx_addr, len);//请求完成则发往USB
    if (len && !len2) {
        uvc_handle[usb_id]->tx_broken = 1;
        uvc_handle[usb_id]->last_len = len;
    } else {
        uvc_handle[usb_id]->tx_broken = 0;
    }
}

//void uvc_g_video_transfer(usb_dev usb_id, u32 ep)
//{
//#if UVC_ISO_MODE
//    uvc_g_iso_transfer(usb_id, ep);
//#else
//    uvc_g_bulk_transfer(usb_id, ep);
//#endif
//}
extern struct usb_ep_addr_t *usb_ep_addr[USB_MAX_HW_NUM];
static void uvc_start_stream(usb_dev usb_id)
{
    if (uvc_handle[usb_id] == NULL) {
        return;
    }
#if UVC_ISO_MODE
    usb_g_ep_config(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc_handle[usb_id]->ep_dma_buffer, (UVC_FIFO_TXMAXP | ((UVC_PKT_SPILT - 1) << 11)));
    usb_g_set_intr_hander(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, uvc_g_iso_transfer);
    pld_hdr[usb_id].bfh_val |= UVC_EOF;
    fill_payload_head(uvc_handle[usb_id]->stream_buf, &pld_hdr[usb_id]);
    usb_g_iso_write(usb_id, UVC_STREAM_EP_IN, uvc_handle[usb_id]->stream_buf, 0x0c);
    uvc_handle[usb_id]->tx_broken = 1;
    uvc_handle[usb_id]->last_len = 0x0c;
#else
    usb_g_ep_config(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, uvc_handle[usb_id]->ep_dma_buffer, UVC_FIFO_TXMAXP);
    usb_g_set_intr_hander(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, uvc_g_bulk_transfer);
    pld_hdr[usb_id].bfh_val |= UVC_EOF;
    fill_payload_head(uvc_handle[usb_id]->stream_buf, &pld_hdr[usb_id]);
    usb_g_bulk_write(usb_id, UVC_STREAM_EP_IN, uvc_handle[usb_id]->stream_buf, 0x0c);
#endif
}

static void usb_video_open_camera(usb_dev usb_id)
{
    log_info("open camera success");
    u32 fps;

    g_still_image_flag = 0;

    pld_hdr[usb_id].frame_cnt = 0;
    pld_hdr[usb_id].sof_cnt = musb_read_sofframe(usb_id);
    pld_hdr[usb_id].bfh_val = 0x80;
    memcpy(&fps, &uvc_slave_probe_commit_control[usb_id][4], 4);
    log_debug("%d %d %d", g_still_image_flag, uvc_slave_probe_commit_control[usb_id][3] - 1, 10000000 / fps);
    if (uvc_handle[usb_id] && uvc_handle[usb_id]->video_open) {
        /*printf("%s %p", __func__, uvc_handle[usb_id]->video_open);*/
        uvc_handle[usb_id]->video_open(usb_id, uvc_slave_probe_commit_control[usb_id][2] - 1,  //Format Index
                                       uvc_slave_probe_commit_control[usb_id][3] - 1,  //Frame  Index
                                       10000000 / fps);
    }
    uvc_start_stream(usb_id);
}

static void usb_video_close_camera(usb_dev usb_id)
{
    log_info("close camera success");
    usb_clr_intr_txe(usb_id, UVC_STREAM_EP_IN);
    usb_g_set_intr_hander(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, NULL);
    usb_g_isr_wait_exit(usb_id);

    if (uvc_handle[usb_id] && uvc_handle[usb_id]->video_close) {
        uvc_handle[usb_id]->video_close(usb_id);
    }
}

typedef struct _USB_VIDEO_STATUS_VAR {
    u8  bStateType;
    u8  bOriginator;
    u8  bEvent;
    u8  bSelector_Value;//当为视频控制接口时，表示控制的类型；当为视频流接口时，表示BUTTON值

    u8  bAttribute;
    u8  bValue;
} sUSB_VIDEO_STATUS_VAR;
static sUSB_VIDEO_STATUS_VAR g_video_status;

static void usb_video_status(u8 *pStatus, u8 interface_type)
{
    log_info("usb_video_status");
}

void usb_video_shutter(usb_dev usb_id)
{
#if 0

    g_video_status.bStateType = VIDEO_CTL_INTFACE_TYPE;
    g_video_status.bOriginator = 0x01;
    g_video_status.bEvent = 0x00;
    g_video_status.bSelector_Value = 0x01;

    usb_video_status((u8 *)&g_video_status, VIDEO_CTL_INTFACE_TYPE);

    g_still_image_flag = 1;

    while (g_still_image_flag);


    g_video_status.bSelector_Value = 0x00;
    usb_video_status((u8 *)&g_video_status, VIDEO_CTL_INTFACE_TYPE);

#else
    log_info("shutter start....");
    g_still_image_flag = 1;
    log_info("shutter end....");
#endif
}

/*
Table 4-3 Defined Bits Containing Capabilities of the Control
D0 1=Supports GET value requests Capability
D1 1=Supports SET value requests Capability
D2 1=Disabled due to automatic mode (under device control) State
D3 1= Autoupdate Control (see section 2.4.2.2 "Status Interrupt Endpoint") Capability
D4 1= Asynchronous Control (see sections 2.4.2.2 "Status Interrupt Endpoint" and 2.4.4, “Control Transfer and Request Processing”) Capability
D7..D5 Reserved (Set to 0)
*/
void uvc_vc_pu_wBacklightCompensation_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBacklightCompensation[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBacklightCompensation[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBacklightCompensation[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBacklightCompensation[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wBacklightCompensation_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wBacklightCompensation[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wBrightness_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBrightness[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBrightness[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBrightness[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wBrightness[UVC_DEF], req->wLength);
        usb_device->wDataLength = req->wLength;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wBrightness_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wBrightness[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wContrast_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wContrast[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wContrast[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wContrast[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wContrast[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wContrast_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wContrast[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wGain_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGain[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGain[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGain[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGain[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wGain_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wGain[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bPowerLineFrequency_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bPowerLineFrequency[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bPowerLineFrequency[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_bPowerLineFrequency_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].bPowerLineFrequency[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wHue_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wHue[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wHue[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wHue[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wHue[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wHue_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wHue[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wSaturation_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSaturation[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSaturation[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSaturation[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSaturation[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wSaturation_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wSaturation[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wSharpness_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSharpness[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSharpness[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSharpness[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wSharpness[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wSharpness_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wSharpness[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wGamma_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGamma[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGamma[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGamma[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x0a;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wGamma[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wGamma_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wGamma[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wWhiteBalanceTemperature_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wWhiteBalanceTemperature_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wWhiteBalanceTemperature[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bWhiteBalanceTemperatureAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_bWhiteBalanceTemperatureAuto_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].bWhiteBalanceTemperatureAuto[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_white_balance_component_control_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].pu_white_balance_component_control[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].pu_white_balance_component_control[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].pu_white_balance_component_control[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].pu_white_balance_component_control[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_white_balance_component_control_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].pu_white_balance_component_control[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bWhiteBalanceComponentAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_bWhiteBalanceComponentAuto_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].bWhiteBalanceComponentAuto[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wMultiplierStep_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierStep[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierStep[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierStep[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierStep[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wMultiplierStep_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wMultiplierStep[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_wMultiplierLimit_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 value;

    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierLimit[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierLimit[UVC_MIN], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierLimit[UVC_MAX], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        value = 0x01;
        memcpy(usb_device->setup_ptr, &value, req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].wMultiplierLimit[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_wMultiplierLimit_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].wMultiplierLimit[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bHueAuto_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bHueAuto[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bHueAuto[UVC_DEF], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vc_pu_bHueAuto_handle;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            memcpy(&video_ctl_value[usb_id].bHueAuto[UVC_CUR], usb_device->setup_ptr, req->wLength);
        } else {
            log_info("setup state err");
        }
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bVideoStandard_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bVideoStandard[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_SET_CUR:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vc_pu_bStatus_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    log_debug("%s() %d", __func__, __LINE__);
    switch (req->bRequest) {
    case UVC_GET_CUR:
        memcpy(usb_device->setup_ptr, &video_ctl_value[usb_id].bStatus[UVC_CUR], req->wLength);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_MAX:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_RES:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_DEF:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    case UVC_SET_CUR:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
}

void uvc_vs_probe_commit_ctl_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 i, len = 0;

    switch (req->bRequest) {
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vs_probe_commit_ctl_handle;
            return;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            uvc_slave_probe_commit_control[usb_id][0] = usb_device->setup_ptr[0];  //bmHint
            uvc_slave_probe_commit_control[usb_id][1] = usb_device->setup_ptr[1];
            uvc_slave_probe_commit_control[usb_id][2] = usb_device->setup_ptr[2];  //bFormatIndex
            uvc_slave_probe_commit_control[usb_id][3] = usb_device->setup_ptr[3];  //bFrameIndex
            uvc_slave_probe_commit_control[usb_id][4] = usb_device->setup_ptr[4];  //dwFrameInterval
            uvc_slave_probe_commit_control[usb_id][5] = usb_device->setup_ptr[5];
            uvc_slave_probe_commit_control[usb_id][6] = usb_device->setup_ptr[6];
            uvc_slave_probe_commit_control[usb_id][7] = usb_device->setup_ptr[7];
#if !UVC_ISO_MODE
            if (HIBYTE(req->wValue) == VS_COMMIT_CONTROL) {
                usb_video_close_camera(usb_id);
                usb_video_open_camera(usb_id);
            }
#endif
        } else {
            log_info("setup state err");
        }
        break;
    case UVC_GET_CUR:
        len = sizeof(uvc_slave_probe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < len; i++) {
            usb_device->setup_ptr[i] = uvc_slave_probe_commit_control[usb_id][i];
        }
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
    case UVC_GET_DEF:
        len = sizeof(uvc_slave_probe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < usb_device->wDataLength; i++) {
            usb_device->setup_ptr[i] = uvc_slave_probe_commit_control[usb_id][i];
        }
        usb_device->setup_ptr[2] = 1;
        usb_device->setup_ptr[3] = 1;
        usb_device->setup_ptr[4] = DW1BYTE(FRAME_FPS);
        usb_device->setup_ptr[5] = DW2BYTE(FRAME_FPS);
        usb_device->setup_ptr[6] = DW3BYTE(FRAME_FPS);
        usb_device->setup_ptr[7] = DW4BYTE(FRAME_FPS);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        len = sizeof(uvc_slave_probe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < usb_device->wDataLength; i++) {
            usb_device->setup_ptr[i] = uvc_slave_probe_commit_control[usb_id][i];
        }
        usb_device->setup_ptr[2] = 1;
        usb_device->setup_ptr[3] = 5;
        usb_device->setup_ptr[4] = DW1BYTE(FRAME_FPS * 2);
        usb_device->setup_ptr[5] = DW2BYTE(FRAME_FPS * 2);
        usb_device->setup_ptr[6] = DW3BYTE(FRAME_FPS * 2);
        usb_device->setup_ptr[7] = DW4BYTE(FRAME_FPS * 2);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        len = sizeof(uvc_slave_probe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        memset(usb_device->setup_ptr, 0, usb_device->wDataLength);
        usb_device->setup_ptr[2] = 1;
        usb_device->setup_ptr[3] = 1;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_LEN:
        len = req->wLength;
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        *(u32 *)usb_device->setup_ptr = sizeof(uvc_slave_probe_commit_control[usb_id]);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    }

    log_debug("vs probe commit control data");
    if (usb_device->bsetup_phase == USB_EP0_STAGE_IN) {
        log_debug_hexdump(usb_device->setup_ptr, usb_device->wDataLength);
    } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
        log_debug_hexdump(uvc_slave_probe_commit_control[usb_id], sizeof(uvc_slave_probe_commit_control[usb_id]));
    }
}

void uvc_vs_still_probe_commit_ctl_handle(usb_dev usb_id, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 i, len = 0;

    switch (req->bRequest) {
    case UVC_SET_CUR:
        if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_OUT);
            uvc_handle[usb_id]->uvc_status = (u32)uvc_vs_still_probe_commit_ctl_handle;
            return;
        } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
            uvc_slave_stillprobe_commit_control[usb_id][0] = usb_device->setup_ptr[0];  //bFormatIndex
            uvc_slave_stillprobe_commit_control[usb_id][1] = usb_device->setup_ptr[1];  //bFrameIndex
            uvc_slave_stillprobe_commit_control[usb_id][2] = usb_device->setup_ptr[2];  //bCompressionIndex
        } else {
            log_info("setup state err");
        }
        break;
    case UVC_GET_CUR:
        len = sizeof(uvc_slave_stillprobe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < usb_device->wDataLength; i++) {
            usb_device->setup_ptr[i] = uvc_slave_stillprobe_commit_control[usb_id][i];
        }
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MIN:
    case UVC_GET_DEF:
        len = sizeof(uvc_slave_stillprobe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < usb_device->wDataLength; i++) {
            usb_device->setup_ptr[i] = uvc_slave_stillprobe_commit_control[usb_id][i];
        }
        usb_device->setup_ptr[0] = 1;  //bFormatIndex
        usb_device->setup_ptr[1] = 1;  //bFrameIndex
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_MAX:
        len = sizeof(uvc_slave_stillprobe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        for (i = 0; i < usb_device->wDataLength; i++) {
            usb_device->setup_ptr[i] = uvc_slave_stillprobe_commit_control[usb_id][i];
        }
        usb_device->setup_ptr[0] = 1;
        usb_device->setup_ptr[1] = 5;  //bFrameIndex
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_RES:
        len = sizeof(uvc_slave_stillprobe_commit_control[usb_id]);
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        memset(usb_device->setup_ptr, 0, usb_device->wDataLength);
        usb_device->setup_ptr[0] = 1;
        usb_device->setup_ptr[1] = 1;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_LEN:
        len = req->wLength;
        usb_device->wDataLength = req->wLength > len ? len : req->wLength;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        *(u32 *)usb_device->setup_ptr = sizeof(uvc_slave_stillprobe_commit_control[usb_id]);
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    case UVC_GET_INFO:
        usb_device->wDataLength = 1;
        if (usb_device->wDataLength < req->wLength) {
            usb_device->bDataOverFlag = 1;
        }
        usb_device->setup_ptr[0] = 0x03;
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_IN);
        break;
    }

    log_info("vs still probe commit control data");
    if (usb_device->bsetup_phase == USB_EP0_STAGE_IN) {
        log_info_hexdump(usb_device->setup_ptr, usb_device->wDataLength);
    } else if (usb_device->bsetup_phase == USB_EP0_STAGE_OUT) {
        log_info_hexdump(uvc_slave_stillprobe_commit_control[usb_id], sizeof(uvc_slave_stillprobe_commit_control[usb_id]));
    }
}

void uvc_set_camera_info(usb_dev usb_id, const struct usb_camera *info)
{
    int i;
    u16 width, height;
    u8 jpg_terminate = 0, yuv_terminate = 0;
    if (uvc_handle[usb_id] != NULL) {
        uvc_handle[usb_id]->video_open = info->video_open;
        uvc_handle[usb_id]->video_reqbuf = info->video_reqbuf;
        uvc_handle[usb_id]->video_close =  info->video_close;
        uvc_handle[usb_id]->processing_unit_response = info->processing_unit_response;
        uvc_handle[usb_id]->private_cmd_response = info->private_cmd_response;
        for (i = 0; i < FMT_MAX_RESO_NUM; i++) {
            uvc_handle[usb_id]->width[i] = 0;
            uvc_handle[usb_id]->height[i] = 0;
            if (info->jpg_info && i < info->jpg_info->num) {
                width = info->jpg_info->reso[i].width;
                height = info->jpg_info->reso[i].height;
                if (!jpg_terminate && (width && height)) {
                    uvc_handle[usb_id]->width[i] = width;
                    uvc_handle[usb_id]->height[i] = height;
                } else {
                    jpg_terminate = 1;
                }
            }
            uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM + i] = 0;
            uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM + i] = 0;
            if (info->yuv_info && i < info->yuv_info->num) {
                width = info->yuv_info->reso[i].width;
                height = info->yuv_info->reso[i].height;
                if (!yuv_terminate && (width && height)) {
                    uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM + i] = width;
                    uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM + i] = height;
                } else {
                    yuv_terminate = 1;
                }
            }
        }
    }
}

static int uvc_video_open(int idx, int fmt, int frame_id, int fps);
static int uvc_video_req_buf(int idx, void *buf, u32 len, u32 *frame_end);
static int uvc_video_close(int idx);

u32 uvc_register(usb_dev usb_id)
{
    if (uvc_handle[usb_id] == NULL) {
#if USB_MALLOC_ENABLE
        uvc_handle[usb_id] = malloc(sizeof(struct usb_uvc_camera));
        if (uvc_handle[usb_id] == NULL) {
            return (u32) - 1;
        }
        memset(uvc_handle[usb_id], 0, sizeof(struct usb_uvc_camera));
        uvc_handle[usb_id]->stream_buf = malloc(UVC_PKT_SPILT * UVC_FIFO_TXMAXP);
        if (uvc_handle[usb_id]->stream_buf == NULL) {
            free(uvc_handle[usb_id]);
            uvc_handle[usb_id] = NULL;
            goto __err_exit;
        }
#else
        uvc_handle[usb_id] = &_uvc_handle[usb_id];
        memset(uvc_handle[usb_id], 0, sizeof(struct usb_uvc_camera));
        uvc_handle[usb_id]->stream_buf = _stream_buf;
#endif
        uvc_handle[usb_id]->ep_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, UVC_STREAM_EP_IN | USB_DIR_IN, UVC_FIFO_TXMAXP * UVC_PKT_SPILT);
    }
    usb_video_feature_init(usb_id);

    struct usb_camera camera = {0};
    const struct uvc_reso_info yuv_fmt_info = {
        /* .num = 2, */
        .num = 1,
        .reso = {
            {UVC_YUV_DATA_WIDTH, UVC_YUV_DATA_HEIGHT},
            /* {320, 240}, */
        },
    };
    const struct uvc_reso_info jpg_fmt_info = {
        /* .num = 2, */
        .num = 1,
        .reso = {
            {UVC_YUV_DATA_WIDTH, UVC_YUV_DATA_HEIGHT},
            /* {320, 240}, */
        },
    };
    camera.video_open = uvc_video_open;
    camera.video_reqbuf = uvc_video_req_buf;
    camera.video_close = uvc_video_close;
#if UVC_FORMAT_MJPG
    camera.jpg_info = &jpg_fmt_info;
#endif
#if UVC_FORMAT_I420 || UVC_FORMAT_YUY2
    camera.yuv_info = &yuv_fmt_info;
#endif
    uvc_set_camera_info(usb_id, &camera);

#if TCFG_USB_SLAVE_MSD_ENABLE
    void isp_tool_init(usb_dev usb_id, struct ispt_param * cfg);
    struct ispt_param isp_cfg = {0};
    /* isp_cfg.get_video_device = get_video_device; */
    isp_tool_init(usb_id, &isp_cfg);
#endif
    return 0;

#if USB_MALLOC_ENABLE
__err_exit:
    if (uvc_handle[usb_id]->stream_buf) {
        free(uvc_handle[usb_id]->stream_buf);
    }
    free(uvc_handle[usb_id]);
    uvc_handle[usb_id] = NULL;
    return (u32) - 1;
#endif
}

u32 uvc_release(usb_dev usb_id)
{
    if (uvc_handle[usb_id]) {
        if (uvc_handle[usb_id]->video_close) {
            uvc_handle[usb_id]->video_close(usb_id);
        }
        if (uvc_handle[usb_id]->ep_dma_buffer) {
            usb_free_ep_dmabuffer(usb_id, uvc_handle[usb_id]->ep_dma_buffer);
            uvc_handle[usb_id]->ep_dma_buffer = NULL;
        }
#if USB_MALLOC_ENABLE
        if (uvc_handle[usb_id]->stream_buf) {
            free(uvc_handle[usb_id]->stream_buf);
        }
        free(uvc_handle[usb_id]);
#else
        uvc_handle[usb_id]->stream_buf = NULL;
#endif
        uvc_handle[usb_id] = NULL;
    }
    return 0;
}

/*------------------------------- uvc video interface -------------------------------*/
#define UVC_YUV_DATA_SOURCE "video0.2"

struct uvc_video_data {
    int pid;
    u8 *buf;
    u32 len;
    u8 *reqbuf;
    u16 reqlen;
    u16 send_len;
    OS_SEM wait_sem;
    OS_SEM req_sem;
    u8 cur_fmt_id;
    u8 cur_frame_id;
    u8 init;
    u8 end;
    u8 err;
    volatile u8 exit;
    volatile u8 complete;
    spinlock_t lock;
    usb_dev usb_id;
    u32 cur_fps;
    u32 offset;
    u16 width;
    u16 height;
#ifdef CONFIG_USR_VIDEO_ENABLE
    void *rt_user_handle;
#endif
};
static struct uvc_video_data *uvc_video_info[USB_MAX_HW_NUM];

#if UVC_BUF_USE_STATIC
static u8 uvc_yuv_buf[UVC_YUV_DATA_WIDTH * UVC_YUV_DATA_HEIGHT * 3 / 2] ALIGNE(32);
#endif

#if UVC_FORMAT_MJPG
static const char mjpg_fmt = 0;
#if UVC_FORMAT_YUY2 || UVC_FORMAT_NV12 || UVC_FORMAT_I420
static const char yuv_fmt = 1;
#else
static const char yuv_fmt = -1;
#endif
#else
static const char mjpg_fmt = -1;
#if UVC_FORMAT_YUY2 || UVC_FORMAT_NV12 || UVC_FORMAT_I420
static const char yuv_fmt = 0;
#else
static const char yuv_fmt = -1;
#endif
#endif

__attribute__((weak)) void *get_static_yuv_buf(void)
{
    return NULL;
}

static u32 uvc_send_buf(void *priv, u8 *buf, u32 len)
{
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;
    int ret;
    int remain;
    u32 offset = 0;

    if (!video_info->reqlen || video_info->end) {
        return 0;
    }
    remain = len;

    while (remain > 0) {
        if (video_info->exit) {
            break;
        }
        ret = os_sem_pend(&video_info->wait_sem, 100);//先等待USB中断来请求数据包
        if (ret) {
            video_info->reqlen = 0;
            log_error("wait uvc request err !!!");
            return 0;
        }
        if (video_info->complete) {
            continue;
        }
        spin_lock(&video_info->lock);
        video_info->reqbuf = (u8 *)(buf + offset);//赋值数据包地址
        video_info->send_len = video_info->reqlen;//赋值数据包大小
        if (video_info->send_len > remain) {
            video_info->send_len = remain;
        }
        offset += video_info->send_len;
        remain -= video_info->send_len;
        if (remain <= 0) {
            video_info->end = true;
        }
        video_info->complete = 1;
        spin_unlock(&video_info->lock);
    }
    return offset;
}

static void uvc_yuv_task(void *priv)
{
    int ret;
    void *video_dev_fd = NULL;
    void *fb = NULL;
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;
    struct fb_map_user map;

    //数据来源video，输出到fb
    struct video_format f = {0};
    struct fb_var_screeninfo fb2_info = {0};
    const char *dev_name = video_info == uvc_video_info[0] ? TCFG_SLAVE_UVC0_YUV_DATA_SOURCE : TCFG_SLAVE_UVC1_YUV_DATA_SOURCE;
    const char *fb_name = video_info == uvc_video_info[0] ? "fb6" : "fb7";//fb5 之后不是合成

    f.type  = VIDEO_BUF_TYPE_VIDEO_OVERLAY;
    /* f.src_width = TCFG_YUV_DATA_IN_WIDTH; */
    /* f.src_height = TCFG_YUV_DATA_IN_HEIGHT; */
    f.win.left = 0;
    f.win.top    = 0;
    f.win.width  = TCFG_YUV_DATA_OUT_WIDTH;
    f.win.height = TCFG_YUV_DATA_OUT_HEIGHT;
    f.win.border_left   = 0;
    f.win.border_top    = 0;
    f.win.border_right  = 0;
    f.win.border_bottom = 0;
    f.pixelformat = VIDEO_PIX_FMT_YUV420;
    f.private_data = (void *)fb_name;

    video_dev_fd = dev_open(dev_name, NULL);
    if (video_dev_fd == NULL) {
        video_info->err = true;
        log_error("%s test open device %s faild", fb_name, dev_name);
        return;
    }

    ret = dev_ioctl(video_dev_fd, VIDIOC_SET_FMT, (u32)&f);
    if (ret) {
        video_info->err = true;
        log_error("VIDIOC_SET_FMT faild");
        dev_close(video_dev_fd);
        return;
    }

    ret = dev_ioctl(video_dev_fd, VIDIOC_OVERLAY, 1);
    if (ret) {
        video_info->err = true;
        log_error("VIDIOC_OVERLAY faild");
        dev_close(video_dev_fd);
        return;
    }

    //打开显示通道
    fb = dev_open(f.private_data, (void *)FB_COLOR_FORMAT_YUV422);
    if (!fb) {
        video_info->err = true;
        log_error(" ||| %s ,,, %d", __FUNCTION__, __LINE__);
        dev_close(video_dev_fd);
        return;
    }

    while (1) {
        if (video_info->exit) {
            break;
        }

        dev_ioctl(fb, FBIOGET_FBUFFER_INUSED, (int)&map);
        if ((u32)map.baddr) {
            //需要使用YUV数据的任务的回调
#if UVC_FORMAT_I420
            uvc_send_buf(priv, map.baddr, map.width * map.height * 3 / 2);//注册YUV回调函数
#elif UVC_FORMAT_YUY2
            uvc_send_buf(priv, map.baddr, map.width * map.height * 2);//注册YUV回调函数
#endif
            //获取到才释放
            dev_ioctl(fb, FBIOPUT_FBUFFER_INUSED, (int)&map);
        } else {
            //获取不到重试
            continue;
        }
    }

    dev_close(fb);
    dev_ioctl(video_dev_fd, VIDIOC_OVERLAY, 0);
    dev_close(video_dev_fd);
}

static void uvc_mjpeg_task(void *priv)
{
    //TODO
}

static int uvc_video_yuv_open(int priv)
{
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;

    //cppcheck-suppress knownConditionTrueFalse
    if (NULL == get_static_yuv_buf()) {
        video_info->buf = malloc(video_info->width * video_info->height * 3 / 2);
        if (!video_info->buf) {
            log_error("video_infoloc err !!!");
            return -1;
        }
    } else {
        video_info->buf = get_static_yuv_buf();
    }
    video_info->len = video_info->width * video_info->height * 3 / 2;
    video_info->err = false;
    video_info->end = false;
    video_info->exit = false;
    video_info->reqbuf = NULL;
    video_info->reqlen = 0;
    video_info->complete = 0;
    os_sem_create(&video_info->wait_sem, 0);
    os_sem_create(&video_info->req_sem, 0);
    if (thread_fork(video_info->usb_id ? "uvc1_yuv_task" : "uvc0_yuv_task", 8, 512, 0, &video_info->pid, uvc_yuv_task, (void *)video_info) < 0) {
        //cppcheck-suppress knownConditionTrueFalse
        if (video_info->buf && NULL == get_static_yuv_buf()) {
            free(video_info->buf);
        }
        video_info->buf = NULL;
        return -1;
    }
    video_info->init = true;
    return 0;
}

static int uvc_video_yuv_close(int priv)
{
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;

    os_sem_set(&video_info->wait_sem, 0);
    os_sem_post(&video_info->wait_sem);
    thread_kill(&video_info->pid, KILL_WAIT);
    video_info->init = false;
    video_info->end = false;
    video_info->err = false;
    video_info->reqbuf = NULL;
    video_info->reqlen = 0;
    os_sem_del(&video_info->wait_sem, OS_DEL_ALWAYS);
    os_sem_del(&video_info->req_sem, OS_DEL_ALWAYS);
    //cppcheck-suppress knownConditionTrueFalse
    if (video_info->buf && NULL == get_static_yuv_buf()) {
        free(video_info->buf);
    }
    video_info->buf = NULL;

    free(video_info);

    return 0;
}

static int uvc_video_mjpeg_open(int priv)
{
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;

    video_info->buf = malloc(video_info->width / 320 * 30 * 1024);
    if (!video_info->buf) {
        log_error("uvc_video_info buf malloc err !!!");
        return -1;
    }
    video_info->len = video_info->width / 320 * 30 * 1024;
    video_info->err = false;
    video_info->end = false;
    video_info->exit = false;
    video_info->reqbuf = NULL;
    video_info->reqlen = 0;
    video_info->complete = 0;
    os_sem_create(&video_info->wait_sem, 0);
    os_sem_create(&video_info->req_sem, 0);

#if UVC_FORMAT_MJPG
#ifdef PRODUCT_NET_SERVER_ENABLE
    if (build_camera_data_conn(uvc_send_buf, video_info) == FALSE) {
        free(video_info->buf);
        os_sem_del(&video_info->wait_sem, OS_DEL_ALWAYS);
        os_sem_del(&video_info->req_sem, OS_DEL_ALWAYS);
        return -1;
    }
#else
#ifdef CONFIG_USR_VIDEO_ENABLE
    extern void set_video_rt_cb(void *handle, u32(*cb)(void *, u8 *, u32), void *priv, u8 id);
    extern void *user_video_rec_open(const char *video_name, u8 id);
    video_info->rt_user_handle = user_video_rec_open(video_info->usb_id ? TCFG_SLAVE_UVC1_JPEG_DATA_SOURCE : TCFG_SLAVE_UVC0_JPEG_DATA_SOURCE, video_info->usb_id);//打开摄像头
    if (video_info->rt_user_handle) {
        set_video_rt_cb(video_info->rt_user_handle, uvc_send_buf, video_info, video_info->usb_id);//注册JPEG回调函数
    }
#endif
#endif
#endif
    video_info->init = true;
    return 0;
}

void set_uvc_handle_cb(void *handle, u8 id)
{
    ASSERT(id == 0 || id == 1);

    struct uvc_video_data *video_info = (struct uvc_video_data *)uvc_video_info[id];
    video_info->rt_user_handle = handle;
    set_video_rt_cb(video_info->rt_user_handle, uvc_send_buf, video_info, id);//注册JPEG回调函数
}

static int uvc_video_mjpeg_close(int priv)
{
    struct uvc_video_data *video_info = (struct uvc_video_data *)priv;

#if UVC_FORMAT_MJPG
    os_sem_set(&video_info->wait_sem, 0);
    os_sem_post(&video_info->wait_sem);
#ifdef PRODUCT_NET_SERVER_ENABLE
    delete_camera_data_conn();
#else
#ifdef CONFIG_USR_VIDEO_ENABLE
    extern int user_video_rec_close(void *handle, u8 id);
    if (video_info->rt_user_handle) {
        user_video_rec_close(video_info->rt_user_handle, video_info->usb_id);
        video_info->rt_user_handle = NULL;
    }
#endif
#endif
#endif
    video_info->init = false;
    video_info->end = false;
    video_info->err = false;
    video_info->reqbuf = NULL;
    video_info->reqlen = 0;
    os_sem_del(&video_info->wait_sem, OS_DEL_ALWAYS);
    os_sem_del(&video_info->req_sem, OS_DEL_ALWAYS);
    if (video_info->buf) {
        free(video_info->buf);
        video_info->buf = NULL;
    }

    free(video_info);

    return 0;
}

static int uvc_video_open(int idx, int fmt, int frame_id, int fps)
{
    usb_dev usb_id = idx;

    if (uvc_video_info[usb_id]) {
        return -1;
    }

    uvc_video_info[usb_id] = malloc(sizeof(struct uvc_video_data));
    if (!uvc_video_info[usb_id]) {
        return -1;
    }

    if (uvc_handle[usb_id] == NULL) {
        return -1;
    }

    memset(uvc_video_info[usb_id], 0, sizeof(struct uvc_video_data));

#if UVC_TEST_ENABLE
    uvc_video_info[usb_id]->offset = 0;
    return 0;
#else  /*UVC_TEST_ENABLE == 0*/
    uvc_video_info[usb_id]->cur_fmt_id = 0xff;
    uvc_video_info[usb_id]->cur_frame_id = 0xff;
    uvc_video_info[usb_id]->cur_fps = 0xffffffff;
    uvc_video_info[usb_id]->usb_id = usb_id;

    if (fmt == yuv_fmt) {
        uvc_video_info[usb_id]->cur_fmt_id = fmt;
        uvc_video_info[usb_id]->cur_frame_id = frame_id;
        uvc_video_info[usb_id]->cur_fps = fps;
        uvc_video_info[usb_id]->width = uvc_handle[usb_id]->width[FMT_MAX_RESO_NUM + frame_id];
        uvc_video_info[usb_id]->height = uvc_handle[usb_id]->height[FMT_MAX_RESO_NUM + frame_id];
        int msg[3] = {(int)uvc_video_yuv_open, 1, (u32)uvc_video_info[usb_id]};
        os_taskq_post_type("sys_timer", Q_CALLBACK, 3, msg);
    } else if (fmt == mjpg_fmt) {
        uvc_video_info[usb_id]->cur_fmt_id = fmt;
        uvc_video_info[usb_id]->cur_frame_id = frame_id;
        uvc_video_info[usb_id]->cur_fps = fps;
        uvc_video_info[usb_id]->width = uvc_handle[usb_id]->width[frame_id];
        uvc_video_info[usb_id]->height = uvc_handle[usb_id]->height[frame_id];
        int msg[3] = {(int)uvc_video_mjpeg_open, 1, (u32)uvc_video_info[usb_id]};
        os_taskq_post_type("sys_timer", Q_CALLBACK, 3, msg);
    }

    return 0;
#endif
}

static int uvc_video_req_buf(int usb_id, void *buf, u32 len, u32 *frame_end)
{
    u32 len2;

#if UVC_TEST_ENABLE
#if !UVC_FORMAT_MJPG && UVC_FORMAT_I420
    len2 = len;
    const u32 size = UVC_YUV_DATA_WIDTH * UVC_YUV_DATA_HEIGHT;
    u32 b_offset = 0;
    u32 frame_offset = uvc_video_info[usb_id]->offset;
    u32 y_len, u_len, v_len;
    u32 bottom;
    u32 top = size;
    if (frame_offset < top) {
        y_len = frame_offset + len2 > top ? top - frame_offset : len2;
        memset((u8 *)buf + b_offset, 0x51, y_len);
        frame_offset += y_len;
        len2 -= y_len;
        b_offset += y_len;
    }
    bottom = size;
    top = size + size / 4;
    if (frame_offset >= bottom && frame_offset < top) {
        u_len = frame_offset + len2 > top ? top - frame_offset : len2;
        memset((u8 *)buf + b_offset, 0x5a, u_len);
        frame_offset += u_len;
        len2 -= u_len;
        b_offset += u_len;
    }
    bottom = size + size / 4;
    top = size + size / 4 + size / 4;
    if (frame_offset >= bottom && frame_offset < top) {
        v_len = frame_offset + len2 > top ? top - frame_offset : len2;
        memset((u8 *)buf + b_offset, 0xef, v_len);
        frame_offset += v_len;
        len2 -= v_len;
        b_offset += v_len;
    }
    if (frame_offset >= size * 3 / 2) {
        *frame_end = 1;
        frame_offset = 0;
    } else {
        *frame_end = 0;
    }
    uvc_video_info[usb_id]->offset = frame_offset;
    return len - len2;
#else
    static u32 cnt = 0 ;
    static volatile u8 frame_dummy = 0 ;
    len = (sizeof(jpeg_data) - uvc_video_info[usb_id]->offset < len) ? (sizeof(jpeg_data) - uvc_video_info[usb_id]->offset) : len;
    memcpy(buf, jpeg_data + uvc_video_info[usb_id]->offset, len);
    uvc_video_info[usb_id]->offset += len;
    /* printf("%d=%d",len,uvc_video_info[usb_id]->offset ); */
    if (uvc_video_info[usb_id]->offset >= sizeof(jpeg_data)) {
        uvc_video_info[usb_id]->offset = 0;
        *frame_end = 1;
    } else {
        *frame_end = 0;
    }
    /* *frame_end = 0; */
    return len;
#endif

#else  /*UVC_TEST_ENABLE == 0*/
    if (uvc_video_info[usb_id]->cur_fmt_id == yuv_fmt || uvc_video_info[usb_id]->cur_fmt_id == mjpg_fmt) {
        if (uvc_video_info[usb_id]->exit) {
            *frame_end = 0;
            return 0;
        }
        uvc_video_info[usb_id]->reqlen = len;
        if (!uvc_video_info[usb_id]->init || uvc_video_info[usb_id]->err) {
            *frame_end = 0;
            /* log_error("err : uvc yuv not open or open video err !!!!"); */
            return 0;
        }
        spin_lock(&uvc_video_info[usb_id]->lock);
        if (!uvc_video_info[usb_id]->complete) {
            spin_unlock(&uvc_video_info[usb_id]->lock);
            os_sem_set(&uvc_video_info[usb_id]->wait_sem, 0);
            os_sem_post(&uvc_video_info[usb_id]->wait_sem);
            *frame_end = 0;
            return 0;
        }
        memcpy(buf, uvc_video_info[usb_id]->reqbuf, uvc_video_info[usb_id]->send_len);
        if (uvc_video_info[usb_id]->end) {
            *frame_end = 1;
        } else {
            *frame_end = 0;
        }
        uvc_video_info[usb_id]->end = false;
        len2 = uvc_video_info[usb_id]->send_len;
        uvc_video_info[usb_id]->complete = 0;
        spin_unlock(&uvc_video_info[usb_id]->lock);
        os_sem_set(&uvc_video_info[usb_id]->wait_sem, 0);
        os_sem_post(&uvc_video_info[usb_id]->wait_sem);
    } else {
        len2 = 0;
        *frame_end = 0;
    }
    return len2;
#endif
}

static int uvc_video_close(int usb_id)
{
#if UVC_TEST_ENABLE
    uvc_video_info[usb_id]->offset = 0;
    free(uvc_video_info[usb_id]);
    uvc_video_info[usb_id] = NULL;
    return 0;
#else  /*UVC_TEST_ENABLE == 0*/
    if (!uvc_video_info[usb_id] || uvc_video_info[usb_id]->init == false) {
        return 0;
    }
    if (uvc_video_info[usb_id]->cur_fmt_id == yuv_fmt) {
        uvc_video_info[usb_id]->exit = true;
        int msg[3] = {(int)uvc_video_yuv_close, 1, (u32)uvc_video_info[usb_id]};
        os_taskq_post_type("sys_timer", Q_CALLBACK, 3, msg);
        uvc_video_info[usb_id] = NULL;
    } else if (uvc_video_info[usb_id]->cur_fmt_id == mjpg_fmt) {
        uvc_video_info[usb_id]->exit = true;
        int msg[3] = {(int)uvc_video_mjpeg_close, 1, (u32)uvc_video_info[usb_id]};
        os_taskq_post_type("sys_timer", Q_CALLBACK, 3, msg);
        uvc_video_info[usb_id] = NULL;
    }

    return 0;
#endif
}

#ifdef PRODUCT_NET_SERVER_ENABLE
void product_camera_reconnect(void)
{
    build_camera_data_conn(uvc_send_buf, uvc_video_info[usb_id]);
}
#endif

#endif
