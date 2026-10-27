/*
 * funconfig.h
 *
 * ch32fun configuration for ch32_hello_world.
 *
 * The clock tree is the board's crystal times the PLL; it is kept here (rather
 * than in the board folder) because ch32fun reads it from this file:
 *
 *     HSE 24 MHz  ->  PLL x6  ->  SYSCLK = HCLK = 144 MHz, APB1 = HCLK/2 = 72 MHz
 *
 * A board with a different crystal has to change the three clock values below.
 * Pins and polarity stay in boards/<board>/board_config.h.
 */
#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

#define CH32V30x 1

/* ------------------------------------------------------------------ *
 * Clock tree
 * ------------------------------------------------------------------ */
#define FUNCONF_USE_HSI 0
#define FUNCONF_USE_HSE 1
#define FUNCONF_USE_PLL 1
#define FUNCONF_PLL_MULTIPLIER 6
#define FUNCONF_SYSTEM_CORE_CLOCK 144000000

#ifndef HSE_VALUE
#define HSE_VALUE 24000000
#endif

/* ------------------------------------------------------------------ *
 * Core / interrupts
 *
 * The image is built with -march=rv32imacxw, so handlers may use the WCH
 * fast-entry form (hardware prologue + epilogue).  ch32fun enables that in
 * handle_reset() and exposes it as INTERRUPT_DECORATOR.
 * ------------------------------------------------------------------ */
#define FUNCONF_ENABLE_HPE 1

/* SysTick counts HCLK cycles; it is the board time base (board_time_ms()). */
#define FUNCONF_SYSTICK_USE_HCLK 1

/* ------------------------------------------------------------------ *
 * BOOT_PRINTF transport: USART1 (PA9), 115200 8N1.  Debug builds only; the
 * Release build compiles the logging (and the formatter behind it) out.
 * ------------------------------------------------------------------ */
#define FUNCONF_USE_DEBUGPRINTF 0
#define FUNCONF_USE_UARTPRINTF 1
#define FUNCONF_UART_PRINTF_BAUD 115200

/* ------------------------------------------------------------------ *
 * Size knobs: a fault handler that prints costs more than it saves here.
 * ------------------------------------------------------------------ */
#define FUNCONF_DEBUG_HARDFAULT 0
#define FUNCONF_SUPPORT_CONSTRUCTORS 0
#define FUNCONF_ICACHE_EN 0

#endif /* _FUNCONFIG_H */
