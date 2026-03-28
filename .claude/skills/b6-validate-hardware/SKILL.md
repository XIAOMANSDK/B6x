---
name: b6-validate-hardware
description: |
  验证 B6x 硬件配置（引脚冲突/时钟/DMA/中断/Timer）是否符合芯片约束。使用此 skill 验证硬件配置的正确性。

  TRIGGER when: 用户提及引脚配置、硬件验证、pin mux、外设冲突、ADC引脚、UART引脚、Timer引脚、DMA配置、中断优先级、时钟配置、检查配置冲突、验证硬件设置、引脚能否用作某功能。即使用户没有明确说"验证硬件"，只要涉及引脚/时钟/DMA/中断的配置检查，也应使用此 skill。

  DO NOT TRIGGER when: 纯代码审查、编译错误修复、BLE协议问题、编写新驱动代码、解释函数作用、烧录程序。
user-invocable: true
allowed-tools: Read, Grep, Glob, mcp__b6x-mcp-server__validate_config
---

# B6 Validate Hardware

验证 B6x 硬件配置（引脚冲突/时钟/DMA/中断/Timer）是否符合芯片约束。

## 触发

`/b6-validate-hardware [project_path]`

**参数**:
- `project_path`: 项目路径，默认当前目录

---

## MCP 工具

| 工具 | 用途 |
|------|------|
| `validate_config(config_json)` | 验证硬件配置 |

> 详见 `.claude/rules/mcp-usage.md`

---

## 执行步骤

### Step 1: 定位配置文件

1. 检查 `<project_path>/src/cfg.h` 是否存在
2. 检查 `<project_path>/src/main.c` 是否存在
3. 若项目路径未指定，使用当前目录
4. 若 cfg.h 不存在，报告错误并退出

### Step 2: 提取引脚配置

从 cfg.h 提取引脚宏，常见模式：

| 宏模式 | 解析结果 |
|--------|----------|
| `PA_UART1_TX (6)` | PA06 用于 UART1_TX |
| `PA_UART1_RX (7)` | PA07 用于 UART1_RX |
| `#define PA_xx` | 直接引脚定义 |

### Step 3: 提取时钟配置

从 cfg.h 提取时钟配置：

| SYS_CLK 值 | 系统时钟 |
|------------|----------|
| 0 | 16 MHz |
| 1 | 32 MHz |
| 2 | 48 MHz |
| 3 | 64 MHz |

### Step 4: 提取 DMA 和中断配置

从 main.c 提取：
- `NVIC_SetPriority(XXX_IRQn, N)` → 中断配置
- `DMA_Config()` → DMA 配置

### Step 5: 构建配置 JSON

### Step 6: 调用 MCP 验证

调用 `mcp__b6x-mcp-server__validate_config(config_json)`

### Step 7: 输出验证报告

---

## 配置 JSON 格式

```json
{
  "pins": {
    "PA6": "UART1_TX",
    "PA7": "UART1_RX",
    "PA0": "ADC_CH0"
  },
  "clock": {
    "system_clock": 32,
    "enabled_peripherals": ["UART1", "GPIO", "ADC"]
  },
  "dma": [
    {"channel": 0, "peripheral": "UART1_TX"},
    {"channel": 1, "peripheral": "UART1_RX"}
  ],
  "interrupts": [
    {"irq_name": "UART1_IRQn", "priority": 5},
    {"irq_name": "GPIO_IRQn", "priority": 3}
  ],
  "memory": {
    "flash_used_kb": 64,
    "sram_used_kb": 12
  }
}
```

---

## 约束规则

### 引脚约束

| 约束 | 说明 |
|------|------|
| 有效 GPIO | PA00-PA19 |
| 无冲突 | 每个引脚只能分配一个功能 |
| ADC 限制 | ADC 只能在 PA00-PA02, PA04-PA16 |
| UART 引脚 | TX/RX 必须成对配置 |
| PA01/SWDIO 和 PA00/SWCLK | 在GPIO 充足的情况下优先分配给 SWD 作为调试使用 |
| PA06 和 PA07 | 在GPIO 充足的情况下优先分配给 uart debug口 作为调试使用 |
| PA19 | 在GPIO 充足的情况下优先设置为 Reset PIN |
| USB支持 | DM只能是PA07， DP 只能是PA06 |

