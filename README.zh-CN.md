# ch32_hello_world

[English](README.md) | [简体中文](README.zh-CN.md)

DFU 引导程序配套的**演示 APP**。

- **LED 心跳**
- **DFU runtime 接口**：`dfu-util -e` → DFU_DETACH → 复位进入 bootloader
- **CDC ACM 虚拟串口**（回环）
- **BOOT 按键长按** → 复位进入 bootloader（仅带按键的 board）

链接地址 `0x00008000`，即引导程序的应用分区。

---

## 目录结构

```
ch32_hello_world/
├── CMakeLists.txt
├── cmake/wch_riscv.cmake
├── SDK/                        # WCH 外设库 + 启动文件
├── third_party_components/
│   └── CherryUSB/              # git 子仓：自己的 fork，分支 ch32v30x-usbhs
├── shared/boot_protocol.h      # 与 bootloader 的分区/握手契约
├── boards/
│   ├── boot_board.h/.c         # 板级通用实现（board 未自带 board.c 时使用）
│   └── ch32v30x_ob/            # 当前板 BSP
│       └── board_config.h      # BOOT 按键 PA6 + LED PA5
├── port/
│   ├── boot_trigger_port.h     # 跨复位握手接口
│   ├── boot_usb_port.h
│   └── ch32v30x/
│       ├── boot_trigger_ch32v30x.c    # BKP 触发
│       └── boot_usb_ch32v30x.c        # USBHS RCC + usb_dc_low_level_*
│                                      # （USBHS 设备驱动在子仓
│                                      #   third_party_components/CherryUSB/port/wch/ch32v30x/）
└── src/
    ├── main.c                  # 点灯 + 按键进 boot + USB
    ├── usb_desc.c              # DFU runtime + CDC ACM
    ├── usb_config.h
    ├── ch32v30x_it.c
    ├── system_ch32v30x.c/.h
    ├── ch32v30x_conf.h / ch32v30x_it.h
    ├── boot_log.h
    └── Link.ld                 # 0x00008000 + 96K
```

`boards/`、`port/`、`shared/`、`cmake/`、`SDK/` 与引导程序工程保持同构，
两边共用同一套 board/chip 抽象与握手协议。

---

## 编译

使用 CMake preset（见 `CMakePresets.json`）：

```bash
git submodule update --init --recursive       # 首次

cmake --list-presets                          # 列出可用 preset
cmake --preset ch32v30x_ob-debug              # 配置
cmake --build --preset ch32v30x_ob-debug      # 编译
```

| preset | 说明 | 输出目录 |
| --- | --- | --- |
| `ch32v30x_ob-debug` | Debug（`BOOT_PRINTF` 打开） | `build/ch32v30x_ob-debug/` |
| `ch32v30x_ob-release` | Release（日志关闭，体积更小） | `build/ch32v30x_ob-release/` |

产物：`<输出目录>/ch32_hello_world.elf | .hex | .bin`
（Debug ≈ 17.4 KB，Release ≈ 10.8 KB，分区 96 KB）。

新增 board 后照葫芦画瓢加一对 preset（`<board>-debug` / `<board>-release`），
也可以不用 preset 直接：

```bash
cmake -S . -B build -DBOARD=<board_name>
cmake --build build -j
```

> 需要 `riscv-wch-elf-` 工具链在 PATH 中（本机 `/opt/Toolchain/RISC-V_Embedded_GCC12`）。

---

## 使用

1. 先烧配套的 DFU 引导程序到 `0x00000000`（它占前 32 KB）。
2. 升级/烧录本 APP：
   ```bash
   dfu-util -e                                                # 进 bootloader（DFU runtime）
   dfu-util -a 0 -s 0x08008000:leave -D build/ch32v30x_ob-debug/ch32_hello_world.bin
   ```
   带按键的 board 也可以**长按 BOOT 键**进入 bootloader。
3. 复位后运行 APP：PA5 LED 心跳闪烁，USB 枚举为
   `DFU runtime + CDC ACM` 复合设备，CDC 回显收到的数据。

---

## 说明

- 按键判定为**持续性**：连续 `APP_BOOT_PRESS_COUNT` 次采样（默认 1000 ms）
  都按下才跳转，避免抖动误触发（`src/main.c`）。
- `ch32v30x_ob` 带 PA6 按键；后续若新增无按键板，把该板 `board_config.h` 的
  `BOARD_HAS_BOOT_BUTTON` 置 0 即可，只能通过 `dfu-util -e` 进入 bootloader。
- **USBHS 驱动**：CherryUSB master 的 `port/wch/usbhs` 与 CH32V30x 的 `USBHSD` 寄存器映射
  不兼容（面向 CH32V205/V4x7/CH32X305/CH58x），直接用会导致 **USB 无法枚举**。
  上游删除了老的 `port/ch32/ch32hs` 且无替代，我们已恢复到自己的 fork
  **`git@github.com:zhangjiance/CherryUSB.git`** 分支 **`ch32v30x-usbhs`** 的
  `port/wch/ch32v30x/`，工程直接引用子仓里的这一份。核心/类仍用该分支 master。
- VID/PID、串口为占位值，正式产品请替换。
- 若为 CH32V307（288 KB），改 `shared/boot_protocol.h` 的 `BOOT_FLASH_SIZE`
  与 `src/Link.ld` 的 `LENGTH`。
