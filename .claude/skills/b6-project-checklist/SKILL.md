---
name: b6-project-checklist
description: |
  B6x SDK 项目完整性验证工具，检查项目结构、配置文件、工具链配置的完整性。

  TRIGGER when: 用户要求检查项目完整性、项目检查清单、checklist、验证项目结构、检查工程配置、项目验收、发布前检查、检查必需文件、验证项目设置。即使用户没有明确说"项目检查"，只要涉及验证 B6x 项目的目录结构、必需文件、工具链配置是否完整，也应使用此 skill。此 skill 专注于"项目结构完整性"而非代码质量或硬件约束验证。

  DO NOT TRIGGER when: 用户要求代码审查（用 /b6-code-review）、硬件配置验证（用 /b6-validate-hardware）、编译构建（用 /b6-build）、烧录调试（用 /b6-auto-debug）。这些 skill 有不同的关注点：code-review 关注代码质量和安全性，validate-hardware 关注引脚/时钟/DMA 约束，build 关注编译错误，而 checklist 关注"项目是否具备所有必需的文件和配置"。
user-invocable: true
allowed-tools: Read, Grep, Glob
---

# B6 Project Checklist

验证 B6x SDK 项目结构和配置完整性，确保项目具备所有必需的文件和配置。

**核心定位**: 此 skill 检查"项目结构完整性"——文件是否存在、配置是否定义。不检查代码质量（用 `/b6-code-review`）或硬件约束（用 `/b6-validate-hardware`）。

## 触发

`/b6-project-checklist [project_path]`

**参数**:
- `project_path`: 项目路径，默认当前目录


## 工具链检测

自动检测项目支持的工具链：
1. 查找 `mdk/*.uvprojx` → Keil 工程
2. 查找 `gnu/CMakeLists.txt` → GCC 工程
3. 两者都存在 → 双工具链项目

---

## 验证项

### 1. 目录结构

| 目录 | Keil | GCC | 说明 |
|------|------|-----|------|
| `mdk/` | ✓ | - | Keil 工程目录 |
| `gnu/` | - | ✓ | GCC/CMake 工程目录 |
| `src/` | ✓ | ✓ | 源代码 |
| `inc/` | ✓ | ✓ | 头文件 |
| `doc/` | 可选 | 可选 | 文档（建议有） |

**为什么重要**: 标准化的目录结构便于团队协作和 CI/CD 集成。

### 2. 必需文件

| 文件 | Keil | GCC | 说明 |
|------|------|-----|------|
| `mdk/*.uvprojx` | ✓ | - | Keil 工程文件 |
| `gnu/CMakeLists.txt` | - | ✓ | CMake 配置 |
| `src/main.c` | ✓ | ✓ | 主程序入口 |
| `inc/cfg.h` | ✓ | ✓ | 项目配置文件 |

**为什么重要**: 缺少必需文件会导致编译失败或配置错误。

### 2.1 GCC 特定文件（SDK 公共，相对于 SDK 根目录检查）

| 文件 | 说明 |
|------|------|
| `{SDK_ROOT}/core/gnu/startup_gnu.S` | GCC 启动文件 |
| `{SDK_ROOT}/core/gnu/link_xip.ld` | XIP 链接脚本 |

检查方式：在 `gnu/CMakeLists.txt` 中搜索 `startup` 和 `link_xip` 的引用是否存在，而非直接检查绝对路径。

### 3. cfg.h 配置定义

**通用（所有项目）**:

| 设置 | 检查内容 | 为什么重要 |
|------|----------|------------|
| `SYS_CLK` | 已定义 | 系统时钟源选择 |
| `DBG_MODE` | 已定义 | 调试输出配置 |

**仅 BLE 项目**（CMakeLists.txt 或 uvprojx 中链接了 BLE 库时才检查）:

| 设置 | 检查内容 | 为什么重要 |
|------|----------|------------|
| `BLE_DEV_NAME` | 已定义且非空 | 蓝牙设备标识 |
| `BLE_ADDR` | 已定义 6 字节地址 | 设备唯一标识 |
| `BLE_LITELIB` 或 `BLE_LARGELIB` | 已定义 | BLE 库选择影响 RAM 使用 |
| `BLE_NB_SLAVE` | 已定义且 > 0 | 从机连接数配置 |

