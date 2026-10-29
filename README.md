# ch32_hello_world

[English](README.md) | [简体中文](README.zh-CN.md)

**Demo application** for the DFU bootloader.

- **LED heartbeat**
- **DFU runtime interface**: `dfu-util -e` → DFU_DETACH → reset into the bootloader
- **CDC ACM virtual serial port** (loopback)
- **Long press on the BOOT button** → reset into the bootloader (boards whose button
  is a GPIO only)

It links at `0x00003000`, the application partition of the bootloader.  Supported
combinations (same triples as the bootloader):

| Chip port | USB controller | Board |
| --- | --- | --- |
| `port/ch32v30x` | USBHS, high speed | `boards/ch32v30x_ob` |
| `port/ch32l103` | USBFS, full speed | `boards/ch32l103_candle` (USB-CANFD adapter) |
| `port/ch32l103` | USBFS, full speed | `boards/ch32l103_bulepill` (bluepill-shaped CH32L103C8T6) |

---

## Layout

```
ch32_hello_world/
├── CMakeLists.txt
├── cmake/riscv.cmake           # riscv-wch-elf- toolchain + flags
├── third_party_components/
│   ├── CherryUSB/              # git submodule: our fork, branch ch32v30x-usbhs
│   └── ch32fun/                # git submodule: cnlohr/ch32fun, pinned
├── shared/boot_protocol.h      # partition/hand-shake contract with the bootloader
├── boards/
│   ├── ch32v30x_ob/            # board BSP (self-contained, copy to reuse)
│   │   ├── board_config.h      # BOARD_* macros: BOOT button PA6 + LED PA5
│   │   ├── board.h             # board primitive interface
│   │   ├── board.c             # board primitives + the periodic tick ISR
│   │   └── board.cmake         # crystal + clock tree
│   ├── ch32l103_candle/        # CH32L103 USB-CANFD adapter (LED1 PA0, no GPIO button)
│   └── ch32l103_bulepill/      # CH32L103 bluepill (LED PB11 active high, BOOT is BOOT0)
├── port/
│   ├── boot_trigger_port.h     # hand-shake interface across the reset
│   ├── boot_usb_port.h         # controller base address + teardown
│   ├── ch32v30x/
│   │   ├── port.cmake                 # rv32imacxw, CONFIG_USB_HS, USBHS driver, Link.ld
│   │   ├── boot_trigger_ch32v30x.c    # BKP trigger
│   │   ├── boot_usb_ch32v30x.c        # USBHS clock + usb_dc_low_level_*
│   │   └── Link.ld                    # 0x00003000 + 116K, 32K RAM
│   │                                  # (the USBHS device driver lives in the
│   │                                  #  submodule, under
│   │                                  #  third_party_components/CherryUSB/port/wch/ch32v30x/)
│   └── ch32l103/
│       ├── port.cmake                 # rv32imac, USBFS driver, Link.ld
│       ├── boot_trigger_ch32l103.c    # BKP trigger
│       ├── boot_usb_ch32l103.c        # USBFS clock + usb_dc_low_level_* + vector
│       └── Link.ld                    # 0x00003000 + 52K, 20K RAM
│                                      # (the USBFS driver is CherryUSB's own,
│                                      #  port/wch/usbfs/)
└── src/
    ├── main.c                  # LED + button-to-boot + USB
    ├── usb_desc.c              # DFU runtime + CDC ACM
    ├── usb_config.h
    ├── fault_handlers.c        # hard fault -> reset (ch32fun defaults to spin)
    ├── funconfig.h             # ch32fun configuration (printf, size knobs)
    └── boot_log.h
```

`boards/`, `port/`, `shared/` and `cmake/` follow the same structure as the
bootloader project, so both share one board/chip abstraction and one hand-shake
protocol.

---

## Build

Use the CMake presets (see `CMakePresets.json`):

```bash
git submodule update --init --recursive       # first time only

cmake --list-presets                          # list the available presets
cmake --preset ch32l103_candle-release        # configure (or ch32v30x_ob-release)
cmake --build --preset ch32l103_candle-release
```

| preset | description | output directory |
| --- | --- | --- |
| `ch32v30x_ob-debug` | Debug (`BOOT_PRINTF` enabled) | `build/ch32v30x_ob-debug/` |
| `ch32v30x_ob-release` | Release (logging off, smaller) | `build/ch32v30x_ob-release/` |
| `ch32l103_candle-debug` | Debug (`BOOT_PRINTF` enabled) | `build/ch32l103_candle-debug/` |
| `ch32l103_candle-release` | Release (logging off, smaller) | `build/ch32l103_candle-release/` |
| `ch32l103_bulepill-debug` | Debug (`BOOT_PRINTF` enabled) | `build/ch32l103_bulepill-debug/` |
| `ch32l103_bulepill-release` | Release (logging off, smaller) | `build/ch32l103_bulepill-release/` |

Artifacts: `<output dir>/ch32_hello_world.elf | .hex | .bin`.  Measured sizes:
CH32V30x Release 9080 B / Debug 11392 B out of the 116 KB partition; CH32L103
Release 9356 B (candle) / 9348 B (bluepill), Debug 12596 B / 12588 B, out of 52 KB.

### What is in the image

