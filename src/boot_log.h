/*
 * boot_log.h
 *
 * Logging wrapper for the application.  Logging is only compiled in when
 * BOOT_ENABLE_LOG is defined (the Debug configuration sets it); the WCH newlib
 * printf() spins forever on an uninitialised USART, so the Release build must
 * keep it out - including the log UART bring-up itself.
 *
 * Deliberately SDK free.  usb_config.h includes this header, so pulling in
 * ch32v30x.h here would drag the WCH USBHS registers into the CherryUSB
 * translation units, where they clash with the stack's own definitions.  The
 * bring-up is therefore application code (see main.c): the board layer owns
 * pins and features, not a log UART.
 */
#ifndef BOOT_LOG_H
#define BOOT_LOG_H

#include <stdio.h>

/* UART used by the WCH Debug library printf() (USART1, PA9). */
#define BOOT_LOG_BAUDRATE 115200

#if defined(BOOT_ENABLE_LOG) && !defined(NDEBUG)
#define BOOT_PRINTF(...) printf(__VA_ARGS__)
#define BOOT_LOG_ENABLED 1
#else
#define BOOT_PRINTF(...) ((void)0)
#define BOOT_LOG_ENABLED 0
#endif

#endif /* BOOT_LOG_H */
