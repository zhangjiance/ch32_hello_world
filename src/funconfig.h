/*
 * funconfig.h
 *
 * ch32fun configuration for ch32_hello_world.
 *
 * The clock tree belongs to the board: boards/<board>/board.cmake sets the
 * crystal and the resulting frequencies.  The values below are only fallbacks,
 * so a build without a board.cmake still comes up on the same tree:
 *
 *     HSE 24 MHz  ->  PLL x6  ->  SYSCLK = HCLK = 144 MHz, APB1 = HCLK/2 = 72 MHz
 *
 * Pins and polarity stay in boards/<board>/board_config.h.
 */
#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

#define CH32V30x 1

/* ------------------------------------------------------------------ *
 * Clock tree
 *
 * Every one of these is a board property, so board.cmake wins with -D and the
 * #ifndef below is only the fallback.  Keeping the fallbacks in the same order
 * as the board file makes it obvious when the two drift apart.
 * ------------------------------------------------------------------ */
#ifndef FUNCONF_USE_HSI
#define FUNCONF_USE_HSI 0
#endif
#ifndef FUNCONF_USE_HSE
#define FUNCONF_USE_HSE 1
#endif
#ifndef FUNCONF_USE_PLL
#define FUNCONF_USE_PLL 1
#endif
#ifndef FUNCONF_PLL_MULTIPLIER
#define FUNCONF_PLL_MULTIPLIER 6
#endif
#ifndef FUNCONF_SYSTEM_CORE_CLOCK
#define FUNCONF_SYSTEM_CORE_CLOCK 144000000
#endif
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
#ifndef FUNCONF_UART_PRINTF_BAUD
#define FUNCONF_UART_PRINTF_BAUD 115200
#endif

/* ------------------------------------------------------------------ *
 * Size knobs: a fault handler that prints costs more than it saves here.
 * ------------------------------------------------------------------ */
#define FUNCONF_DEBUG_HARDFAULT 0
#define FUNCONF_SUPPORT_CONSTRUCTORS 0
#define FUNCONF_ICACHE_EN 0

#endif /* _FUNCONFIG_H */
