/*
 * boot_flash_port.h
 *
 * Chip-agnostic flash abstraction used by the DFU bootloader.  The firmware
 * only ever talks to the application partition; the implementation
 * (port/<chip>/boot_flash_<chip>.c) owns the erase/program primitives.
 *
 * Addresses are the *physical* 0x08xxxxxx aliases the flash controller is
 * driven through.
 */
#ifndef BOOT_FLASH_PORT_H
#define BOOT_FLASH_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* First address (physical) of the application partition. */
uint32_t boot_flash_app_start(void);

/* Size of the application partition in bytes. */
uint32_t boot_flash_app_size(void);

/* Granularity boot_flash_erase() operates on (one DfuSe sector). */
uint32_t boot_flash_erase_size(void);

/* Erase [addr, addr+len); len is rounded up to boot_flash_erase_size(). */
int boot_flash_erase(uint32_t addr, uint32_t len);

/* Program [addr, addr+len).  [addr] must be page aligned. */
int boot_flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

/* Read [addr, addr+len). */
int boot_flash_read(uint32_t addr, uint8_t *data, uint32_t len);

/* true when [addr, addr+len) lies entirely inside the application partition
 * (i.e. a download may never touch the bootloader). */
bool boot_flash_addr_in_app(uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_FLASH_PORT_H */