**注意**: 此 skill 只检查配置是否已定义。数值有效性和硬件约束由 `/b6-validate-hardware` 验证。

### 4. main.c 结构检查

| 检查项 | 说明 |
|--------|------|
| 系统初始化函数 | 存在 `sysInit()` 函数定义 |
| 设备初始化函数 | 存在 `devInit()` 函数定义 |
| 主循环结构 | 包含 `while(1)` 循环 |
| 头文件包含 | 包含 `cfg.h` |

**注意**: 代码质量（返回值检查、错误处理）由 `/b6-code-review` 深入检查。

### 5. 工具链特定检查

#### Keil 项目

| 检查项 | 说明 |
|--------|------|
| `startup.s` 存在 | 启动文件 |
| Scatter File 存在 | `.sct` 链接脚本 |

#### GCC 项目

| 检查项 | 说明 |
|--------|------|
| CMakeLists.txt 语法 | 基本语法正确 |
| 项目名称定义 | `project(...)` 存在 |
| 源文件列表 | `add_executable` 包含源文件 |
| 链接库配置 | `target_link_libraries` 存在 |

---

## 执行步骤

### 阶段一：结构完整性检查

1. **检测工具链**: 查找 `mdk/*.uvprojx` 和 `gnu/CMakeLists.txt`，确定项目类型（Keil / GCC / Both）
2. **检查目录和文件**: 用 Glob 逐项检查目录存在性，记录缺失项
3. **验证 cfg.h 定义**: 用 Grep 检查必需宏定义（通用 + BLE 项目额外项）
4. **检查 main.c 结构**: 验证 `sysInit()` / `devInit()` / `while(1)` / `#include "cfg.h"`
5. **工具链特定检查**: Keil（startup.s / .sct）/ GCC（CMakeLists.txt 语法 / project / add_executable / target_link_libraries）
6. **输出结构检查报告**: 包含通过/警告/错误统计

### 阶段二：强制深度验证（结构检查通过后必须执行）

> 以下三项为项目验收的**强制步骤**，不可跳过。每一项依赖前一项通过。

7. **执行 `/b6-code-review [project_path]`**: 代码质量审查（MISRA C / 嵌入式最佳实践 / BLE 规范 / 安全性）
8. **执行 `/b6-sram-analyze [project_path]`**: SRAM 用量分析（RAM 分区 / 栈溢出风险 / BLE 库内存占用）
9. **执行 `/b6-validate-hardware [project_path]`**: 硬件配置验证（引脚冲突 / 时钟 / DMA / 中断 / Timer 约束）

### 执行规则

- **阶段一有错误时**: 输出报告并列出错误，提示用户修复后重新运行。阶段二**不执行**。
- **阶段一全部通过**: 自动依次执行阶段二的三个强制验证 skill。
- **阶段二中任一步骤失败**: 输出失败原因，提示修复后从失败步骤重新执行。
- **全部通过**: 输出最终验收报告。

---

## 输出格式

### 阶段一报告

```
B6 Project Checklist — {项目名} — {工具链}

目录结构: [x] mdk/ [x] gnu/ [x] src/ [x] inc/ [ ] doc/ (可选)
必需文件: [x] 按工具链列出
cfg.h 配置: [x] 逐项列出已定义宏
main.c 结构: [x] sysInit / devInit / while(1)
工具链特定: [x] 按工具链列出

统计: N/M 通过 | X 建议 | Y 错误
建议: {具体建议列表}
```

### 阶段二报告（每项完成后追加）

```
── 阶段二：深度验证 ──
[1/3] /b6-code-review     : [PASS/FAIL] {摘要}
[2/3] /b6-sram-analyze    : [PASS/FAIL] {摘要}
[3/3] /b6-validate-hardware: [PASS/FAIL] {摘要}

最终验收: [PASS] 项目具备发布条件 / [FAIL] 请修复上述问题
```

## 注意事项

1. **检查范围**: 阶段一专注"文件存在性"和"配置定义"，不深入验证数值正确性
2. **快速检查**: 作为发布前的第一道检查，快速发现结构性问题
3. **强制流程**: 阶段二三项验证为项目验收的必经步骤，确保代码质量、内存安全、硬件配置三位一体验证
4. **工具链**: 阶段二中的 skill 自动检测 Keil/GCC 工具链，无需额外参数
