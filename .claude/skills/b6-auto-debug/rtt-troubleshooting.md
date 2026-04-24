# RTT 故障排查手册

## 概述

SEGGER RTT 是 B6x 调试的主要输出通道。RTT 问题按发生阶段分为三类：
CB 地址提取、RTT 附加、RTT 数据读取。每类都有独立的追溯链。

## 1. RTT Control Block 地址

### 提取方法

从编译后的 map 文件提取 `_RTT` 符号地址：

```bash
grep "\.bss\._RTT\| _RTT" build/map/$PROJECT.map | grep -oE "0x[0-9a-fA-F]+" | head -1
```

### 地址验证

| 检查项 | 期望值 | 失败处理 |
|--------|--------|---------|
| 地址格式 | `0x2000xxxx`（SRAM 范围） | 用更精确的模式重试 |
| 符号归属 | `.bss` 段 | 可能匹配到错误符号 |
| 唯一性 | grep 仅返回一个结果 | 多结果时用 `.bss\._RTT` 过滤 |

更精确的 grep（符号模糊匹配时）：
```bash
grep "\.bss\._RTT" build/map/$PROJECT.map | grep -oE "0x[0-9a-fA-F]+"
```

### 地址提取失败追溯链

```
grep 无结果 → RTT 源文件未加入编译？
            → 检查 CMakeLists.txt 或 .uvprojx 是否包含 SEGGER_RTT.c
            → 检查 cfg.h 中 DBG_MODE 是否为 2（可能被条件编译排除）

地址不在 SRAM → 匹配到错误符号？
              → 用 "\.bss\._RTT" 精确过滤
              → 检查链接脚本 SRAM 起始地址是否正确
```

**关键规则：** 每次编译后必须重新提取地址，不缓存。代码变更会移动符号位置。

## 2. RTT 附加失败

### 正常附加流程

```json
{
  "session_id": "$SESSION_ID",
  "control_block_address": "$RTT_CB_ADDR"
}
```

验证：`rtt_channels(session_id)` 确认通道可见。

### 失败诊断 — 不要更换地址，先读取内存验证

读取 CB 地址处的内存：`read_memory(session_id, RTT_CB_ADDR, 16)`

| 内存内容 | 含义 | 下一步 |
|---------|------|--------|
| `SEGGER RTT` 魔术字节 | CB 结构正确，附加时机问题 | 复位目标让其运行，再重新附加 |
| 全零 | 固件未执行到 RTT 初始化 | 追溯初始化路径 |
| 随机数据 | CB 地址可能错误 | 回到阶段 1 重新提取地址 |

### 附加失败追溯链

```
rtt_attach 失败 → read_memory(CB_ADDR, 16) 检查内容
  → 魔术字节存在 → 时机问题 → reset 目标后重试附加
  → 全零 → RTT_Init 未执行
    → cfg.h 中 DBG_MODE 是否为 2？
    → RTT 初始化函数是否被调用？
    → 初始化是否在 main() 执行路径上？
    → 是否有 early return 跳过了初始化？
  → 随机数据 → CB 地址错误
    → 回到 map 文件重新提取
    → 检查链接脚本是否正确
```

### 三次升级

| 次数 | 动作 |
|------|------|
| 第 1 次 | 重试同一 CB 地址 |
| 第 2 次 | `read_memory` 验证 CB 内容 |
| 第 3 次 | 检查 cfg.h 中 DBG_MODE、RTT 源文件是否加入编译，向用户报告 |

## 3. RTT 无数据

### 读取方法

```json
{
  "session_id": "$SESSION_ID",
  "channel": 0,
  "max_bytes": 2048,
  "timeout_ms": 3000
}
```

停止条件：连续 5 次无数据。

### 无数据诊断 — 不要继续空读

连续 3 次无数据后，进入诊断：

#### Step 1: 检查目标状态

`get_status(session_id)` — 目标是否仍在运行？

| 目标状态 | 可能原因 | 下一步 |
|---------|---------|--------|
| Running | 固件未执行到 debug 输出，或无输出逻辑 | 继续监控，告知用户 |
| Halted | HardFault 或断点 | 读取故障寄存器 |
| 断开 | 调试器连接丢失 | 重新连接 |

#### Step 2: 检查 RTT 结构

`read_memory(session_id, RTT_CB_ADDR, 16)` — CB 结构是否完整？

#### Step 3: 追溯代码路径

如果目标 Running 且 CB 结构完整，但无输出：

```
RTT 无数据 → 目标 running?
  → 是 → CB 结构完整?
    → 是 → 追溯代码路径
      → RTT 初始化是否完成？
      → 是否有代码调用 SEGGER_RTT_Write() / RTT_printf()？
      → 调试点是否在可达的执行路径上？
      → 是否有条件分支跳过了调试点？
      → 是否栈溢出导致 RTT 缓冲区被覆盖？
    → 否 → RTT_Init 未完成或被破坏
      → 检查初始化顺序
      → 检查是否有内存越界写入
  → 否 → 目标异常
    → Halted → 读取 SHCSR、HFSR、MMFAR、BFAR 故障寄存器
    → 断开 → 重新连接调试器
```

### 三次升级

| 次数 | 动作 |
|------|------|
| 第 1 次 | `get_status` 检查目标 |
| 第 2 次 | `read_memory` 检查 RTT 结构 |
| 第 3 次 | 检查固件 RTT 初始化顺序、栈溢出、HardFault，向用户报告 |

## 4. RTT 相关红旗

| 想法 | 为什么禁止 |
|------|-----------|
| "换个 CB 地址试试" | CB 地址来自 map 文件，如果错了应该调查链接脚本 |
| "CB 地址应该和上次一样" | 代码变化会移动符号，每次编译后必须重新提取 |
| "RTT 无数据 = 烧录失败" | 可能是：固件未执行到 RTT 初始化、CB 地址错误、DBG_MODE 不对 |
| "不检查 DBG_MODE 就开始 RTT" | RTT 需要 DBG_MODE==2，其他模式无输出 |
| "run_firmware 自动 RTT 就够了" | 手动指定 CB 地址附加更可靠，自动扫描可能失败 |

## 5. HardFault 诊断（目标 Halted 时）

当 `get_status` 显示目标 Halted 且排除断点原因后，读取故障寄存器：

| 寄存器 | 地址偏移 | 含义 |
|--------|---------|------|
| SCB->HFSR | `0xE000ED2C` | HardFault 状态 |
| SCB->MMFAR | `0xE000ED34` | MemManage 故障地址 |
| SCB->BFAR | `0xE000ED38` | BusFault 故障地址 |
| SCB->CFSR | `0xE000ED28` | 可配置故障状态 |

读取方法：
```json
{
  "session_id": "$SESSION_ID",
  "address": "0xE000ED2C",
  "size": 4,
  "format": "words32"
}
```

**目的：** 确定是哪种故障（栈溢出、空指针、非法地址）导致目标停止，而非盲目重试。
