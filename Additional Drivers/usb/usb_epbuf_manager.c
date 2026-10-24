#include "usb_config.h"
#include "app_config.h"
#include "lbuf.h"
#include "system/init.h"
#include "host_uvc.h"
#include "asm/debug.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE

#define USB_RAW_ALLOC_EN  1  //ram动态malloc

#define USB_DMA_BUF_ALIGN 64  //husb的ep1, ep2, ep5, ep6的dma地址需要64字节对齐，ep3, ep4的dma地址需要8对齐

//#ifndef USB_DMA_BUF_MAX_SIZE
//#define USB_DMA_BUF_MAX_SIZE (HID_DMA_SIZE + AUDIO_DMA_SIZE + MSD_DMA_SIZE * 2 + CDC_DMA_SIZE + VIDEO_DMA_SIZE)
//#endif

#if (USB_RAW_ALLOC_EN == 0)
//先定义一个足够大部分情况复合设备使用的buffer大小，供host和device一起
//使用，后续再细调大小，需要注意的是，每次调用lbuf_alloc()都需要额外的
//空间来保存头信息，因此需要预留比实际所需稍大的buffer空间
#if (defined USB_DEVICE_CLASS_CONFIG_2_0) && (USB_DEVICE_CLASS_CONFIG_2_0 != 0) && (defined USB_DEVICE_CLASS_CONFIG) && (USB_DEVICE_CLASS_CONFIG != 0)
static u8 usb_dma_buf[64 * 1024] SEC(.usb_fifo) __attribute__((aligned(64)));
#else
static u8 usb_dma_buf[4 * 1024] SEC(.usb_fifo) __attribute__((aligned(64)));
#endif
static struct lbuff_head *usb_dma_lbuf = NULL;


static int usb_memory_init(void)
{
    usb_dma_lbuf = lbuf_init(usb_dma_buf, sizeof(usb_dma_buf), USB_DMA_BUF_ALIGN, 0);
    log_info("%s() total dma size %d @%x", __func__, sizeof(usb_dma_buf), usb_dma_buf);
    return 0;
}
early_initcall(usb_memory_init);

#endif

void *usb_epbuf_alloc(const usb_dev usb_id, u32 size)
{
    if (usb_id == 0) {
        size += 8;  //usb0的rx，需要额外4字节存放接收的crc，使用ping-pong buffer就需要两份，即4 x 2
    }

#if (USB_RAW_ALLOC_EN == 0)
    return lbuf_alloc(usb_dma_lbuf, size);
#else
    return ram_malloc(size);
#endif
}

void usb_epbuf_free(const usb_dev usb_id, void *buf)
{
#if (USB_RAW_ALLOC_EN == 0)
    lbuf_free(buf);
#else
    ram_free(buf);
#endif
}

u32 usb_epbuf_free_space(void)
{
#if (USB_RAW_ALLOC_EN == 0)
    return lbuf_free_space(usb_dma_lbuf);
#else
    return 0;
#endif
}

#if (USB_RAW_ALLOC_EN == 0)
void usb_epbuf_limit_check_open(const usb_dev usb_id, int limit_ranage_out)
{
    if (limit_ranage_out) {
        mpu_enable_by_index(usb_id, (u32)&usb_dma_buf, (u32)&usb_dma_buf + sizeof(usb_dma_buf) - 1, 1, "CrwxPrw0rw", get_debug_dev_id("DBG_USB"));
    } else {
        mpu_enable_by_index(usb_id, (u32)&usb_dma_buf, (u32)&usb_dma_buf + sizeof(usb_dma_buf) - 1, 0, "Crw0rw", get_debug_dev_id("DBG_USB"));
    }
}

void usb_epbuf_limit_check_close(const usb_dev usb_id)
{
    mpu_disable_by_index(usb_id);
}
#endif

#endif
