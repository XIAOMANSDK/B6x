# AT 指令参考文档

**设备**: B6x BLE 带 GSM 音频解码器
**固件**: bleUartAT-gsmDecoder
**版本**: 1.0.1
**默认波特率**: 115200

---

## 目录

- [基础命令](#basic-commands)
- [系统命令](#system-commands)
- [BLE 配置](#ble-configuration)
- [连接命令](#connection-commands)
- [数据传输](#data-transfer)
- [GSM 音频命令](#gsm-audio-commands)
- [错误代码](#error-codes)

---

## 基础命令

### AT

测试命令。

**格式**: `AT`
**响应**: `[AT]OK`

**示例**:
```
AT
```
**响应**:
```
[AT]OK
```

---

## 系统命令

### AT+ALL

显示所有配置参数。

**格式**: `AT+ALL`
**响应**: `[AT]OK` 后跟所有配置数据

---

## BLE 配置

### AT+DEV_NAME?

获取设备名称。

**格式**: `AT+DEV_NAME?`
**响应**: `[AT]OK` 后跟设备名称

**参数**:
- 最大名称长度：11 个字符

---

### AT+MAC?

获取设备 MAC 地址。

**格式**: `AT+MAC?`
**响应**: `[AT]OK` 后跟 MAC 地址

**MAC 地址格式**: `AA:BB:CC:DD:EE:FF`（使用冒号分隔字节）

---

### AT+BAUD?

获取当前 UART 波特率。

**格式**: `AT+BAUD?`
**响应**: `[AT]OK` 后跟波特率

**支持的波特率**:
- 4800
- 9600
- 19200
- 38400
- 57600
- 115200（默认）
- 230400
- 460800
- 921600

**配置说明**:
- 修改后需要重启（`AT+RESET` 或电源循环）才能生效
- 配置会保存到 Flash 非易失存储区

---

## 连接命令

### AT+CON_MAC=

连接到指定 MAC 地址的设备（主机模式）。

**格式**: `AT+CON_MAC=<mac>`
**参数**:
- `<mac>`: MAC 地址（格式：`AA:BB:CC:DD:EE:FF`）

**响应**: `[AT]OK`
**效果**:
- 设备主动连接到指定 MAC 地址的设备
- 连接成功后进入透传模式
- LED 变为常亮

**示例**:
```
AT+CON_MAC=AA:BB:CC:DD:EE:FF
```
**响应**:
```
[AT]OK
[DA]Connecting
```

---

## 数据传输

### AT+DISCON=1

断开当前 BLE 连接。

**格式**: `AT+DISCON=1`
**响应**: `[AT]OK`（如果已连接）或 `[AT]ERR[3]`（如果未连接）

**效果**:
- 立即断开 BLE 连接
- 返回 AT 命令模式
- LED 变为慢闪

---

## GSM 音频命令

> **注意**: GSM 音频命令仅在 `GSM_DECODE_EN=1` 时可用

### AT+GSMPLAY

播放默认 Flash 地址的 GSM 音频。

**格式**: `AT+GSMPLAY`
**Flash 地址**: `0x18020000`（默认，由 `GSM_FLASH_ADDR` 定义）

**行为**: 播放一次（不循环）

**响应**: `[AT]OK`
**数据输出**: `[DA]Playing 112 frames from 0x18020000`

---

### AT+GSMPLAY=<addr>

播放指定 Flash 地址的 GSM 音频。

**格式**: `AT+GSMPLAY=<addr>`
**参数**:
- `<addr>`: Flash 地址（16 进制，如 `18010000` 或 `0x18020000`

**行为**: 播放一次（不循环）

**响应**: `[AT]OK`
**数据输出**: `[DA]Playing 56 frames from 0x18010000`

---

### AT+GSMSTOP

停止当前 GSM 音频播放。

**格式**: `AT+GSMSTOP`
**响应**: `[AT]OK`
**效果**:
- 立即停止播放
- 重置音频状态机

---

### AT+GSMSTATUS?

查询 GSM 音频播放状态。

**格式**: `AT+GSMSTATUS?`
**响应**: `[AT]OK` 后跟播放状态

**状态值**:
- `GSM Status=Playing`: 正在播放
- `GSM Status=Stopped`: 已停止

---

## 错误代码

| 代码 | 描述 |
|-------|-------------|
| ERR[0] | 无错误 |
| ERR[3] | 操作失败（如未连接时发送连接命令） |
| ERR[4] | 无效参数 |
| ERR[5] | 协议错误 |

---

## 快速参考

| 命令 | 功能 | 示例 |
|------|------|----------|
| `AT` | 测试命令 |
| `AT+ALL` | 显示所有配置 |
| `AT+VER?` | 获取固件版本 |
| `AT+DEV_NAME=MyDevice` | 设置设备名称 |
| `AT+CON_MAC=AA:BB:CC:DD:EE:FF` | 连接到指定设备 |
| `AT+DISCON=1` | 断开连接 |
| `AT+GSMPLAY` | 播放音频 |
| `AT+GSMSTATUS?` | 查询播放状态 |
| `AT+HELP` | 显示帮助 |

---

*文档版本*: 1.0
*最后更新*: 2026-02-12