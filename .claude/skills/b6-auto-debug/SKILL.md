---
name: b6-auto-debug
description: |
  B6x 自动烧录调试 - 编译(GCC) → 烧录(DAPLink/J-Link) → RTT监控 的完整流程。

  TRIGGER when: 用户要求烧录、调试、下载固件、RTT监控、DAPLink/J-Link 操作、
  自动调试、flash编程、运行固件

  DO NOT TRIGGER when: 仅编译请求（用 /b6-build）、纯代码审查、无硬件连接

user-invocable: true
allowed-tools: Read, Grep, Glob, Edit, Write, Bash
compatibility: |
  Requires: embedded-debugger MCP server
  Hardware: DAPLink debug probe
  Target: B61/B62/B63/B66 (ARM Cortex-M0+)
---

# B6x 自动烧录调试

自动完成：编译 → 烧录 → RTT 监控的完整流程。

## 参数
- `$ARGUMENTS`: 项目名称（默认：bleUart）

## 执行流程

### 1. 确定目标项目
项目名：`$ARGUMENTS` 或默认 `bleUart`

设置变量 `PROJECT=$ARGUMENTS`

### 2. 编译项目（含错误修复）
使用 CMake 编译指定项目：
```bash
cmake -DTARGET_NAME=$PROJECT -B build -G Ninja && cmake --build build
```

**编译失败处理**：
- 如果有 ERROR：读取错误信息 → 定位问题代码 → 修复 → 重新编译
- 重复直到 0 ERROR 且 Warning ≤ 5
- 如果无法自动修复，报告详细错误信息后停止

### 3. 查找 RTT Control Block 地址
编译成功后，从 map 文件提取 `_RTT` 变量地址：
```bash
grep "\.bss\._RTT\| _RTT" build/map/$PROJECT.map | grep -oE "0x[0-9a-fA-F]+" | head -1
```
- map 文件路径：`build/map/$PROJECT.map`
- 搜索模式：优先 `.bss._RTT`，备用 `_RTT`
- 提取地址保存为 `RTT_CB_ADDR`（通常在 0x2000xxxx）

### 4. 连接调试器
使用 embedded-debugger MCP 连接，**如果失败则先断开旧会话重试**：

**首次连接尝试**：
```json
{
  "probe_selector": "auto",
  "speed_khz": 1000,
  "connect_under_reset": true,
  "halt_after_connect": true
}
```

**如果连接失败，执行重试流程**：
1. 断开可能存在的旧会话（使用之前的 session_id 或空字符串）
2. 等待 1-2 秒
3. 重新执行连接

**重要**：连接成功后，**保存返回的 `session_id`**，后续所有操作都需要此 ID。

```
connect() 失败 → disconnect() → sleep(1) → connect()
```

### 5. 烧录固件
推荐使用 `run_firmware` 一步完成（不附加 RTT，后续手动附加）：
```json
{
  "session_id": "$SESSION_ID",
  "file_path": "build/elf/$PROJECT.elf",
  "format": "auto",
  "reset_after_flash": false,
  "attach_rtt": false
}
```

**备选方案**（仅在需要单独控制时）：
```json
// 步骤 5a: 擦除
{"session_id": "$SESSION_ID", "erase_type": "all"}

// 步骤 5b: 烧录
{
  "session_id": "$SESSION_ID",
  "file_path": "build/elf/$PROJECT.elf",
  "format": "auto",
  "verify": true
}
```

### 6. 手动附加 RTT（关键步骤）
**必须**使用步骤 3 获取的地址手动附加 RTT：
```json
{
  "session_id": "$SESSION_ID",
  "control_block_address": "$RTT_CB_ADDR"
}
```
- 这是解决 `run_firmware` 自动 RTT 附加失败的关键
- RTT CB 通常位于 SRAM 区域（0x2000xxxx）

### 7. 复位运行
```json
{
  "session_id": "$SESSION_ID",
  "reset_type": "hardware",
  "halt_after_reset": false
}
```

### 8. 监控 RTT 输出
循环读取 RTT 日志（建议读取 3-5 次或直到用户中断）：
```json
{
  "session_id": "$SESSION_ID",
  "channel": 0,
  "max_bytes": 2048,
  "timeout_ms": 3000
}
```

## 错误处理

| 错误类型 | 处理方式 |
|---------|---------|
| 编译失败 | 自动修复错误并重新编译，无法修复则停止 |
| 连接失败 | 执行 `disconnect` → `sleep(1)` → `connect` 重试 |
| 烧录失败 | 尝试 `flash_erase` 后重新烧录 |
| RTT 附加失败 | 确保目标已运行 2 秒后再附加，检查 CB 地址 |
| RTT 无数据 | 检查 `DBG_MODE=2`，确认 CB 地址正确 |

## 连接失败自动重试流程
```
┌─────────────┐
│  connect()  │
└──────┬──────┘
       │
   ┌───▼───┐     失败
   │ 成功? │────────────┐
   └───┬───┘            │
       │ 是             │
       ▼                ▼
   ┌───────┐      ┌─────────────┐
   │ 继续  │      │ disconnect()│
   └───────┘      └──────┬──────┘
                         │
                    ┌────▼────┐
                    │ sleep(1)│
                    └────┬────┘
                         │
                    ┌────▼────┐
                    │connect()│
                    └────┬────┘
                         │
                    ┌────▼────┐
                    │ 成功?   │──失败──► 报告错误
                    └────┬────┘
                         │成功
                         ▼
                    ┌───────┐
                    │ 继续  │
                    └───────┘
```

## 注意事项
- 确保 DAPLink/J-Link 已连接到目标板
- 目标板已供电
- SWD 接线正确
- `DBG_MODE` 必须设置为 `2` (RTT 模式)
- 建议在烧录前关闭其他调试工具（Keil、J-Link Commander 等）
