# B6x MCP Server

B6x SDK 的 Model Context Protocol (MCP) 服务器，为 AI 助手提供深度 SDK 知识。

---

## 快速开始

### 1. 安装依赖

```bash
cd xm_b6_mcp
# 安装 MCP 服务器依赖 参数 -i 指定清华源安装
pip install -r docs/requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple
```

### 2. 启动服务

```bash
python run.py
```

### 3. 在 Claude Code 中配置

**Windows:**
```bash
claude mcp add b6x-sdk "python" "D:\\svn\\bxx_DragonC1\\sdk6\\xm_b6_mcp\\run.py"
```

**macOS / Linux:**
```bash
claude mcp add b6x-sdk "python" "/path/to/sdk6/xm_b6_mcp/run.py"
```

**配置后需重启 Claude Code 生效**

---

## 核心工具

| 工具 | 功能 | 使用场景 |
|------|------|----------|
| **`search_sdk()`** | 搜索 SDK | 查找 API、文档、寄存器、示例 |
| **`inspect_node()`** | 查看详情 | 查看 API 定义、实现、依赖 |
| **`validate_config()`** | 验证配置 | 检查引脚、时钟、内存冲突 |
| **`clang_analyze()`** | 语义分析 | 实时代码分析、类型推导、引用查找 |

---

## 使用示例

### 搜索 API

```python
# 搜索 UART 相关 API
await search_sdk("UART init", scope="api")

# 搜索所有域
await search_sdk("low power", scope="all")
```

### 查看详情

```python
# 查看 API 定义
await inspect_node("api:B6x_UART_Init", view_type="definition")

# 查看实现位置
await inspect_node("api:B6x_UART_Init", view_type="implementation")

# 查看依赖关系
await inspect_node("api:B6x_UART_Init", view_type="dependencies")
```

### 验证配置

```python
# 验证引脚配置
await validate_config({
    "pins": {"PA09": "UART1_TX", "PA10": "UART1_RX"},
    "clock": {"system_clock": 32, "enabled_peripherals": ["UART1"]}
})
```

### 语义分析 (Clang)

```python
# 查找符号引用
await clang_analyze(code="B6x_UART_Init", type="refs")

# 推导表达式类型
await clang_analyze(code="cfg->baudrate", type="type")

# 检查代码错误
await clang_analyze(code="B6x_UART_Init(UART1, &cfg);", type="check")
```

---

## 参数说明

### search_sdk()

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `query` | 搜索关键词 | - |
| `scope` | 搜索域 | `api`, `docs`, `registers`, `examples`, `all` |
| `max_results` | 最大结果数 | 默认 10 |

### inspect_node()

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `node_id` | 节点 ID | 如 `api:B6x_UART_Init` |
| `view_type` | 视图类型 | `summary`, `definition`, `implementation`, `dependencies` |

### validate_config()

| 配置项 | 说明 |
|--------|------|
| `pins` | 引脚配置 `{"PA09": "UART1_TX"}` |
| `clock` | 时钟配置 `{"system_clock": 32}` |
| `dma` | DMA 配置 |
| `interrupts` | 中断配置 |
| `api_calls` | API 调用列表 |

### clang_analyze()

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `code` | C 代码片段或符号名 | - |
| `type` | 分析类型 | `refs` (查找引用), `type` (类型推导), `check` (编译诊断) |
| `files` | 上下文文件（最多 3 个）| - |
| `brief` | 精简输出 | 默认 `true` |

---

## SDK 更新后重建索引

```bash
# 增量构建（推荐，30-60秒）
python scripts/build_all_indices.py --incremental

# 完整重建（2-5分钟）
python scripts/build_all_indices.py
```

---

## 文档

| 文档 | 说明 |
|------|------|
| [architecture.md](docs/architecture.md) | 系统架构设计 |
| [build-guide.md](docs/build-guide.md) | 构建指南 |
| [config_directory_guide.md](docs/config_directory_guide.md) | 配置文件说明 |
| [tools-usage-guide.md](docs/tools-usage-guide.md) | 工具使用指南 |
| [clang-integration.md](docs/clang-integration.md) | Clang 集成方案 |
| [CHANGELOG.md](docs/CHANGELOG.md) | 版本历史 |

---

## 常见问题

### Q: 配置后 Claude Code 无法识别？

确保：
1. 路径使用双反斜杠 `\\` 或正斜杠 `/`
2. 已重启 Claude Code
3. Python 环境已激活

### Q: SDK 更新后如何重新构建索引？

```bash
python scripts/build_all_indices.py --incremental
```

### Q: 支持哪些芯片？

B61/B62/B63/B66 系列 BLE SoC。

---

## 版本历史

### v3.5 (Current)
- Clang 集成，支持实时语义分析
- 新增 `clang_analyze()` 工具
- 引用查找、类型推导、编译诊断

### v3.4.1
- 文档重构，简化用户指南
- 合并冗余文档

### v3.4
- 增量构建支持
- 性能优化

详见 [CHANGELOG.md](docs/CHANGELOG.md)

---

**许可**: Proprietary
