---
name: b6-power-analyze
description: |
  B6x BLE 项目功耗分析工具。

  TRIGGER when: 用户要求分析功耗、功耗分析、电池续航、功耗优化、省电、低功耗优化、电流计算、电池寿命估算、BLE 功耗、蓝牙功耗、连接参数优化、广播参数优化、功耗报告。即使用户没有明确说"功耗分析"，只要涉及 B6x 项目的电池续航、电流消耗、省电策略分析，也应使用此 skill。

  DO NOT TRIGGER when: 用户要求 BLE 库选择（用 /b6-library-recommend）、代码审查（用 /b6-code-review）、硬件配置验证（用 /b6-validate-hardware）、编译构建（用 /b6-build）。
user-invocable: true
disable-model-invocation: true
allowed-tools: Read, Grep, Glob, WebSearch, mcp__b6x-mcp-server__search_sdk
---

# B6 Power Consumption Analyzer

分析 B6x BLE 项目功耗，计算平均电流消耗，估算电池续航，并给出优化建议。

## 触发

`/b6-power-analyze <project_path> [options]`

**参数**:
- `project_path`: 项目路径，默认当前目录
- `--battery XXX`: 电池容量 (mAh)，默认 200

---

## B6x 芯片功耗参考数据 (3.3V)

| 工作模式 | 平均功耗 | 持续时间 | 说明 |
|----------|----------|----------|------|
| Sleep 模式 | 15 µA | 持续 | 轻度睡眠，RAM 保持 |
| PowerOff 模式 | 2 µA | 持续 | 深度睡眠，最低功耗 |
| Sleep 广播 | 4.6 mA | 5.8 ms | 从唤醒到睡眠的 active 时间 |
| Sleep 空包连接 | 3.25 mA | 4.7 ms | CI=30ms 时的 active 时间 |
| PowerOff 广播 | 4.0 mA | 8.3 ms | 从唤醒到睡眠的 active 时间 |
| PowerOff 空包连接 | 3.0 mA | 7.5 ms | CI=30ms 时的 active 时间 |

---

## BLE 参数说明

### 广播参数

| 参数 | 范围 | 对功耗的影响 |
|------|------|--------------|
| Advertising Interval | 20ms ~ 10.24s | 间隔越长，平均功耗越低 |
| Advertising Type | 可连接/不可连接/定向 | 定向广播功耗是普通广播的 2-3 倍 |
| Advertising Duration | 可配置 | 超时后进入深度睡眠 |

### 连接参数

| 参数 | 范围 | 对功耗的影响 |
|------|------|--------------|
| Connection Interval (CI) | 7.5ms ~ 4s | 间隔越长，唤醒次数越少，功耗越低 |
| Slave Latency | 0 ~ 499 | 可跳过的连接事件数，越大功耗越低 |
| Supervision Timeout | 100ms ~ 32s | 影响连接稳定性，对功耗影响较小 |

**关键概念 - 有效连接间隔**:
```
Effective_CI = CI × (1 + Slave Latency)
```
例如: CI=100ms, Latency=9 → 有效间隔 = 1000ms，从机每秒只唤醒一次。

---

## 功耗计算公式

### 广播态功耗

```
I_avg = (I_active × T_active + I_sleep × T_sleep) / T_interval

其中:
- I_active: 广播事件平均电流 (4.0~4.6 mA，根据睡眠模式)
- T_active: 广播事件持续时间 (5.8~8.3 ms，根据睡眠模式)
- I_sleep: 睡眠电流 (15µA Sleep 或 2µA PowerOff)
- T_sleep: 睡眠持续时间 = T_interval - T_active
- T_interval: 广播间隔
```

**示例计算** (Sleep 模式, 广播间隔 100ms):
```
I_avg = (4600µA × 5.8ms + 15µA × 94.2ms) / 100ms
      = (26680 + 1413) / 100
      = 280.93 µA
```

### 连接态功耗

```
I_avg = (I_active × T_active + I_sleep × T_sleep) / Effective_CI

其中:
- I_active: 连接事件平均电流 (3.0~3.25 mA，根据睡眠模式)
- T_active: 连接事件持续时间 (4.7~7.5 ms，根据睡眠模式)
- I_sleep: 睡眠电流 (15µA Sleep 或 2µA PowerOff)
- T_sleep: 睡眠持续时间 = Effective_CI - T_active
- Effective_CI: 有效连接间隔 = CI × (1 + Latency)
```

**示例计算** (PowerOff 模式, CI=100ms, Latency=9):
```
Effective_CI = 100 × (1 + 9) = 1000ms
I_avg = (3000µA × 7.5ms + 2µA × 992.5ms) / 1000ms
      = (22500 + 1985) / 1000
      = 24.49 µA
```

---

## 执行步骤

### Step 1: 读取项目配置

用 Glob 在项目目录下动态查找源文件，再用 Grep 定位包含 BLE 参数宏的文件：

