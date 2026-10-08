/*
 * boot_log.h
 *
 * Logging wrapper for the DFU bootloader.  Logging is only compiled in when
 * BOOT_ENABLE_LOG is defined (the Debug configuration sets it); the WCH newlib
 * printf() spins forever on an uninitialised USART, so the Release build must
 * keep it out.
 */
#ifndef BOOT_LOG_H
#define BOOT_LOG_H

#include <stdio.h>

#if defined(BOOT_ENABLE_LOG) && !defined(NDEBUG)
#define BOOT_PRINTF(...) printf(__VA_ARGS__)
#else
#define BOOT_PRINTF(...) ((void)0)
#endif

#endif /* BOOT_LOG_H */
