/*
 * boot_log.h
 *
 * Logging wrapper for the application.  Logging is only compiled in when
 * BOOT_ENABLE_LOG is defined (the Debug configuration sets it); the Release
 * build keeps it out, which also keeps the formatter out of the image.
 *
 * Deliberately header free.  usb_config.h includes this file, so pulling in
 * ch32fun.h here would drag its USBHS registers into the CherryUSB
 * translation units, where they clash with the stack's own definitions.
 */
#ifndef BOOT_LOG_H
#define BOOT_LOG_H

/* ch32fun.c provides printf() itself - its own formatter, no newlib, no
 * malloc - and SystemInit() has already brought USART1 up at
 * FUNCONF_UART_PRINTF_BAUD.  Declaring it here keeps the header free of
 * ch32fun.h, which usb_config.h relies on. */
int printf(const char *fmt, ...);

#if defined(BOOT_ENABLE_LOG) && !defined(NDEBUG)
#define BOOT_PRINTF(...) printf(__VA_ARGS__)
#define BOOT_LOG_ENABLED 1
#else
#define BOOT_PRINTF(...) ((void)0)
#define BOOT_LOG_ENABLED 0
#endif

#endif /* BOOT_LOG_H */
