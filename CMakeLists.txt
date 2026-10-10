cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# ch32_hello_world - demo application for the DFU bootloader.
#
# Standalone project: LED blink + DFU runtime + CDC ACM, and a BOOT-button path
# that resets into the bootloader.
# It links at 0x00008000, the application partition of the bootloader.
# ==============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_definitions(-DCONFIG_USB_HS)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME ch32_hello_world)

# CherryUSB device stack: CDC ACM (the DFU runtime is a small custom handler).
set(CONFIG_CHERRYUSB_DEVICE 1)
set(CONFIG_CHERRYUSB_DEVICE_CDC_ACM 1)

set(CHERRYUSB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party_components/CherryUSB"
    CACHE PATH "CherryUSB submodule root")
set(SDK_ROOT       "${CMAKE_CURRENT_SOURCE_DIR}/SDK"
    CACHE PATH "WCH ch32v30x peripheral library root")

# ------------------------------------------------------------------------------
# Board / chip port selection
# ------------------------------------------------------------------------------
set(BOARD "ch32v30x_ob" CACHE STRING "Target board directory under boards/")
set(BOARD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}")

file(GLOB _board_cfgs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/boards" "${CMAKE_CURRENT_SOURCE_DIR}/boards/*/board_config.h")
set(_available_boards "")
foreach(_cfg ${_board_cfgs})
    get_filename_component(_dir "${_cfg}" DIRECTORY)
    list(APPEND _available_boards "${_dir}")
endforeach()

if(NOT EXISTS "${BOARD_DIR}/board_config.h")
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Available boards: ${_available_boards}")
endif()

# Board Support Package.  A board directory is self-contained - board_config.h
# (pins/features), board.h and board.c (primitives) - so copying one under
# boards/ and selecting it with -DBOARD=<name> is all it takes to reuse it.
if(NOT EXISTS "${BOARD_DIR}/board.c")
    message(FATAL_ERROR "Board '${BOARD}' has no board.c (expected boards/${BOARD}/board.c)")
endif()
set(BOARD_SRC "${BOARD_DIR}/board.c")

# Optional per-board compile definitions.
if(EXISTS "${BOARD_DIR}/board.cmake")
    include("${BOARD_DIR}/board.cmake")
endif()

set(CHIP_PORT "ch32v30x" CACHE STRING "Chip port directory under port/")
set(CHIP_PORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/port/${CHIP_PORT}")
if(NOT EXISTS "${CHIP_PORT_DIR}")
    message(FATAL_ERROR "Unknown CHIP_PORT '${CHIP_PORT}'")
endif()

if(NOT EXISTS ${CHERRYUSB_ROOT}/class/cdc/usbd_cdc_acm.c)
    message(FATAL_ERROR "CherryUSB submodule missing; run: git submodule update --init --recursive")
endif()

message(STATUS "Board: ${BOARD}   Chip port: ${CHIP_PORT}")
message(STATUS "  board src: ${BOARD_SRC}")

include("cmake/wch_riscv.cmake")
include("${CHERRYUSB_ROOT}/cherryusb.cmake")

# CherryUSB master has no usable CH32V30x USBHS port: port/wch/usbhs targets a
# different USBHS IP (CH32V205/V4x7/CH32X305/CH58x) with an incompatible
# register map, and CH32V30x is only covered under USBFS.  The chip port
# therefore ships the validated USBHS device driver itself
# (port/ch32v30x/usb_dc_ch32v30x.c, from CherryUSB v1.6.0's port/ch32/ch32hs).
# The core/classes still come from the submodule.

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
enable_language(C ASM)

project(${CMAKE_PROJECT_NAME})

set(PROJECT_TARGET ${CMAKE_PROJECT_NAME}.elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/Link.ld)

add_link_options(
    -T ${LINKER_SCRIPT}
    -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
)

add_executable(${PROJECT_TARGET})

# ------------------------------------------------------------------------------
# WCH ch32v30x peripheral library + startup
# ------------------------------------------------------------------------------
add_library(ch32v30x_lib STATIC)

target_include_directories(ch32v30x_lib PUBLIC
    ${SDK_ROOT}/Core
    ${SDK_ROOT}/Debug
    ${SDK_ROOT}/Peripheral/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB SDK_CORE_SRC   ${SDK_ROOT}/Core/*.c)
file(GLOB SDK_DEBUG_SRC  ${SDK_ROOT}/Debug/*.c)
file(GLOB SDK_DEVICE_SRC ${SDK_ROOT}/Peripheral/src/*.c)

target_sources(ch32v30x_lib PRIVATE
    ${SDK_CORE_SRC}
    ${SDK_DEBUG_SRC}
    ${SDK_DEVICE_SRC}
    ${SDK_ROOT}/Startup/startup_ch32v30x_D8C.S
)

# ------------------------------------------------------------------------------
# Application sources
# ------------------------------------------------------------------------------
target_sources(${PROJECT_TARGET} PRIVATE
    ${cherryusb_srcs}
    # chip port: hand-shake + USB device controller
    ${CHIP_PORT_DIR}/boot_trigger_${CHIP_PORT}.c
    ${CHIP_PORT_DIR}/boot_usb_${CHIP_PORT}.c
    # CH32V30x USBHS device controller, from the CherryUSB fork
    # (branch ch32v30x-usbhs, port/wch/ch32v30x)
    ${CHERRYUSB_ROOT}/port/wch/ch32v30x/usb_dc_ch32v30x.c
    # board (board-specific board.c, or the shared generic implementation)
    ${BOARD_SRC}
    # application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_desc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/system_ch32v30x.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ch32v30x_it.c
)

target_include_directories(${PROJECT_TARGET} PRIVATE
    ${cherryusb_incs}
    ${CHERRYUSB_ROOT}/port/wch/ch32v30x
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/port
    ${BOARD_DIR}
    ${CHIP_PORT_DIR}
)

target_compile_definitions(${PROJECT_TARGET} PRIVATE
    $<$<CONFIG:Debug>:BOOT_ENABLE_LOG=1>
)

target_link_libraries(${PROJECT_TARGET}
    ch32v30x_lib
)

add_custom_command(TARGET ${PROJECT_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.bin
)
