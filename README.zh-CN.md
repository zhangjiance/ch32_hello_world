# ch32_hello_world

[English](README.md) | [简体中文](README.zh-CN.md)

DFU 引导程序配套的**演示 APP**。

- **LED 心跳**
- **DFU runtime 接口**：`dfu-util -e` → DFU_DETACH → 复位进入 bootloader
- **CDC ACM 虚拟串口**（回环）
- **BOOT 按键长按** → 复位进入 bootloader（仅按键接在 GPIO 上的 board）

链接地址 `0x00003000`，即引导程序的应用分区。支持的芯片/板卡组合与引导程序一致：

| 芯片端口 | USB 控制器 | 板卡 |
| --- | --- | --- |
| `port/ch32v30x` | USBHS，高速 | `boards/ch32v30x_ob` |
| `port/ch32l103` | USBFS，全速 | `boards/ch32l103_candle`（USB-CANFD 适配板） |
| `port/ch32l103` | USBFS，全速 | `boards/ch32l103_bulepill`（bluepill 形态 CH32L103C8T6） |

---

## 目录结构

```
ch32_hello_world/
├── CMakeLists.txt
├── cmake/riscv.cmake           # riscv-wch-elf- 工具链与编译选项
├── third_party_components/
│   ├── CherryUSB/              # git 子仓：自己的 fork，分支 ch32v30x-usbhs
│   └── ch32fun/                # git 子仓：cnlohr/ch32fun，已固定提交
├── shared/boot_protocol.h      # 与 bootloader 的分区/握手契约
├── boards/
│   ├── ch32v30x_ob/            # 板 BSP（自包含，拷贝即复用）
│   │   ├── board_config.h      # BOARD_* 宏：BOOT 按键 PA6 + LED PA5
│   │   ├── board.h             # 板级原语接口
│   │   ├── board.c             # 板级原语 + 周期 tick 中断
│   │   └── board.cmake         # 晶振 + 时钟树
│   ├── ch32l103_candle/        # CH32L103 USB-CANFD 适配板（LED1 PA0，无 GPIO 按键）
│   └── ch32l103_bulepill/      # CH32L103 bluepill（LED PB11 高电平点亮，BOOT 键接 BOOT0）
├── port/
│   ├── boot_trigger_port.h     # 跨复位握手接口
│   ├── boot_usb_port.h         # 控制器基地址 + 收尾
│   ├── ch32v30x/
│   │   ├── port.cmake                 # rv32imacxw、CONFIG_USB_HS、USBHS 驱动、Link.ld
│   │   ├── boot_trigger_ch32v30x.c    # BKP 触发
│   │   ├── boot_usb_ch32v30x.c        # USBHS 时钟 + usb_dc_low_level_*
│   │   └── Link.ld                    # 0x00003000 + 116K，32K RAM
│   │                                  # （USBHS 设备驱动在子仓
│   │                                  #   third_party_components/CherryUSB/port/wch/ch32v30x/）
│   └── ch32l103/
│       ├── port.cmake                 # rv32imac、USBFS 驱动、Link.ld
│       ├── boot_trigger_ch32l103.c    # BKP 触发
│       ├── boot_usb_ch32l103.c        # USBFS 时钟 + usb_dc_low_level_* + 中断向量
│       └── Link.ld                    # 0x00003000 + 52K，20K RAM
│                                      # （USBFS 驱动用 CherryUSB 自带 port/wch/usbfs/）
└── src/
    ├── main.c                  # 点灯 + 按键进 boot + USB
    ├── usb_desc.c              # DFU runtime + CDC ACM
    ├── usb_config.h
    ├── fault_handlers.c        # 硬件异常 -> 复位（ch32fun 默认是死循环）
    ├── funconfig.h             # ch32fun 配置（printf、体积开关）
    └── boot_log.h
```

`boards/`、`port/`、`shared/`、`cmake/` 与引导程序工程保持同构，
两边共用同一套 board/chip 抽象与握手协议。

---

## 编译

使用 CMake preset（见 `CMakePresets.json`）：

```bash
git submodule update --init --recursive       # 首次

cmake --list-presets                          # 列出可用 preset
cmake --preset ch32l103_candle-release        # 配置（或 ch32v30x_ob-release）
cmake --build --preset ch32l103_candle-release
```

| preset | 说明 | 输出目录 |
| --- | --- | --- |
| `ch32v30x_ob-debug` | Debug（`BOOT_PRINTF` 打开） | `build/ch32v30x_ob-debug/` |
| `ch32v30x_ob-release` | Release（日志关闭，体积更小） | `build/ch32v30x_ob-release/` |
| `ch32l103_candle-debug` | Debug（`BOOT_PRINTF` 打开） | `build/ch32l103_candle-debug/` |
| `ch32l103_candle-release` | Release（日志关闭，体积更小） | `build/ch32l103_candle-release/` |
| `ch32l103_bulepill-debug` | Debug（`BOOT_PRINTF` 打开） | `build/ch32l103_bulepill-debug/` |
| `ch32l103_bulepill-release` | Release（日志关闭，体积更小） | `build/ch32l103_bulepill-release/` |

产物：`<输出目录>/ch32_hello_world.elf | .hex | .bin`。实测：CH32V30x Release
9080 B / Debug 11392 B（应用分区 116 KB）；CH32L103 Release 9356 B（candle）/
9348 B（bulepill）、Debug 12596 B / 12588 B（应用分区 52 KB）。

