# ==============================================================================
# Toolchain and flags for the CH32 DFU demo application (ch32fun based)
#
# There is no vendor startup file and no vendor library: ch32fun.c brings
# handle_reset() and the vector table (in .init, at address 0), which is why the
# entry point is InterruptVector rather than a _start stub.
# ==============================================================================
set(CMAKE_SYSTEM_NAME               Generic)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# riscv-wch-elf- must be part of path environment
set(TOOLCHAIN_PREFIX                riscv-wch-elf-)

set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER              ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_AR                        ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_DEBUG "-Os -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -flto -g0")

add_compile_options(
        -fmessage-length=0
        -fsigned-char
        -ffunction-sections
        -fdata-sections
        -fno-common
        -fno-ident
        -Wunused
        -Wuninitialized
)

# The ISA string is a chip property and comes from the chip port
# (CHIP_MARCH in port/<chip>/port.cmake):
#   CH32V30x  -> rv32imacxw  (the "xw" extension is the WCH fast-entry ISA
#                             every handler in that port is written for)
#   CH32L103  -> rv32imac    (the L103 core has no "xw"; its handlers use the
#                             WCH compiler's interrupt attribute, which does not
#                             need the extension, only HPE enabled at run time)
if(NOT DEFINED CHIP_MARCH)
    set(CHIP_MARCH "rv32imacxw")
endif()

add_compile_options(-march=${CHIP_MARCH} -mabi=ilp32)

add_link_options(
        -march=${CHIP_MARCH} -mabi=ilp32
        -msmall-data-limit=8 -msave-restore
        -nostartfiles
        -Xlinker
        --gc-sections
        -lm
        -fno-ident
        -Wl,--print-memory-usage
        --specs=nano.specs --specs=nosys.specs
)
