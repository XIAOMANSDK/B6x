---
name: b6-build
description: |
  编译 B6x SDK 工程 (Keil/GCC 自动检测)，解析编译日志，迭代修复错误和警告直到 0 ERROR 且 WARNING ≤ 5。

  TRIGGER when: 用户要求编译、build、构建项目，修复编译错误/警告，检查编译结果，或提及 Keil/GCC 编译问题。

  DO NOT TRIGGER when: 用户要求烧录/调试/下载固件（用 /b6-auto-debug）、纯代码审查、BLE协议问题、无硬件连接。
user-invocable: true
allowed-tools: Read, Grep, Glob, Edit, Write, Bash
---

# B6 Build

编译 B6x SDK 工程，自动检测工具链 (Keil/GCC)，解析编译日志，迭代修复错误和警告，直到达到编译标准。

## 触发

`/b6-build [project_path] [--toolchain keil|gcc]`

**参数**:
- `project_path`: 项目路径，默认当前目录
- `--toolchain`: 强制指定工具链，默认自动检测

## 编译标准

| 指标 | 要求 | 说明 |
|------|------|------|
| ERROR | **0** | 必须无错误 |
| WARNING | **≤ 5** | 警告数量不超过 5 个 |

## 红旗 — 看到这些想法时停止

| 想法 | 为什么禁止 |
|------|-----------|
| "加 `-w` 或 `--no-warnings`" | 隐藏问题不等于解决问题 |
| "用 `#pragma` 全局禁用警告" | 掩盖真实的代码缺陷 |
| "WARNING 不重要，能编译就行" | 嵌入式代码中警告常是潜在 bug |
| "这个 WARNING 修不了，跳过" | 必须≤5 条达标才算成功 |

**禁止**: 通过编译选项全局禁用警告。个别抑制必须有注释说明原因。

---

## Step 1: 检测工具链

### 自动检测逻辑

```
1. 查找 *.uvprojx → Keil 工程存在
2. 查找 gnu/CMakeLists.txt → GCC 工程存在
3. 若两者都存在 → 优先 Keil
4. 若都不存在 → 报错退出
```

### 检测方法

使用 Glob 工具（不用 find 命令）：

```
Glob pattern: <project_path>/mdk/*.uvprojx      → Keil 工程
Glob pattern: <project_path>/gnu/CMakeLists.txt → GCC 工程
```

---

## Step 2: 执行编译

### Keil 编译

```bash
"C:/Keil_v5/UV4/UV4.exe" -r "<project>.uvprojx" -t "<target>" -j0 -o"build_log.txt"
```

**获取 target 名称**：读取 uvprojx 文件，搜索 `<TargetName>` 标签取第一个值。

### GCC 编译

进入项目的 `gnu/` 目录执行（项目名已在 CMakeLists.txt 的 `project()` 中定义，无需额外参数）：

```bash
cd <project_path>/gnu
cmake -B build -G Ninja
cmake --build build
```

输出产物位于 `<project_path>/gnu/build/bin/` 下。

---

## Step 3: 解析日志

### Keil 错误模式

```regex
ERROR:   ".+\.c"\((\d+)\): error #\d+: .+
WARNING: ".+\.c"\((\d+)\): warning #\d+: .+
```

### GCC 错误模式

```regex
ERROR:   ".+\.c":(\d+):\d+: error: .+
WARNING: ".+\.c":(\d+):\d+: warning: .+
LINKER:  undefined reference to .+
```

---

## Step 4: 修复问题

### ERROR 修复

- 缺少头文件: 包含对应 `drivers/api/*.h` 或 `ble/api/*.h`
- 链接错误: Keil → 检查工程文件 Source Groups; GCC → 检查 CMakeLists.txt 的 target_sources

### WARNING 修复优先级

1. **Critical (ERROR)**: 必须立即修复
2. **High (WARNING > 5)**: 需要修复以达标
3. **Medium (WARNING ≤ 5)**: 可选修复

### WARNING 修复顺序

移除无用代码 → 显式类型转换 → `(void)var` 抑制 → 最后才考虑 `#pragma`（禁止全局禁用）

---

## 最大迭代次数

为防止无限循环，设置最大迭代次数：**5 次**

---

## 退出条件

| 结果 | 条件 | 操作 |
|------|------|------|
| ✅ 成功 | 达到编译标准 | 报告成功，**使用 `/b6_verification-before-completion` 证据模板输出** |
| ⚠️ 部分成功 | ERROR=0 且 WARNING>5 | 报告警告详情，询问是否继续修复 |
| ❌ 失败 | 迭代5次后仍有 ERROR | 报告剩余问题，建议用户手动检查 |
| ❌ 失败 | 无法检测工具链 | 提示用户检查工程文件 |

**退出时必须输出编译证据**（无论成功或失败）：
```
编译验证证据：
   工具链: {Keil MDK / GCC}
   输出: {N} Error(s), {M} Warning(s)
   产物: {文件路径}
   退出码: {0 / 非0}
```

---

## 修复示例

**错误**: `error: #20: identifier "B6x_UART_Init" is undefined`

**修复**: 包含项目头文件 `#include "drivers/api/uart.h"`

---
