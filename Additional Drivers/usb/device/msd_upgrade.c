#include "system/includes.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb_config.h"
#include "app_config.h"
#include "cpu.h"
#include "asm/debug.h"

#define ISP_TOOL_CMD                    0xFA
#define WRITE_FLASH                     0xFB
#define READ_FLASH                      0xFD
#define OTHER_CMD                       0xFC

#define TCFG_MSD_BULK_TEST_EN           0    //msd读写测试(搭配上位机)

typedef enum {
    UPGRADE_NULL = 0,
    UPGRADE_USB_HARD_KEY,
    UPGRADE_USB_SOFT_KEY,
    UPGRADE_UART_KEY,
} UPGRADE_STATE;

/*extern void nvram_set_boot_state(u32 state);*/
/*extern void hw_mmu_disable(void);*/
/*extern void ram_protect_close(void);*/

void __attribute__((weak)) nvram_set_boot_state(u32 state) {}
void __attribute__((weak)) hw_mmu_disable(void) {}
void __attribute__((weak)) ram_protect_close(void) {}

AT(.volatile_ram_code)
void go_mask_usb_updata()
{
    CPU_CRITICAL_ENTER();
    ram_protect_close();
    hw_mmu_disable();
    nvram_set_boot_state(UPGRADE_USB_SOFT_KEY);

    JL_CLOCK->PWR_CON |= (1 << 4);
    /* chip_reset(); */
    /* cpu_reset(); */
    while (1);
}

#if TCFG_PC_UPDATE
__attribute__((weak)) u32 isp_tool_parser(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
    printf("isp tools parse null\n");
    return FALSE;
}

u32 _usb_bulk_rw_test(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw);

u32 private_scsi_cmd(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
#if TCFG_MSD_BULK_TEST_EN
    if (_usb_bulk_rw_test(usb_device, cbw)) {
        return TRUE;
    }
#endif

    switch (cbw->operationCode) {
//////////////////////Boot Loader Custom CMD
    case WRITE_FLASH:
    case READ_FLASH:
    case OTHER_CMD:
        printf("goto mask pc mode\n");
        go_mask_usb_updata();
        break;
    case ISP_TOOL_CMD:
        return isp_tool_parser(usb_device, cbw);
    default:
        return FALSE;
    }

    return TRUE;
}
#else
u32 private_scsi_cmd(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
    return FALSE;
}
#endif

