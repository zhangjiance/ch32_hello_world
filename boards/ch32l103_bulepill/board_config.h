/*
 * board_config.h - "ch32l103_bulepill"
 *
 * Board Support Package for the CH32L103C8T6 "bluepill" board: a 48-pin LQFP
 * socket wired exactly like an STM32F103C8T6 / AIR32F103CBT6 bluepill (the
 * schematic's part is the AIR32 footprint), populated with a CH32L103C8T6.  The
 * pin numbering matches, so the classic bluepill headers apply unchanged:
 *
 *   USB-C       : D- = PA11, D+ = PA12, i.e. the CH32L103's dedicated USBFS
 *                 pins (the socket's UDM/UDP), no GPIO setup needed.
 *                 CC1/CC2 have 5.1k pull-downs, SBU1/SBU2 go to the USB PD
 *                 path, ESD array on both data lines.
 *   LED1 (D4)   : PB11 -> series resistor -> LED -> GND
 *   LED2 (D5)   : PB10   (same circuit)
 *   LED3 (D6)   : PB2    (same circuit)
 *   BOOT switch : SW1 pulls BOOT0 (pin 44) to 3V3, 10k pull-down holds it low
 *   RST switch  : SW2 on NRST, 10k pull-up + 1 uF
 *   HSE         : 8 MHz crystal on OSC_IN/OSC_OUT (PD0/PD1)
 *   LSE         : 32.768 kHz crystal on PC14/PC15
 *   SWD         : DAPLINK header (debug only)
 *
 * The LEDs hang between pin and GND (pin -> resistor -> LED -> ground), so they
 * are driven HIGH to light up - the opposite of the usual bluepill PC13 LED.
 *
 * DO NOT EXPECT A BOOT BUTTON PRIMITIVE HERE.  SW1 drives BOOT0, pin 44, which
 * on this part is a dedicated boot-mode input, not a GPIO: the chip samples it
 * in hardware reset and either starts the flash image (BOOT0 low) or WCH's
 * system-memory ISP loader (BOOT0 high).  It cannot be read back by firmware,
 * so this bootloader cannot use it as a "stay in DFU" request.  Holding the
 * switch while resetting therefore enters the WCH USB ISP loader - which is how
 * the bootloader itself gets flashed the first time - and DFU mode here is
 * entered from the application (dfu-util -e -> BKP marker + reset) or
 * automatically when no valid application is found.
 *
 * Pins are spelled the way ch32fun addresses them, port * 16 + number
 * (ch32l103hw.h: PB11, PA11, ...); board.c turns that into the CFGLR/BSHR
 * writes.
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_NAME              "ch32l103_bulepill"

/* ---- BOOT button ----
 * None that firmware can read: SW1 is wired to BOOT0 (see above). */
#define BOARD_HAS_BOOT_BUTTON   0

/* ---- LEDs ----
 * Driven high to light up (see the schematic note above).
 *   BOARD_LED_*  is the status LED the bootloader toggles per flash block.
 *   LED2/LED3    are the other two GPIO LEDs of the board; they are described
 *                here because board_config.h is the board's fact sheet, but
 *                nothing in this project drives them. */
#define BOARD_HAS_LED           1
#define BOARD_LED_PIN           PB11
#define BOARD_LED_ACTIVE_LOW    0

#define BOARD_HAS_LED2          1
#define BOARD_LED2_PIN          PB10

#define BOARD_HAS_LED3          1
#define BOARD_LED3_PIN          PB2

/* ---- CAN transceiver termination: none on this board ----
 * (kept explicit so a copied board file cannot inherit another board's pin) */
#define BOARD_HAS_TERMINATION   0

/* ---- application UART ----
 * No dedicated one.  USART1 TX/RX (PA9/PA10) are on the headers, but that is
 * the instance BOOT_PRINTF uses (see user/funconfig.h), so it is left to the
 * debug log; USART2 (PA2/PA3) and USART3 (PB10/PB11) are on the headers too and
 * belong to the application if it wants them. */
#define BOARD_HAS_APP_UART      0

/* ---- GPIO port clocks this board needs ----
 * RCC->APB2PCENR bits, one per port named above (PB11/PB10/PB2).
 * PA11/PA12 need no clock setup: the USBFS takes them over itself. */
#define BOARD_GPIO_RCC  (RCC_APB2Periph_GPIOB)

/* ---- periodic tick ----
 * BOARD_TICK_TIM_ISR has to be the handler name ch32fun's vector table uses
 * for that timer, otherwise the tick interrupt never arrives (TIM3_IRQHandler,
 * TIM2_IRQHandler, ... are all in it). */
#define BOARD_TICK_TIM        TIM3
#define BOARD_TICK_TIM_RCC    RCC_APB1Periph_TIM3
#define BOARD_TICK_TIM_IRQ    TIM3_IRQn
#define BOARD_TICK_TIM_ISR    TIM3_IRQHandler

#endif /* BOARD_CONFIG_H */
