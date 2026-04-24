---
name: b6-auto-debug
description: |
  B6x 系统化烧录调试 — 编译 → 诊断 → 烧录 → RTT 监控。
  先理解失败原因，再采取行动。

  TRIGGER when: 用户要求烧录、调试、下载固件、RTT监控、DAPLink/J-Link 操作、
  自动调试、flash编程、运行固件、连接失败、烧录失败、RTT无数据

  DO NOT TRIGGER when: 仅编译请求（用 /b6-build）、纯代码审查、无硬件连接

user-invocable: true
allowed-tools: Read, Grep, Glob, Edit, Write, Bash
compatibility: |
  Requires: embedded-debugger MCP server
  Hardware: DAPLink or J-Link debug probe
  Target: B61/B62/B63/B66 (ARM Cortex-M0+)
---

# B6x 系统化烧录调试

## 铁律

```
没有诊断就不重试 — 理解失败原因后才采取行动
```

每个失败点都有诊断步骤。如果你没有收集证据，就不能提出重试。

## 参数
- `$ARGUMENTS`: 项目路径（如 `projects/bleUart`）
- 设置变量 `PROJECT` 为项目名（路径最后一段）

## 适用场景

适用于任何硬件调试问题：
- 烧录失败
- 调试器连接失败
- RTT 无输出
- 固件运行异常

**尤其适用于：**
- "试了好几次都不行"（需要系统化诊断）
- "之前能用的现在不行了"（先检查变化）
- 时间紧迫时（系统化比反复尝试更快）

## 阶段 0：预检查

在开始任何操作之前，验证前提条件。

### 0.1 确定目标项目
- 若 `$ARGUMENTS` 已提供，使用该路径作为项目目录
- 若未提供，询问用户项目路径后再继续
- 设置变量 `PROJECT` 为项目名（路径最后一段）

### 0.2 检查调试模式
读取项目的 `src/cfg.h`，确认 `DBG_MODE` 设置：

| DBG_MODE | 状态 | 操作 |
|----------|------|------|
| 0 | 调试已禁用 | 停止，告知用户需要设置为 2 |
| 1 | UART 模式 | 告知用户 RTT 不可用，询问是否切换 |
| 2 | RTT 模式 | 通过，继续 |

### 0.3 检查环境
- 确认无其他调试工具占用（Keil 调试模式、J-Link Commander 等）
- 若有占用，提示用户关闭后重试

**阶段 0 通过条件：** 项目路径确定 + `DBG_MODE == 2` + 无工具冲突

## 阶段 1：编译与提取

**进入条件：** 阶段 0 通过

### 1.1 编译项目
调用 `/b6-build` skill 完成编译，传入项目路径。

编译成功（0 ERROR）后继续；否则按 b6-build 的结果处理。

### 1.2 提取 RTT Control Block 地址

按 `rtt-troubleshooting.md` 第 1 节提取并验证 CB 地址。

### 1.3 编译失败诊断

| 现象 | 诊断步骤 |
|------|---------|
| 编译错误 | 由 `/b6-build` 自动处理（最多 5 轮迭代） |
| RTT CB 地址未找到 | 按 `rtt-troubleshooting.md` "地址提取失败追溯链"排查 |

**阶段 1 通过条件：** 0 ERROR 编译成功 + 有效 RTT_CB_ADDR（`0x2000xxxx`）

## 阶段 2：连接与烧录

**进入条件：** 阶段 1 通过（有效 ELF + RTT_CB_ADDR）

### 2.1 连接调试器

首次连接：
```json
{
  "probe_selector": "auto",
  "speed_khz": 1000,
  "connect_under_reset": true,
  "halt_after_connect": true
}
```

**验证连接：** 连接后立即调用 `get_status(session_id)` 确认状态。保存 `session_id`。

### 2.2 连接失败诊断

**不要直接重试，先收集证据：**

1. 调用 `list_probes()` — 是否有调试器可见？

| list_probes 结果 | 可能原因 | 下一步 |
|-----------------|---------|--------|
| 无设备 | USB 未连接、驱动问题 | 提示用户检查 USB 和驱动 |
| 有设备但连接失败 | 目标板未供电、SWD 接线问题 | 提示用户检查供电和接线 |
| 有设备且之前有会话 | 旧会话未释放 | `disconnect` 旧会话后重试 |

2. 仅在诊断完成后，执行一次重试：
   - 调用 `disconnect(session_id)` 清理旧会话
   - 重新 `connect`

### 2.3 烧录固件

```json
{
  "session_id": "$SESSION_ID",
  "file_path": "build/elf/$PROJECT.elf",
  "format": "auto",
  "reset_after_flash": false,
  "attach_rtt": false
}
```

**验证烧录：** 调用 `get_status(session_id)` 确认目标处于 halted 状态。

### 2.4 烧录失败诊断

| 现象 | 诊断步骤 |
|------|---------|
| ELF 文件不存在 | 回到阶段 1 重新编译 |
| 烧录报错 | 先 `flash_erase(erase_type: "all")`，再重试烧录 |
| 目标未 halt | 调用 `halt(session_id)` 强制停止 |

**阶段 2 通过条件：** 固件已烧录 + `get_status` 确认目标 halted + `session_id` 有效

## 阶段 3：RTT 激活

**进入条件：** 阶段 2 通过（固件已烧录，目标 halted）

### 3.1 附加 RTT

