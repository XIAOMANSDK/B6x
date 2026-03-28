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

---

## Step 1: 检测工具链

### 自动检测逻辑

```
1. 查找 *.uvprojx → Keil 工程存在
2. 查找 gnu/CMakeLists.txt → GCC 工程存在
3. 若两者都存在 → 优先 Keil
4. 若都不存在 → 报错退出
```

### 检测命令

```bash
# Keil 工程
find <project_path> -name "*.uvprojx" -type f 2>/dev/null | head -1

# GCC 工程
find <project_path> -path "*/gnu/CMakeLists.txt" -type f 2>/dev/null | head -1
```

---

## Step 2: 执行编译

### Keil 编译

```bash
"C:\Keil_v5\UV4\UV4.exe" -r "<project>.uvprojx" -t "<target>" -j0 -o"build_log.txt"
```

### GCC 编译

```bash
cd D:\svn\bxx_DragonC1\sdk6
cmake -B build -G Ninja -DTARGET_NAME=<project_name>
cmake --build build
```

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

### ERROR 修复策略

| 错误类型 | 常见原因 | 修复方法 |
|----------|----------|----------|
| 语法错误 | 缺少分号、括号不匹配 | 检查语法，添加缺失符号 |
| 未定义符号 | 变量/函数未声明 | 添加声明或包含头文件 |
| 类型错误 | 类型不匹配 | 添加显式类型转换 |
| 链接错误 | 缺少库/源文件 | Keil: 检查工程配置; GCC: 检查 CMakeLists.txt |
| 找不到头文件 | include 路径未配置 | Keil: Options → C/C++; GCC: target_include_directories |

### WARNING 修复策略

| 警告类型 | 常见原因 | 修复方法 |
|----------|----------|----------|
| 未使用变量 | 声明但未使用 | 移除变量或 `(void)var;` |
| 隐式转换 | 类型不匹配 | 添加显式类型转换 |
| 未初始化变量 | 变量未初始化 | 添加初始化值 |
| 不可达代码 | 死代码 | 移除或重构 |
| 严格别名 (GCC) | 指针类型转换 | 使用 union 或 memcpy |

### 修复优先级

1. **Critical (ERROR)**: 必须立即修复
2. **High (WARNING > 5)**: 需要修复以达标
3. **Medium (WARNING ≤ 5)**: 可选修复

---

## 最大迭代次数

为防止无限循环，设置最大迭代次数：**5 次**

---

## 工具链对照

| 特性 | Keil | GCC |
|------|------|-----|
| 构建系统 | uvprojx | CMake + Ninja |
| 编译器 | armcc/armclang | arm-none-eabi-gcc |
| 链接脚本 | .sct | .ld |
| 启动文件 | startup.s | startup_gnu.S |
| 调试 | ULINK/J-Link | GDB + J-Link |
| CI/CD | 有限支持 | 原生支持 |

---

## 与其他 Skill 协作

| Skill | 协作方式 |
|-------|----------|
| `/b6-code-review` | 编译前进行代码审查 |
| `/b6-validate-hardware` | 编译前验证硬件配置 |
| `/b6-project-checklist` | 编译前检查项目完整性 |
| `/b6-cmake-init` | 为现有项目生成 CMakeLists.txt (GCC) |
| `/b6-translate-error` | 翻译 BLE 相关错误码 |

---

## 退出条件

| 结果 | 条件 | 操作 |
|------|------|------|
| ✅ 成功 | ERROR=0 且 WARNING≤5 | 报告成功，输出文件位置 |
| ⚠️ 部分成功 | ERROR=0 且 WARNING>5 | 报告警告详情，询问是否继续修复 |
| ❌ 失败 | 迭代5次后仍有 ERROR | 报告剩余问题，建议用户手动检查 |
| ❌ 失败 | 无法检测工具链 | 提示用户检查工程文件 |

---

## 修复示例

### 示例 1: 未定义符号

**错误**: `error: #20: identifier "B6x_UART_Init" is undefined`

**诊断**: 缺少头文件包含

**修复前**:
```c
void uart_test(void) {
    B6x_UART_Init(UART1, &cfg);  // error: undefined
}
```

**修复后**:
```c
#include "drivers/api/uart.h"  // 添加头文件

void uart_test(void) {
    B6x_UART_Init(UART1, &cfg);  // OK
}
```

### 示例 2: 类型不匹配 (GCC)

**错误**: `warning: implicit conversion from 'float' to 'int'`

**修复前**:
```c
int value = 3.14f;  // warning: implicit conversion
```

**修复后**:
```c
int value = (int)3.14f;  // explicit cast
```

### 示例 3: 未使用变量

**错误**: `warning: unused variable 'temp'`

**修复方案 A** (移除):
```c
// 删除未使用的变量
```

**修复方案 B** (抑制):
```c
int temp;
(void)temp;  // 明确标记为有意未使用
```

---
