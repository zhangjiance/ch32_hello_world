/*
 * boot_usb_ch32v30x.c
 *
 * CH32V30x USBHS device-controller port glue:
 *   - the CherryUSB low-level hooks (usb_dc_low_level_init / _deinit)
 *   - the teardown used before jumping to the application
 *
 * Register level, so the PHY/PLL setup does not depend on a vendor library.
 */
#include "boot_usb_port.h"

#include "ch32fun.h"

void usb_dc_low_level_init(uint8_t busid)
{
    (void)busid;

    RCC->APB2PCENR |= RCC_APB2Periph_GPIOB;

    /* 48 MHz kernel clock from the USBHS PHY PLL. */
    RCC->CFGR2 &= ~(1UL << 31);
    RCC->CFGR2 |= (uint32_t)RCC_USBCLK48MCLKSource_USBPHY << 31;

    /* PLL reference: HSE, divided to the 4 MHz reference the PHY expects. */
    RCC->CFGR2 &= ~(1UL << 27);
    RCC->CFGR2 |= (uint32_t)RCC_HSBHSPLLCLKSource_HSE << 27;
    RCC->CFGR2 &= ~(3UL << 28);
    RCC->CFGR2 |= (uint32_t)RCC_USBHSPLLCKREFCLK_4M << 28;

    /* 24 MHz / 6 = 4 MHz reference -> 480 MHz PHY PLL. */
    RCC->CFGR2 &= ~(7UL << 24);
    RCC->CFGR2 |= (uint32_t)RCC_USBPLL_Div6 << 24;

    /* Power up the PHY. */
    RCC->CFGR2 |= (1UL << 30);

    RCC->AHBPCENR |= RCC_AHBPeriph_USBHS;

    NVIC_EnableIRQ(USBHS_IRQn);
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    (void)busid;
    NVIC_DisableIRQ(USBHS_IRQn);
}

uintptr_t boot_usb_reg_base(void)
{
    return (uintptr_t)USBHS_BASE;
}

void boot_usb_port_deinit(void)
{
    NVIC_DisableIRQ(USBHS_IRQn);

    /* Drop the D+ pull-up so the host sees a disconnect and re-enumerates the
     * application instead of reusing the bootloader's stale state. */
    USBHSD->CONTROL = 0;
    USBHSD->HOST_CTRL = 0x10;       /* USBHS_PHY_SUSPENDM */
    USBHSD->INT_EN = 0;
    RCC->AHBPCENR &= ~RCC_AHBPeriph_USBHS;

    /* >10 ms so the host definitely samples the disconnect: 500000 iterations
     * of the empty loop below are about 10 ms at 144 MHz (3 cycles each). */
    for (volatile uint32_t settle = 0U; settle < 500000U; ++settle) {
    }
}
