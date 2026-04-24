# MCP 工具规范

> **优先 MCP > Agent 搜索**：索引化查询更快更准。

## 工具速查

| 工具 | 用途 | 关键参数 |
|------|------|----------|
| `search_sdk` | 发现 SDK 内容 | `query`, `scope`: api/docs/registers/examples/all |
| `inspect_node` | 查看节点详情 | `node_id`: api:/reg:/ex:/macro:/doc:XXX |
| `validate_config` | 验证硬件配置 | `config_json`: {pins, clock, dma, interrupts} |
| `clang_analyze` | C 代码语义分析 | `code`, `type`: refs/type/check, `files`[:3] |

## 使用流程

```
找信息 → search_sdk → inspect_node (详情)
验证配置 → validate_config
分析代码 → clang_analyze (refs/type/check)
```

## 寄存器搜索模式

`search_sdk(query, scope="registers")` 支持多种搜索格式：

| 搜索格式 | 示例 | 匹配结果 |
|---------|------|----------|
| 完整名称 | `UART1_BRR` | 精确匹配 UART1 的 BRR 寄存器 |
| 短名称 | `BRR` | 匹配所有外设的 BRR 寄存器 |
| 外设前缀 | `UART1` | 匹配 UART1 的所有寄存器 |
| 模糊搜索 | `baud` | 在寄存器名称和描述中搜索 |

**命名规则**：SVD 中寄存器结构为 `{peripheral: "UART1", name: "BRR"}`，搜索自动支持 `UART1_BRR` 格式。

## 节点 ID 格式

标准格式：`类型:标识符`

| 类型 | 前缀 | 示例 |
|------|------|------|
| API | `api:` | `api:B6x_UART_Init`, `api:gpio_put` |
| 寄存器 | `reg:` | `reg:UART1_BRR`, `reg:GPIOA_ODR` |
| 文档 | `doc:` | `doc:uart_guide` |
| 示例 | `ex:` | `ex:examples/spiMaster/src/main.c` |
| 宏 | `macro:` | `macro:SPIM_CR_DFLT` |

旧格式（自动转换）：`B6x_UART_Init` → `api:B6x_UART_Init`

---

## Embedded Debugger 工具

用于 DAPLink/J-Link 调试器烧录和 RTT 监控：

| 工具 | 用途 |
|------|------|
| `list_probes` | 列出已连接的调试器 |
| `connect` | 连接调试器（`probe_selector: "auto"`） |
| `disconnect` | 断开会话 |
| `flash_erase` | 擦除 Flash |
| `flash_program` | 烧录固件（ELF/HEX/BIN） |
| `run_firmware` | 一键烧录并运行 |
| `reset` | 复位目标 |
| `rtt_attach` | 附加 RTT（需提供 CB 地址） |
| `rtt_read` | 读取 RTT 输出 |
| `rtt_write` | 发送数据到目标 |

### RTT Control Block 地址获取

编译后从 map 文件提取：
```bash
grep "_RTT" build/map/$PROJECT.map | grep -oE "0x[0-9a-fA-F]+" | head -1
```

> 完整调试流程见 `/b6-auto-debug` skill
