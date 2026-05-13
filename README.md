# DAPLink on Zephyr RTOS

CMSIS-DAP 调试器固件移植到 Zephyr RTOS，支持黑魔法探针功能。

## 功能

| 功能 | black_f407ve | stm32_min_dev |
|---|---|---|
| CMSIS-DAP v1 (HID) | ✅ | ✅ |
| CMSIS-DAP v2 (BULK) | ✅ | ❌ |
| CDC ACM 虚拟串口 | ✅ | ✅ |
| MSC 拖拽烧录 (FAT12) | ✅ | ❌ |
| Intel HEX 解析 | ✅ | ❌ |
| SWD 调试 | ✅ | ✅ |
| JTAG 调试 | ✅ | ✅ |
| SWO 追踪 | stub | stub |

## 硬件要求

### 已验证的板子

| 板子 | SoC | Flash | RAM | USB |
|---|---|---|---|---|
| black_f407ve | STM32F407VET6 | 512KB | 192KB | OTG_FS |
| stm32_min_dev | STM32F103C8 | 64KB | 20KB | USB |

### 引脚分配

**black_f407ve (SWJ):**
- SWCLK → PB13
- SWDIO → PB14
- nRESET → PB12
- HID LED → PA6 (低电平亮)
- CDC LED → PA7 (低电平亮)

**stm32_min_dev (SWJ):**
- SWCLK → PB0
- SWDIO → PB1
- nRESET → PB2
- LED → PB12

可通过 `boards/<board>.overlay` 自定义引脚。

## 构建

```bash
# 前提：已安装 Zephyr SDK 和 west
export ZEPHYR_BASE=/path/to/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# black_f407ve (完整功能)
west build -b black_f407ve .

# stm32_min_dev (最小配置)
west build -b stm32_min_dev .
```

输出固件：`build/zephyr/zephyr.elf`

## 架构

```
src/
├── main.c                  # 入口 + 事件循环
├── DAP.h                   # CMSIS-DAP 协议定义
├── DAP.c                   # 协议命令处理引擎
├── DAP_config.h            # 平台配置 (CPU_CLOCK, 引脚映射)
├── IO_Config.h             # GPIO 抽象接口
├── SW_DP.c                 # SWD 协议位操作
├── JTAG_DP.c               # JTAG 协议位操作
├── SWO.c                   # SWO 追踪 (stub)
├── DAP_queue.c/h           # DAP 响应队列
├── debug_cm.h              # Cortex-M 调试寄存器定义
├── swd_host.c/h            # SWD 主机驱动
├── gpio_port.c             # Zephyr GPIO 适配
├── uart_port.c             # Zephyr UART 适配
├── info.c/h                # 设备信息字符串
│
├── usbd_user_hid.c         # USB HID 接口 (CMSIS-DAP v1)
├── usbd_dap_v2.c           # USB BULK 接口 (CMSIS-DAP v2)
├── usbd_user_cdc_acm.c     # USB CDC ACM 虚拟串口
├── usbd_user.h             # USB 接口声明
│
├── msc_daplink.c           # FAT12 虚拟文件系统
├── msc_lun.c               # MSC LUN 注册
├── intelhex.c/h            # Intel HEX 解析器
├── flash_blob.h            # Flash 算法描述符
├── flash_blob_stm32f103.c  # Flash 算法示例 (STM32F103)
├── flash_prog.c            # 固件写检测 + 烧录
│
└── util.h                  # 工具宏

boards/
├── black_f407ve.overlay    # F407 板级 DTS
├── stm32_min_dev.overlay   # F103 板级 DTS
└── stm32_min_dev.conf      # F103 板级 Kconfig

prj.conf                    # 全局 Kconfig
Kconfig                     # 应用 Kconfig
CMakeLists.txt              # 构建
```

## 配置

### 全局 (prj.conf)

| 选项 | 默认值 | 说明 |
|---|---|---|
| CONFIG_DAPLINK_PACKET_SIZE | 64 | CMSIS-DAP 包大小 |
| CONFIG_DAPLINK_PACKET_COUNT | 4 | 协议队列深度 |
| CONFIG_DAPLINK_SWD | y | SWD 支持 |
| CONFIG_DAPLINK_JTAG | y | JTAG 支持 |

### 板级 (boards/<board>.conf)

通过 `boards/<board>.conf` 覆盖全局配置。例如 stm32_min_dev 关闭了 MSC 和日志。

### 引脚 (boards/<board>.overlay)

通过 `boards/<board>.overlay` 中的 devicetree 节点配置 SWJ 引脚和 LED。

## 使用

连接 USB 后：

1. **CMSIS-DAP 调试器** — OpenOCD / PyOCD / GDB 直接识别
   ```
   openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg
   ```
2. **虚拟串口** — 出现 `/dev/ttyACM0` 或 COM 端口
3. **拖拽烧录 (F407)** — 出现 DAPLINK 盘，复制 .hex 文件自动烧录

## License

CMSIS-DAP 协议代码基于 ARM CMSIS-DAP v2.1.0 (Apache-2.0)。
其余代码基于 Apache-2.0。
