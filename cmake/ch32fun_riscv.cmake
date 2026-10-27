# ==============================================================================
# ch32fun backend toolchain/flags (selected with -DSDK_BACKEND=ch32fun)
#
# Same compiler, same optimisations and the same linker script as the WCH SDK
# backend, so a size comparison between the two isolates the SDK itself.  The
# only differences are the ones ch32fun asks for:
#   - entry point is InterruptVector (ch32fun's vector table lives in .init)
#   - no WCH startup .S / system_ch32v30x.c: handle_reset() is in ch32fun.c
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

# -march=rv32imacxw: the "xw" extension is what makes the WCH fast interrupt
# entry (INTERRUPT_DECORATOR with FUNCONF_ENABLE_HPE) and the hardware
# prologue painless - the same ISA the WCH SDK build uses.
add_compile_options(-march=rv32imacxw -mabi=ilp32)

add_link_options(
        -march=rv32imacxw -mabi=ilp32
        -msmall-data-limit=8 -msave-restore
        -nostartfiles
        -Xlinker
        --gc-sections
        -lm
        -fno-ident
        -Wl,--print-memory-usage
        # ch32fun has no _start: the vector table at address 0 is the entry,
        # and user/Link.ld keeps ENTRY(_start) for the WCH SDK backend.
        -Wl,-e,InterruptVector
        --specs=nano.specs --specs=nosys.specs
)
