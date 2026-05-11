# ClaudeCode 安装配置以及使用说明

---

## 目录

1. [Windows 安装方法](#1-windows-安装方法)
2. [Mac 安装方法](#2-mac-安装方法)
3. [配置 GLM 模型方法](#3-配置-glm-模型方法)
4. [工程相关工具及使用方法](#4-工程相关工具及使用方法)

---

## 1. Windows 安装方法

### 1.1 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10/11 |
| Node.js | >= 18.0 |
| Python | >= 3.10 |
| Keil MDK | 5.x (用于编译) |

### 1.2 安装步骤

**Step 1: 安装 Node.js**

```powershell
# 下载并安装 Node.js LTS
# https://nodejs.org/

# 验证安装
node --version
npm --version
```

**Step 2: 安装 Claude Code CLI**

```powershell
# 全局安装 Claude Code
npm install -g @anthropic-ai/claude-code

# 验证安装
claude --version
```

获取API Key: 我们为部分客户提供API KEY，请联系相关销售。

配置Claude Code：
- 编辑或新增 `settings.json` 文件
- MacOS & Linux 为 `~/.claude/settings.json`
- Windows 为`用户目录/.claude/settings.json`
- 新增或修改里面的 env 字段
- 注意替换里面的 `your_zhipu_api_key` 为您上一步获取到的 API Key
  {
    "env": {
      "ANTHROPIC_AUTH_TOKEN": "sk-XXX",
      "ANTHROPIC_BASE_URL": "http://www.xiao-man.com:3000",
      "API_TIMEOUT_MS": "3000000",
      "CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC": "1",
      "ANTHROPIC_DEFAULT_HAIKU_MODEL": "glm-5",
      "ANTHROPIC_DEFAULT_SONNET_MODEL": "glm-5",
      "ANTHROPIC_DEFAULT_OPUS_MODEL": "glm-5.1"
    }
  }
- 再编辑或新增 `.claude.json` 文件
- MacOS & Linux 为 `~/.claude.json`
- Windows 为`用户目录/.claude.json`
- 新增 `hasCompletedOnboarding` 参数
{
  "hasCompletedOnboarding": true
}

**Step 3: 安装 xm_b6_mcp 的 Python 依赖**

```powershell
# 安装 MCP 服务器依赖 参数 -i 指定清华源安装
cd sdk\xm_b6_mcp
pip install -r docs/requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple

# 安装文档提取依赖 docs/requirements.txt已包含
#pip install python-docx openpyxl pymupdf
```

**Step 4: 安装 MCP 工具**

```powershell
# 安装 context7
npm install -g @upstash/context7-mcp

# 安装 sequential-thinking
npm install -g @modelcontextprotocol/server-sequential-thinking
```

**Step 5: 安装 Embedded Debugger MCP (可跳过,已提供embedded-debugger-mcp_B6x)**

Embedded Debugger MCP 是用于 ARM Cortex-M/RISC-V 嵌入式调试的 MCP 服务器，基于 probe-rs，支持 CMSIS-DAP 等调试器。
*** 注意： 使用  DAPLink / CMSIS-DAP 调试器， 采用JLink 会与Keil的USB驱动冲突，不能共存 ***

**系统要求**:

| 项目 | 要求 |
|------|------|
| Rust | >= 1.70 (需要 rustup) |
| probe-rs | >= 0.24 |
| 调试器 | J-Link / ST-Link / CMSIS-DAP |

**安装步骤**:

```powershell
# 1. 安装 Rust (如未安装)
# 下载并运行: https://rustup.rs/
# 或使用 winget:
winget install Rustlang.Rustup

# 验证 Rust 安装
rustc --version
cargo --version

# 2. 安装 probe-rs 工具链 (二选一)
# 方法 A: 官方安装脚本 (推荐)
irm https://github.com/probe-rs/probe-rs/releases/latest/download/probe-rs-tools-installer.ps1 | iex

# 方法 B: 使用 cargo-binstall
cargo install cargo-binstall
cargo binstall probe-rs-tools

# 验证 probe-rs 安装
probe-rs --version

# 3. embedded-debugger-mcp 存放目录
  "sdk\\xm_b6_mcp\\embedded-debugger\\embedded-debugger-mcp_B6x.exe"
```

**配置 .mcp.json**:

```json
{
  "mcpServers": {
    "embedded-debugger": {
      "command": "embedded-debugger-mcp_B6x.exe",
      "args": [],
      "env": {
        "RUST_LOG": "info"
      }
    }
  }
}
```

**主要功能**:

| 功能 | 说明 |
|------|------|
| Flash 烧录 | 通过 DAP-Link/ST-Link 烧录固件 |
| 断点调试 | 设置断点、单步执行 |
| 寄存器查看 | 读取/写入 CPU 和外设寄存器 |
| 内存查看 | 读取/写入内存区域 |
| 变量监视 | 实时监视全局/局部变量 |
| RTT 日志 | 支持 SEGGER RTT 调试输出 |

**Step 6: 配置项目**

```powershell
# 进入 SDK 目录
cd sdk

# 启动 Claude Code
claude
```

### 1.3 项目配置文件

项目级配置位于 `.claude/settings.json`:

```json
{
  "env": {
    "CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS": "1"
  },
  "permissions": {
    "allow": [
      "mcp__b6x-mcp-server__search_sdk",
      "mcp__b6x-mcp-server__inspect_node",
      "mcp__b6x-mcp-server__validate_config"
    ]
  }
}
```

---

## 2. Mac 安装方法

### 2.1 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | macOS 12+ |
| Node.js | >= 18.0 |
| Python | >= 3.10 |
| Homebrew | 推荐 |

### 2.2 安装步骤

**Step 1: 安装 Homebrew (如未安装)**

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**Step 2: 安装 Node.js**

```bash
# 使用 Homebrew 安装
brew install node

# 验证安装
node --version
npm --version
```

**Step 3: 安装 Claude Code CLI**

```bash
# 全局安装 Claude Code
npm install -g @anthropic-ai/claude-code

# 验证安装
claude --version
```

**Step 4: 安装 Python 依赖**

```bash
# 安装 MCP 服务器依赖
cd /path/to/sdk6/xm_b6_mcp
pip3 install -r requirements.txt

# 安装文档提取依赖
pip3 install python-docx openpyxl pymupdf
```

**Step 5: 安装 MCP 工具**

```bash
# 安装 context7
npm install -g @upstash/context7-mcp

# 安装 sequential-thinking
npm install -g @modelcontextprotocol/server-sequential-thinking
```

**Step 6: 配置 MCP 路径**

修改 `.mcp.json` 中的路径为 Mac 格式:

```json
{
  "mcpServers": {
    "b6x-mcp-server": {
      "command": "python3",
      "args": ["/path/to/sdk6/xm_b6_mcp/run.py"],
      "env": {
        "B6X_SDK_PATH": "/path/to/sdk6",
        "B6X_LOG_LEVEL": "INFO"
      }
    }
  }
}
```

---

## 3. 配置 GLM 模型方法

### 3.1 获取 API Key

1. 访问智谱 AI 开放平台: https://open.bigmodel.cn
2. 注册/登录账号
3. 在 API Keys 页面获取 API Key

### 3.2 配置全局设置

编辑 `~/.claude/settings.json` (Windows: `C:\Users\<用户名>\.claude\settings.json`):

```json
{
  "env": {
    "ANTHROPIC_AUTH_TOKEN": "your_api_key_here",
    "ANTHROPIC_BASE_URL": "https://open.bigmodel.cn/api/anthropic",
    "API_TIMEOUT_MS": "3000000",
    "CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC": "1",
    "ANTHROPIC_DEFAULT_HAIKU_MODEL": "glm-5",
    "ANTHROPIC_DEFAULT_SONNET_MODEL": "glm-5",
    "ANTHROPIC_DEFAULT_OPUS_MODEL": "glm-5"
  },
  "permissions": {
    "allow": [
      "WebSearch"
    ]
  }
}
```

### 3.3 配置说明

| 参数 | 说明 |
|------|------|
| `ANTHROPIC_AUTH_TOKEN` | 智谱 API Key |
| `ANTHROPIC_BASE_URL` | 智谱 API 端点 |
| `API_TIMEOUT_MS` | 请求超时时间 (毫秒) |
| `ANTHROPIC_DEFAULT_*_MODEL` | 模型选择 (glm-5/glm-4) |


---

## 4. 工程相关工具及使用方法

### 4.1 工具总览

#### MCP 服务器 (B6x SDK 专用)

| 工具 | 用途 | 调用方式 |
|------|------|----------|
| `search_sdk()` | 搜索 SDK 内容 | `mcp__b6x-mcp-server__search_sdk` |
| `inspect_node()` | 查看节点详情 | `mcp__b6x-mcp-server__inspect_node` |
| `validate_config()` | 验证硬件配置 | `mcp__b6x-mcp-server__validate_config` |

#### Skills (用户可调用命令)

| Skill | 命令 | 用途 |
|-------|------|------|
| 创建项目 | `/b6-create-project` | 7阶段引导式项目创建 |
| 编译工程 | `/b6-keil-build [path]` | 编译 Keil 工程 (0 ERROR, WARN ≤ 5) |
| 代码审查 | `/b6-code-review <path>` | MISRA C/BLE/嵌入式规范检查 |
| 库推荐 | `/b6-library-recommend` | BLE 库选择和 cfg.h 配置 |
| 硬件验证 | `/b6-validate-hardware [path]` | 引脚/时钟/DMA/中断验证 |
| 项目检查 | `/b6-project-checklist [path]` | 项目结构和配置完整性 |
| 错误翻译 | `/b6-translate-error <code>` | BLE 错误码翻译和排查 |
| SRAM 分析 | `/b6-sram-analyze [path]` | MAP 文件 SRAM 使用分析 |
| 功耗分析 | `/b6-power-analyze <path>` | BLE 功耗计算和优化建议 |
| 文档提取 | `/extract-doc <file>` | Word/Excel/PDF/图片 内容提取 |

#### Subagents (自动调用)

| Agent | 用途 | 调用场景 |
|-------|------|----------|
| `b6x-designer` | 架构设计 | 创建项目 Phase 4 |
| `b6x-builder` | 项目构建 | 创建项目 Phase 5 |
| `b6x-explorer` | 代码探索 | 创建项目 Phase 2 |
| `b6x-reviewer` | 代码审查 | 创建项目 Phase 6 |

---

### 4.2 MCP 工具详解

#### search_sdk - SDK 内容搜索

**用途**: 发现 SDK 中的 API、文档、寄存器、示例

**参数**:
- `query`: 搜索关键词
- `scope`: `api` / `docs` / `registers` / `examples` / `all`

**示例**:
```
用户: 搜索 UART 初始化相关的 API

Claude 调用: mcp__b6x-mcp-server__search_sdk("UART init", "api")
```

#### inspect_node - 节点详情查看

**用途**: 查看特定节点的详细信息

**参数**:
- `node_id`: 节点标识 (如 `api:B6x_UART_Init`)
- `view_type`: `summary` / `definition` / `implementation`

**示例**:
```
用户: 查看 B6x_UART_Init 函数的定义

Claude 调用: mcp__b6x-mcp-server__inspect_node("api:B6x_UART_Init", "definition")
```

#### validate_config - 硬件配置验证

**用途**: 验证引脚、时钟、DMA、中断配置是否合规

**参数**:
- `config_json`: 配置 JSON 字符串

**示例 JSON**:
```json
{
  "pins": {
    "PA6": "UART1_TX",
    "PA7": "UART1_RX"
  },
  "clock": {
    "system_clock": 32,
    "enabled_peripherals": ["UART1", "GPIO"]
  },
  "dma": [
    {"channel": 0, "peripheral": "UART1_TX"}
  ],
  "interrupts": [
    {"irq_name": "UART1_IRQn", "priority": 5}
  ]
}
```

---

### 4.3 Skills 使用详解

#### /b6-create-project - 创建新项目

**触发**: `/b6-create-project`

**流程**: 7 阶段引导式开发

| 阶段 | 名称 | 内容 |
|------|------|------|
| 1 | 需求收集 | 类型、名称、外设、BLE 配置 |
| 2 | SDK 探索 | 启动 b6x-explorer Agents 搜索模板 |
| 3 | 需求澄清 | 边界条件、错误处理、性能要求 |
| 4 | 多方案设计 | 启动 b6x-designer Agents 设计方案 |
| 5 | 项目构建 | 启动 b6x-builder Agent 生成文件 |
| 6 | 代码审查 | 启动 b6x-reviewer Agents 检查代码 |
| 7 | 验证总结 | 调用 validate-hardware 和 project-checklist |

**输出**:
- `docs/spec.md` - 需求规格
- `docs/plan.md` - 执行计划
- `mdk/*.uvprojx` - Keil 工程
- `src/main.c` - 主程序
- `inc/cfg.h` - 配置文件

---

#### /b6-keil-build - 编译工程

**触发**: `/b6-keil-build [project_path]`

**标准**:
- ERROR: 0 (必须无错误)
- WARNING: ≤ 5

**流程**:
1. 定位 .uvprojx 文件
2. 执行 Keil 编译
3. 解析编译日志
4. 迭代修复问题 (最多 5 次)
5. 输出结果

**示例输出**:
```
================================================================================
                        ✓ Build Successful
================================================================================

工程: projects/bleHid/mdk/bleHid.uvprojx
Target: Flash
编译时间: 3.2s

输出文件:
  - mdk/output/bleHid.hex
  - mdk/output/bleHid.bin

警告统计: 3 (标准: ≤5)
```

---

#### /b6-code-review - 代码审查

**触发**: `/b6-code-review <file_or_directory>`

**审查类别**:

| 类别 | 检查项 | 严重性 |
|------|--------|--------|
| MISRA C | 固定宽度类型、变量初始化、禁止 malloc | Critical |
| 嵌入式 | ISR 简短性、volatile 使用、位操作宏 | Critical/Warning |
| BLE | UUID 备案、连接参数、Profile 完整性 | Warning |
| 代码风格 | 命名规范、缩进、Doxygen 注释 | Info |
| 安全性 | 数组边界、空指针检查、memcpy 长度 | Critical |

---

#### /b6-library-recommend - BLE 库推荐

**触发**: `/b6-library-recommend [描述]`

**库选择规则**:

| 条件 | 库 | RAM | 角色 |
|------|-----|-----|------|
| 仅从设备 1 个连接 / HID / 传感器 | `ble6_lite.lib` | ~6KB | Slave Only |
| 主从共 2~3 个连接 / 通用应用 | `ble6.lib` | ~14.6KB | Master + Slave |
| 主从共 4-6 连接 / 网关 / Mesh | `ble6_8act_6con.lib` | ~16.2KB | Master + Slave |

**输出**:
- 推荐的 BLE 库
- cfg.h 配置代码
- 资源使用估算

---

#### /b6-validate-hardware - 硬件配置验证

**触发**: `/b6-validate-hardware [project_path]`

**验证项**:

| 类型 | 约束 |
|------|------|
| 引脚 | PA0-PA19 有效，无冲突，ADC 限制 PA0-PA16 |
| 时钟 | 16/32/48/64 MHz |
| DMA | 通道 0-7，无冲突 |
| 中断 | 优先级 0-7 |

**优先引脚分配**:
- PA0/SWCLK, PA1/SWDIO → SWD 调试
- PA6, PA7 → UART 调试口
- PA19 → Reset PIN
- PA6(DP), PA7(DM) → USB

---

#### /b6-project-checklist - 项目完整性检查

**触发**: `/b6-project-checklist [project_path]`

**检查项**:

| 类别 | 检查内容 |
|------|----------|
| 目录结构 | mdk/, src/, inc/, doc/ |
| 必需文件 | *.uvprojx, main.c, cfg.h |
| cfg.h 配置 | BLE_DEV_NAME, BLE_ADDR, SYS_CLK |
| main.c 质量 | 初始化顺序、返回值检查、主循环 |
| BLE 配置 | 库选择、UUID 备案、Profile 完整性 |
| 内存空间 | SRAM/Flash 分配 |

---

#### /b6-translate-error - 错误码翻译

**触发**: `/b6-translate-error <error_code>`

**支持格式**: `0x85` / `133` / `ATT_ERR_INVALID_HANDLE`

**错误分层**:

| 范围 | 层 | 前缀 |
|------|-----|------|
| 0x00-0x0D | GAP | `GAP_ERR_` |
| 0x80-0x90 | ATT | `ATT_ERR_` |
| 0xA0-0xAF | L2C | `L2C_ERR_` |
| 0xB0-0xB6 | GATT | `GATT_ERR_` |
| 0xC0-0xDC | SMP | `SMP_ERR_` |

---

#### /b6-sram-analyze - SRAM 分析

**触发**: `/b6-sram-analyze [project_path]`

**B6x SRAM 布局**:

| 区域 | 起始地址 | 大小 | 用途 |
|------|----------|------|------|
| RAM | 0x20003000 | 20KB | 主 SRAM |
| EM | 0x20008000 | 8KB | BLE Exchange Memory |
| **总计** | - | **28 KB** | - |

**警告阈值**:

| 指标 | 警告 | 错误 |
|------|------|------|
| SRAM 使用率 | > 90% | > 95% |
| Stack | < 1 KB | < 512 B |
| Heap | < 2 KB | < 1 KB |

---

#### /b6-power-analyze - 功耗分析

**触发**: `/b6-power-analyze <project_path>`

**B6x 功耗参考**:

| 模式 | 平均功耗 | 说明 |
|------|----------|------|
| Sleep | 15 µA | 睡眠 < 40ms |
| PowerOff | 2 µA | 睡眠 > 40ms |
| 正常广播 | 1.46 mA | 间隔 22.5ms |
| 发送数据 | 354.6 µA | CI=20ms |
| 待机连接 | 41.3 µA | 无数据传输 |

---

#### /extract-doc - 文档内容提取

**触发**: `/extract-doc <file_path>`

**支持格式**:

| 格式 | 扩展名 | 方法 |
|------|--------|------|
| Word | .docx | python-docx |
| Excel | .xlsx | openpyxl |
| PDF | .pdf | pymupdf / MCP OCR |
| 图片 | .png/.jpg | MCP OCR |

**依赖安装**:
```bash
pip install python-docx openpyxl pymupdf
```

---

### 4.4 典型开发流程

```
需求分析
    ↓
/b6-create-project     → 生成 spec.md + plan.md + 项目文件
    ↓
/b6-validate-hardware  → 验证硬件配置
    ↓
/b6-code-review        → 代码审查
    ↓
/b6-project-checklist  → 项目完整性检查
    ↓
/b6-keil-build         → 编译工程
    ↓
(如有错误) /b6-translate-error → 翻译错误码
```

---

### 4.5 配置文件位置

| 文件 | 路径 | 用途 |
|------|------|------|
| 全局设置 | `~/.claude/settings.json` | GLM 模型配置 |
| 项目设置 | `.claude/settings.json` | 项目权限配置 |
| MCP 配置 | `.mcp.json` | MCP 服务器配置 |
| 代码规范 | `.claude/rules/code-style.md` | 编码规范 |
| BLE 规范 | `.claude/rules/ble-conventions.md` | BLE 开发规范 |
| MCP 指南 | `.claude/rules/mcp-usage.md` | MCP 工具使用 |

---

## 附录

### A. 常见问题

**Q: MCP 服务器启动失败？**

```bash
# 检查 Python 环境
python --version  # 需要 >= 3.10

# 检查依赖
pip install -r xm_b6_mcp/requirements.txt

# 检查 SDK 路径
# 在 .mcp.json 中确认 B6X_SDK_PATH 正确
```

**Q: Keil 编译找不到？**

确保 Keil 安装在默认路径 `C:\Keil_v5\UV4\UV4.exe`，或修改 `/b6-keil-build` 中的路径。

**Q: 模型选择？**

优选Claude Opus模型，备选GLM。



---

*文档由 Claude Code 自动生成*
