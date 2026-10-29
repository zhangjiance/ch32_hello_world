/*
 * boot_trigger_ch32v30x.c
 *
 * CH32V30x implementation of the cross-reset DFU request.
 *
 * CH32V30x has no general-purpose retention register in a backup domain, so a
 * battery-backed BKP data register is used.  It survives NVIC_SystemReset() and
 * is cleared by a power cycle (which is fine: after a power cycle the bootloader
 * re-evaluates the boot pin / application signature anyway).
 */
#include "boot_trigger_port.h"
#include "boot_protocol.h"
#include "boot_usb_port.h"

#include "boot_log.h"
#include "ch32fun.h"

/* BOOT_TRIGGER_BKP_DR is the WCH "BKP_DR1" style offset (BKP base + 4), i.e.
 * the first data register, which ch32fun names DATAR1 directly. */
_Static_assert(BOOT_TRIGGER_BKP_DR == 0x0004,
               "boot_trigger_ch32v30xfun.c hardcodes BKP->DATAR1: fix BOOT_TRIGGER_BKP_DR");

static void bkp_access_enable(void)
{
    RCC->APB1PCENR |= RCC_APB1Periph_PWR | RCC_APB1Periph_BKP;
    PWR->CTLR |= (1U << 8);         /* BKPWEN: backup domain write enable */
}

void boot_trigger_request(void)
{
    bkp_access_enable();
    BKP->DATAR1 = BOOT_TRIGGER_MAGIC;
}

bool boot_trigger_check_and_clear(void)
{
    bool triggered;

    bkp_access_enable();
    triggered = (BKP->DATAR1 == BOOT_TRIGGER_MAGIC);
    if (triggered) {
        BKP->DATAR1 = 0;
    }
    return triggered;
}

void boot_system_reset(void)
{
    __asm volatile("fence.i");
    NVIC_SystemReset();
    while (1) {
    }
}

void boot_trigger_reboot_to_boot(void)
{
    boot_trigger_request();
    BOOT_PRINTF("[BOOT] DFU requested, rebooting into bootloader\r\n");

    /* Release D+ before the reset: the bootloader only starts a few hundred
     * microseconds later, and the host has to see this device go away before it
     * can pick the bootloader up.  The teardown holds the line low for the
     * disconnect window, so nothing more is needed here. */
    boot_usb_port_deinit();

    boot_system_reset();
}
