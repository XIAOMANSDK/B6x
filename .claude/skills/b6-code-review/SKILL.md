---
name: b6-code-review
description: |
  B6x SDK 代码审查工具，检查 MISRA C 合规性、嵌入式最佳实践、BLE 规范和安全性问题。

  TRIGGER when: 用户要求代码审查、code review、检查代码质量、检查 MISRA 合规性、检查嵌入式代码规范、检查 BLE 代码、安全审查、代码评审、看看代码有没有问题、检查这段代码。即使用户没有明确说"代码审查"，只要涉及检查 B6x 嵌入式代码的质量、安全性或规范性，也应使用此 skill。

  DO NOT TRIGGER when: 用户要求编译构建（用 /b6-build）、烧录调试（用 /b6-auto-debug）、硬件配置验证（用 /b6-validate-hardware）、创建新项目（用 /b6-create-project）、纯 BLE 协议问题咨询。
user-invocable: true
allowed-tools: Read, Grep, Glob, Agent, Bash, mcp__b6x-mcp-server__search_sdk, mcp__b6x-mcp-server__inspect_node
---

# B6 Code Review

B6x SDK 代码审查工具，使用并行多 Agent 架构，检查 MISRA C 合规性、嵌入式最佳实践、BLE 规范和安全性问题，并通过置信度评分过滤误报。

## 触发

`/b6-code-review <file_or_directory>`

**参数**:
- 单文件: `/b6-code-review projects/bleHid/src/main.c`
- 目录: `/b6-code-review projects/bleHid/src/`
- 项目: `/b6-code-review projects/bleHid`

---

## 执行流程

### 红旗 — 看到这些想法时停止

| 想法 | 为什么禁止 |
|------|-----------|
| "只检查 Critical，跳过 Warning" | Warning 如 volatile 遗漏在嵌入式系统中会导致实际 bug |
| "MISRA 规则太多了，挑几个查" | 必须执行全部 M01-M10 检查，置信度过滤会处理误报 |
| "这个文件没改，不用查" | Step 0 预检会判断，不应手动跳过 |
| "只做代码风格检查就够了" | 必须启动全部 5 个 Agent 并行扫描 |
| "builder 说已经修好了" | 必须独立读代码验证，不接受自报结果 |
| "审查报告出来了，直接全部修复" | 必须先验证每个 issue 再动手（Step 6 自动执行） |

### Step 0: 预检 (Haiku Agent)

跳过以下情况，直接告知用户无需审查：
- 自动生成的文件：`mdk/output/*`、`*.uvprojx`、`*.bin`、`*.hex`、`*.map`
- 预编译库：`ble/lib/*.lib`
- 仅包含注释或空白行的修改
- 文件不存在或路径无效

### Step 1: 定位目标

1. 使用 Glob 获取范围内所有 `.c` 和 `.h` 文件列表
2. 按文件大小排序，识别核心文件（应用层、驱动、BLE Profile）
3. 记录文件总数和行数

### Step 1.5: 规范合规审查（条件触发）

**触发条件**: 目标目录存在 `docs/spec.md` 或 `docs/plan.md` 时自动执行。不存在则跳过此步骤。

**目的**: 验证代码是否做了要求的事（不多不少），在进入代码质量审查前先确保方向正确。

启动独立 Agent 读取 spec.md/plan.md，对比实际代码：

| 检查项 | 说明 |
|--------|------|
| 缺失功能 | spec 要求但代码未实现的功能 |
| 多余功能 | 代码实现了但 spec 未要求的功能 |
| 需求偏差 | 功能实现了但与 spec 描述不一致 |

**如果规范合规失败，报告问题后停止，不进入后续代码质量审查。** 先修正方向再谈质量。

### Step 2: 并行多 Agent 扫描

**同时启动 5 个 Agent**（独立执行，互不等待）：

#### Agent A — MISRA C + 代码风格
检查规则 M01-M10、S01-S07（见下方审查类别）。返回每个问题的行号、规则 ID、描述。

#### Agent B — 嵌入式缺陷 + 安全性 
检查规则 E01-E08、C01-C05。重点关注 ISR、volatile、指针安全、缓冲区溢出。返回每个问题的行号、规则 ID、描述。

#### Agent C — BLE 规范 + MCP API 验证
检查规则 B01-B05。对代码中出现的 SDK API 调用：
1. 使用 `search_sdk(api_name)` 查询正确用法
2. 使用 `inspect_node(node_id)` 确认参数要求
3. 对比实现与 API 规范，报告不符之处