### 镜像里有什么

[ch32fun](https://github.com/cnlohr/ch32fun) 提供启动、向量表、时钟树和 `printf`，
其余全部是 `board.c` 与芯片 port 里的寄存器操作——构建里没有任何厂商 HAL。
ch32fun 是 git 子仓并固定在验证过的提交，`git submodule update --init --recursive`
会自动拉取；也可用 `-DCH32FUN_ROOT=<path>` 指定别的副本。

WCH 版 Release 是 12152 B，差距来自 C 库：newlib 的 `printf` 加上 `__udivdi3`
（时基里的 64 位除法），ch32fun 版用自带的小格式化器，时基也改成 32 位。

新增 board 后照葫芦画瓢加一对 preset（`<board>-debug` / `<board>-release`），
也可以不用 preset 直接：

```bash
cmake -S . -B build -DBOARD=<board_name> -DCHIP_PORT=<chip_port>
cmake --build build -j
```

> 需要 `riscv-wch-elf-` 工具链在 PATH 中（本机 `/opt/Toolchain/RISC-V_Embedded_GCC12`）。

---

## 使用

1. 先烧配套的 DFU 引导程序到 `0x00000000`（它占前 12 KB）。
2. 升级/烧录本 APP：
   ```bash
   dfu-util -e                                                # 进 bootloader（DFU runtime）
   dfu-util -a 0 -s 0x08003000:leave -D build/ch32v30x_ob-debug/ch32_hello_world.bin
   ```
    按键接在 GPIO 上的 board 也可以**长按 BOOT 键**进入 bootloader；两块 L103 板的
   BOOT 键接的是 BOOT0（固件读不到），只能走上面的 APP/bootloader 路径。
3. 复位后运行 APP：板上的状态 LED 心跳闪烁，USB 枚举为
   `DFU runtime + CDC ACM` 复合设备，CDC 回显收到的数据。

---

## 说明

- **从 bootloader 交接时钟**：bootloader 退出时 `SYSCLK` 已经挂在 PLL 上，而
  ch32fun 的 `SystemInit()` 是**赋值**方式重建时钟树，第一条写 `RCC->CTLR` 就会清掉
  `PLLON`——正好停掉内核正在用的时钟，程序在 `main()` 第一条指令就死了（LED 不闪、
  USB 不枚举）。因此 `src/main.c` 在 `SystemInit()` 之前先调用
  `clock_fallback_to_hsi()`，与 WCH 启动文件的顺序一致。接在这个 bootloader 后面的
  其他应用镜像同样需要这一步交接。
- **从 bootloader 交接 USB**：主机必须**先看到设备断开**，APP 再挂上去；否则主机会保留
  bootloader 的设备实例，新的设备节点永远不出现。CH32V30x 侧的做法是挂起 USBHS PHY 并保持
  >10 ms；USBFS 没有 PHY 挂起位，所以 `src/main.c` 自己先释放 D+
  （`boot_usb_port_deinit()`，在 USBFS 端口里同时复位 SIE），等 200 ms 再
  `app_usb_init()`。
  反方向（DFU_DETACH / BOOT 键 → bootloader）由 `boot_trigger_reboot_to_boot()` 处理：
  DFU_DETACH 请求先应答完（收尾动作延后到主循环），再把 D+ 硬拉低约 15 ms 后复位，
  bootloader 侧再等 20 ms 才挂上——远超主机需要的 10 ms，但不拖慢二次枚举。
- 按键判定为**持续性**：连续 `APP_BOOT_PRESS_COUNT` 次采样（默认 1000 ms）
  都按下才跳转，避免抖动误触发（`src/main.c`）。
- `ch32v30x_ob` 带 PA6 按键；无按键的板子——或者按键接在 **BOOT0** 上的板子（两块 L103
  板都是如此）——把该板 `board_config.h` 的 `BOARD_HAS_BOOT_BUTTON` 置 0 即可，只能通过
  `dfu-util -e` 进入 bootloader。
- **USBHS 驱动**：CherryUSB master 的 `port/wch/usbhs` 与 CH32V30x 的 `USBHSD` 寄存器映射
  不兼容（面向 CH32V205/V4x7/CH32X305/CH58x），直接用会导致 **USB 无法枚举**。
  上游删除了老的 `port/ch32/ch32hs` 且无替代，我们已恢复到自己的 fork
  **`git@github.com:zhangjiance/CherryUSB.git`** 分支 **`ch32v30x-usbhs`** 的
  `port/wch/ch32v30x/`，工程直接引用子仓里的这一份。核心/类仍用该分支 master。
- **USBFS 驱动（CH32L103）**：不定义 `CONFIG_USB_HS`，控制器驱动用 CherryUSB 自带的
  `port/wch/usbfs/usb_dc_usbfs.c`，`usb_desc.c` 的速度回调自然会给全速配置描述符
  （bulk 端点 64 字节）。芯片端口补上驱动无法知道的部分：`RCC->CFGR0.USBPRE = PLL/2`
  （96 MHz 得 48 MHz）、USBFS 的 AHB 时钟，以及把 `USBFS_IRQHandler` 转到
  `USBD_IRQHandler(0)`。
- VID/PID、串口为占位值，正式产品请替换。
- 若为 CH32V307（288 KB），改 `boards/ch32v30x_ob/board.cmake` 的 `BOOT_FLASH_SIZE`
  与 `port/ch32v30x/Link.ld` 的 `LENGTH`。
