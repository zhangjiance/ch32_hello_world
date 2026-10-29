# ==============================================================================
# port/ch32v30x/port.cmake
#
# Chip-specific build settings, included by the top level CMakeLists.txt when
# CHIP_PORT selects this directory.  It is the counterpart of
# boards/<board>/board.cmake: the board owns the crystal and the pins, the chip
# port owns the core ISA, the USB controller and the memory map.
# ==============================================================================

# ------------------------------------------------------------------------------
# Core
#
# -march=rv32imacxw: the "xw" extension is what makes the WCH fast interrupt
# entry (INTERRUPT_DECORATOR with FUNCONF_ENABLE_HPE) and the hardware prologue
# painless, and every handler in this port uses that form.  -DCH32V30x is what
# selects ch32v30xhw.h in ch32fun.
# ------------------------------------------------------------------------------
add_compile_definitions(CH32V30x=1)
set(CHIP_MARCH "rv32imacxw")

# Human readable chip name for the boot banner and the USB strings.
set(BOOT_CHIP_NAME "CH32V30x")

# ------------------------------------------------------------------------------
# USB device controller
#
# The CH32V305/307 USBHS block runs in high speed, and the descriptor selected
# for the negotiated speed depends on it, so the define must match the wiring.
# The controller driver lives in our CherryUSB fork (port/wch/ch32v30x); it is
# not upstream master, see the note in the top level CMakeLists.txt.
# usb_dc_low_level_init/_deinit are in boot_usb_ch32v30x.c.
# ------------------------------------------------------------------------------
add_compile_definitions(CONFIG_USB_HS)
set(CHIP_USB_SRCS ${CHERRYUSB_ROOT}/port/wch/ch32v30x/usb_dc_ch32v30x.c)
set(CHIP_USB_INCS ${CHERRYUSB_ROOT}/port/wch/ch32v30x)

# ESIG unique ID (96 bit) behind the serial number string (user/dfu_desc.c).
add_compile_definitions(BOOT_UID_BASE=0x1FFFF7E8UL)

# ------------------------------------------------------------------------------
# Memory map
#
# 12 KB bootloader at the bottom of the flash; the RAM size belongs to the part
# (32 KB in the CH32V305 build).
# ------------------------------------------------------------------------------
set(LINKER_SCRIPT ${CHIP_PORT_DIR}/Link.ld)