#### Agent D — 代码简化审查 (via /simplify)
使用 `/simplify` skill 对目标文件进行代码审查，检查代码复用性、质量和效率问题。`/simplify` 会分析代码结构、识别冗余模式、建议简化方案。返回每个发现的问题描述、行号和优化建议。

#### Agent E — PR 级代码审查 (via /code-review)
使用 `/code-review` skill 对目标文件进行 PR 级代码审查，检查 bug、安全漏洞、逻辑错误和代码质量问题。返回按置信度过滤的高优先级问题列表。


### Step 3: 置信度评分

对 Step 2 汇总的每个 Issue，启动独立 Haiku Agent 评分（0-100）：

| 分数 | 含义 |
|------|------|
| 0 | 误报，代码上下文证明不存在问题，或为预先存在的问题 |
| 25 | 可能是问题，但无法确认；风格问题未在规范中明确提及 |
| 50 | 可以确认是问题，但属于细节或极少发生 |
| 75 | 高置信度，很可能在实践中触发；明确违反规范 |
| 100 | 确定性问题，频繁触发，有直接证据 |

**评分注意**：
- 检查问题是否已在代码中通过注释显式豁免（如 `/* MISRA-IGNORE */`）
- 对比规范文档，确认规则确实适用于该场景
- 预先存在于未修改代码中的问题评 0 分

### Step 4: 过滤

**仅保留 score ≥ 75 的问题。** 若无满足条件的问题，报告 "✅ No issues found"。

### Step 5: 生成报告

**验证门控**: 报告数据必须来自 Step 2-4 的实际执行结果，禁止推断或猜测。完成后使用 `/b6_verification-before-completion` 的代码审查证据模板输出验证结果。在生成报告前确认：
- Step 2 的 Agent 是否全部返回了结果？
- Step 3 的评分是否基于实际代码上下文？
- Step 4 的过滤是否正确执行？

按下方输出格式汇总，按严重性排序。

### Step 6: 接收处理（自动衔接）

报告生成后，自动进入 Receiving 阶段。使用 `b6-review-reception` skill 处理审查结果：
验证每个 issue → 按优先级修复 → 复审闭环。

---

## 审查类别

### 1. MISRA C 合规性

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| M01 | 固定宽度类型 | 必须使用 `uint8_t`/`int16_t`，禁止裸 `int`/`unsigned` | 🔴 Critical |
| M02 | 变量初始化 | 所有局部变量必须显式初始化 | 🔴 Critical |
| M03 | const 修饰 | 只读指针参数必须用 `const` | 🟡 Warning |
| M04 | static 限制 | 仅内部使用的函数/变量必须 `static` | 🟡 Warning |
| M05 | 参数数量 | 参数 > 4 个需用结构体封装 | 🟡 Warning |
| M06 | 禁止动态内存 | 禁止 `malloc`/`free`/`calloc`/`realloc` | 🔴 Critical |
| M07 | 禁止递归 | 禁止递归函数调用 | 🔴 Critical |
| M08 | 禁止 goto | 禁止 `goto` 语句 | 🟡 Warning |
| M09 | 显式类型转换 | 禁止隐式类型转换 | 🟡 Warning |
| M10 | 返回值检查 | SDK 函数返回值必须检查 | 🔴 Critical |

### 2. 嵌入式专用检查

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| E01 | ISR 简短性 | ISR ≤ 50 行 | 🟡 Warning |
| E02 | ISR 非阻塞 | ISR 内禁止 `while` 等待、延时调用 | 🔴 Critical |
| E03 | volatile 使用 | ISR 与主程序共享变量必须 `volatile` | 🔴 Critical |
| E04 | 寄存器访问 | 硬件寄存器指针必须 `volatile` | 🔴 Critical |
| E05 | 位操作宏 | 使用 `BIT_SET`/`BIT_CLEAR`/`BIT_TOGGLE` | 🟡 Warning |
| E06 | 空闲等待 | 空闲循环应调用 `__WFI()` 或 `__WFE()` | 💡 Info |
| E07 | 大数组栈分配 | 局部数组 > 256 字节应 `static` 或全局 | 🟡 Warning |
| E08 | 未使用变量 | 未使用的变量应移除 | 💡 Info |