```
Glob: <project_path>/src/*.c  /  <project_path>/inc/*.h
Grep: APP_ADV_INT|APP_CONN_INT|APP_CONN_LATENCY|APP_CONN_TIMEOUT|APP_ADV_DURATION
```

提取关键参数（单位换算见括号）:
```c
APP_ADV_INT_MIN / APP_ADV_INT_MAX   // 广播间隔 (× 0.625ms = 实际 ms)
APP_CONN_INT_MIN / APP_CONN_INT_MAX // 连接间隔 (× 1.25ms = 实际 ms)
APP_CONN_LATENCY                    // 从机延迟 (无单位)
APP_CONN_TIMEOUT                    // 超时时间 (× 10ms = 实际 ms)
APP_ADV_DURATION                    // 广播超时 (× 10ms = 实际 ms)
```

若未找到宏定义，提示用户手动提供参数值后继续计算。

### Step 2: 识别睡眠模式

检查代码中的睡眠模式配置:
- `pmu_sleep_enter()` → Sleep 模式
- `pmu_poweroff_enter()` → PowerOff 模式

### Step 3: 计算各状态功耗

计算:
1. **广播态平均电流** - 根据广播间隔和睡眠模式
2. **连接态平均电流** - 根据连接参数和睡眠模式
3. **待机态电流** - 深度睡眠时的静态电流

### Step 4: 估算电池续航

```
续航时间 (小时) = 电池容量 (mAh) / 平均电流 (mA) × 效率系数
续航天数 = 续航时间 / 24
```

效率系数通常取 0.7-0.8 (考虑电池老化、温度等因素)

### Step 5: 优化建议

根据应用场景推荐配置:

| 应用场景 | 连接间隔 | 从机延迟 | 广播间隔 | 预期续航 |
|----------|----------|----------|----------|----------|
| HID 键盘/鼠标 | 30-100ms | 4-10 | 100-500ms | 数月 |
| 传感器 (低频) | 100-1000ms | 0-9 | 1-10s | 数年 |
| 信标 (Beacon) | N/A | N/A | 1-10s | 数年 |
| 实时数据传输 | 7.5-30ms | 0 | 20-100ms | 数天 |
| 智能手表 | 30-100ms | 2-5 | 100-500ms | 数周 |

---

## 输出格式

使用以下格式生成报告（以实际计算数值替代示例值）:

```markdown
# B6x BLE 功耗分析报告

**项目**: bleHid_Uart | **分析时间**: 2026-03-23

## 当前配置

| 参数 | 当前值 | 单位 | 建议范围 |
|------|--------|------|----------|
| 广播间隔 | 160 | ms | 20-10240 |
| 连接间隔 | 30 | ms | 7.5-4000 |
| 从机延迟 | 4 | - | 0-499 |
| 超时时间 | 5000 | ms | 100-32000 |
| 睡眠模式 | Sleep | - | Sleep/PowerOff |

## 功耗计算

| 状态 | 平均电流 | 计算过程 |
|------|----------|----------|
| 广播态 | 281.0 µA | (4600×5.8 + 15×154.2) / 160 = 281.0 µA |
| 连接态 | 548.7 µA | Eff_CI=150ms(30×5); (3250×4.7+15×145.3)/150 = 548.7 µA |
| 待机态 | 15.0 µA | 睡眠电流 |

## 电池续航估算

场景: 8h/天连接, 16h/天待机

| 状态 | 时间占比 | 平均电流 |
|------|----------|----------|
| 广播 | 5% | 281.0 µA |
| 连接 | 33% | 548.7 µA |
| 待机 | 62% | 15.0 µA |

综合平均电流: 197.4 µA
电池: 200 mAh | 效率: 0.75
**续航: 200 / 0.1974 × 0.75 = 760 小时 ≈ 31.7 天**

## 优化建议

1. **[高]** 增加从机延迟: 4→9, 预期连接态电流降 ~50%, 续航+15天
2. **[中]** 切换 PowerOff 模式: Sleep→PowerOff, 待机电流-87%, 续航+5天 (需评估唤醒时间)
3. **[低]** 增加广播间隔: 160→500ms, 广播态电流-60% (增加连接延迟)

## 注意事项

- 理论值参考，实际需电流表测量
- iOS: CI 30-120ms, Latency ≤ 30; Android 各厂商有差异
- 定向广播功耗是普通广播的 2-3 倍
```

---

## 注意事项

1. **理论值 vs 实际值**: 计算结果为理论参考，实际功耗需用电流表测量
2. **平台限制**: iOS/Android 对 BLE 参数有不同限制，优化时需考虑兼容性
3. **响应速度权衡**: 低功耗通常意味着更高的延迟，需平衡用户体验
4. **温度影响**: 低温环境下电池容量会下降，需留有余量
5. **唤醒时间**: PowerOff 模式唤醒时间比 Sleep 长，需评估是否满足实时性要求