使用阶段 1 提取的地址手动附加：
```json
{
  "session_id": "$SESSION_ID",
  "control_block_address": "$RTT_CB_ADDR"
}
```

**验证附加：** 调用 `rtt_channels(session_id)` 确认通道可见。

### 3.2 RTT 附加失败诊断

**不要更换地址或反复附加，按 `rtt-troubleshooting.md` 第 2 节排查。**

### 3.3 复位并运行

```json
{
  "session_id": "$SESSION_ID",
  "reset_type": "hardware",
  "halt_after_reset": false
}
```

**验证运行：** 调用 `get_status(session_id)` 确认目标处于 running 状态。
- 如果仍处于 halted：调用 `run(session_id)` 强制运行

**阶段 3 通过条件：** `rtt_channels` 可见 + `get_status` 确认目标 running

## 阶段 4：监控与诊断

**进入条件：** 阶段 3 通过（RTT 通道可见，目标 running）

### 4.1 读取 RTT 输出

循环读取 RTT 日志：
```json
{
  "session_id": "$SESSION_ID",
  "channel": 0,
  "max_bytes": 2048,
  "timeout_ms": 3000
}
```

**停止条件：**
- 用户明确要求停止
- 连续 5 次读取均无数据

### 4.2 无数据诊断

连续 3 次无数据时，不要继续空读，**按 `rtt-troubleshooting.md` 第 3 节追溯排查。**

**阶段 4 结束条件：** 用户要求停止，或诊断发现目标异常

## 三次失败升级

按 `hardware-escalation.md` 执行升级排查和架构质疑。

## 红旗 — 看到这些想法时停止

| 想法 | 为什么禁止 |
|------|-----------|
| "跳过编译直接烧录旧固件" | 固件与代码不同步，调试结果不可靠 |
| "烧录失败就直接重试" | 必须先诊断：连接状态？目标 halted？ELF 有效？ |
| "用 sleep 等待就绪" | 必须用条件检查：get_status、rtt_channels |
| "再试一次"（已失败 2 次以上） | 3 次失败 = 升级排查根因，不重试 |

**验证门控**: 每个阶段通过时，必须使用 `/b6_verification-before-completion` 的证据模板输出验证结果，然后才能声称该阶段通过。

RTT 相关红旗见 `rtt-troubleshooting.md` 第 4 节。

## 常见自我辩解

| 借口 | 现实 |
|------|------|
| "问题简单，直接重试就行" | 简单失败也有根因。诊断比盲目重试更快。 |
| "没时间诊断，先烧了再说" | 系统化诊断花 30 秒；反复盲试花 10 分钟。 |
| "sleep 2 秒应该够了" | 任意延时不可靠。检查实际条件。 |
| "CB 地址没变过" | 代码变更会移动符号。每次编译后都从 map 文件重新提取。 |
| "RTT 附加失败，固件肯定有问题" | RTT 附加失败有很多原因：时机、CB 地址、目标状态。先诊断。 |
| "调试器应该没问题" | 用 `list_probes()` 确认。假设浪费时间。 |
| "用 run_firmware 一步到位" | 手动分步操作 + 指定 CB 地址才是可靠方法。 |
| "第 3 次一定能成" | 3 次失败说明存在系统性问题。调查，不要重试。 |

## 条件等待技术

**原则：** 用实际条件验证替代任意延时。每个操作后验证真实状态。

| 操作 | 旧做法 | 新做法 |
|------|--------|--------|
| 连接调试器后 | sleep(1-2) | `get_status(session_id)` 验证已连接 |
| 烧录完成后 | 假设成功 | `get_status(session_id)` 验证目标 halted |
| RTT 附加后 | 假设已附加 | `rtt_channels(session_id)` 验证通道可见 |
| 复位运行后 | sleep(1) | `get_status(session_id)` 验证目标 running |
| RTT 读取 | 假设有数据 | `rtt_read` 自带 timeout_ms（已是条件等待） |

**超时限制：** 每个条件验证在 5 秒内完成。超时视为失败，进入诊断流程。

## 速查表

| 阶段 | 关键操作 | 通过条件 | 失败升级 |
|------|---------|---------|---------|
| **0. 预检查** | 检查 DBG_MODE、关闭冲突工具 | DBG_MODE==2，无冲突 | 告知用户，停止 |
| **1. 编译提取** | /b6-build，grep RTT CB 地址 | 0 ERROR，有效 0x2000xxxx | 检查编译配置、RTT 源文件 |
| **2. 连接烧录** | connect → get_status → flash → get_status | 固件已烧录，目标 halted | 检查探针、USB、SWD、ELF |
| **3. RTT 激活** | rtt_attach → rtt_channels → reset → get_status | 通道可见，目标 running | 检查 CB 地址、DBG_MODE |
| **4. 监控诊断** | rtt_read 循环 | 用户停止或 5 次无数据 | 检查目标状态、RTT 结构 |

### MCP 工具调用序列（正常流程）

```
list_probes → connect → get_status → flash_program → get_status →
rtt_attach(CB_ADDR) → rtt_channels → reset → get_status → rtt_read(循环)
```

## 注意事项

- `DBG_MODE` 必须设置为 `2`（RTT 模式）在项目 `src/cfg.h` 中
- 每次编译后从新 map 文件提取 RTT CB 地址，不缓存旧地址
- 建议在烧录前关闭其他调试工具（Keil 调试模式、J-Link Commander 等）
- `session_id` 在整个调试会话中保持不变，妥善保存
