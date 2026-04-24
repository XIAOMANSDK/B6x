# B6x SDK 文档

> **项目类型**: 蓝牙BLE芯片专用SDK文件包
> **芯片系列**: B61、B62、B63、B66
> **架构**: ARM M0+

---

## 文档索引

### 快速导航

- [SDK获取更新渠道](#sdk获取更新渠道)
- [在线文档](#在线文档)
- [项目目录结构](#项目目录结构)
- [芯片文档参考](#芯片文档参考)
- [BLE协议栈详细说明](#ble协议栈详细说明)
- [SOC外设驱动程序](#drivers-soc外设驱动程序)
- [外设应用实例](#examples-外设应用实例)
- [BLE蓝牙Mesh模块](#mesh-ble蓝牙mesh模块)
- [功能模块](#modules-功能模块)
- [工程项目](#projects-工程项目)
- [USB相关目录](#usb-usb相关目录)
- [工具链支持](#工具链支持)
- [关键词索引](#关键词索引)
- [AI检索提示](#ai检索提示)

---

## SDK获取更新渠道

- B6xSDK Github: <https://github.com/XIAOMANSDK/B6x>
- B6xSDK GitLab: <https://gitlab.com/xiao-man.com/B6x>
- B6xSDK 官方站: <https://www.xiao-man.com/support>

---

## 在线文档

- B6x_Docs 官方文档: <https://bx-docs.readthedocs.io/>

---

## 项目目录结构

```markdown
sdk6/
├── 2g4/                    # [2.4G目录]
│   ├── doc/                        # 2.4G设计参考文档
│   ├── examples/                   # 2.4G参考demo
│   └── proprietary/                # 2.4G底层实现
│
├── ble/                    # [BLE协议栈核心目录]
│   ├── api/                        # BLE协议层API头文件及说明
│   ├── app/                        # BLE GATT层实现文件
│   ├── lib/                        # BLE协议栈库文件(.lib .a)
│   └── prf/                        # Profile层应用实例(Server端)
│
├── core/                   # ARM M0+内核相关头文件
│   ├── gnu/                          # GCC启动文件和链接脚本
│   │   ├── startup_gnu.S                     # Flash启动(标准版)
│   │   ├── startup_gnu_mesh.S                # Mesh启动
│   │   ├── startup_sram_gnu.S                # SRAM启动
│   │   ├── link_xip.ld                       # XIP链接脚本
│   │   ├── link_xip_ble.ld                   # BLE链接脚本
│   │   ├── link_xip_blelite.ld               # BLE精简版链接脚本
│   │   ├── link_xip_mesh.ld                  # Mesh链接脚本
│   │   └── link_sram.ld                      # SRAM链接脚本
│   └── mdk/                          # Keil MDK启动文件
│
├── doc/                    # [芯片文档目录]
│   ├── APIs/                         # HTML格式BLE API文档目录
│   ├── DataSheet/                    # 芯片数据手册
│   ├── SW_Spec/                      # 芯片外设寄存器手册、FLASH/SRAM空间分配、IO映射、
│   ├── HW_Spec/                      # 硬件参考设计及注意事项、天线设计参考库、天线匹配电路调整方法。
│   ├── MP_Spec/                      # 量产文档及工具
│   └── B6x无线认证资料/              # 无线认证方法文档及工具
│
├── drivers/                # [SOC外设驱动程序目录]
│   ├── api/                          # 驱动API头文件及说明
│   └── lib/                          # 驱动库文件
│
├── examples/               # [外设应用实例目录]
│   ├── _blank/                      # 空白工程模板
│   ├── adpcm/                       # ADPCM音频编解码示例
│   ├── dmaTest/                     # DMA示例
│   ├── extiTest/                    # 外部中断示例
│   ├── faultTest/                   # 异常处理示例
│   ├── gpioTest/                    # GPIO示例
│   ├── gsmDecoder/                  # GSM解码示例
│   ├── helixMP3Decoder/             # MP3解码及PWM播放示例
│   ├── i2cNtag/                     # I2C NTAG读卡示例
│   ├── i2cTest/                     # I2C示例
│   ├── infraredTest/                # 红外接收示例
│   ├── iso7816/                     # ISO7816协议示例
│   ├── iwdtTest/                    # 独立看门狗示例
│   ├── ldoTest/                     # LDO示例
│   ├── loader/                      # OTA Boot loader加载器示例
│   ├── powerTest/                   # 电源管理示例
│   ├── pwcTest/                     # PWC示例
│   ├── pwm_led_ws2812/              # PWM控制WS2812 LED示例
│   ├── pwmSpeaker/                  # PWM扬声器驱动示例
│   ├── pwmTest/                     # PWM示例
│   ├── rcoTest/                     # RCO示例
│   ├── rtcTest/                     # RTC示例
│   ├── sadcTest/                    # SAR ADC示例
│   ├── spiLCD/                      # SPI驱动LCD示例
│   ├── spiMaster/                   # SPI主模式示例
│   ├── spiSlave/                    # SPI从模式示例
│   ├── stepperMotor/                # 步进电机控制示例
│   ├── uartTest/                    # UART示例
│   ├── usb2audio/                   # USB转音频示例
│   ├── usb2hid/                     # USB转HID示例
│   ├── usb2uart/                    # USB转UART示例
│   ├── usbDFU/                      # USB DFU升级示例
│   └── usbMsc/                      # USB MSC存储类示例
│
├── mesh/                   # [BLE蓝牙Mesh模块目录]
│   ├── api/                        # Mesh API头文件
│   ├── lib/                        # Mesh库文件
│   ├── src/                        # Mesh源码
│   ├── model/                      # Mesh模型定义
│   ├── genie/                      # 天猫精灵适配
│   ├── gnu/                        # GCC编译配置
│   └── mdk/                        # Keil MDK工程配置
│
├── modules/                # [功能模块目录]
│   ├── api/                        # 模块API头文件
│   ├── class/                      # 模块类定义
│   ├── lib/                        # 模块库文件
│   └── src/                        # 模块源码
│
├── projects/                # [BLE 工程项目目录]
│   ├── bleAdv_change/              # 广播参数修改示例
│   ├── bleFCC/                     # BLE FCC示例
│   ├── bleFCC_Store_Config/        # BLE FCC存储配置
│   ├── bleHid/                     # BLE HID键盘示例
│   ├── bleHid_Keybd/               # BLE HID键盘(单独)
│   ├── bleHid_Mouse/               # BLE HID鼠标
│   ├── bleHid_Uart/                # BLE HID UART
│   ├── bleMaster/                  # BLE主模式示例
│   ├── bleMasterSlave/             # BLE主从模式示例
│   ├── bleMesh/                    # BLE Mesh示例
│   ├── bleOTA/                     # BLE OTA升级示例
│   ├── bleOTA_Dongle/              # BLE OTA Dongle
│   ├── blePowerBank/               # BLE电源管理示例
│   ├── bleRemoteControl/           # BLE遥控器示例
│   ├── bleUart/                    # BLE转UART示例
│   ├── bleUart2USB/                # BLE转UART再转USB示例
│   ├── bleUartAT/                  # BLE UART AT指令示例
│   ├── bleUartAT-gsmDecodet/       # BLE UART AT + GSM解码
│   ├── bleUartCMD/                 # BLE UART命令处理示例
│   ├── bleWeChat/                  # 微信小程序示例
│   └── bleWeChatETC/               # 微信ETC小程序示例
│
├── tools/                  # [开发工具目录]
│   ├── KeilPack/                   # Keil 设备包 (Pack文件、FLM烧录算法)
│   ├── HidVoc/                     # HID语音工具 (adpcm2wav, hidVoc)
│   ├── YunWu/                      # 云雾相关工具
│   ├── BxOTA.PS1/cmd               # OTA升级脚本
│   ├── addr2line.exe               # MDK地址转行号工具
│   └── srec_cat.exe                # 格式转换工具
│
├── usb/                    # [USB相关目录]
│   ├── api/                        # USB API头文件
│   ├── class/                      # USB类定义
│   │   ├── audio/                      # 音频类
│   │   ├── cdc/                        # CDC串口类
│   │   ├── dfu/                        # DFU升级类
│   │   ├── hid/                        # HID类
│   │   └── msc/                        # MSC存储类
│   └── lib/                        # USB库文件
│       ├── usbd.lib                    # Keil库
│       ├── usbd_lite.lib               # Keil精简库
│       ├── libusbd.a                   # GCC库
│       └── libusbd_lite.a              # GCC精简库
│
├── CMakeLists.txt          # GCC顶层CMake配置
└── sdk.cmake               # GCC公共配置(工具链、编译选项)
```

---

## 芯片文档参考

### [doc/] 芯片文档目录

```markdown
sdk6/doc/
├── APIs/                         # BLE API文档目录
├── DataSheet/                    # 芯片数据手册
│   └── B6x_DataSheet_v3.4.docx   # 芯片手册
│
├── SW_Spec/                               # 软件规范文档
│   ├── B6x_IO_MAP.xlsx                    # IO映射表
│   ├── B6x-Flash-Map.xlsx                 # Flash空间分配表
│   ├── B6x_BLE-功耗参考.xlsx              # BLE功耗计算参考
│   ├── B6x_BLE-Sram空间分配.xlsx          # BLE SRAM空间分配
│   ├── B6x_BLE芯片使用指南_V1.0.*.docx    # 芯片寄存器说明
│   ├── B6x系列芯片Flash及SRAM说明.md      # Flash/SRAM说明
│   ├── ClaudeCode安装配置以及使用说明.md  # ClaudeCode使用指南
│   ├── JFlash配置说明.docx                # JFlash工具配置说明
│   ├── GNU开发环境配置/                   # GCC开发环境配置
│   │   └── GNU开发环境.md                 # GNU开发环境说明文档
│   ├── musbfsfc_pg.pdf                    # USB协议指南
│   └── musbfsfc_ps.pdf                    # USB编程规范
│
├── HW_Spec/                          # 硬件参考
│   ├── QFN32/                        # QFN32封装及硬件参考设计资料
│   ├── 常用天线库.PcbLib             # 天线库文件
│   ├── B6x阻抗-频偏调试方法.rar      # 阻抗和频偏调试方法
│   ├── 典型Layout设计错误(必看).docx # 常见设计错误
│   ├── 开发板使用说明.docx           # 开发板使用说明
│   └── 硬件设计指导.docx             # 硬件设计指导
│
├── MP_Spec/                      # 量产相关文档
│   ├── UartTool/                 # UART工具
│   ├── 量产软件/                 # 量产软件
│   └── 量产软件源码/             # 量产软件源码
│
├── B6x无线认证资料/              # 无线认证相关资料
│
├── B6x BLE兼容性列表.xlsx        # BLE兼容性列表
├── B6x_api(部分)设计指导.docx    # API设计指导文档
├── B6x常见应用解答.docx          # 常见问题解答
├── B6x无线认证指导.docx          # 无线认证指导
└── bxISP_Programmer_User_Guide_V1.0_CN.docx  # ISP烧录器用户指南
```

---

## BLE协议栈详细说明

### 目录层级

```markdown
ble/
├── api/                    # API层 - BLE协议层接口
├── app/                    # 应用层 - 应用程序实现
├── lib/                    # 库层 - 预编译库文件
└── prf/                    # Profile层 - 服务配置文件
```

### [API层] BLE协议层API头文件

```markdown
ble/api/
├── att.h            # 属性协议(Attribute Protocol)
│                    # 功能: UUID定义、服务/特征描述符、属性权限、GATT相关定义
│
├── attm_api.h       # 属性数据库管理API
│                    # 功能: 服务创建、属性管理
│
├── bledef.h         # BLE栈接口函数
│                    # 核心函数: ble_init(), ble_reset(), ble_sleep(), ble_wakeup()
│
├── blelib.h         # BLE库基础定义
│
├── fcc.h            # FCC相关定义
│
├── gap.h            # 通用访问协议(Generic Access Profile)
│                    # 功能: 广播参数、连接参数、地址类型定义
│
├── gapc_api.h       # GAP客户端API
│                    # 功能: 连接管理
│
├── gapc.h           # GAP客户端实现
│
├── gapm_api.h       # GAP管理器API
│                    # 功能: 设备配置、角色管理
│
├── gapm.h           # GAP管理器实现
│
├── gatt_api.h       # GATT API
│                    # 功能: 通用属性协议操作
│
├── gatt.h           # GATT协议定义
│
├── ke_api.h         # 内核事件API
│                    # 功能: 消息和任务管理
│
├── l2cc.h           # L2CAP连接控制
│
├── le_err.h         # 错误码定义
│                    # 包含: GAP/ATT/L2C/GATT/SMP/LL/Profile错误码
├── list.h           # 链表操作工具
└── task.h           # 任务管理定义
```

### [应用层] 应用层实现文件

```markdown
ble/app/
├── app.c/h          # 应用主程序及配置
│                    # 功能: 状态机、初始化、配置流程
│
├── app_actv.c/h     # 活动管理
│                    # 功能: 广播、扫描、发起连接等BLE活动
│
├── app_gapc.c       # GAP客户端应用实现
│
├── app_gapm.c       # GAP管理器应用实现
├── app_gatt.c       # GATT应用实现
└── app_msg.c        # 消息分发和处理
```

### [库层] BLE库文件

```markdown
ble/lib/
├── ble6.lib                 # 标准BLE库 (Keil MDK)
├── ble6_lite.lib            # 轻量级BLE库(内存占用更小)
├── ble6_8act_6con.lib       # 完整库:支持8个活动和6个连接
├── libble6.a                # 标准BLE库 (GCC)
├── libble6_lite.a           # 轻量级BLE库 (GCC)
└── libble6_8act_6con.a      # 完整库 (GCC)
```

### [Profile层] Profile层应用实例

```markdown
ble/prf/
├── prf.h               # Profile通用定义和数据结构
│
├── prf_api.h           # Profile API头文件
│
├── prf_bass.c/h        # 电池服务(Battery Service)
│                       # 功能: 电量查询、电量通知
│
├── prf_diss.c/h        # 设备信息服务
│                       # 功能: 厂商信息、版本号、硬件信息等
│
├── prf_hids.c/h        # HID服务(HID Service)
│                       # 功能: 键盘、鼠标等HID设备
│
├── prf_ota.c/h         # OTA空中升级服务(OTA Service)
│                       # 功能: 固件升级
│
├── prf_ptss.c/h        # profile test服务
│
├── prf_scps.c/h        # 扫描参数服务(Scan Parameters Service)
│
└── prf_sess.c/h        # 自定义透传服务
```

---

## [drivers/] SOC外设驱动程序

### 目录结构

```markdown
drivers/
├── api/                          # 驱动API头文件
├── src/                          # 驱动源码
└── lib/                          # 驱动库文件
```

### [API层] 驱动API头文件

```markdown
drivers/api/
├── btmr.h                       # 基本定时器(Basic Timer)
├── core.h                       # 内核相关定义
├── dma.h                        # DMA(Direct Memory Access)控制器
├── drvs.h                       # 驱动通用定义
├── exti.h                       # 外部中断(External Interrupt)
├── flash.h                      # Flash存储控制器
├── fshc.h                       # Flash高速缓存
├── gpio.h                       # GPIO通用输入输出
├── i2c.h                        # I2C总线控制器
├── iopad.h                      # IO PAD配置
├── iwdt.h                       # 独立看门狗(Independent Watchdog)
├── pwm.h                        # PWM脉冲宽度调制
├── rcc.h                        # 复位和时钟控制(Reset & Clock Control)
├── rco.h                        # RC振荡器
├── rtc.h                        # 实时时钟(Real Time Clock)
├── sadc.h                       # SAR ADC(逐次逼近型ADC)
├── spi.h                        # SPI串行外设接口
├── sysdbg.h                     # 系统调试
├── timer.h                      # 通用定时器
├── trim.h                       # 校准参数库，内部使用
├── uart.h                       # UART通用异步收发传输器
└── utils.h                      # 工具函数
```

### [源码层] 驱动源码文件

```markdown
drivers/src/
├── btmr.c                       # 基本定时器驱动实现
├── dma.c                        # DMA控制器驱动实现
├── exti.c                       # 外部中断驱动实现
├── fshc.c                       # Flash高速缓存驱动实现
├── i2c.c                        # I2C总线驱动实现
├── iopad.c                      # IO PAD配置实现
├── iwdt.c                       # 独立看门狗驱动实现
├── pwm.c                        # PWM驱动实现
├── rcc.c                        # 复位和时钟控制实现
├── rco.c                        # RC振荡器实现
├── rtc.c                        # RTC驱动实现
├── sadc.c                       # SAR ADC驱动实现
├── spi.h                        # SPI驱动实现
├── timer.h                      # 通用定时器实现
└── uart.c                       # UART驱动实现
```

---

## [examples/] 外设应用实例

### examples/ 目录结构

```markdown
examples/                        # 注: 每个示例项目包含 gnu/ 子目录用于 GCC 编译
├── _blank/                      # 空白工程模板
├── adpcm/                       # ADPCM音频编解码示例
├── dmaTest/                     # DMA测试示例
├── extiTest/                    # 外部中断测试示例
├── faultTest/                   # 异常处理测试示例
├── gpioTest/                    # GPIO测试示例
├── gsmDecoder/                  # GSM解码示例
├── helixMP3Decoder/             # MP3解码示例
├── i2cNtag/                     # I2C NTAG读卡示例
├── i2cTest/                     # I2C通信测试示例
├── infraredTest/                # 红外接收测试示例
├── iso7816/                     # ISO7816协议示例
├── iwdtTest/                    # 独立看门狗测试示例
├── ldoTest/                     # LDO测试示例
├── loader/                      # 加载器示例
├── powerTest/                   # 电源管理测试示例
├── pwcTest/                     # PWC测试示例
├── pwm_led_ws2812/              # PWM控制WS2812 LED示例
├── pwmSpeaker/                  # PWM扬声器驱动示例
├── pwmTest/                     # PWM测试示例
├── rcoTest/                     # RCO测试示例
├── rtcTest/                     # RTC测试示例
├── sadcTest/                    # SAR ADC测试示例
├── spiLCD/                      # SPI驱动LCD示例
├── spiMaster/                   # SPI主模式测试示例
├── spiSlave/                    # SPI从模式测试示例
├── stepperMotor/                # 步进电机控制示例
├── uartTest/                    # UART测试示例
├── usb2audio/                   # USB转音频示例
├── usb2hid/                     # USB转HID示例
├── usb2uart/                    # USB转UART示例
├── usbDFU/                      # USB DFU升级示例
└── usbMsc/                      # USB MSC存储类示例
```

---

## [mesh/] BLE蓝牙Mesh模块

### mesh/ 目录结构

```markdown
mesh/
├── api/                # Mesh API头文件
├── lib/                # Mesh库文件
├── src/                # Mesh源码
├── model/              # Mesh模型定义
├── genie/              # 天猫精灵适配
├── gnu/                # GCC静态库构建
└── mdk/                # Keil MDK工程配置
```

---

## [modules/] 功能模块

### modules/ 目录结构

```markdown
modules/
├── api/                # 模块API头文件
├── class/              # 模块类定义
├── lib/                # 模块库文件
└── src/                # 模块源码
```

---

## [projects/] 工程项目

### projects/ 目录结构

```markdown
projects/                 # 注: 每个项目包含 gnu/ 子目录用于 GCC 编译
├── bleAdv_change/        # 广播参数修改示例
├── bleFCC/               # BLE FCC示例
├── bleFCC_Store_Config/  # BLE FCC存储配置
├── bleHid/               # BLE HID示例(通用)
├── bleHid_Keybd/         # BLE HID键盘
├── bleHid_Mouse/         # BLE HID鼠标
├── bleHid_Uart/          # BLE HID UART
├── bleMaster/            # BLE主模式示例
├── bleMasterSlave/       # BLE主从模式示例
├── bleMesh/              # BLE Mesh示例
├── bleOTA/               # BLE OTA升级示例
├── bleOTA_Dongle/        # BLE OTA Dongle
├── blePowerBank/         # BLE电源管理示例
├── bleRemoteControl/     # BLE远程控制示例
├── bleUart/              # BLE转UART示例
├── bleUart2USB/          # BLE转UART再转USB示例
├── bleUartAT/            # BLE UART AT指令示例
├── bleUartAT-gsmDecodet/ # BLE UART AT + GSM解码
├── bleUartCMD/           # BLE UART命令处理示例
├── bleWeChat/            # 微信小程序示例
├── bleWeChatETC/         # 微信ETC小程序示例
└── others/               # 其他示例
```

---

## [usb/] USB相关目录

### usb/ 目录结构

```markdown
usb/
├── api/                # USB API头文件
├── class/              # USB类定义
│   ├── audio/          # 音频类
│   ├── cdc/            # CDC串口类
│   ├── dfu/            # DFU升级类
│   ├── hid/            # HID类
│   └── msc/            # MSC存储类
├── lib/                # USB库文件
│   ├── usbd.lib        # Keil标准库
│   ├── usbd_lite.lib   # Keil精简库
│   ├── libusbd.a       # GCC标准库
│   └── libusbd_lite.a  # GCC精简库
```

### USB示例快速索引

```markdown
- USB HID → usb/src/demo_hid_*.c
- USB MSC → usb/src/demo_msc.c
- USB DFU → usb/src/demo_dfu.c
- USB CDC → usb/src/demo_cdc_uart.c
- 音频HID → usb/src/demo_audio_hid.c
```

---

## 工具链支持

B6x SDK 支持两种开发工具链：

### Keil MDK

| 项目     | 说明                   |
|----------|------------------------|
| 工程文件 | `mdk/*.uvprojx`        |
| 输出目录 | `mdk/output/`          |
| 启动文件 | `core/mdk/startup_*.s` |
| 链接脚本 | `*.sct` (Scatter File) |

### GCC (ARM Cortex-M0+)

| 项目     | 说明                              |
|----------|----------------------------------|
| 构建系统 | CMake 3.20.0+                   |
| 工具链   | `arm-none-eabi-gcc`             |
| 启动文件 | `core/gnu/startup_gnu.S`        |
| 链接脚本 | `core/gnu/link_*.ld`            |
| 输出目录 | `build/elf/`, `build/hex/`, `build/bin/` |

#### GCC 构建命令

```bash
# 批量构建所有项目
cmake -B build -G Ninja
cmake --build build

# 单目标构建
cmake -B build -DTARGET_NAME=bleUart -G Ninja
cmake --build build
```

#### GCC 支持的项目

| 类型     | 数量  | 目录                |
|---------|------|---------------------|
| 示例项目 | 31个 | `examples/*/gnu/`  |
| BLE项目 | 24个 | `projects/*/gnu/`   |

---

## 关键词索引

| 分类     | 关键词                                   | 相关文件/目录        |
|---------|------------------------------------------|----------------------|
| BLE协议栈 | GAP, GATT, ATT, L2CAP, SMP            | ble/api/            |
| Profile服务 | BASS, DISS, HIDS, OTA, PTSS, Sess | ble/prf/ |
| 应用层 | app_init, app_gapc, app_gapm, app_gatt  | ble/app/            |
| 库文件 | ble6.lib, ble6_lite.lib                | ble/lib/            |
| 芯片文档 | Datasheet, SW_Spec, HW_Design | doc/ |
| 驱动程序 | GPIO, UART, SPI, I2C, PWM, ADC, DMA, Timer | drivers/api/ |
| GCC工具链 | CMake, arm-none-eabi-gcc, 链接脚本 | core/gnu/link_*.ld |
| 驱动API | btmr, gpio, uart, spi, i2c, pwm, sadc, rcc, rtc | drivers/api/*.h |
| RTT调试 | RTT, Trace, SEGGER | drivers/src/RTT/ |
| 应用示例 | example, demo | examples/ |
| 外设测试 | gpioTest, uartTest, spiTest, i2cTest, pwmTest | examples/ |
| 音频编解码 | ADPCM, MP3, GSM | examples/adpcm/, helixMP3Decoder/, gsmDecoder/ |
| USB应用 | USB HID, USB MSC, USB DFU, USB CDC | examples/usb2*/, usb/class/ |
| Mesh网络 | Mesh, Model, Genie | mesh/ |
| USB | USB驱动, USB库, USB示例 | usb/ |
| 错误码 | GAP_ERR, ATT_ERR, L2C_ERR, GATT_ERR, SMP_ERR | ble/api/le_err.h |

---

## AI检索提示

当查询此SDK时，可使用以下检索模式：

### 按功能检索

- "BLE初始化" → 参考 `ble/api/bledef.h` 中的 `ble_init()`
- "连接管理" → 参考 `ble/api/gapc_api.h`
- "服务创建" → 参考 `ble/api/attm_api.h`
- "Profile配置" → 参考 `ble/prf/` 对应服务文件
- "GPIO配置" → 参考 `drivers/api/gpio.h`
- "UART配置" → 参考 `drivers/api/uart.h`
- "SPI配置" → 参考 `drivers/api/spi.h`
- "I2C配置" → 参考 `drivers/api/i2c.h`
- "PWM配置" → 参考 `drivers/api/pwm.h`
- "ADC采集" → 参考 `drivers/api/sadc.h`
- "DMA传输" → 参考 `drivers/api/dma.h`
- "定时器" → 参考 `drivers/api/btmr.h` 或 `drivers/api/timer.h`
- "RTC时间" → 参考 `drivers/api/rtc.h`
- "看门狗" → 参考 `drivers/api/iwdt.h`
- "RTT调试" → 参考 `drivers/src/RTT/RTT.h`
- "寄存器定义" → 参考 `core/reg/*.h`

### 按模块检索

- "BLE API" → 查看各 `ble/api/*.h` 文件
- "BLE应用" → 查看 `ble/app/` 目录
- "BLE Profile" → 查看 `ble/prf/` 目录
- "BLE库" → 查看 `ble/lib/` 目录
- "驱动API" → 查看 `drivers/api/*.h` 文件
- "驱动源码" → 查看 `drivers/src/` 目录
- "应用示例" → 查看 `examples/` 目录
- "BLE项目" → 查看 `projects/` 目录
- "芯片文档" → 查看 `doc/` 目录
- "芯片寄存器" → 查看 `doc/SW_Spec/` 目录
- "硬件设计" → 查看 `doc/HW_Spec/` 目录
- "量产文档" → 查看 `doc/MP_Spec/` 目录

### 按示例类型检索

- "GPIO测试" → 查看 `examples/gpioTest/`
- "UART测试" → 查看 `examples/uartTest/`
- "SPI测试" → 查看 `examples/spiMaster/` 或 `examples/spiSlave/`
- "I2C测试" → 查看 `examples/i2cTest/` 或 `examples/i2cNtag/`
- "PWM测试" → 查看 `examples/pwmTest/` 或 `examples/pwmSpeaker/`
- "ADC测试" → 查看 `examples/sadcTest/`
- "DMA测试" → 查看 `examples/dmaTest/`
- "RTC测试" → 查看 `examples/rtcTest/`
- "看门狗测试" → 查看 `examples/iwdtTest/`
- "ADPCM编解码" → 查看 `examples/adpcm/`
- "MP3解码" → 查看 `examples/helixMP3Decoder/`
- "GSM解码" → 查看 `examples/gsmDecoder/`

### 按错误码检索

- "连接失败" → 搜索 `GAP_ERR_*` 错误码
- "属性错误" → 搜索 `ATT_ERR_*` 错误码
- "配对失败" → 搜索 `SMP_ERR_*` 错误码
- "GATT错误" → 搜索 `GATT_ERR_*` 错误码
- "Profile错误" → 搜索 `PRF_ERR_*` 错误码

---
