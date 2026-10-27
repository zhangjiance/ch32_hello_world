/*
 * main.c - ch32_hello_world
 *
 * Demo application running from 0x00008000 behind the DFU bootloader:
 *
 *   - LED heartbeat
 *   - DFU runtime interface  -> `dfu-util -e` reboots into the bootloader
 *   - CDC ACM VCOM (loopback)
 *   - BOOT button held       -> reboot into the bootloader (boards that have
 *     one; on boards without a button only the detach path remains)
 */
#include "ch32fun.h"

#include "board_config.h"
#include "board.h"
#include "boot_log.h"
#include "boot_protocol.h"
#include "boot_trigger_port.h"

extern void app_usb_init(uint8_t busid, uintptr_t reg_base);

/* Heartbeat + button sampling period. */
#define APP_TICK_MS          (200U)
/* Consecutive pressed samples before we hand over (1000 ms with APP_TICK_MS). */
#define APP_BOOT_PRESS_COUNT (5U)

/*
 * Clock hand-over from the bootloader.
 *
 * ch32fun's SystemInit() rebuilds the tree by *assigning* RCC->CTLR, and its
 * first write clears PLLON.  From a hardware reset that is harmless: SYSCLK is
 * HSI, and the RM protects HSION while it is the system clock.  This
 * application, however, is entered from the bootloader with SYSCLK already on
 * the PLL, so that write stops the very clock the core is running on and the
 * chip dies on the first instruction of main() (no LED, no USB).
 *
 * Do what the WCH startup did: switch back to HSI first, then let SystemInit()
 * build the tree up again.  A software reset (NVIC_SystemReset) resets RCC, so
 * when the application starts from reset instead, this is a no-op.
 */
static void clock_fallback_to_hsi(void)
{
    RCC->CTLR |= RCC_HSION;
    while ((RCC->CTLR & RCC_HSIRDY) == 0U) {
    }

    RCC->CFGR0 &= ~RCC_SW;                          /* SW = HSI             */
    while ((RCC->CFGR0 & RCC_SWS) != 0U) {          /* wait for SWS = HSI   */
    }
}

int main(void)
{
    /* ch32fun's handle_reset() jumps straight to main(); SystemInit() is what
     * brings the clock tree (HSE x6 = 144 MHz) and the printf UART up. */
    clock_fallback_to_hsi();
    SystemInit();

    board_init();

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
        board_led_toggle();
        ++tick;

        /* BOOT button held -> request DFU mode and reset into the bootloader.
         * (board_read_boot_pin() is always false on boards without a button.) */
        if (board_read_boot_pin()) {
            if (++press >= APP_BOOT_PRESS_COUNT) {
                BOOT_PRINTF("[APP] BOOT button held, entering bootloader\r\n");
                boot_trigger_reboot_to_boot();
            }
        } else {
            press = 0;
        }

        board_delay_ms(APP_TICK_MS);
    }
}