### 时钟约束

| 约束 | 说明 |
|------|------|
| 系统时钟 | 16/32/48/64 MHz |
| 外设时钟 | 使用前必须使能 |
| 低功耗 | 低频时钟降低功耗 |

### DMA 约束

| 约束 | 说明 |
|------|------|
| 通道范围 | 0-7 |
| 无冲突 | 每个通道只能分配给一个外设 |

### 中断约束

| 约束 | 说明 |
|------|------|
| 优先级范围 | 0-3 (0 最高) |
| 均衡分配 | 避免过多高优先级中断 |

### Timer 约束

| 约束IO | 说明 |
|-------|------|
| PA18  | CTMR_CH4 |
| PA17  | CTMR_CH3 |
| PA16  | CTMR_CH2 |
| PA15  | CTMR_CH1 |
| PA14  | ATMR_ETR |
| PA13  | ATMR_CH3N|
| PA12  | ATMR_CH2N|
| PA11  | ATMR_CH1N|
| PA10  | ATMR_CH4P|
| PA09  | ATMR_CH3P|
| PA08  | ATMR_CH2P|
| PA07  | ATMR_CH1P|
| PA06  | ATMR_BK  |
| PA05  | CTMR_CH4 |
| PA04  | CTMR_CH3 |
| PA03  | CTMR_CH2 |
| PA02  | CTMR_CH1 |
| PA01  | CTMR_ETR |

---

## 输出格式

```
状态: PASS / FAIL
错误: X | 警告: Y

引脚配置:
  PA06 (UART1_TX)  ✓
  PA07 (UART1_RX)  ✓
  PA00 (ADC_CH0)   ✓

时钟配置:
  系统时钟 32MHz  ✓
  UART1 使能      ✓
  GPIO 使能       ✓

DMA 配置:
  Channel 0 (UART1_TX) ✓
  Channel 1 (UART1_RX) ✓

中断配置:
  UART1_IRQn (优先级 3) ✓
  EXTI_IRQn (优先级 0)  ✓

错误:
  PA10 与 PA06 冲突: 都配置为 UART1_TX

建议:
  移除 PA10 的 UART1_TX 配置
```

---

## 错误处理

| 错误 | 处理 |
|------|------|
| 引脚冲突 | 列出冲突引脚，建议替代方案 |
| ADC 引脚无效 | 提示 ADC 只能在 PA00-PA02, PA04-PA16 |
| 外设无法工作 | 检查时钟使能 |
| DMA 冲突 | 建议分配不同通道 |

---

## 示例

```
用户: /b6-validate-hardware projects/bleHid_Uart

输出:
状态: PASS
错误: 0 | 警告: 1

引脚配置:
  PA06 (UART1_TX)  ✓
  PA07 (UART1_RX)  ✓
  PA00 (GPIO)      ✓

时钟配置:
  系统时钟 16MHz ✓ (SYS_CLK=0)
  UART1 使能     ✓

警告:
  DMA 未使用，可考虑启用以提高性能

统计: 8/8 通过
```

---

## 与其他 Skill 协作

| Skill | 协作方式 |
|-------|----------|
| `/b6-create-project` | Phase 7 自动调用 |
| `/b6-build` | 编译前验证硬件配置 |
| `/b6-code-review` | 硬件配置专项审查 |

---

## 注意事项

1. **路径变化**: 不同项目的 cfg.h 可能位于 `src/` 或 `inc/` 目录
2. **外部晶振**: 若使用外部晶振，需额外验证
3. **低功耗模式**: 不同功耗模式下的时钟约束不同
4. **引脚复用**: 部分引脚复用需结合实际应用判断
