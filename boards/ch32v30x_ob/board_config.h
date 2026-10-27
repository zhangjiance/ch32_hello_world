/*
 * board_config.h - "ch32v30x_ob"
 *
 * Board Support Package for the CH32V30x onboard target board.
 *
 * Each board lives in its own, self-contained folder under boards/:
 *     boards/<board>/board_config.h   pins / polarity / features (this file)
 *     boards/<board>/board.h          board primitive interface (copy as is)
 *     boards/<board>/board.c          board primitive implementation (copy as is)
 *     boards/<board>/board.cmake      OPTIONAL extra CMake definitions
 *
 * It is the default board of every project that uses it.  Because the whole
 * folder is self-contained, copying it under boards/ of another project and
 * selecting it with -DBOARD=<board> just works.
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
 * USART3 on PB10 (TX) / PB11 (RX): the board's UART for application traffic.
 * The BMP build drives it as its USB2UART hardware; the hello-world build
 * leaves it unused - it never calls board_init_app_uart(), and its own log
 * output is its business (see main.c). */
#define BOARD_HAS_APP_UART        1
#define BOARD_APP_UART_BAUDRATE   115200
#define BOARD_APP_UART_TX_PIN     PB10
#define BOARD_APP_UART_RX_PIN     PB11

#endif /* BOARD_CONFIG_H */
