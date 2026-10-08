/*
 * boot_board.c
 *
 * Generic board bring-up.  Everything hardware specific comes from the board
 * config header selected by -DBOARD=<name> (see boards/boot_board.h).
 */
#include "boot_board.h"
#include "board_config.h"

#include "boot_log.h"
#include "boot_usb_port.h"

#include "debug.h"      /* ch32v30x.h -> GPIO / RCC / AFIO */

void boot_board_init(void)
{
    SystemCoreClockUpdate();
    Delay_Init();

#ifdef BOOT_ENABLE_LOG
    USART_Printf_Init(115200);
#endif

#if BOARD_HAS_BOOT_BUTTON
    RCC_APB2PeriphClockCmd(BOARD_BOOT_RCC, ENABLE);
    {
        GPIO_InitTypeDef gpio = { 0 };

        gpio.GPIO_Pin   = BOARD_BOOT_PIN;
        /* Released level must be the "boot the application" level. */
        gpio.GPIO_Mode  = BOARD_BOOT_ACTIVE_LOW ? GPIO_Mode_IPU : GPIO_Mode_IPD;
        gpio.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(BOARD_BOOT_GPIO, &gpio);
    }
#endif

#if BOARD_HAS_LED
#if BOARD_LED_NEEDS_JTAG_DISABLE
    /* e.g. PB4 is JTDO by default: disable JTAG-DP but keep SW-DP alive so the
     * pin becomes GPIO while the debug link stays usable. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    AFIO->PCFR1 = (AFIO->PCFR1 & 0xF8FFFFFFu) | 0x02000000u;
#endif
    RCC_APB2PeriphClockCmd(BOARD_LED_RCC, ENABLE);
    {
        GPIO_InitTypeDef gpio = { 0 };

        gpio.GPIO_Pin   = BOARD_LED_PIN;
        gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
        gpio.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(BOARD_LED_GPIO, &gpio);
    }
#endif
}

void boot_board_deinit(void)
{
    boot_usb_port_deinit();
}

bool boot_board_has_boot_button(void)
{
#if BOARD_HAS_BOOT_BUTTON
    return true;
#else
    return false;
#endif
}

bool boot_board_read_bootpin(void)
{
#if BOARD_HAS_BOOT_BUTTON
    BitAction level = GPIO_ReadInputDataBit(BOARD_BOOT_GPIO, BOARD_BOOT_PIN);

    return BOARD_BOOT_ACTIVE_LOW ? (level == Bit_RESET) : (level == Bit_SET);
#else
    return false;
#endif
}

void boot_board_led_toggle(void)
{
#if BOARD_HAS_LED
    static uint8_t state;
    uint8_t on;

    state ^= 1u;
    on = state;
#if BOARD_LED_ACTIVE_LOW
    on = (uint8_t)!on;
#endif
    GPIO_WriteBit(BOARD_LED_GPIO, BOARD_LED_PIN, on ? Bit_SET : Bit_RESET);
#endif
}
