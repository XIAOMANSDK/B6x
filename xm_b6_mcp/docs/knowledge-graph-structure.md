# B6x 知识图谱结构详解

本文档详细说明 B6x MCP Server 中知识图谱的数据结构、域分类和关系类型。

---

## 目录

1. [总体架构](#总体架构)
2. [四维域分类](#四维域分类)
3. [条目类型详解](#条目类型详解)
4. [关系类型详解](#关系类型详解)
5. [数据模型](#数据模型)
6. [示例图谱](#示例图谱)
7. [查询模式](#查询模式)

---

## 总体架构

```
═══════════════════════════════════════════════════════════════════════════════
                      B6x 四维知识图谱架构
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────────────────────────────────────────────────────┐
│                         Unified Knowledge Graph                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    Knowledge Graph Container                         │  │
│  │                                                                      │  │
│  │  entries: Dict[str, KnowledgeGraphEntry]                             │  │
│  │  domains: Dict[DomainType, List[str]]                                │  │
│  │  relations: List[CrossDomainRelation]                                │  │
│  │  metadata: Dict[str, Any]                                             │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              ↕                                           │
│         ┌─────────────────────────────────────────────┐                   │
│         │         Four Dimensions (4 Domains)         │                   │
│         └─────────────────────────────────────────────┘                   │
│                                                                             │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│   │  Domain 1    │  │  Domain 2    │  │  Domain 3    │  │  Domain 4    │  │
│   │  Hardware    │  │  Drivers     │  │  BLE         │  │  Applications│  │
│   │              │  │              │  │              │  │              │  │
│   │ 442 entries  │  │ 412 entries  │  │ 38 entries   │  │ 0 entries    │  │
│   └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│          │                 │                 │                 │           │
│          └─────────────────┴─────────────────┴─────────────────┘           │
│                              │                                           │
│                    ┌───────┴───────┐                                     │
│                    │  119 Relations │                                     │
│                    └───────┬───────┘                                     │
│                            │                                           │
└────────────────────────────┼───────────────────────────────────────────────┘
                             │
                             ▼
═══════════════════════════════════════════════════════════════════════════════
```

---

## 四维域分类

### Domain 1: Hardware (硬件域)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Domain 1: Hardware & Registers (442 entries)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Entry Types:                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  REGISTER (368)         - 硬件寄存器定义                                │   │
│  │  PERIPHERAL            - 外设 (UART, SPI, I2C, etc.)                   │   │
│  │  PIN_MUX (65)          - 引脚复用配置                                  │   │
│  │  MEMORY_REGION (2)     - 内存区域 (FLASH, SRAM, XIP)                   │   │
│  │  INTERRUPT_VECTOR      - 中断向量 / ISR                               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Data Sources:                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • SVD File (gen/SVD/DragonC.svd)                                      │   │
│  │ • Excel Specs (doc/SW_Spec/*.xlsx)                                    │   │
│  │ • Linker Scripts (core/**/*.ld)                                        │   │
│  │ • Startup Files (core/**/startup*.s)                                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Example Entry:                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  {                                                                         │   │
│    "id": "hardware:register:UART1_CTRL",                                  │   │
│    "name": "UART1_CTRL",                                                   │   │
│    "brief": "UART1 Control Register",                                      │   │
│    "attributes": {                                                         │   │
│      "peripheral": "UART1",                                                │   │
│      "address": "0x40011000",                                             │   │
│      "access": "read-write",                                               │   │
│      "fields": [...]                                                      │   │
│    }                                                                       │   │
│  }                                                                         │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Domain 2: Drivers (驱动域)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Domain 2: SOC Drivers (412 entries)                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Entry Types:                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  FUNCTION (18)          - 驱动函数 / API                               │   │
│  │  STRUCT (38)            - 结构体定义 (配置结构体)                      │   │
│  │  MACRO (293)           - 预处理器宏 / 常量                            │   │
│  │  ENUM (89)             - 枚举类型                                      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Data Sources:                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • C Header Files (drivers/api/*.h)                                   │   │
│  │ • C Source Files (drivers/src/*.c)                                   │   │
│  │ • USB Headers (usb/api/*.h)                                           │   │
│  │ • USB Sources (usb/src/*.c)                                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Example Entry:                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  {                                                                         │   │
│    "id": "drivers:function:B6x_UART_Init",                                │   │
│    "name": "B6x_UART_Init",                                                │   │
│    "brief": "Initialize UART peripheral",                                  │   │
│    "file_path": "drivers/api/uart.h",                                     │   │
│    "attributes": {                                                         │   │
│      "return_type": "void",                                                │   │
│      "parameters": [                                                       │   │
│        {"type": "UART_TypeDef*", "name": "UARTx"},                          │   │
│        {"type": "UART_Config*", "name": "config"}                           │   │
│      ]                                                                     │   │
│    }                                                                       │   │
│  }                                                                         │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Domain 3: BLE (BLE 协议栈域)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Domain 3: BLE Stack (38 entries)                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Entry Types:                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  BLE_API               - BLE 协议栈 API                               │   │
│  │  GATT_SERVICE          - GATT 服务定义                                │   │
│  │  GATT_CHARACTERISTIC   - GATT 特征定义                                │   │
│  │  BLE_ERROR (160)       - BLE 错误码                                   │   │
│  │  MESH_API             - Mesh API                                        │   │
│  │  MESH_MODEL           - Mesh 模型                                      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Data Sources:                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • BLE Headers (ble/api/*.h)                                            │   │
│  │ • BLE Sources (ble/src/*.c)                                            │   │
│  │ • GATT Profiles (ble/prf/*)                                            │   │
│  │ • Error Codes (ble/api/le_err.h)                                       │   │
│  │ • Mesh APIs (mesh/api/*.h)                                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Example Entry:                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  {                                                                         │   │
│    "id": "ble:ble_api:ke_connected",                                      │   │
│    "name": "ke_connected",                                                 │   │
│    "brief": "Connection complete event",                                   │   │
│    "attributes": {                                                         │   │
│      "event_id": "KE_CONNECTED",                                           │   │
│      "parameters": ["conn_idx", "role"]                                   │   │
│    }                                                                       │   │
│  }                                                                         │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Domain 4: Applications (应用域)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Domain 4: Applications & Context (待扩展)                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Entry Types:                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  EXAMPLE               - 代码示例 / 片段                               │   │
│  │  CALL_CHAIN            - 函数调用链分析                               │   │
│  │  INIT_SEQUENCE         - 外设初始化序列                               │   │
│  │  PROJECT_CONFIG        - 项目配置 (编译选项等)                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Data Sources:                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • Examples (examples/*)                                                │   │
│  │ • Projects (projects/*)                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 条目类型详解

### EntryType 枚举

```python
class EntryType(Enum):
    # Domain 1: Hardware
    REGISTER         = "register"           # 硬件寄存器
    PERIPHERAL       = "peripheral"         # 外设
    PIN_MUX          = "pin_mux"            # 引脚复用
    MEMORY_REGION    = "memory_region"      # 内存区域
    INTERRUPT_VECTOR = "interrupt_vector"   # 中断向量

    # Domain 2: Drivers
    FUNCTION         = "function"           # 驱动函数
    MACRO            = "macro"              # 宏定义
    ENUM             = "enum"               # 枚举类型
    STRUCT           = "struct"             # 结构体

    # Domain 3: BLE
    BLE_API          = "ble_api"            # BLE API
    GATT_SERVICE     = "gatt_service"       # GATT 服务
    GATT_CHARACTERISTIC = "gatt_characteristic"  # GATT 特征
    BLE_ERROR        = "ble_error"          # BLE 错误码

    # Domain 4: Applications
    EXAMPLE          = "example"            # 示例代码
    CALL_CHAIN       = "call_chain"         # 调用链
    INIT_SEQUENCE    = "init_sequence"      # 初始化序列
    PROJECT_CONFIG   = "project_config"     # 项目配置
```

---

## 关系类型详解

### 跨域关系 (Cross-Domain Relations)

```
═══════════════════════════════════════════════════════════════════════════════
                        跨域关系类型
═══════════════════════════════════════════════════════════════════════════════

Hardware ↔ Drivers:
┌─────────────────────────────────────────────────────────────────────────────┐
│  CONTROLS_REGISTER      → API 写入/控制寄存器                                │
│  READS_REGISTER         → API 读取寄存器                                     │
│  CONFIGURES_REGISTER    → API 配置寄存器 (通常是初始化函数)                    │
│  REGISTERS_FOR          → 寄存器属于某个外设                                 │
└─────────────────────────────────────────────────────────────────────────────┘

Drivers ↔ BLE:
┌─────────────────────────────────────────────────────────────────────────────┐
│  USES_DRIVER            → BLE API 使用驱动 API (如 BLE UART 使用 DMA)           │
│  DEPENDS_ON             → 通用依赖关系                                         │
│  IMPLEMENTS_PROFILE      → Profile 实现 GATT 服务                            │
└─────────────────────────────────────────────────────────────────────────────┘

All → Applications:
┌─────────────────────────────────────────────────────────────────────────────┐
│  USED_IN_EXAMPLE        → API/结构体在示例中使用                              │
│  DEMONSTRATES           → 示例展示了某个 API/功能                            │
│  CALLED_IN_SEQUENCE      → API 按特定顺序调用 (初始化序列)                   │
└─────────────────────────────────────────────────────────────────────────────┘

Source Code Relations (P1):
┌─────────────────────────────────────────────────────────────────────────────┐
│  IMPLEMENTED_IN          → API 声明在源文件中实现                             │
│  DECLARED_IN            → API 在头文件中声明                                 │
└─────────────────────────────────────────────────────────────────────────────┘

Power Relations (P2):
┌─────────────────────────────────────────────────────────────────────────────┐
│  CONSUMES_POWER          → API/函数在特定模式下消耗功耗                       │
└─────────────────────────────────────────────────────────────────────────────┘

Dependency Relations:
┌─────────────────────────────────────────────────────────────────────────────┐
│  REQUIRES               → 需要某个前置条件                                   │
│  REQUIRES_CLOCK         → API 需要外设时钟使能                               │
│  REQUIRES_GPIO          → API 需要 GPIO 配置                               │
│  CONFLICTS_WITH         → 互相排斥 (如引脚冲突)                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 数据模型

### KnowledgeGraphEntry

```python
@dataclass
class KnowledgeGraphEntry:
    # 基本标识
    id: str                    # 格式: domain:type:name
    domain: DomainType          # hardware, drivers, ble, applications
    entry_type: EntryType       # 具体类型

    # 内容
    name: str                  # 名称
    brief: str                 # 简短描述
    detailed: str              # 详细描述

    # AI 上下文
    ai_use_case: str           # AI 使用提示
    ai_context: Dict           # AI 特定上下文

    # 源码位置
    file_path: str             # 源文件路径
    line_number: int           # 行号

    # 关系
    relations: List[CrossDomainRelation]  # 跨域关系列表

    # 域特定数据
    attributes: Dict           # 灵活的 JSON 数据

    # 元数据
    tags: List[str]            # 搜索/组织标签
    confidence: float          # 解析置信度 (0-1)
    created_at: str            # 创建时间戳
    updated_at: str            # 更新时间戳
```

### CrossDomainRelation

```python
@dataclass
class CrossDomainRelation:
    source_id: str             # 源条目 ID
    target_id: str             # 目标条目 ID
    relation_type: RelationType  # 关系类型
    source_domain: DomainType  # 源域
    target_domain: DomainType  # 目标域
    metadata: Dict             # 关系特定数据
    confidence: float          # 置信度 (0-1)
```

### ID 格式

```
格式: domain:entry_type:name

示例:
┌─────────────────────────────────────────────────────────────┐
│ hardware:register:UART1_CTRL                                 │
│ hardware:pin_mux:PA09                                       │
│ drivers:function:B6x_UART_Init                              │
│ drivers:struct:UART_Config                                 │
│ drivers:macro:UART_WORD_LENGTH_8B                           │
│ ble:ble_api:ke_connected                                    │
│ ble:ble_error:ERR_INVALID_HDL                              │
│ applications:example:uart_basic_main                         │
└─────────────────────────────────────────────────────────────┘
```

---

## 示例图谱

### 完整示例: UART 外设

```
═══════════════════════════════════════════════════════════════════════════════
                    示例图谱: UART 外设完整关系
═══════════════════════════════════════════════════════════════════════════════

Domain 1: Hardware
───────────────────────────────────────────────────────────────────────────────

hardware:register:UART1_CTRL
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: UART1_CTRL                                                        │
  │ brief: UART1 Control Register                                          │
  │ attributes: {address: "0x40011000", access: "read-write"}              │
  └──────────────────────────────────────────────────────────────────────┘
                             │
                             │ configures_register
                             ▼
hardware:register:UART1_BRR
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: UART1_BRR                                                         │
  │ brief: UART1 Baud Rate Register                                        │
  │ attributes: {address: "0x40011004", access: "read-write"}              │
  └──────────────────────────────────────────────────────────────────────┘
                             │
                             │ configures_register
                             ▼

Domain 2: Drivers
───────────────────────────────────────────────────────────────────────────────

drivers:function:B6x_UART_Init
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: B6x_UART_Init                                                     │
  │ brief: Initialize UART peripheral                                      │
  │ file_path: drivers/api/uart.h                                         │
  │                                                                         │
  │ relations:                                                             │
  │   ├─ configures_register → hardware:register:UART1_CTRL             │
  │   ├─ configures_register → hardware:register:UART1_BRR             │
  │   ├─ requires_clock → "RCC_PERIPH_UART1"                            │
  │   ├─ requires_gpio → "PA09 (TX), PA10 (RX)"                         │
  │   ├─ declared_in → "drivers/api/uart.h"                             │
  │   └─ implemented_in → "drivers/src/uart.c:125"                      │
  └──────────────────────────────────────────────────────────────────────┘

drivers:struct:UART_Config
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: UART_Config                                                       │
  │ brief: UART configuration structure                                    │
  │ attributes: {                                                           │
  │   fields: [                                                              │
  │     {name: "baudrate", type: "uint32_t"},                              │
  │     {name: "word_length", type: "UART_WordLength"},                    │
  │     {name: "stop_bits", type: "UART_StopBits"}                         │
  │   ]                                                                     │
  │ }                                                                       │
  └──────────────────────────────────────────────────────────────────────┘

drivers:macro:UART_WORD_LENGTH_8B
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: UART_WORD_LENGTH_8B                                              │
  │ brief: 8-bit word length                                               │
  │ attributes: {value: "0x00000000"}                                     │
  └──────────────────────────────────────────────────────────────────────┘
                             │
                             │ used_in_example
                             ▼

Domain 4: Applications
───────────────────────────────────────────────────────────────────────────────

applications:example:uart_basic_example
  ┌──────────────────────────────────────────────────────────────────────┐
  │ name: uart_basic_example                                                │
  │ brief: Basic UART send/receive example                                │
  │ file_path: examples/peripheral/uart/uart_basic/main.c                  │
  │                                                                         │
  │ relations:                                                             │
  │   ├─ uses → drivers:function:B6x_UART_Init                           │
  │   ├─ uses → drivers:function:B6x_UART_Transmit                        │
  │   ├─ uses → drivers:struct:UART_Config                               │
  │   └─ demonstrates → "UART initialization and data transfer"          │
  └──────────────────────────────────────────────────────────────────────┘
```

### 关系网络可视化

```
                    硬件域                    驱动域                    应用域
                      │                        │                        │
┌─────────────────────┐         ┌─────────────────────┐         ┌─────────────────────┐
│  UART1_CTRL         │◄────────┤  B6x_UART_Init      │◄────────┤  uart_basic_example  │
│  (register)          │         │  (function)          │         │  (example)           │
│  address: 0x40011000 │         │  file: uart.h        │         │  file: main.c       │
└─────────────────────┘         └─────────────────────┘         └─────────────────────┘
        │                                │                                │
        │ configures_register          │ used_in_example               │
        ▼                                ▼                                ▼
┌─────────────────────┐         ┌─────────────────────┐         ┌─────────────────────┐
│  UART1_BRR          │         │  UART_Config         │         │  init_sequence      │
│  (register)          │         │  (struct)            │         │  (call_chain)        │
│  address: 0x40011004 │         │  baudrate, etc.      │         │  [Init → TX → RX]    │
└─────────────────────┘         └─────────────────────┘         └─────────────────────┘
        │
        │ configures_register
        ▼
┌─────────────────────┐
│  PA09, PA10         │
│  (pin_mux)           │
│  UART1_TX, RX        │
└─────────────────────┘
```

---

## 查询模式

### 1. 按域查询

```python
# 获取硬件域所有条目
hardware_entries = graph.get_domain_entries("hardware")

# 获取驱动域所有条目
driver_entries = graph.get_domain_entries("drivers")
```

### 2. 按名称查询

```python
# 查找所有名为 UART_Init 的条目
results = graph.find_by_name("UART_Init")

# 在特定域中查找
uart_apis = graph.find_by_name("B6x_UART_Init", domain="drivers")
```

### 3. 按关系类型查询

```python
# 获取条目的所有关系
entry = graph.get_entry("drivers:function:B6x_UART_Init")

# 获取特定类型的关系
config_relations = entry.get_relations_by_type(RelationType.CONFIGURES_REGISTER)

# 获取指向特定域的关系
hardware_relations = entry.get_relations_to_domain("hardware")
```

### 4. 跨域遍历

```python
# 查找 API 控制的所有寄存器
api = graph.get_entry("drivers:function:B6x_UART_Init")
for relation in api.relations:
    if relation.relation_type == RelationType.CONFIGURES_REGISTER:
        register = graph.get_entry(relation.target_id)
        print(f"{api.name} configures {register.name}")
```

### 5. 关系链追踪

```python
# 追踪完整的初始化链
# API → 寄存器 → 外设 → 引脚
api = graph.get_entry("drivers:function:B6x_UART_Init")

for relation in api.relations:
    if relation.relation_type == RelationType.CONFIGURES_REGISTER:
        register = graph.get_entry(relation.target_id)
        print(f"  → {register.name} (register)")

        # 查找引脚配置
        if register.entry_type == EntryType.REGISTER:
            for pin_rel in register.get_relations_by_type(RelationType.REGISTERS_FOR):
                pin = graph.get_entry(pin_rel.target_id)
                print(f"    → {pin.name} (pin)")
```

---

## 统计信息

### 当前图谱规模

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Total Entries:      892                                                     │
│  Total Relations:     119                                                     │
│                                                                             │
│  Domain Statistics:                                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  hardware:      442 entries  (49.6%)                                 │   │
│  │  drivers:       412 entries  (46.2%)                                 │   │
│  │  ble:            38 entries  ( 4.3%)                                 │   │
│  │  applications:    0 entries  ( 0.0%)                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Entry Type Statistics:                                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  register:       368                                                 │   │
│  │  function:        18                                                 │   │
│  │  struct:          38                                                 │   │
│  │  macro:          293                                                 │   │
│  │  enum:            89                                                 │   │
│  │  ble_api:         ...                                                 │   │
│  │  ble_error:      160                                                 │   │
│  │  pin_mux:         65                                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 相关文档

- **[架构设计](architecture.md)** - 系统架构
- **[构建流程图](build-flow-diagram.md)** - 数据构建流程
- **[build_guide.md](build_guide.md)** - 构建指南
