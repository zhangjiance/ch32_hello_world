# ==============================================================================
# boards/ch32l103_bulepill/board.cmake
#
# Board-specific build definitions, included by the top level CMakeLists.txt
# when BOARD selects this directory.
# ==============================================================================

# ------------------------------------------------------------------------------
# Crystal and clock tree
#
# ch32fun derives the PLL setup, every delay, every baud rate and the USB clock
# from these numbers, and they describe the hardware fitted on the board - which
# is why they live here and not in funconfig.h (that file only keeps the ch32fun
# behaviour switches and repeats these values as fallbacks).
#
#     HSE 8 MHz  ->  PLL x12  ->  SYSCLK = HCLK = 96 MHz, APB1 = HCLK/2 = 48 MHz
#
# The bluepill board carries an 8 MHz crystal on OSC_IN/OSC_OUT (PD0/PD1) and a
# 32.768 kHz one on PC14/PC15 (unused here).  96 MHz is the CH32L103 maximum and
# ch32fun picks the matching flash latency (FLASH_ACTLR_LATENCY_2 above 72 MHz).
# The USBFS clock is PLL / 2 = 48 MHz, which is exactly what the SDK's
# USBFS_RCC_Init() selects for a 96 MHz core.
# ------------------------------------------------------------------------------
add_compile_definitions(
    HSE_VALUE=8000000
    FUNCONF_PLL_MULTIPLIER=12
    FUNCONF_SYSTEM_CORE_CLOCK=96000000
)

# ------------------------------------------------------------------------------
# Flash size
#
# The socket takes the 48-pin STM32F103C8/AIR32F103CBT6 footprint; the part this
# port targets is a CH32L103C8T6, 64 KB of code flash.  The bootloader owns the
# first 12 KB, so the application partition is 52 KB (13 whole DfuSe sectors) and
# the layout string the device reports is "@Internal Flash /0x08003000/13*004Kg".
# ------------------------------------------------------------------------------
add_compile_definitions(BOOT_FLASH_SIZE=65536)
