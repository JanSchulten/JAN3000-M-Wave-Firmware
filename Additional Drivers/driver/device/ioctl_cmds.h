#ifndef __IOCTL_CMDS_H__
#define __IOCTL_CMDS_H__

/**
 * \name ioctl cmd for sdio/msd/usb_storage/flash/ram_disk
 * \{
 */
#define IOCTL_FLUSH                     1
#define IOCTL_GET_STATUS                2
#define IOCTL_CMD_RESUME                3
#define IOCTL_CMD_SUSPEND               4
#define IOCTL_SET_ASYNC_MODE            5
#define IOCTL_SET_ACTIVE_STATUS         6
#define IOCTL_POWER_RESUME              7
#define IOCTL_POWER_SUSPEND             8
#define IOCTL_SET_SPEED                 9
#define IOCTL_GET_ID                    10
#define IOCTL_GET_SECTOR_SIZE           11
#define IOCTL_GET_BLOCK_SIZE            12
#define IOCTL_GET_CAPACITY              13
#define IOCTL_GET_BLOCK_NUMBER          14
#define IOCTL_GET_CLASS                 15
/* \} name */

/**
 * \name ioctl cmd for usb_storage
 * \{
 */
#define IOCTL_GET_MAX_LUN               30
#define IOCTL_GET_CUR_LUN               31
#define IOCTL_SET_CUR_LUN               32
#define IOCTL_SET_FORCE_RESET           33
#define IOCTL_CHECK_WRITE_PROTECT       34
/* \} name */

/**
 * \name ioctl cmd for flash
 * \{
 */
#define IOCTL_ERASE_SECTOR              40
#define IOCTL_ERASE_BLOCK               41
#define IOCTL_ERASE_CHIP                42
#define IOCTL_ERASE_PAGE                43
#define IOCTL_GET_CURR_ERASE_STATE      44
#define IOCTL_GET_PART_INFO             45
#define IOCTL_SFC_NORFLASH_READ_NO_ENC  46
#define IOCTL_SFC_NORFLASH_WRITE_NO_ENC 47
#define IOCTL_SET_VM_INFO               48
#define IOCTL_GET_VM_INFO               49
#define IOCTL_GET_UNIQUE_ID             50
#define IOCTL_SET_WRITE_PROTECT         51
#define IOCTL_GET_WRITE_PROTECT_VALUE   52
#define IOCTL_ENTER_4BYTE_ADDR_MODE     53
#define IOCTL_EXIT_4BYTE_ADDR_MODE      54
#define IOCTL_SET_SPI_ATTR              55
/* \} name */


#endif

