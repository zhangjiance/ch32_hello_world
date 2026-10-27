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

# ------------------------------------------------------------------------------
# Peripheral library backend
#
#   wch      WCH ch32v30x peripheral library + startup (SDK/, default)
#   ch32fun  ch32fun (https://github.com/cnlohr/ch32fun), register level, no HAL
#
# Both produce a firmware with the same behaviour: same clock tree, same pins,
# same USB device.  Only the layer under boards/ and port/ changes.
# ------------------------------------------------------------------------------
set(SDK_BACKEND "wch" CACHE STRING "Peripheral library backend: wch | ch32fun")
set_property(CACHE SDK_BACKEND PROPERTY STRINGS wch ch32fun)

set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDK"
    CACHE PATH "WCH ch32v30x peripheral library root")
set(CH32FUN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party_components/ch32fun"
    CACHE PATH "ch32fun checkout (the submodule, or a path of your own)")

if(SDK_BACKEND STREQUAL "wch")
    set(_toolchain_cmake "cmake/wch_riscv.cmake")
elseif(SDK_BACKEND STREQUAL "ch32fun")
    if(NOT EXISTS "${CH32FUN_ROOT}/ch32fun/ch32fun.c")
        message(FATAL_ERROR
            "ch32fun not found at ${CH32FUN_ROOT}.  Run "
            "git submodule update --init --recursive, or point -DCH32FUN_ROOT=... at it.")
    endif()
    set(_toolchain_cmake "cmake/ch32fun_riscv.cmake")
else()
    message(FATAL_ERROR "Unknown SDK_BACKEND '${SDK_BACKEND}' (use wch or ch32fun)")
endif()

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

# ------------------------------------------------------------------------------
# Backend-specific sources
#
# The *_ch32v30xfun.c files implement the same interfaces (boot_trigger_port.h,
# boot_usb_port.h) on top of ch32fun, so nothing above the port layer has to
# know which library is underneath.
# ------------------------------------------------------------------------------
if(SDK_BACKEND STREQUAL "ch32fun")
    set(_port_srcs
        ${CHIP_PORT_DIR}/boot_trigger_${CHIP_PORT}fun.c
        ${CHIP_PORT_DIR}/boot_usb_${CHIP_PORT}fun.c)
    set(_board_src "${BOARD_DIR}/board_ch32fun.c")
    set(_app_startup_srcs
        ${CH32FUN_ROOT}/ch32fun/ch32fun.c)
    if(NOT EXISTS "${_board_src}")
        message(FATAL_ERROR "Board '${BOARD}' has no board_ch32fun.c")
    endif()
else()
    set(_port_srcs
        ${CHIP_PORT_DIR}/boot_trigger_${CHIP_PORT}.c
        ${CHIP_PORT_DIR}/boot_usb_${CHIP_PORT}.c)
    set(_board_src ${BOARD_SRC})
    # WCH startup + system_ch32v30x.c/ch32v30x_it.c are added with the library.
    set(_app_startup_srcs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system_ch32v30x.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ch32v30x_it.c)
endif()

message(STATUS "Board: ${BOARD}   Chip port: ${CHIP_PORT}   Backend: ${SDK_BACKEND}")
message(STATUS "  board src: ${_board_src}")

# Toolchain before project() (sets the riscv-wch-elf- compilers).
include(${_toolchain_cmake})
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
#
# Only built for SDK_BACKEND=wch: the ch32fun backend needs no library at all,
# its startup/clock/printf live in ch32fun.c and everything else is register
# access in board_ch32fun.c / boot_*_ch32v30xfun.c.
# ------------------------------------------------------------------------------
if(SDK_BACKEND STREQUAL "wch")
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
endif()

# ------------------------------------------------------------------------------
# Application sources
# ------------------------------------------------------------------------------
target_sources(${PROJECT_TARGET} PRIVATE
    ${cherryusb_srcs}
    # chip port: hand-shake + USB device controller
    ${_port_srcs}
    # CH32V30x USBHS device controller, from the CherryUSB fork
    # (branch ch32v30x-usbhs, port/wch/ch32v30x).  Register level, so it is the
    # same file for both backends.
    ${CHERRYUSB_ROOT}/port/wch/ch32v30x/usb_dc_ch32v30x.c
    # board (board-specific board.c / board_ch32fun.c)
    ${_board_src}
    # application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_desc.c
    # startup/system glue for the selected backend
    ${_app_startup_srcs}
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

if(SDK_BACKEND STREQUAL "ch32fun")
    target_include_directories(${PROJECT_TARGET} PRIVATE
        ${CH32FUN_ROOT}/ch32fun
    )
    target_compile_definitions(${PROJECT_TARGET} PRIVATE BOOT_USE_CH32FUN=1)
    # ch32fun.c is one big translation unit that also carries the whole mini libc
    # (printf/snprintf/sprintf/strstr/setjmp/...), all of it as weak symbols.  In
    # a -flto build LTO keeps those weak definitions alive and merges them into a
    # single .text, where --gc-sections can no longer drop them, so ~1.3 KB of
    # code the application never calls is linked anyway.  Compiling just this file
    # without LTO gives every function its own section again.
    set_source_files_properties(${CH32FUN_ROOT}/ch32fun/ch32fun.c
        PROPERTIES COMPILE_OPTIONS "-fno-lto")
else()
    target_link_libraries(${PROJECT_TARGET} ch32v30x_lib)
endif()

add_custom_command(TARGET ${PROJECT_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.bin
)
