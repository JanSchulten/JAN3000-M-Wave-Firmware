#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "typedef.h"
#include "ioctl.h"
#include "device/device.h"
#include "system/spinlock.h"


#define 	FBIOGET_VSCREENINFO 		_IOR('F', 0, sizeof(struct fb_var_screeninfo))
#define 	FBIOSET_VSCREENINFO 		_IOW('F', 0, sizeof(struct fb_var_screeninfo))
#define 	FBIOGETMAP 					_IOR('F', 1, sizeof(struct fb_map_user))
#define 	FBIOPUTMAP 					_IOW('F', 1, sizeof(struct fb_map_user))
#define 	FBIOBLANK 					_IOW('F', 2, sizeof(int))
#define 	FBIOSET_TRANSP 				_IOW('F', 3, sizeof(int))
#define 	FBIOSET_PALLET 				_IOW('F', 4, sizeof(int))
#define 	FBIOSET_FBUFFER_NUM			_IOW('F', 5, sizeof(int))
#define     FBIOGET_FBUFFER_NUM         _IOR('F', 5, sizeof(int))
#define 	FBIOGET_FBUFFER 			_IOW('F', 6, sizeof(int))
#define 	FBIOPUT_FBUFFER 			_IOW('F', 7, sizeof(int))
#define 	FBIOSET_ENABLE 				_IOW('F', 8, sizeof(int))
#define     FBIOSET_BACKCOLOR           _IOW('F', 9, sizeof(int))
#define     FBIOGET_FBUFFER_INUSED      _IOR('F', 10, sizeof(int))
#define     FBIOPUT_FBUFFER_INUSED      _IOW('F', 10, sizeof(int))
#define     FBIOGET_DEVSCREENINFO       _IOR('F', 11, sizeof(struct lcd_screen_info))
#define     FBIOSET_BLOCK               _IOW('F', 12, sizeof(int))
#define     FBIOGET_PRIVATE_ATTR        _IOR('F', 13, sizeof(void *))
#define     FBIOSET_EVENT_HANDLER       _IOR('F', 14, sizeof(void *))
#define     FBIOGET_ALLOC_FBUFFER       _IOR('F', 15, sizeof(int))
#define     FBIOPUT_FREE_FBUFFER        _IOW('F', 16, sizeof(int))
#define 	FBIOGET_MAP_PENDING_STATUS	_IOR('F', 17, sizeof(int))

#define FB_MAX_OUT_NUM  (2)

enum fb_event {
    FB_EVT_PAUSE,
    FB_EVT_RESUME,
};

enum {
    FB_BUFFER_IDLE,
    FB_BUFFER_LOCK,
    FB_BUFFER_PENDING,
    FB_BUFFER_INUSED,
};

enum {
    MODE_SHARED,
};

//注：以下顺序不可更改
enum {
    FB_COLOR_FORMAT_ARGB8888 = 0x00,
    FB_COLOR_FORMAT_ARGB8565 = 0x01,
    FB_COLOR_FORMAT_ARGB1555 = 0x02,
    FB_COLOR_FORMAT_ARGB4444 = 0x03,
    FB_COLOR_FORMAT_RGB888   = 0x04,
    FB_COLOR_FORMAT_RGB565   = 0x05,
    FB_COLOR_FORMAT_YUV422, //yuyv uyvy
    FB_COLOR_FORMAT_YUV422_1,
    FB_COLOR_FORMAT_AL88,
    FB_COLOR_FORMAT_AL44 = 0x10,
    FB_COLOR_FORMAT_AL22 = 0x11,
    FB_COLOR_FORMAT_L8 = 0x12,
    FB_COLOR_FORMAT_L4 = 0x13,
    FB_COLOR_FORMAT_L2 = 0x14,
    FB_COLOR_FORMAT_L1 = 0x15,
    FB_COLOR_FORMAT_A8 = 0x16,
    FB_COLOR_FORMAT_A4 = 0x17,
    FB_COLOR_FORMAT_A2 = 0x18,
    FB_COLOR_FORMAT_A1 = 0x19,
    //新增
    FB_COLOR_FORMAT_YUV420P,
    FB_COLOR_FORMAT_YUV422P,
    FB_COLOR_FORMAT_RGB8565A8,

};

struct fb_buffer {
    struct list_head entry;
    u8 status;
    u8 *baddr;
};

