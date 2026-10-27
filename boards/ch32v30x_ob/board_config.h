/*
 * board_config.h - "ch32v30x_ob"
 *
 * Board Support Package for the CH32V30x onboard target board.
 *
 * Each board lives in its own, self-contained folder under boards/:
 *     boards/<board>/board_config.h   pins / polarity / features (this file)
 *     boards/<board>/board.h          board primitive interface (copy as is)
 *     boards/<board>/board.c          board primitive implementation (copy as is)
 *     boards/<board>/board.cmake      crystal + clock tree, optional extras
 *
 * Adding a board should be nothing more than copying this folder, editing this
 * file and the crystal in board.cmake, and selecting it with -DBOARD=<board>:
 * no file outside boards/ has to change.
 *
 * Pins are spelled the way ch32fun addresses them, port * 16 + number
 * (ch32v30xhw.h: PA5, PB10, ...); board.c turns that into the CFGLR/BSHR
 * writes.
 *
 *   BOOT button : PA6 to GND, internal pull-up, active low
 *   status LED  : PA5, active low
 *   app UART    : USART3, PB10 (TX) / PB11 (RX)
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_NAME              "ch32v30x_ob"

/* ---- BOOT button: PA6 to GND, pull-up, active low ----
 * Set BOARD_HAS_BOOT_BUTTON to 0 on a board without a button: DFU is then
 * entered only through the application detach path (dfu-util -e). */
#define BOARD_HAS_BOOT_BUTTON   1
#define BOARD_BOOT_PIN          PA6
#define BOARD_BOOT_ACTIVE_LOW   1

/* ---- status LED: PA5 (plain GPIO, no JTAG remap needed) ----
 * BOARD_LED_ACTIVE_LOW: 1 = LED on when the pin is low. */
#define BOARD_HAS_LED           1
#define BOARD_LED_PIN           PA5
#define BOARD_LED_ACTIVE_LOW    1

/* ---- application UART ----
 * The board's UART for application traffic; board_init_app_uart() sets up the
 * instance and the two pins.  The images shipped in this project never call
 * it, so it only has to be right for boards that do. */
#define BOARD_HAS_APP_UART        1
#define BOARD_APP_UART            USART3
#define BOARD_APP_UART_RCC        RCC_APB1Periph_USART3
#define BOARD_APP_UART_BAUDRATE   115200
#define BOARD_APP_UART_TX_PIN     PB10
#define BOARD_APP_UART_RX_PIN     PB11

/* ---- GPIO port clocks this board needs ----
 * RCC->APB2PCENR bits, one per port named above. */
#define BOARD_GPIO_RCC  (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB)

/* ---- periodic tick ----
 * BOARD_TICK_TIM_ISR has to be the handler name ch32fun's vector table uses
 * for that timer, otherwise the tick interrupt never arrives (TIM3_IRQHandler,
 * TIM2_IRQHandler, ... are all in it). */
#define BOARD_TICK_TIM        TIM3
#define BOARD_TICK_TIM_RCC    RCC_APB1Periph_TIM3
#define BOARD_TICK_TIM_IRQ    TIM3_IRQn
#define BOARD_TICK_TIM_ISR    TIM3_IRQHandler

#endif /* BOARD_CONFIG_H */
