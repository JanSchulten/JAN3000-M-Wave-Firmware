#ifndef DEVICE_CAMERA_H
#define DEVICE_CAMERA_H



#include "device/device.h"
#include "asm/isp_dev.h"
// #include "video/video.h"


#define VIDEO_TAG_CAMERA        VIDEO_TAG('c', 'a', 'm', 'e')
#define VIDEO_TAG_UVC           VIDEO_TAG('u', 'v', 'c', ' ')
#define VIDEO_TAG_MASS           VIDEO_TAG('m', 'a', 's', 's')


#define ENDPOINT_NUM 	5
#define CAMERA_DEVICE_NUM  13

#define CSI2_X0_LANE    0
#define CSI2_X1_LANE    1
#define CSI2_X2_LANE    2
#define CSI2_X3_LANE    3
#define CSI2_X4_LANE    4


#define DVP0_INTERFACE  0
#define DVP1_INTERFACE  1
#define CSI2_INTERFACE  2

// #define UVC_INTERFACE  3

#define MCV0_INTERFACE  4   //修改序号需要修改跟之相关的逻辑(非作者本人勿动)
#define MCV1_INTERFACE  5
#define MCV2_INTERFACE  6
#define MCV3_INTERFACE  7


struct camera_platform_data {
    char *iic_sel;
    u8 xclk_gpio;
    u8 reset_gpio;
    u8 pwdn_gpio;
    u8 power_value;
    u32 interface;
    bool (*online_detect)();
    union {
        struct {
            u32 pclk_gpio;
            u32 hsync_gpio;
            u32 vsync_gpio;
            u8 group_port;  //DVP0 0: PD9~PE2 1: PA4~PA11  DVP1 0: PB0_PB7 1: PB8_PB15
            u8 reverse;     //0: 正序 1: 反序
            u8 shift;       //数据位偏移量
            u8 timing_mode; //时序选择 0:DVP 1:BT601 2:VSYNC 3:BT656
            u8 scan_mode;   //扫描模式 0:逐行扫描(default) 1:奇场数据 2:偶场数据
            u8 width_mode;  //位宽选择 0:全位宽(8/10bit) 1: 4bit 2: 2bit 3: 1bit
            u32 h_blank; //水平消隐后肩周期数, 默认填0
            u32 v_blank; //垂直消隐后肩行数, 默认填0
            u32 data_gpio[10];
        } dvp;
        struct {
            u8 data_lane_num;
            u8 clk_rmap;
            u8 clk_inv;
            u8 d0_rmap;
            u8 d0_inv;
            u8 d1_rmap;
            u8 d1_inv;
            u8 d2_rmap;
            u8 d2_inv;
            u8 d3_rmap;
            u8 d3_inv;
            u8 tval_hstt;
            u8 tval_stto;
        } csi2;
    };
};

struct camera_device_info {
    u16 fps;
    u16 sen_width;
    u16 sen_height;
    u16 width;
    u16 height;
    u32 real_fps;
};


#define CAMERA_PLATFORM_DATA_BEGIN(data) \
	static const struct camera_platform_data data = { \


#define CAMERA_PLATFORM_DATA_END() \
	};

#define CAMERA_CMD_BASE		0x00400000
#define CAMERA_GET_ISP_SRC_SIZE		(CAMERA_CMD_BASE + 1)
#define CAMERA_GET_ISP_SIZE		(CAMERA_CMD_BASE + 2)
#define CAMERA_SET_CROP_SIZE		(CAMERA_CMD_BASE + 3)
#define CAMERA_CROP_TRIG		(CAMERA_CMD_BASE + 4)
#define CAMERA_NEED_REMOUNT		(CAMERA_CMD_BASE + 5)
#define CAMERA_GET_SENSOR_ID    (CAMERA_CMD_BASE + 6)
#define CAMERA_GET_CAPTURE_RAW  (CAMERA_CMD_BASE + 7)
#define CAMERA_SET_TEST_KICK    (CAMERA_CMD_BASE + 8)
#define CAMERA_GET_TEST_DONE    (CAMERA_CMD_BASE + 9)

extern const struct device_operations camera_dev_ops;
// extern struct camera_device;

//dvp sensor io sel
#define DVP_SENSOR0(sel)   (((sel?1:0) << 24) | (22))//sel 0 PC3~PC13  1 PA6~PA15
#define DVP_SENSOR1(sel)   (((sel?1:0) << 24) | (23))//sel 0 PH1~PH11  1 PD0~PD10


int camera_init(const char *name, void *camera_data);

void *camera_driver_open(int id, struct camera_device_info *info);
int camera_driver_close(void *_camera);
int camera_driver_force_close(void *_camera);
int camera_driver_ioctl(void *_camera, u32 cmd, void *arg);
void *camera_get_sen_dev(void *c);
void *camera_get_platform_data(void *c);
int get_camera_devices_fps(int id);
int camera_get_in_format(void *c);

void sensor_driver_close(int channel, int interface_type);
void *sensor_driver_open(int channel, int interface_type);
int sensor_driver_init(void *_camera);

void camera_info_copy(int interface_type, void *camera);


int camera_mux_init(const char *name, void *camera_data);
bool camera_mux_online(const char *name);
int camera_mux_close(void *_camera);
void *camera_mux_open(int id, struct camera_device_info *info);


#endif