struct fb_map_user {
    u16 xoffset;
    u16 yoffset;
    u16 width;
    u16 height;
    u16 real_width;
    u16 real_height;
    u16 rotate;
    u16 mirror;
    u8  buf_index;
    u8  *baddr;
    u8  *yaddr;
    u8  *uaddr;
    u8  *vaddr;
    u8  *baddr_bk;
    u8 transp;
    u8 format;
};

struct fb_var_screeninfo {
    u8  ref;
    u16 s_xoffset;            //显示区域x坐标
    u16 s_yoffset;            //显示区域y坐标
    u16 s_xres;               //显示区域宽度
    u16 s_yres;               //显示区域高度
    u16 v_xoffset;      //屏幕的虚拟x坐标
    u16 v_yoffset;      //屏幕的虚拟y坐标
    u16 v_xres;         //屏幕的虚拟宽度
    u16 v_yres;         //屏幕的虚拟高度
    u16 rotate;
    u16 mirror;
    u16 combine;        //是否需要参与fb合成
    u8 out_id;          //fb输出设备ID(用于多屏)
    void *out_dev;      //fb输出句柄
};

struct fb_info;

struct fb_ops {
    int (*fb_open)(struct fb_info *fb, void *arg);
    int (*fb_ioctl)(struct fb_info *fb, int cmd,  unsigned long arg);
    int (*fb_close)(struct fb_info *fb);
};

struct fb_evt_handler {
    void *priv;
    int (*handler)(void *, enum fb_event evt);
};

struct fb_platform_data {
    u8 num;
};

struct fb_info {
    struct list_head entry;
    const char *name;
    u8 index;
    u8 format;
    u8 block;
    u8 pause;
    u8 fb_num;
    struct fb_var_screeninfo var;
    const struct fb_ops *fbops;
    struct device device;
    void *private_data;
    void *pd;
    spinlock_t lock;
    struct fb_buffer *pending;
    struct fb_buffer *inused;
    struct fb_buffer fb_buf[3];
    struct list_head head;
    struct fb_evt_handler evt_hdl;
};


struct fb_out_t {
    struct list_head entry;
    struct device device;
    u8 out_id;  //输出设备ID(用于多屏)
    char fb_name[4];
    u8 buf_num;
    u32 buf_addr[3];
    u8 buf_lock[3];
    u8 z_order;
    volatile u8 ready_combine;
    volatile u8 lock;
    void *fb;
    struct fb_map_user map;
};

struct fb_draw_info {
    void *priv;
    char *name;       //显示图层名"fb0" "fb1" "fb2" ...
    u16 x;            //显示起始x坐标
    u16 y;            //显示起始y坐标
    u16 width;        //显示区域宽度(显存)
    u16 height;       //显示区域高度(显存)
    u16 real_width;   //实际显示宽度
    u16 real_height;  //实际显示高度
    u16 format;       //显示格式
    u8  z_order;      //显示顺序
    u8  fb_num;       //显存个数
    u8  combine;      //是否需要参与fb合成
    u8  out_id;       //输出设备ID(用于多屏)
    char *out_name;   //输出设备
    u16 rotate;
    u16 mirror;
};

struct fb_out_info {
    char *name;       //显示设备名
    u16 x;            //显示起始x坐标
    u16 y;            //显示起始y坐标
    u16 width;        //显示区域宽度
    u16 height;       //显示区域高度
    u16 format;       //显示格式
    u8  out_id;       //显示设备ID号(用于多屏)
    u8 *out_addr;     //显存初始地址
    u8  out_buf_num;  //显存个数
    u8  ext_buf;      //显示buffer指定方式 0:malloc 1:由外部指定
    u16 rotate;
};

extern const struct device_operations fb_dev_ops;
extern const struct device_operations fb_out_dev_ops;

int register_fbinfo(const char *fb_name, void *pd);

int fb_pause(struct fb_info *fb);

void fb_resume(struct fb_info *fb);

int fb_malloc_buffer(struct fb_info *fb);

void fb_free_buffer(struct fb_info *fb);

void __fb_put_map_complete(struct fb_info *fb, u32 baddr);

void __fb_gc_buffer_complete(void);

void fb_combine_set_out_cb(u32 arg);

void *fb_draw_output_config(struct fb_out_info *out_info);

void *fb_draw_open(struct fb_draw_info *arg);

int fb_draw_getmap(void *fb, struct fb_map_user *map);

int fb_draw_putmap(void *fb, struct fb_map_user *map);

void fb_draw_close(void *fb);












#endif

