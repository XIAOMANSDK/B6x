# B6x MCP 服务器架构文档

本文档介绍 B6x MCP 服务器的系统架构和核心概念。

---

## 目录

1. [系统架构总览](#1-系统架构总览)
2. [三层统一架构 + Clang 集成](#2-三层统一架构--clang-集成)
3. [四维知识图谱](#3-四维知识图谱)
4. [数据构建流程](#4-数据构建流程)
5. [核心解析模块](#5-核心解析模块)
6. [技术特性](#6-技术特性)
7. [数据统计](#7-数据统计)
8. [独特优势](#8-独特优势)
9. [扩展性](#9-扩展性)

---

## 1. 系统架构总览

### 1.1 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                   B6x MCP 服务器 v3.5                        │
├─────────────────────────────────────────────────────────────┤
│
│  ┌─────────────────────────────────────────────────────┐    │
│  │         三层统一架构 (Three-Layer Architecture)     │    │
│  │                                                      │    │
│  │  Layer 1: search_sdk()      - 发现层               │    │
│  │  Layer 2: inspect_node()    - 检查层               │    │
│  │  Layer 3: validate_config() - 验证层               │    │
│  └─────────────────────────────────────────────────────┘    │
│                           ↓                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │         Clang 语义分析 (Semantic Analysis)          │    │
│  │                                                      │    │
│  │  clang_analyze() - 实时代码分析、类型推导、引用查找 │    │
│  └─────────────────────────────────────────────────────┘    │
│                           ↓                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │         四维知识图谱 (4D Knowledge Graph)           │    │
│  │                                                      │    │
│  │  Domain 1: Hardware & Registers (368 寄存器)       │    │
│  │  Domain 2: SOC Drivers (412 条目)                  │    │
│  │  Domain 3: BLE Stack (38 条目)                     │    │
│  │  Domain 4: Applications (示例代码)                 │    │
│  └─────────────────────────────────────────────────────┘    │
│                           ↓                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │         数据源 (Data Sources)                       │    │
│  │                                                      │    │
│  │  📄 SDK 文档 (doc/) → Whoosh 索引                  │    │
│  │  📊 Excel 规格 → 硬件约束 JSON                     │    │
│  │  💻 C 代码 (drivers/, ble/) → 知识图谱              │    │
│  │  📝 SVD 文件 → 寄存器数据库                        │    │
│  │  ⚙️  config/ → API 映射 + 依赖关系                  │    │
│  │  🔧 libclang → 实时语义分析                        │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心数据

**Whoosh 索引内容**:

| 条目类型 | 数量 | 数据内容 |
|----------|------|----------|
| 函数 | 280 | 名称、参数、返回值、注释 |
| 宏 | 260 | 名称、值、类型 |
| 枚举 | 172 | 名称、类型、枚举值 |
| 文档 | 25 | SDK 文档全文 |
| **总计** | **737** | 完整的 SDK 知识库 |

---

## 2. 三层统一架构 + Clang 集成

### 2.1 架构概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         B6x MCP Server v3.5                                │
│                 Three-Layer Unified Architecture + Clang                     │
│                                                                              │
│  ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐        │
│  │   Layer 1:       │   │   Layer 2:       │   │   Layer 3:       │        │
│  │   Discovery      │ → │   Detail         │ → │   Validation     │        │
│  │   "信息在哪里"   │   │   "长什么样"     │   │   "行不行"       │        │
│  └──────────────────┘   └──────────────────┘   └──────────────────┘        │
│                                                                              │
│  ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐        │
│  │  search_sdk()    │   │ inspect_node()   │   │validate_config() │        │
│  │  search_composer │   │ node_inspector   │   │ config_validator │        │
│  └──────────────────┘   └──────────────────┘   └──────────────────┘        │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────┐      │
│  │                   Clang Integration (Optional)                    │      │
│  │                   clang_analyze() - 语义分析                      │      │
│  │                   "这段代码语义是什么"                             │      │
│  └──────────────────────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 设计模式

| 层级 | 工具 | 设计模式 | 核心问题 |
|------|------|----------|----------|
| **Layer 1** | `search_sdk()` | Composer Pattern | "Where is information?" |
| **Layer 2** | `inspect_node()` | Strategy Table Pattern | "What does it look like?" |
| **Layer 3** | `validate_config()` | Chain of Responsibility Pattern | "Will it work?" |
| **Clang** | `clang_analyze()` | - | "What does this code mean?" |

### 2.3 Layer 1: search_sdk() - 发现层

**功能**: 搜索 SDK 中的所有信息

**文件**: `src/layer1_discovery.py`

**搜索引擎架构**:
```
┌─────────────────────────────────────────────────────────────────────────┐
│                        SDKSearchComposer                                 │
│                         (search_sdk)                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │              Parallel Search (并行搜索)                           │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │                                                                   │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │  │
│  │  │ APIEngine    │  │ DocEngine    │  │ MacroEngine  │           │  │
│  │  │              │  │              │  │              │           │  │
│  │  │ Whoosh Index │  │ Whoosh Index │  │ Whoosh Index │           │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘           │  │
│  │                                                                   │  │
│  │  ┌──────────────┐  ┌──────────────┐                             │  │
│  │  │ RegisterEng  │  │ ExampleEng   │                             │  │
│  │  │              │  │              │                             │  │
│  │  │ SVD Parser   │  │ ExampleScan  │                             │  │
│  │  └──────────────┘  └──────────────┘                             │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │              Result Merger (结果合并)                             │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │  merge_mode:                                                       │  │
│  │    - interleave: 按相关性混合排序 (默认)                           │  │
│  │    - group: 按域分组显示                                          │  │
│  │    - rank: 全局加权排序                                           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

**搜索引擎**:
- **APIEngine** - 搜索 API 函数（Whoosh）
- **DocEngine** - 搜索 SDK 文档（Whoosh）
- **MacroEngine** - 搜索宏定义（Whoosh）
- **RegisterEngine** - 搜索硬件寄存器（SVD）
- **ExampleEngine** - 搜索示例代码

**搜索域与数据源**:

| 搜索域 | Engine | 数据源 | 索引方式 | 检索内容 |
|--------|--------|--------|----------|----------|
| **API** | APIEngine | `whoosh_index/` | Whoosh 全文索引 | 函数名、参数、返回值、文档注释 |
| **Docs** | DocEngine | `whoosh_index/` | Whoosh 全文索引 | PDF、Word、Markdown 文档 |
| **Macros** | MacroEngine | `whoosh_index/` | Whoosh 全文索引 | #define 宏、枚举常量 |
| **Registers** | RegisterEngine | `core/B6x.svd` | SVD 解析 | 寄存器名、外设名、位域 |
| **Examples** | ExampleEngine | `examples/` | 文件扫描 | 示例项目名、外设、代码片段 |

**搜索域 (scope)**:
- `"all"` - 所有类型
- `"api"` - API 函数
- `"macros"` - 宏定义
- `"enums"` - 枚举类型
- `"docs"` - SDK 文档
- `"registers"` - 硬件寄存器
- `"examples"` - 示例代码

**合并模式 (merge_mode)**:
- `"interleave"` - 按相关性混合结果（默认，推荐）
- `"group"` - 按域分组显示
- `"rank"` - 使用域权重全局排序

**检索流程**:
```
用户查询: "UART init"
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 1. Parse Scope (解析范围)                                    │
│    scope = "all" → [api, docs, macros, registers, examples] │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. Parallel Search (并行搜索)                                │
│    asyncio.gather(*[engine.search() for engine in engines]) │
│                                                              │
│  APIEngine     → Whoosh: "UART init" → 10 results           │
│  DocEngine     → Whoosh: "UART init" → 8 results            │
│  RegisterEng   → SVD: "UART" → 5 results                    │
│  ExampleEng    → Scanner: "uart" → 3 results                │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. Merge Results (合并结果)                                  │
│    mode = "interleave" (混合排序)                            │
│                                                              │
│  [API] B6x_UART_Init         (score: 0.95)                  │
│  [Reg] UART1_CTRL            (score: 0.88)                  │
│  [Doc] UART Guide            (score: 0.85)                  │
│  [API] B6x_UART_Transmit     (score: 0.82)                  │
│  [Ex]  uart_basic            (score: 0.78)                  │
└─────────────────────────────────────────────────────────────┘
```

**示例**:
```python
# 搜索所有 UART 相关内容
await search_sdk("UART", "all")

# 只搜索 API
await search_sdk("B6x_UART_Init", "api")

# 搜索寄存器
await search_sdk("UART1_CTRL", "registers")
```

### 2.4 Layer 2: inspect_node() - 检查层

**功能**: 查看特定节点的详细信息

**文件**: `src/layer2_detail.py`

**策略表架构**:
```
┌─────────────────────────────────────────────────────────────────────────┐
│                         NodeInspector                                   │
│                        (inspect_node)                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  Strategy Table: (node_type, view_type) → handler_method                │
│                                                                           │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                        API Node                                   │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  (api, auto)           → _get_api_summary()                        │ │
│  │  (api, definition)      → _get_api_definition()                     │ │
│  │  (api, implementation)  → _get_api_implementation()                 │ │
│  │  (api, dependencies)    → _get_api_dependencies()                   │ │
│  │  (api, call_chain)      → _get_api_call_chain()                     │ │
│  │  (api, usage_examples)  → _get_api_examples()                       │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                           │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                      Register Node                                 │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  (register, auto)       → _get_register_summary()                  │ │
│  │  (register, register_info) → _get_register_info()                  │ │
│  │  (register, bit_fields)  → _get_bit_fields()                       │ │
│  │  (register, memory_map)  → _get_memory_map()                       │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                           │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                       Document Node                                │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  (docs, auto)            → _get_doc_summary()                      │ │
│  │  (docs, doc_content)     → _get_doc_content()                      │ │
│  │  (docs, doc_summary)     → _get_doc_summary()                      │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

**数据源和解析器**:

| 解析器 | 用途 | 数据源 |
|--------|------|--------|
| **WhooshSearcher** | API 定义、宏定义 | `whoosh_index/` |
| **TreeSitterCParser** | 源代码解析、依赖提取 | `drivers/src/*.c` |
| **SVDParser** | 寄存器信息、位域 | `core/B6x.svd` |
| **ConfigLoader** | API 依赖关系、寄存器映射 | `dependency_overrides.yaml` |

**节点类型 (NodeType)**:
- `API` - API 函数
- `REGISTER` - 硬件寄存器
- `DOCS` - SDK 文档
- `MACROS` - 宏定义
- `EXAMPLES` - 示例代码

**视图类型 (view_type)**:

**通用视图**:
- `"auto"` - 自动选择最佳视图
- `"summary"` - 摘要信息
- `"full"` - 完整信息

**API 视图**:
- `"definition"` - 函数声明（参数、返回值）
- `"implementation"` - 实现位置（源文件、行号）
- `"dependencies"` - 依赖关系（前置 API、硬件要求）⭐
- `"call_chain"` - 调用链（API 调用顺序）
- `"usage_examples"` - 使用示例

**寄存器视图**:
- `"register_info"` - 寄存器信息（地址、访问权限、访问 API）⭐
- `"bit_fields"` - 位字段定义
- `"memory_map"` - 内存映射

**文档视图**:
- `"doc_content"` - 文档内容
- `"doc_summary"` - 文档摘要

**检索流程**:
```
用户请求: inspect_node("B6x_UART_Init", "dependencies")
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 1. Parse node_id (解析节点ID)                               │
│    "B6x_UART_Init" → (NodeType.API, "B6x_UART_Init")        │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. Auto-select view (自动选择视图)                           │
│    view_type = "auto" → "summary"                           │
│    (或使用用户指定的 "dependencies")                         │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. Look up strategy (查找策略)                              │
│    STRATEGIES[(API, DEPENDENCIES)]                           │
│    → ("_get_api_dependencies", None)                        │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. Execute strategy (执行策略)                               │
│                                                              │
│  Step 4a: Check dependency_overrides.yaml                   │
│          → Found: uart_init                                 │
│          → Extract: pre_requisites, call_sequence, notes    │
│                                                              │
│  Step 4b: Get register mapping from api_register_mapping.yaml │
│          → Registers: UARTx_CTRL, UARTx_BRR, GPIOx_CSC...  │
│                                                              │
│  Step 4c: Build response                                    │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 5. Return Detail Response                                  │
│  {                                                          │
│    "success": true,                                         │
│    "node_id": "B6x_UART_Init",                              │
│    "view_type": "dependencies",                             │
│    "data": {                                                │
│      "api_name": "uart_init",                               │
│      "pre_requisites": ["system_clock_init", ...],          │
│      "requirements": {                                      │
│        "clock": true,                                       │
│        "gpio": true,                                        │
│        "dma": false,                                        │
│        "interrupt": false                                   │
│      },                                                     │
│      "registers_accessed": [                                │
│        {"register": "UART1_CTRL", "access": "configure"},   │
│        {"register": "GPIOA_CSC", "access": "configure"}     │
│      ],                                                     │
│      "call_sequence": [...],                                │
│      "notes": ["I2C requires 180us delay..."]              │
│    },                                                       │
│    "related_nodes": [                                       │
│      {"node_id": "UART1_CTRL", "relation": "uses_register"} │
│    ]                                                        │
│  }                                                          │
└─────────────────────────────────────────────────────────────┘
```

**示例**:
```python
# 查看 API 定义
await inspect_node("B6x_UART_Init", "definition")

# 查看寄存器位字段
await inspect_node("UART1_CTRL", "bit_fields")

# 查看实现位置
await inspect_node("B6x_UART_Init", "implementation")

# 查看 API 依赖关系 ⭐
await inspect_node("uart_init", "dependencies")

# 查看寄存器的访问 API ⭐
await inspect_node("GPIOA_CSC", "register_info")
```

### 2.5 Layer 3: validate_config() - 验证层

**功能**: 验证硬件配置是否符合芯片约束

**文件**: `src/layer3_validation.py`

**验证链架构**:
```
┌─────────────────────────────────────────────────────────────────────────┐
│                        ConfigValidator                                  │
│                       (validate_config)                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  Validation Chain (验证链 - 按优先级顺序)                                │
│                                                                           │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 1. API Dependencies (api_calls)        ← ERROR (must pass)         │ │
│  │    → Check prerequisite APIs are called                            │ │
│  │    → Check clock/GPIO/DMA requirements                             │ │
│  │    → Check special cases (I2C 180us delay)                         │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 2. Register Conflicts (register_conflicts) ← ERROR                 │ │
│  │    → Check multiple APIs writing to same register                  │ │
│  │    → Use api_register_mapping.yaml                                 │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 3. Pin Configuration (pins)              ← ERROR (must pass)       │ │
│  │    → Check pin function availability                              │ │
│  │    → Check function conflicts                                     │ │
│  │    → Use io_map.json                                              │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 4. Clock Configuration (clock)            ← ERROR (must pass)       │ │
│  │    → Check system clock validity                                  │ │
│  │    → Check peripheral clock enable                                │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 5. DMA Allocation (dma)                  ← ERROR (must pass)       │ │
│  │    → Check DMA channel conflicts                                  │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 6. Interrupt Priority (interrupts)       ← WARNING                 │ │
│  │    → Check priority range (0-15)                                  │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 7. Memory Usage (memory)                 ← WARNING                 │ │
│  │    → Check Flash/SRAM usage                                       │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 8. Power Estimation (power)              ← INFO                    │ │
│  │    → Estimate current consumption                                 │ │
│  │    → Estimate battery life                                        │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              ↓                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │ 9. Compatibility Check (compatibility)    ← INFO                    │ │
│  │    → Check BLE role compatibility                                  │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

**验证链** (Chain of Responsibility):
1. **API 依赖检查** - 使用 `dependency_overrides.yaml` ⭐ NEW
2. **寄存器冲突检测** - 使用 `api_register_mapping.yaml` ⭐ NEW
3. 引脚冲突检测 - 使用硬件约束数据
4. 时钟配置验证
5. DMA 分配检查
6. 中断优先级检查
7. 内存使用检查
8. 功耗估算
9. 兼容性检查

**配置数据源**:

| 配置项 | 数据源 | 用途 |
|--------|--------|------|
| **api_calls** | 用户输入 | 要调用的 API 列表 |
| **dependency_overrides.yaml** | `config/` | API 依赖关系要求 |
| **api_register_mapping.yaml** | `config/` | API-寄存器映射 |
| **io_map.json** | `data/constraints/` | 引脚功能约束 |
| **pins** | 用户输入 | 引脚配置 |
| **clock** | 用户输入 | 时钟配置 |
| **dma** | 用户输入 | DMA 配置 |
| **interrupts** | 用户输入 | 中断配置 |
| **memory** | 用户输入 | 内存使用量 |
| **ble** | 用户输入 | BLE 配置 |

**新增验证功能** (v3.2):

#### 1. API 依赖验证

使用 `dependency_overrides.yaml` 验证:
- 所有前置 API 是否已调用
- 时钟要求是否满足
- GPIO 要求是否满足
- DMA 要求是否满足
- 中断要求是否满足
- 特殊情况（如 I2C 180us 延迟、UART RX 等待）

```yaml
# dependency_overrides.yaml 示例
dependencies:
  - api: uart_init
    pre_requisites: [B6x_RCC_EnablePeriphClock, B6x_GPIO_Init]
    requires_clock: true
    requires_gpio: true
```

#### 2. 寄存器冲突检测

使用 `api_register_mapping.yaml` 检测:
- 多个 API 写入同一寄存器（错误）
- 多个 API 以不同访问类型访问同一寄存器（警告）

```yaml
# api_register_mapping.yaml 示例
mappings:
  - api: uart_init
    registers: [GPIOA_CSC, UART1_CR]
    access_type: configure
  - api: spi_init
    registers: [GPIOA_CSC, SPI1_CR]
    access_type: configure
```

**验证流程**:
```
用户配置:
{
  "api_calls": ["uart_init", "i2c_init"],
  "pins": {"PA0": "UART1_TX", "PA1": "I2C1_SDA"},
  "clock": {"system_clock": 32, "enabled_peripherals": ["UART"]}
}
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 1. API Dependencies Validation                             │
│    - Check uart_init prerequisites                         │
│    - Check i2c_init prerequisites                          │
│    - ✅ system_clock_init is called                        │
│    - ❌ I2C peripheral clock not enabled                   │
│    - ❌ Missing GPIO config for I2C                        │
│    - ❌ Missing 180us delay for I2C                        │
│    → ADD 3 ERRORS                                          │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. Register Conflicts Validation                           │
│    - uart_init accesses: UART1_CTRL, GPIOA_CSC             │
│    - i2c_init accesses: I2C1_CTRL, GPIOA_CSC              │
│    - ⚠️  Both APIs configure GPIOA_CSC                     │
│    → ADD 1 WARNING                                         │
└─────────────────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. Pin Configuration Validation                            │
│    - PA0 supports UART1_TX ✅                              │
│    - PA1 supports I2C1_SDA ✅                              │
│    - No function conflicts ✅                              │
│    → PASS                                                  │
└─────────────────────────────────────────────────────────────┘
        ↓
... (continue through remaining validators) ...
        ↓
┌─────────────────────────────────────────────────────────────┐
│ Final Validation Report                                   │
│  {                                                          │
│    "is_valid": false,                                       │
│    "summary": {                                             │
│      "total_issues": 5,                                     │
│      "errors": 4,                                           │
│      "warnings": 1,                                         │
│      "info": 0                                              │
│    },                                                       │
│    "issues_by_severity": {                                  │
│      "errors": [...],                                       │
│      "warnings": [...]                                      │
│    },                                                       │
│    "suggestions": [...]                                     │
│  }                                                          │
└─────────────────────────────────────────────────────────────┘
```

**配置项**:
```python
await validate_config({
    "pins": {
        "PA09": "UART1_TX",
        "PA10": "UART1_RX"
    },
    "clock": {
        "system_clock": 16000000,
        "enabled_peripherals": ["UART1"]
    },
    "api_calls": ["uart_init"],          # ⭐ NEW
    "provided_calls": [...]              # ⭐ NEW
})
```

### 2.6 三层协作流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          User Workflow                                   │
│                                                                          │
│  Step 1: Discovery (发现信息)                                           │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ search_sdk("UART init", "all")                                  │   │
│  │                                                                  │   │
│  │ Results:                                                         │   │
│  │  - [API] B6x_UART_Init           (id: B6x_UART_Init)            │   │
│  │  - [REG] UART1_CTRL              (id: UART1_CTRL)               │   │
│  │  - [DOC] UART Guide              (id: uart_guide)               │   │
│  │  - [EX]  uart_basic              (id: uart_basic)               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                              ↓                                          │
│  Step 2: Detail (查看详情)                                              │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ inspect_node("B6x_UART_Init", "dependencies")                    │   │
│  │                                                                  │   │
│  │ Details:                                                         │   │
│  │  - Prerequisites: system_clock_init, rcc_enable_clock           │   │
│  │  - Requirements: clock=true, gpio=true, dma=false               │   │
│  │  - Registers: UART1_CTRL, UARTx_BRR, GPIOx_CSC                  │   │
│  │  - Notes: "Check RX pin high after reset"                       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                              ↓                                          │
│  Step 3: Validation (验证配置)                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ validate_config({                                               │   │
│  │   "api_calls": ["uart_init"],                                   │   │
│  │   "pins": {"PA0": "UART1_TX"},                                  │   │
│  │   "clock": {"system_clock": 32}                                 │   │
│  │ })                                                               │   │
│  │                                                                  │   │
│  │ Report:                                                          │   │
│  │  - ❌ Missing UART peripheral clock                             │   │
│  │  - ⚠️  System clock not initialized                            │   │
│  │  - ✅ Pin configuration valid                                    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                              ↓                                          │
│  Step 4: Iterate (迭代改进)                                           │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ Fix issues → Re-validate → Pass → Implement                     │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.7 文档检索路径总结

#### Layer 1 检索路径

```
search_sdk(query) 检索路径:

├─ API Engine
│  └─ whoosh_index/
│     └─ [API entries indexed from drivers/api/*.h]
│        └─ 检索: name, brief, parameters, return_type
│
├─ Docs Engine
│  └─ whoosh_index/
│     └─ [Document entries indexed from doc/, examples/]
│        └─ 检索: title, content, brief, name
│
├─ Macros Engine
│  └─ whoosh_index/
│     └─ [Macro entries indexed from drivers/api/*.h]
│        └─ 检索: name, brief, macro_value, macro_type
│
├─ Registers Engine
│  └─ core/B6x.svd
│     └─ [SVD parsed register database]
│        └─ 检索: peripheral.name, register.name, description
│
└─ Examples Engine
   └─ examples/
      └─ [Example project scanner]
         └─ 检索: project name, peripheral, description
```

#### Layer 2 检索路径

```
inspect_node(node_id, view_type) 检索路径:

├─ API Node (B6x_*)
│  ├─ Definition → whoosh_index/ (API entry)
│  ├─ Implementation → Tree-sitter parser (source files)
│  ├─ Dependencies → dependency_overrides.yaml
│  │                 └─ api_register_mapping.yaml
│  └─ Call Chain → dependency_overrides.yaml (call_sequence)
│
├─ Register Node (*_CTRL, *_REG)
│  ├─ Info → SVD Parser (core/B6x.svd)
│  │       └─ Enhanced by: api_register_mapping.yaml
│  ├─ Bit Fields → SVD Parser (fields enumeration)
│  └─ Memory Map → SVD Parser (address offset)
│
└─ Document Node
   ├─ Summary → whoosh_index/ (document entry)
   └─ Content → Original file (PDF/Word/MD)
```

#### Layer 3 检索路径

```
validate_config(config) 检索路径:

├─ API Dependencies
│  └─ dependency_overrides.yaml
│     ├─ prerequisites (API call order)
│     ├─ requires_clock/gpio/dma/interrupt
│     └─ notes (special cases)
│
├─ Register Conflicts
│  └─ api_register_mapping.yaml
│     └─ Cross-reference APIs → Registers
│
├─ Pin Constraints
│  └─ data/constraints/io_map.json
│     ├─ pin_name → functions[]
│     └─ default_function
│
├─ Clock Validity
│  └─ Hardcoded valid clocks: [16, 32, 48, 64] MHz
│
└─ Memory Constraints
   └─ User-provided or inferred from chip specs
```

### 2.8 架构优势

| 指标 | 旧架构 (52 tools) | 新架构 (3 tools) | 改进 |
|------|-------------------|------------------|------|
| **工具数量** | 68 | **3** | 96% ↓ |
| **Schema 大小** | ~35 KB | **~5 KB** | 86% ↓ |
| **上下文消耗** | 高 | **低** | 显著降低 |
| **维护复杂度** | 分散 | **集中** | 大幅简化 |
| **用户体验** | 需要了解很多工具 | **3 个工具覆盖全流程** | 简化 |

### 2.9 核心设计模式

1. **Layer 1 - Composer Pattern**: 并行聚合多个搜索引擎
2. **Layer 2 - Strategy Table Pattern**: 映射视图类型到处理方法
3. **Layer 3 - Chain of Responsibility Pattern**: 顺序验证链

### 2.10 Clang 语义分析 (v3.5 新增)

**功能**: 实时 C 代码语义分析

**文件**: `src/clang_tools.py`, `src/core/clang_parser.py`

**与三层架构的关系**: Clang 作为独立工具与三层架构并行，不修改现有工具职责。

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          Clang Integration                               │
│                         (clang_analyze)                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │              Analysis Types (分析类型)                            │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │                                                                   │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │  │
│  │  │   "refs"     │  │   "type"     │  │   "check"    │           │  │
│  │  │  查找引用    │  │  类型推导    │  │  编译诊断    │           │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘           │  │
│  │                                                                   │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    SDK Context (自动注入)                         │  │
│  │  • Include paths: drivers/api, ble/api, core                     │  │
│  │  • Defines: __B6X__, ARM_MATH_CM0_PLUS                           │  │
│  │  • Target: arm-none-eabi                                         │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    libclang Parser                                │  │
│  │  • 实时 AST 解析                                                  │  │
│  │  • 类型信息提取                                                   │  │
│  │  • 编译错误检测                                                   │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

**Clang 与 Tree-sitter 关系**:

| 解析器 | 职责 | 使用场景 |
|--------|------|----------|
| **Tree-sitter** | 快速语法解析 | 构建索引、快速搜索 |
| **Clang** | 深度语义分析 | 类型推断、跨文件引用、宏展开 |

**使用示例**:
```python
# 查找符号引用
clang_analyze(code="B6x_UART_Init", type="refs")

# 推导表达式类型
clang_analyze(code="cfg->baudrate", type="type")

# 检查代码错误
clang_analyze(code="B6x_UART_Init(UART1, &cfg);", type="check")
```

详细文档请参考 [clang-integration.md](clang-integration.md)。

---

## 3. 四维知识图谱

### 3.1 数据域

**Domain 1: Hardware & Registers** (442 条目)
- 寄存器定义: 368 个
- 引脚配置: 65 个
- 功耗模式: 7 种
- 内存区域: 2 个

**Domain 2: SOC Drivers** (412 条目)
- API 函数: 18 个
- 结构体: 38 个
- 枚举: 89 个
- 宏定义: 293 个

**Domain 3: BLE Stack** (38 条目)
- BLE API
- GATT Profile
- 错误码: 160 个
- Mesh API

**Domain 4: Applications** (待扩展)
- 示例代码
- 项目配置

### 3.2 关系类型

| 关系类型 | 说明 | 示例 |
|----------|------|------|
| `implemented_in` | API 实现位置 | `B6x_UART_Init` → `uart.c` |
| `uses` | API 使用关系 | Profile → Driver API |
| `contains` | 包含关系 | 外设 → 寄存器 |
| `depends_on` | 依赖关系 | API → 前置 API |
| `accesses` | 访问关系 | API → 寄存器 | ⭐ NEW

### 3.3 知识图谱规模

- **条目总数**: 892
- **关系总数**: 119
- **数据来源**: 7 个（C 代码、Excel、SVD、文档等）

---

## 4. 数据构建流程

### 4.1 构建阶段

```
Phase 1: Hardware & Registers
  ├─ SVD 寄存器解析 (368 个)
  ├─ Excel 硬件约束
  └─ 链接脚本、中断向量

Phase 2: SOC Drivers
  ├─ Tree-sitter 解析驱动 API
  ├─ 结构体定义 (38 个)
  ├─ 宏定义 (293 个)
  └─ 枚举定义 (89 个)

Phase 3: BLE Stack
  ├─ BLE API 解析
  ├─ GATT Profile 解析
  ├─ BLE 错误码 (160 个)
  └─ Mesh API 解析

Phase 4: Applications
  └─ 示例代码扫描

Phase 5: 跨域关系构建
  └─ 119 个关系

Phase 6: 配置文件 (⭐ NEW)
  ├─ api_register_mapping.yaml - API ↔ 寄存器映射
  └─ dependency_overrides.yaml - API 依赖关系

Phase 7: 增量构建支持 (⭐ v3.4 NEW)
  ├─ BuildCacheManager - 文件哈希缓存
  ├─ file_hashes.json - 缓存数据库
  └─ MD5 检测文件修改
```

### 4.2 数据源

| 数据源 | 解析器 | 输出 |
|--------|--------|------|
| **C 代码** | Tree-sitter | 知识图谱 + Whoosh 索引 |
| **Excel 规格** | Excel Parser | 硬件约束 JSON |
| **SVD 文件** | SVD Parser | 寄存器数据库 |
| **SDK 文档** | Document Parser | Whoosh 索引 |
| **config/ (NEW)** | YAML Loader | API 映射 + 依赖关系 |

### 4.3 搜索引擎

B6x MCP Server 使用 5 个专用搜索引擎：

### 4.4 增量构建机制 (⭐ v3.4 NEW)

**BuildCacheManager** 是一个智能缓存系统,用于加速 SDK 更新后的索引重建。

**核心原理**:
```
┌─────────────────────────────────────────────────────────────────────────┐
│                        BuildCacheManager                                 │
│                         (build_all_indices.py)                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  Step 1: 计算文件哈希                                                     │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │ for file in source_files:                                         │  │
│  │   hash = MD5(file_content)                                       │  │
│  │   if hash != cached_hash:                                         │  │
│  │     dirty_files.append(file)                                     │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  Step 2: 选择性解析                                                     │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │ if incremental_mode:                                              │  │
│  │   for file in dirty_files:                                       │  │
│  │     parse_and_index(file)                                        │  │
│  │ else:                                                            │  │
│  │   for file in source_files:                                      │  │
│  │     parse_and_index(file)                                        │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  Step 3: 更新缓存                                                       │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │ save file_hashes.json                                             │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

**缓存数据结构** (`data/file_hashes.json`):
```json
{
  "build_all_indices": {
    "doc/SW_Spec/B6x_IO_MAP.xlsx": "a1b2c3d4...",
    "drivers/api/uart.h": "e5f6g7h8...",
    "drivers/src/uart.c": "i9j0k1l2..."
  },
  "build_document_index": {
    "doc/APIs/UART_Guide.pdf": "m3n4o5p6..."
  }
}
```

**性能提升**:
| 场景 | 全量构建 | 增量构建 | 加速比 |
|------|---------|----------|--------|
| 单个文件修改 | 2-5 分钟 | 30-60 秒 | **4-5x** |
| 少量文件修改 (< 10) | 2-5 分钟 | 30-60 秒 | **4-5x** |
| 大量文件修改 (> 50) | 2-5 分钟 | 1-2 分钟 | **2x** |

**使用方法**:
```bash
# 增量构建 (自动检测修改)
python scripts/build_all_indices.py --incremental

# 强制全量重建
python scripts/build_all_indices.py --force

# 查看缓存统计
python scripts/build_all_indices.py --incremental --verbose
```

**细粒度清理**:
```bash
# 只清理特定类型的缓存
python scripts/build_all_indices.py --clean whoosh
python scripts/build_all_indices.py --clean domain:drivers
```

---

### 4.5 搜索引擎 (原 4.3)

B6x MCP Server 使用 5 个专用搜索引擎：

| 搜索引擎 | 搜索域 | 数据源 |
|----------|--------|--------|
| **APIEngine** | `scope="api"` | Whoosh (函数索引) |
| **MacroEngine** | `scope="macros"` | Whoosh (宏索引) |
| **EnumEngine** | `scope="enums"` | Whoosh (枚举索引) |
| **DocEngine** | `scope="docs"` | Whoosh (文档索引) |
| **RegisterEngine** | `scope="registers"` | SVD 数据库 |
| **ExampleEngine** | `scope="examples"` | 示例代码 |

**关键特性**:
- 所有代码搜索引擎（API/Macro/Enum）使用实际 Whoosh 索引
- 不再返回 mock 数据
- 统一的搜索体验

---

## 5. 核心解析模块

### 5.1 模块分类

`src/core/` 目录包含 30+ 个核心解析模块：

**代码解析 (6 个)**
- `tree_sitter_parser.py` - AST 级别 C 代码解析
- `struct_extractor.py` - 结构体解析
- `source_file_mapper.py` - 文件映射
- `profile_dependency_parser.py` - BLE Profile 依赖
- `profile_parser.py` - GATT Profile 定义

**文档处理 (4 个)**
- `document_parser.py` - Word/PDF/Excel 解析
- `document_indexer.py` - Whoosh 文档索引
- `excel_parser.py` - 硬件约束解析
- `hardware_constraint_indexer.py` - 约束索引

**知识图谱 (3 个)**
- `knowledge_graph_schema.py` - 数据模型
- `knowledge_graph_query.py` - 查询引擎
- `knowledge_graph.py` - 加载器

**搜索索引 (1 个)**
- `whoosh_indexer.py` - Whoosh 全文搜索引擎

**关系映射 (4 个)**
- `api_register_mapper.py` - API ↔ 寄存器映射 ⭐
- `dependency_extractor.py` - API 依赖提取
- `call_chain_extractor.py` - 调用链提取
- `relation_mapper.py` - 跨域关系构建

**配置加载 (2 个)**
- `config_loader.py` - 配置文件加载器 ⭐
- `enhanced_config_loader.py` - 增强配置加载 ⭐

**错误处理 (3 个)**
- `ble_error_parser.py` - BLE 错误码知识库
- `api_error_mapper.py` - API ↔ 错误码映射
- `error_code_parser.py` - 通用错误码解析

**硬件分析 (5 个)**
- `svd_parser.py` - SVD 寄存器解析
- `interrupt_parser.py` - 中断向量表解析
- `linker_parser.py` - 链接脚本解析
- `mesh_parser.py` - BLE Mesh 解析

**元数据 (4 个)**
- `example_scanner.py` - 示例代码扫描
- `project_config_parser.py` - 项目配置解析
- `api_manifest_generator.py` - API 使用清单
- `dependency_graph_builder.py` - 依赖图构建

### 5.2 模块依赖关系

```
         Tree-sitter Parser
                ↓
    ┌───────────┼───────────┐
    ↓           ↓           ↓
Struct     Source      Profile
Extractor   File Mapper  Dependency
                ↓           ↓
            Whoosh    Knowledge Graph
                ↓           ↓
            All Search Engines
                ↓
        ┌───────┴────────┐
        ↓                ↓
  Config Loader    API/Register Mapper
        ↓                ↓
    Validation Layer (NEW)
```

### 5.3 配置文件系统 (NEW)

`config/` 目录结构:

```
config/
├── api_register_mapping.yaml       # API ↔ 寄存器映射
│   └── mappings[]
│       ├── api                     # API 名称
│       ├── registers[]             # 访问的寄存器列表
│       ├── access_type             # read/write/configure
│       └── confidence              # 映射可信度
│
└── dependency_overrides.yaml       # API 依赖关系
    └── dependencies[]
        ├── api                     # API 名称
        ├── pre_requisites[]        # 前置 API 列表
        ├── requires_clock          # 是否需要时钟
        ├── requires_gpio           # 是否需要 GPIO
        ├── requires_dma            # 是否需要 DMA
        ├── requires_interrupt      # 是否需要中断
        ├── call_sequence[]         # 推荐调用顺序
        ├── notes[]                 # 特殊情况说明
        ├── parameters[]            # 参数说明
        └── see_also[]              # 相关 API
```

### 5.4 源码目录结构 (src/)

`src/` 目录是 MCP 服务器的核心代码目录，包含三层架构实现和所有解析模块。

#### 目录结构

```
src/
├── main.py                    # MCP 服务器入口点
├── layer1_discovery.py        # Layer 1: 发现层 (search_sdk)
├── layer2_detail.py           # Layer 2: 详情层 (inspect_node)
├── layer3_validation.py       # Layer 3: 验证层 (validate_config)
├── domain_config.py           # 四维域配置
├── common/                    # 通用工具模块
│   ├── config_schema.py       # 配置 Schema 定义
│   ├── errors.py              # 错误类型定义
│   ├── node_id.py             # Node ID 标准化
│   ├── peripheral_utils.py    # 外设工具函数
│   └── result_types.py        # 结果类型定义
├── core/                      # 核心解析模块 (30+ 文件)
│   ├── whoosh_indexer.py      # Whoosh 全文索引
│   ├── tree_sitter_parser.py  # Tree-sitter C 代码解析
│   ├── svd_parser.py          # SVD 寄存器解析
│   ├── knowledge_graph.py     # 知识图谱加载
│   ├── config_loader.py       # 配置加载器
│   └── ...                    # 其他解析模块
└── modules/                   # 模块工具 (兼容层)
    ├── module_a/              # SDK 参考工具
    ├── module_b/              # 约束验证工具
    └── module_c/              # BLE 工具
```

#### 核心文件说明

| 文件 | 功能 | 设计模式 |
|------|------|----------|
| `main.py` | MCP 服务器入口，注册 3 个核心工具 | - |
| `layer1_discovery.py` | 实现 `search_sdk()` 跨域搜索 | Composer Pattern |
| `layer2_detail.py` | 实现 `inspect_node()` 节点详情查看 | Strategy Table Pattern |
| `layer3_validation.py` | 实现 `validate_config()` 配置验证 | Chain of Responsibility |
| `domain_config.py` | 定义四维域 (Hardware/Drivers/BLE/Apps) | - |

#### main.py - 服务器入口

**功能**:
- 初始化 MCP 服务器
- 加载知识图谱
- 注册 3 个核心工具
- 提供 stdio 交互模式（开发调试）

**核心类**:
```python
class B6xServerConfig:
    """服务器配置"""
    sdk_path: Path
    knowledge_graph_path: Path
    whoosh_index_dir: Path
    server_name: str = "b6x-mcp-server"
    server_version: str = "3.3.0"

class B6xMCPServerState:
    """全局服务器状态"""
    knowledge_graph: SDKKnowledgeGraph
    stats: Dict[str, int]  # 请求统计
```

**工具注册**:
```python
# Layer 1: 搜索工具
register_layer1_tools(server)  # search_sdk()

# Layer 2: 详情工具
register_layer2_tools(server)  # inspect_node()

# Layer 3: 验证工具
register_layer3_tools(server)  # validate_config()
```

#### layer1_discovery.py - 发现层

**核心类**: `SDKSearchComposer`

**搜索引擎**:
| Engine | 数据源 | 搜索内容 |
|--------|--------|----------|
| `APIEngine` | Whoosh | API 函数 |
| `DocEngine` | Whoosh | SDK 文档 |
| `MacroEngine` | Whoosh | 宏定义 |
| `RegisterEngine` | SVD | 硬件寄存器 |
| `ExampleEngine` | Scanner | 示例代码 |

**Token 限制机制**:
```python
class TokenEstimator:
    DEFAULT_HARD_LIMIT = 6000  # 硬限制
    DEFAULT_SOFT_LIMIT = 4000  # 软限制

    def estimate_result_tokens(self, result: SearchResult) -> int:
        """估算单个结果的 token 数"""
        return len(json.dumps(result_dict)) // 4
```

**合并模式**:
- `interleave` - 按相关性混合（默认）
- `group` - 按域分组
- `rank` - 全局加权排序

#### layer2_detail.py - 详情层

**核心类**: `NodeInspector`

**策略表映射**:
```python
STRATEGIES = {
    # API 视图
    (NodeType.API, ViewType.SUMMARY): ("_get_api_summary", None),
    (NodeType.API, ViewType.DEFINITION): ("_get_api_definition", None),
    (NodeType.API, ViewType.IMPLEMENTATION): ("_get_api_implementation", None),
    (NodeType.API, ViewType.DEPENDENCIES): ("_get_api_dependencies", None),
    (NodeType.API, ViewType.CALL_CHAIN): ("_get_api_call_chain", [...]),

    # 寄存器视图
    (NodeType.REGISTER, ViewType.REGISTER_INFO): ("_get_register_info", None),
    (NodeType.REGISTER, ViewType.BIT_FIELDS): ("_get_bit_fields", None),

    # 文档视图
    (NodeType.DOCS, ViewType.DOC_CONTENT): ("_get_doc_content", None),
}
```

**Node ID 格式**:
- 标准格式: `type:identifier` (如 `api:B6x_UART_Init`)
- 自动推断: `B6x_UART_Init` → 自动识别为 API

#### layer3_validation.py - 验证层

**核心类**: `ConfigValidator`

**验证链顺序**:
```
1. API Dependencies     ← ERROR (必须通过)
2. Register Conflicts   ← ERROR (必须通过)
3. Pin Configuration    ← ERROR (必须通过)
4. Clock Configuration  ← ERROR (必须通过)
5. DMA Allocation       ← ERROR (必须通过)
6. Interrupt Priority   ← WARNING
7. Memory Usage         ← WARNING
8. Power Estimation     ← INFO
9. Compatibility Check  ← INFO
```

**验证模式**:
- `incremental` - 只验证提供的字段（默认）
- `strict` - 验证所有必需字段
- `estimate` - 估算缺失字段并警告

**数据类**:
```python
@dataclass
class ValidationIssue:
    severity: Literal["error", "warning", "info"]
    category: str
    message: str
    location: Optional[str]
    suggestion: Optional[str]
    affected_items: List[str]

@dataclass
class ValidationReport:
    is_valid: bool
    mode: str
    errors: List[ValidationIssue]
    warnings: List[ValidationIssue]
    info: List[ValidationIssue]
    validated_fields: List[str]
    missing_fields: List[str]
```

#### domain_config.py - 域配置

**四维域定义**:
```python
class DomainType(Enum):
    HARDWARE = "hardware"       # 硬件与寄存器
    DRIVERS = "drivers"         # SOC 驱动
    BLE = "ble"                 # BLE 协议栈
    APPLICATIONS = "applications"  # 应用示例
```

**域构建顺序** (依赖关系):
```
Hardware (无依赖)
    ↓
Drivers (依赖 Hardware)
    ↓
BLE (依赖 Hardware + Drivers)
    ↓
Applications (依赖 Drivers + BLE)
```

#### common/ - 通用模块

| 文件 | 功能 |
|------|------|
| `config_schema.py` | 配置 Schema 定义，支持增量验证 |
| `errors.py` | 自定义错误类型 |
| `node_id.py` | Node ID 标准化处理 |
| `peripheral_utils.py` | 外设相关工具函数 |
| `result_types.py` | 统一结果类型 (`success_result`, `error_result`) |

---

## 6. 技术特性

### 6.1 Tree-sitter 与 Whoosh 集成

**Tree-sitter** (代码解析):
- 构建 AST（抽象语法树）
- 提取函数声明、宏定义、枚举类型
- 解析 Doxygen 注释

**Whoosh** (全文搜索):
- Schema 定义（字段映射）
- 文档添加（批量写入）
- 全文索引（分词、词干提取）
- TF-IDF 相关性评分

**数据流向**:
```
C 源代码
    ↓
Tree-sitter 解析
    ↓
Python 数据对象
    ↓
Whoosh 索引构建
    ↓
可搜索的索引
```

### 6.2 搜索功能

**支持的搜索类型**:

1. **函数搜索** (`scope="api"`)
   - 函数名、参数、返回值
   - Doxygen 注释

2. **宏搜索** (`scope="macros"`)
   - 宏名称、值
   - 寄存器地址、位掩码、常量

3. **枚举搜索** (`scope="enums"`)
   - 枚举类型、枚举值
   - 配置选项、状态码

4. **文档搜索** (`scope="docs"`)
   - SDK 文档全文
   - 支持中英文搜索

5. **寄存器搜索** (`scope="registers"`)
   - 寄存器名称、位字段
   - 访问权限、复位值

**搜索结果排序**:
- TF-IDF 相关性评分
- 字段加权（名称权重最高）
- 多域合并（interleave/group/rank）

### 6.3 验证功能 (NEW)

**API 依赖验证**:
- 检查所有前置 API 是否已调用
- 验证时钟、GPIO、DMA、中断要求
- 特殊情况检查（I2C 180us 延迟等）

**寄存器冲突检测**:
- 检测多个 API 写入同一寄存器
- 检测不同访问类型的潜在问题

---

## 7. 数据统计

### 7.1 知识图谱统计

| 指标 | 数值 |
|------|------|
| 总条目数 | 892 |
| 总关系数 | 119 |
| Hardware 条目 | 442 |
| Driver 条目 | 412 |
| BLE 条目 | 38 |
| Application 条目 | 0 (待扩展) |

### 7.2 搜索索引统计

| 索引类型 | 条目数 | 说明 |
|----------|--------|------|
| 函数索引 | 280 | API 函数 |
| 宏索引 | 260 | 宏定义 |
| 枚举索引 | 172 | 枚举类型 |
| 文档索引 | 25 | SDK 文档 |
| **总计** | **737** | 完整知识库 |

### 7.3 数据源统计

| 数据源 | 文件数 | 提取数据 |
|--------|--------|----------|
| C 头文件 | 18 | API、结构体、枚举 |
| C 源文件 | 18 | 实现位置 |
| Excel 文件 | 3 | 引脚配置、功耗数据 |
| SVD 文件 | 1 | 368 个寄存器 |
| SDK 文档 | 25 | 全文搜索 |
| config/ (NEW) | 2 | API 映射、依赖关系 |

---

## 8. 独特优势

### 8.1 与通用工具对比

| 特性 | 通用代码搜索 | B6x MCP Server |
|------|-------------|----------------|
| 硬件约束 | ❌ 无 | ✅ 芯片规格集成 |
| 引脚配置 | ❌ 无 | ✅ 冲突检测 |
| 功耗分析 | ❌ 无 | ✅ 实测数据 |
| 寄存器信息 | ❌ 无 | ✅ 368 个寄存器 |
| BLE 知识 | ❌ 无 | ✅ 160 个错误码 |
| 寄存器冲突检测 | ❌ 无 | ✅ API-寄存器映射 ⭐ |
| API 依赖验证 | ❌ 无 | ✅ 调用链验证 ⭐ |

### 8.2 核心价值

1. **即时导航**: 从 API 声明跳转到实现（30x 加速）
2. **硬件感知**: 集成厂商硬件约束与源代码
3. **设计决策**: 基于实测数据提供设计建议
4. **预防验证**: 在编码前发现配置问题
5. **冲突检测**: 自动检测寄存器访问冲突 ⭐
6. **依赖验证**: 自动验证 API 调用顺序 ⭐

---

## 9. 扩展性

### 9.1 可扩展数据域

**当前已实现**:
- Domain 1: Hardware & Registers
- Domain 2: SOC Drivers
- Domain 3: BLE Stack
- Domain 5: 跨域关系

**待扩展**:
- Domain 4: Applications（示例代码）
- 更多芯片系列支持
- 更多外设类型

### 9.2 可扩展解析器

**易于添加新的数据源**:
1. 创建新的解析器模块（参考 `src/core/`）
2. 在 `build_index_v2.py` 中添加解析阶段
3. 定义新的知识图谱条目类型
4. 更新 `relation_mapper.py` 添加关系

### 9.3 配置文件扩展 (NEW)

**添加 API-寄存器映射**:
编辑 `config/api_register_mapping.yaml`:
```yaml
mappings:
  - api: your_new_api
    registers: [REG1, REG2]
    access_type: configure
    confidence: 1.0
```

**添加 API 依赖关系**:
编辑 `config/dependency_overrides.yaml`:
```yaml
dependencies:
  - api: your_new_api
    pre_requisites: [prereq_api1, prereq_api2]
    requires_clock: true
    requires_gpio: false
```

---

## 版本历史

### v3.5.0 (Clang Integration) ⭐ CURRENT
- **新增**: `clang_analyze()` 工具 - 实时 C 代码语义分析
- **新增**: Clang 集成 - 类型推导、引用查找、编译诊断
- **新增**: `src/clang_tools.py` - Clang MCP 工具实现
- **新增**: `src/core/clang_parser.py` - libclang 封装
- **新增**: `src/core/sdk_context.py` - SDK 编译上下文
- **改进**: 架构文档更新，添加 Clang 层说明

### v3.4.1 (Documentation Update)
- **新增**: 完整的 `src/` 目录源码文档
- **新增**: 5.4 节 - 源码目录结构详解
- **改进**: 文档包含核心文件说明、设计模式、代码示例
- **改进**: build-guide.md 更新构建脚本文档

### v3.4 (Incremental Build)
- **新增**: 增量构建支持 (`--incremental`)
- **新增**: 细粒度清理选项 (`--clean whoosh`, `--clean domain:*`)
- **新增**: BuildCacheManager 自动缓存未修改文件
- **改进**: 知识图谱构建已集成到主构建脚本
- **性能提升**: SDK 小幅更新后重建时间从 2-5 分钟降至 30-60 秒

### v3.3 (Edge Case Optimization)
- **新增**: Token 限制机制防止结果爆炸
- **新增**: Node ID 标准化确保跨层数据一致性
- **新增**: 增量验证支持部分配置验证
- **改进**: 分页机制处理大量搜索结果
- **改进**: 更友好的错误消息和建议

### v3.2 (Three-Layer Architecture)
- **96% 工具减少**: 68 个工具 → 3 个核心工具
- **86% Schema 减少**: ~35KB → ~5KB
- **新增**: 寄存器冲突检测
- **新增**: API 依赖关系验证
- **新增**: 配置文件系统 (`config/`)
- **新增**: 特殊情况检查（I2C 180us 延迟、UART RX 等待）
- 向后兼容: 所有原始工具仍可用

### v3.0
- 初始版本

---

## 相关文档

- **[README](../README.md)** - 快速开始指南
- **[配置指南](config_directory_guide.md)** - 高级配置说明
- **[构建指南](build-guide.md)** - 数据构建流程（开发者）
- **[Clang 集成](clang-integration.md)** - Clang 语义分析方案
- **[工具使用指南](tools-usage-guide.md)** - 开发工具使用说明