### 3. BLE 规范检查

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| B01 | UUID 备案 | 自定义 UUID 必须在 MEMORY.md 记录 | 🟡 Warning |
| B02 | 连接参数 | Interval 7.5-100ms, Latency 0-4, Timeout 1000-6000ms | 🟡 Warning |
| B03 | Profile 完整性 | 服务必须有 init/read/write/notify 回调 | 🟡 Warning |
| B04 | BLE 宏定义 | 使用 `BLE_NB_SLAVE` 而非硬编码连接数 | 💡 Info |
| B05 | 事件处理 | 所有 BLE 事件必须有处理分支 | 🟡 Warning |

### 4. 代码风格检查

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| S01 | 命名规范 | 文件小写下划线，函数模块_功能 | 💡 Info |
| S02 | 全局变量前缀 | 全局变量使用 `g_` 前缀 | 💡 Info |
| S03 | 宏命名 | 宏使用大写+下划线 | 💡 Info |
| S04 | 缩进 | 4 空格，无 Tab | 💡 Info |
| S05 | 行宽 | ≤ 100 字符 | 💡 Info |
| S06 | Doxygen 注释 | 函数应有 `@brief` `@param` `@return` | 💡 Info |
| S07 | 文件头注释 | 文件应有 `@file` `@brief` | 💡 Info |

### 5. 安全性检查

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| C01 | 数组边界 | 数组访问必须有边界检查 | 🔴 Critical |
| C02 | 空指针检查 | 指针使用前必须检查非空 | 🔴 Critical |
| C03 | 整数溢出 | 算术运算需检查溢出 | 🟡 Warning |
| C04 | memcpy 长度 | `memcpy` 目标缓冲区必须足够大 | 🔴 Critical |
| C05 | 格式化字符串 | 禁止用户输入作为格式化字符串 | 🔴 Critical |

### 6. CMake 配置检查 (GNU)

仅当目标目录包含 `gnu/CMakeLists.txt` 时执行。

| ID | 检查项 | 规则 | 严重性 |
|----|--------|------|--------|
| K01 | cmake 版本 | `cmake_minimum_required(VERSION 3.20.0+)` | 🟡 Warning |
| K02 | 项目名称 | `project()` 名称与目录一致 | 🟡 Warning |
| K03 | SDK_ROOT | 正确设置相对路径 | 🔴 Critical |
| K04 | sdk.cmake | 必须包含 `include(sdk.cmake)` | 🔴 Critical |
| K05 | 源文件路径 | `aux_source_directory` 路径正确 | 🔴 Critical |
| K06 | 驱动库链接 | `target_link_libraries` 包含 `${DRVS_LIB}` | 🔴 Critical |
| K07 | BLE 库链接 | BLE 项目必须包含 `${BLE_LIB}` | 🔴 Critical |
| K08 | 输出生成 | 必须调用 `generate_project_output` | 🟡 Warning |

---

## 误报排除规则

以下情况**评分为 0，直接跳过**：

- 预编译库文件 (`ble/lib/`) 不检查 MISRA
- 带 `/* MISRA-IGNORE */` 或 `/* NOLINT */` 注释的行跳过对应规则
- `drivers/src/` 内部实现中使用 `int` 类型视为合规（SDK 内部约定）
- ISR 中调用 BLE 状态查询 API（如 `app_env_get`）不算 E02 违规
- 问题存在于**未修改的代码行**（仅审查变更范围）
- 历史代码中长期存在且未被本次修改影响的问题

---

## 严重性级别

| 级别 | 标识 | 处理 |
|------|------|------|
| 🔴 Critical | 必须修复，可能导致系统崩溃或安全问题 | 阻断 |
| 🟡 Warning | 建议修复，影响质量或可维护性 | 警告 |
| 💡 Info | 可选修复，代码风格建议（仅 score ≥ 75 时输出）| 提示 |

---

## 输出格式

单文件报告：
```
B6x Code Review Report — {文件路径} — {日期}
状态: {⚠️ N Issues Found / ✅ No issues found}

🔴 Critical (N) — 表格: 行号 | ID | 置信度 | 问题 | 修复建议
🟡 Warning (N)  — 同上格式
💡 Info (N)     — 同上格式（仅 score ≥ 75）

修复优先级: 按级别+置信度降序列出
过滤统计: 发现 N 个潜在问题，过滤 M 个低置信度误报，输出 K 个
```

批量汇总：
```
B6x Code Review Summary — {目录} — {日期}
文件数: N

| 文件 | Critical | Warning | Info | 过滤误报 | 状态 |

需要立即修复: {Critical > 0 的文件}
```

---

## 规范引用

| 规范 | 路径 |
|------|------|
| 代码风格 | `.claude/rules/code-style.md` |
| MCP 指南 | `.claude/rules/mcp-usage.md` |
