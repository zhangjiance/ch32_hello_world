/*
 * main.c - ch32_hello_world
 *
 * Demo application running from 0x00008000 behind the ch32_dfu_boot
 * bootloader.  Mirrors hpm_hello_world:
 *
 *   - LED heartbeat
 *   - DFU runtime interface  -> `dfu-util -e` reboots into the bootloader
 *   - CDC ACM VCOM (loopback)
 *   - BOOT button held       -> reboot into the bootloader (boards that have
 *     one; on boards without a button only the detach path remains)
 */
#include "debug.h"

#include "board_config.h"
#include "boot_log.h"
#include "boot_board.h"
#include "boot_protocol.h"
#include "boot_trigger_port.h"

extern void app_usb_init(uint8_t busid, uintptr_t reg_base);

/* Heartbeat + button sampling period. */
#define APP_TICK_MS          (200U)
/* Consecutive pressed samples before we hand over (1000 ms with APP_TICK_MS). */
#define APP_BOOT_PRESS_COUNT (5U)

int main(void)
{
    boot_board_init();

    BOOT_PRINTF("\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("  CH32V30x Hello World (%s)\r\n", BOARD_NAME);
    BOOT_PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    BOOT_PRINTF("  LED blink + DFU runtime + CDC ACM\r\n");
    BOOT_PRINTF("  dfu-util -e  -> reboot into bootloader\r\n");
    BOOT_PRINTF("========================================\r\n\r\n");

    app_usb_init(0, USBHS_BASE);

    uint32_t tick = 0;
    uint32_t press = 0;

    while (1) {
        /* LED heartbeat */
        boot_board_led_toggle();
        ++tick;

        /* BOOT button held -> request DFU mode and reset into the bootloader.
         * (Only meaningful on boards that have a button.) */
        if (boot_board_has_boot_button() && boot_board_read_bootpin()) {
            if (++press >= APP_BOOT_PRESS_COUNT) {
                BOOT_PRINTF("[APP] BOOT button held, entering bootloader\r\n");
                boot_trigger_reboot_to_boot();
            }
        } else {
            press = 0;
        }

        Delay_Ms(APP_TICK_MS);
    }
}
