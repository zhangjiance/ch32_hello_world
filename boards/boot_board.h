/*
 * boot_board.h
 *
 * Board-level interface.  The implementation (boards/boot_board.c) is generic
 * and is parameterised by ONE board config header selected at build time:
 *
 *     -DBOARD=ch32v30x_key   ->  boards/ch32v30x_key/board_config.h
 *
 * Adding a new board therefore only means adding a board_config.h; the
 * firmware itself does not change.
 */
#ifndef BOOT_BOARD_H
#define BOOT_BOARD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System clock + console + boot button + LED bring-up. */
void boot_board_init(void);

/* Shut down what the bootloader owns before handing over to the application. */
void boot_board_deinit(void);

/* true when this board has a BOOT button wired up. */
bool boot_board_has_boot_button(void);

/* true => stay in the bootloader (button pressed).  Always false without a
 * button, in which case DFU is entered through the application detach path. */
bool boot_board_read_bootpin(void);

/* Status LED feedback during erase/program (no-op when the board has none). */
void boot_board_led_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_BOARD_H */