[ch32fun](https://github.com/cnlohr/ch32fun) supplies the startup, the vector
table, the clock tree and `printf`; everything else is register access in
`board.c` and the chip port.  There is no vendor HAL in the build.  ch32fun is a
submodule pinned to a known-good commit, so
`git submodule update --init --recursive` fetches it; point `-DCH32FUN_ROOT=<path>`
at another checkout to override.

The WCH build was 12152 B (Release): the difference is the C library - newlib's
`printf` plus `__udivdi3` (the 64-bit divide in the time base), which the
ch32fun build replaces with its own small formatter and a 32-bit time base.

For a new board, add a pair of presets (`<board>-debug` / `<board>-release`) the same
way, or configure directly:

```bash
cmake -S . -B build -DBOARD=<board_name> -DCHIP_PORT=<chip_port>
cmake --build build -j
```

> The `riscv-wch-elf-` toolchain has to be in PATH (on this machine:
> `/opt/Toolchain/RISC-V_Embedded_GCC12`).

---

## Usage

1. Flash the DFU bootloader to `0x00000000` first (it owns the first 12 KB).
2. Update/flash this application:
   ```bash
   dfu-util -e                                                # enter the bootloader (DFU runtime)
   dfu-util -a 0 -s 0x08003000:leave -D build/ch32v30x_ob-debug/ch32_hello_world.bin
   ```
   Boards whose button is a GPIO can also enter the bootloader by holding BOOT; on
  the two L103 boards the switch is wired to BOOT0 (not readable), and the
  application/bootloader path above is the way in.
3. After the reset the application runs: the board's status LED blinks, USB
   enumerates as a `DFU runtime + CDC ACM` composite device, and the CDC echoes
   what it receives.

---

## Notes

- **Clock hand-over from the bootloader**: the bootloader leaves `SYSCLK` on
  the PLL, but ch32fun's `SystemInit()` rebuilds the tree by *assigning*
  `RCC->CTLR`, and its first write clears `PLLON` - which stops the clock the
  core is running on, so the image dies on the first instruction of `main()`
  (no LED, no USB).  `src/main.c` therefore calls `clock_fallback_to_hsi()`
  before `SystemInit()`, the same order the WCH startup used.  Any new
  application image behind this bootloader needs the same hand-over.
- **USB hand-over from the bootloader**: the host has to *see* the device go away
  before the application attaches, otherwise it keeps the bootloader's device
  instance and nothing new enumerates.  The CH32V30x bootloader does this by
  suspending the USBHS PHY and holding it for >10 ms; for the USBFS parts there is
  no PHY-suspend bit, so `src/main.c` releases D+ itself (`boot_usb_port_deinit()`,
  which also resets the SIE on the USBFS parts), waits 200 ms and only then calls
  `app_usb_init()`.
  The reverse direction (DFU_DETACH / BOOT button -> bootloader) is covered by
  `boot_trigger_reboot_to_boot()`, which tears the controller down before
  resetting: the bootloader only starts a few hundred microseconds later and the
  host has to see the disconnect first: the DFU_DETACH request is answered first
  (the teardown is deferred to the main loop), then `boot_usb_port_deinit()`
  drives D+ low for about 15 ms before resetting and the bootloader waits another
  20 ms before it attaches - the gap is well over the 10 ms the host needs, without
  slowing the re-enumeration down.
- Button detection is **continuous**: it requires `APP_BOOT_PRESS_COUNT` consecutive
  samples (1000 ms by default) to be pressed, so contact bounce cannot trigger a jump
  into the bootloader (`src/main.c`).
- `ch32v30x_ob` has the PA6 button; a board without one - or whose switch sits on
  **BOOT0**, as on both L103 boards - sets `BOARD_HAS_BOOT_BUTTON` to 0 in its
  `board_config.h` and can then only enter the bootloader with `dfu-util -e`.
- **USBHS driver**: CherryUSB master's `port/wch/usbhs` does not match the CH32V30x
  `USBHSD` register map (it targets CH32V205/V4x7/CH32X305/CH58x), and using it leaves
  **USB unable to enumerate**. Upstream removed the old `port/ch32/ch32hs` without a
  replacement, so it was restored in our own fork
  **`git@github.com:zhangjiance/CherryUSB.git`**, branch **`ch32v30x-usbhs`**, under
  `port/wch/ch32v30x/`; the project uses that copy from the submodule directly. The
  core and the class drivers still come from that branch's master.
- **USBFS driver (CH32L103)**: no `CONFIG_USB_HS`; the controller is CherryUSB's own
  `port/wch/usbfs/usb_dc_usbfs.c`, and `usb_desc.c` naturally serves the full-speed
  config descriptor (64 byte bulk endpoints) through its speed callback.  The chip
  port supplies what the driver cannot know: `RCC->CFGR0.USBPRE = PLL/2` -> 48 MHz at
  SYSCLK 96 MHz, the USBFS AHB clock, and the `USBFS_IRQHandler` vector that forwards
  to `USBD_IRQHandler(0)`.
- VID/PID and the serial number are placeholders - replace them for a real product.
- For a CH32V307 (288 KB) change `BOOT_FLASH_SIZE` in
  `boards/ch32v30x_ob/board.cmake` and `LENGTH` in `port/ch32v30x/Link.ld`.
