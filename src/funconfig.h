/*
 * funconfig.h
 *
 * ch32fun configuration for ch32_hello_world.  One file for every chip port: the
 * part is selected with -DCH32V30x / -DCH32L103, which is what the chip port's
 * port.cmake passes (the same way ch32fun's own Makefile does it).
 *
 * The clock tree belongs to the board: boards/<board>/board.cmake sets the
 * crystal and the resulting frequencies.  The values below are only fallbacks,
 * so a build without a board.cmake still comes up on that board's tree:
 *
 *     CH32V30x   HSE 24 MHz -> PLL x6  -> 144 MHz, APB1 = 72 MHz
 *     CH32L103   HSE  8 MHz -> PLL x12 ->  96 MHz, APB1 = 48 MHz
 *
 * Pins and polarity stay in boards/<board>/board_config.h.
 */
#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

/* A build without a chip port falls back to the part this project started as. */
#if !defined(CH32V30x) && !defined(CH32L103)
#define CH32V30x 1
#endif

/* ------------------------------------------------------------------ *
 * Clock tree
 *
 * Every one of these is a board property, so board.cmake wins with -D and the
 * #ifndef below is only the fallback.  Keeping the fallbacks in the same order
 * as the board file makes it obvious when the two drift apart.
 * ------------------------------------------------------------------ */
#if defined(CH32L103)
#define FUNCONF_FALLBACK_PLL_MULTIPLIER   12
#define FUNCONF_FALLBACK_CORE_CLOCK       96000000
#define FUNCONF_FALLBACK_HSE_VALUE        8000000
#else
#define FUNCONF_FALLBACK_PLL_MULTIPLIER   6
#define FUNCONF_FALLBACK_CORE_CLOCK       144000000
#define FUNCONF_FALLBACK_HSE_VALUE        24000000
#endif

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
#define FUNCONF_PLL_MULTIPLIER FUNCONF_FALLBACK_PLL_MULTIPLIER
#endif
#ifndef FUNCONF_SYSTEM_CORE_CLOCK
#define FUNCONF_SYSTEM_CORE_CLOCK FUNCONF_FALLBACK_CORE_CLOCK
#endif
#ifndef HSE_VALUE
#define HSE_VALUE FUNCONF_FALLBACK_HSE_VALUE
#endif

/* ------------------------------------------------------------------ *
 * Core / interrupts
 *
 * FUNCONF_ENABLE_HPE = 1 lets the core stack the registers on interrupt entry
 * (WCH calls it the hardware prologue) and makes INTERRUPT_DECORATOR the
 * "WCH-Interrupt-fast" form, i.e. handlers compile to a bare mret.
 *
 * Both parts support it - WCH's own startup files set INTSYSCR and their EVT
 * handlers use that attribute.  It is not an ISA extension: the CH32L103 build
 * is plain rv32imac and the attribute only suppresses the compiler's prologue,
 * so what it needs is HPE enabled at run time, not "-march=...xw".
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
