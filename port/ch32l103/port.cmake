# ==============================================================================
# port/ch32l103/port.cmake
#
# Chip-specific build settings, included by the top level CMakeLists.txt when
# CHIP_PORT selects this directory.  It is the counterpart of
# boards/<board>/board.cmake: the board owns the crystal and the pins, the chip
# port owns the core ISA, the USB controller and the memory map.
# ==============================================================================

# ------------------------------------------------------------------------------
# Core
#
# CH32L103 is plain rv32imac: its core has no "xw" extension, so it must not be
# compiled with the CH32V30x ISA string.  -DCH32L103 is what selects
# ch32l103hw.h in ch32fun (ch32fun's own Makefile passes the same define).
# ------------------------------------------------------------------------------
add_compile_definitions(CH32L103=1)
set(CHIP_MARCH "rv32imac")

# Human readable chip name for the banner and the USB strings.
set(BOOT_CHIP_NAME "CH32L103")

# ------------------------------------------------------------------------------
# USB device controller
#
# The CH32L103 carries the full-speed USBFS cell, which CherryUSB's own WCH
# port supports (port/wch/usbfs, "Support Chip List: USBFS - CH32L103"), so no
# local driver copy is needed - unlike the CH32V30x USBHS.  Full speed means no
# -DCONFIG_USB_HS, and the CDC ACM bulk endpoints stay at 64 bytes
# (config_descriptor_fs in src/usb_desc.c).
# usb_dc_low_level_init/_deinit and the vector entry are in
# boot_usb_ch32l103.c.
# ------------------------------------------------------------------------------
set(CHIP_USB_SRCS ${CHERRYUSB_ROOT}/port/wch/usbfs/usb_dc_usbfs.c)
set(CHIP_USB_INCS ${CHERRYUSB_ROOT}/port/wch/usbfs)

# ------------------------------------------------------------------------------
# Memory map
#
# Application partition: 0x00003000 (the DFU bootloader owns the first 12 KB) and
# the remaining 52 KB of the 64 KB flash, 20 KB SRAM.
# ------------------------------------------------------------------------------
set(LINKER_SCRIPT ${CHIP_PORT_DIR}/Link.ld)
