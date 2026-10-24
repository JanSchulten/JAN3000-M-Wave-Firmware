#include "system/includes.h"
#include "usb/device/usb_stack.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb_config.h"
#include "device/device.h"
#include "app_config.h"
#include "fs/fs.h"

/*
使用方法
1、在app_config.h使用USB从机的MASSSTORAGE_CLASS功能
2、在版籍board_xxx.c声明:extern const struct device_operations cdrom_dev_ops;
3、在版籍board_xxx.c添加
在REGISTER_DEVICES(device_table) = {
	//数组中添加
    {"cdrom",  &cdrom_dev_ops, NULL},
*/

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_MSD_CDROM_ENABLE

#define BLOCK_SIZE 512u

struct msd_cdrom_handle {
    volatile u8 open;
    u32 cdrom_file_size;
    FILE *cdrom_file;
    struct device cdrom_dev;
};

static struct msd_cdrom_handle _msd_cdrom_handle;
#define __this      (&_msd_cdrom_handle)

static int cdrom_dev_init(const struct dev_node *node, void *arg)
{
    memset(__this, 0, sizeof(struct msd_cdrom_handle));

    return 0;
}

static int cdrom_dev_open(const char *name, struct device **device, void *arg)
{
    /* printf("cdrom_dev open =%s\n",name ); */
    const char *cdrom_file_path = SDFILE_RES_ROOT_PATH"audlogo/cdrom.iso";
    __this->cdrom_file = fopen(cdrom_file_path, "r");
    if (!__this->cdrom_file) {
        return -1;
    }

    __this->cdrom_file_size = flen(__this->cdrom_file);
    log_info("cdrom_dev open sucess! %s %d", cdrom_file_path, __this->cdrom_file_size);
    *device = (struct device *)&__this->cdrom_dev;
    (*device)->private_data = NULL;

    return 0;
}

static int cdrom_dev_close(struct device *device)
{
    if (__this->cdrom_file) {
        fclose(__this->cdrom_file);
        __this->cdrom_file = NULL;
    }
    return 0;
}

static int cdrom_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    log_debug("cdrom read =%d %d", len, offset);

    if (__this->cdrom_file) {
        fseek(__this->cdrom_file, offset * BLOCK_SIZE, SEEK_SET);
        int rlen = fread(buf, len * BLOCK_SIZE, 1, __this->cdrom_file);
        if (rlen != len * BLOCK_SIZE) {
            log_error("cdrom read err %d", rlen);
            return 0;
        }
    }

    return len;
}

static int cdrom_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    return len;
}

static int cdrom_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    switch (cmd) {
    case IOCTL_GET_ID:
        *((int *)arg) = 0;
        break;
    case IOCTL_GET_BLOCK_NUMBER:
        break;
    case IOCTL_GET_SECTOR_SIZE:
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *((int *)arg) = BLOCK_SIZE;
        break;
    case IOCTL_GET_CAPACITY:
        if (__this->cdrom_file) {
            *((int *)arg) = __this->cdrom_file_size;
        }
        break;
    case  IOCTL_GET_STATUS:
        *((int *)arg) = 1;
        break;
    default:
        break;
    }

    return 0;
}

static bool cdrom_dev_online(const struct dev_node *node)
{
    return true;
}

const struct device_operations cdrom_dev_ops = {
    .init  = cdrom_dev_init,
    .open  = cdrom_dev_open,
    .online = cdrom_dev_online,
    .read  = cdrom_dev_read,
    .write = cdrom_dev_write,
    .ioctl = cdrom_dev_ioctl,
    .close = cdrom_dev_close,
};

#endif

