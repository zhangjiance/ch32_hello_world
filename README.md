# ch32_hello_world

[English](README.md) | [简体中文](README.zh-CN.md)

**Demo application** for the DFU bootloader.

- **LED heartbeat**
- **DFU runtime interface**: `dfu-util -e` → DFU_DETACH → reset into the bootloader
- **CDC ACM virtual serial port** (loopback)
- **Long press on the BOOT button** → reset into the bootloader (boards with a button only)

It links at `0x00008000`, the application partition of the bootloader.

---

## Layout

```
ch32_hello_world/
├── CMakeLists.txt
├── cmake/wch_riscv.cmake
├── SDK/                        # WCH peripheral library + startup files
├── third_party_components/
│   └── CherryUSB/              # git submodule: our fork, branch ch32v30x-usbhs
├── shared/boot_protocol.h      # partition/hand-shake contract with the bootloader
├── boards/
│   └── ch32v30x_ob/            # current board BSP (self-contained, copy to reuse)
│       ├── board_config.h      # BOARD_* macros: BOOT button PA6 + LED PA5
│       ├── board.h             # board primitive interface
│       └── board.c             # board primitives + the periodic tick ISR
├── port/
│   ├── boot_trigger_port.h     # hand-shake interface across the reset
│   ├── boot_usb_port.h
│   └── ch32v30x/
│       ├── boot_trigger_ch32v30x.c    # BKP trigger
│       └── boot_usb_ch32v30x.c        # USBHS RCC + usb_dc_low_level_*
│                                      # (the USBHS device driver lives in the
│                                      #  submodule, under
│                                      #  third_party_components/CherryUSB/port/wch/ch32v30x/)
└── src/
    ├── main.c                  # LED + button-to-boot + USB
    ├── usb_desc.c              # DFU runtime + CDC ACM
    ├── usb_config.h
    ├── ch32v30x_it.c
    ├── system_ch32v30x.c/.h
    ├── ch32v30x_conf.h / ch32v30x_it.h
    ├── boot_log.h
    └── Link.ld                 # 0x00008000 + 96K
```

`boards/`, `port/`, `shared/`, `cmake/` and `SDK/` follow the same structure as the
bootloader project, so both share one board/chip abstraction and one hand-shake
protocol.

---

## Build

Use the CMake presets (see `CMakePresets.json`):

```bash
git submodule update --init --recursive       # first time only

cmake --list-presets                          # list the available presets
cmake --preset ch32v30x_ob-debug              # configure
cmake --build --preset ch32v30x_ob-debug      # build
```

| preset | description | output directory |
| --- | --- | --- |
| `ch32v30x_ob-debug` | Debug (`BOOT_PRINTF` enabled) | `build/ch32v30x_ob-debug/` |
| `ch32v30x_ob-release` | Release (logging off, smaller) | `build/ch32v30x_ob-release/` |

Artifacts: `<output dir>/ch32_hello_world.elf | .hex | .bin`
(Debug ≈ 17.4 KB, Release ≈ 10.8 KB, out of the 96 KB partition).

For a new board, add a pair of presets (`<board>-debug` / `<board>-release`) the same
way, or configure directly:

```bash
cmake -S . -B build -DBOARD=<board_name>
cmake --build build -j
```

> The `riscv-wch-elf-` toolchain has to be in PATH (on this machine:
> `/opt/Toolchain/RISC-V_Embedded_GCC12`).

---

## Usage

1. Flash the DFU bootloader to `0x00000000` first (it owns the first 32 KB).
2. Update/flash this application:
   ```bash
   dfu-util -e                                                # enter the bootloader (DFU runtime)
   dfu-util -a 0 -s 0x08008000:leave -D build/ch32v30x_ob-debug/ch32_hello_world.bin
   ```
   Boards with a button can also enter the bootloader by holding BOOT.
3. After the reset the application runs: the PA5 LED blinks, USB enumerates as a
   `DFU runtime + CDC ACM` composite device, and the CDC echoes what it receives.

---

## Notes

- Button detection is **continuous**: it requires `APP_BOOT_PRESS_COUNT` consecutive
  samples (1000 ms by default) to be pressed, so contact bounce cannot trigger a jump
  into the bootloader (`src/main.c`).
- `ch32v30x_ob` has the PA6 button; a board without one sets `BOARD_HAS_BOOT_BUTTON`
  to 0 in its `board_config.h` and can then only enter the bootloader with
  `dfu-util -e`.
- **USBHS driver**: CherryUSB master's `port/wch/usbhs` does not match the CH32V30x
  `USBHSD` register map (it targets CH32V205/V4x7/CH32X305/CH58x), and using it leaves
  **USB unable to enumerate**. Upstream removed the old `port/ch32/ch32hs` without a
  replacement, so it was restored in our own fork
  **`git@github.com:zhangjiance/CherryUSB.git`**, branch **`ch32v30x-usbhs`**, under
  `port/wch/ch32v30x/`; the project uses that copy from the submodule directly. The
  core and the class drivers still come from that branch's master.
- VID/PID and the serial number are placeholders - replace them for a real product.
- For a CH32V307 (288 KB) change `BOOT_FLASH_SIZE` in `shared/boot_protocol.h` and
  `LENGTH` in `src/Link.ld`.
