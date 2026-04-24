# B6x MCP Server 数据构建流程图

本文档详细说明了 `build_all_indices.py` 和 `build_index_v2.py` 两个构建脚本的数据流程。

---

## 目录

1. [总体架构](#总体架构)
2. [构建脚本分工](#构建脚本分工)
3. [详细流程图](#详细流程图)
4. [数据统计](#数据统计)
5. [使用方式](#使用方式)

---

## 总体架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SDK6 原始文件                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │   doc/          │  │   drivers/      │  │   ble/          │             │
│  │   ├── *.pdf     │  │   ├── api/*.h   │  │   ├── api/*.h   │             │
│  │   ├── *.docx    │  │   └── src/*.c   │  │   ├── src/*.c   │             │
│  │   └── SW_Spec/  │  │                 │  │   └── prf/      │             │
│  │       ├── *.xlsx │  │  usb/           │  │                 │             │
│  │       └── *.doc  │  │   ├── api/*.h   │  │  mesh/          │             │
│  │                 │  │   └── src/*.c   │  │   └── api/*.h   │             │
│  │  core/          │  │                 │  │                 │             │
│  │   ├── *.svd     │  │  examples/      │  │                 │             │
│  │   └── *.ld      │  │   └── *.c      │  │                 │             │
│  │                 │  │                 │  │                 │             │
│  │  projects/      │  │                 │  │                 │             │
│  │   └── *.c      │  │                 │  │                 │             │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 构建脚本分工

| 脚本 | 主要功能 | 输出 |
|------|---------|------|
| **build_all_indices.py** | 1. 清理索引<br>2. 构建 Excel 约束<br>3. 构建文档索引<br>4. 构建 API 索引 | Whoosh 索引<br>constraints/*.json<br>parsed_documents.json |
| **build_index_v2.py** | 1. 构建 4 个域<br>2. 构建跨域关系<br>3. 导出统一图谱 | domain/*.json<br>relations/*.json<br>knowledge_graph.json |

---

## 详细流程图

### 构建脚本入口点

```
═══════════════════════════════════════════════════════════════════════════════
                          构建脚本入口点
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────────────────────────────────────────────────────┐
│  python scripts/build_all_indices.py [OPTIONS]                             │
│  ─────────────────────────────────────────────────────────                │
│  Master Script - 完整重建                                                   │
│                                                                             │
│  Options:                                                                   │
│    --incremental    增量构建 (只处理修改的文件) ⭐ NEW                        │
│    --force          强制全量重建                                            │
│    --clean <TARGET> 选择性清理 [all|whoosh|constraints|documents|cache|     │
│                     domain:hardware|domain:drivers|domain:ble] ⭐ NEW       │
│    --verbose        详细输出                                                │
│                                                                             │
│  Step 0: 初始化 BuildCacheManager ⭐ NEW                                    │
│         └── 加载 file_hashes.json                                           │
│         └── 检测文件修改 (MD5 hash)                                          │
│                                                                             │
│  Step 1: 清理旧索引 (如果指定 --clean) ⭐ NEW                               │
│         └── 支持细粒度清理: whoosh, constraints, cache, domain:*           │
│                                                                             │
│  Step 2: python scripts/build_excel_constraints.py                          │
│         └── 生成 data/constraints/*.json                                     │
│                                                                             │
│  Step 3: python scripts/build_document_index.py [--incremental] ⭐ NEW      │
│         └── 只解析修改的文档 (使用缓存)                                     │
│         └── 解析 SDK 文档 → Whoosh                                           │
│                                                                             │
│  Step 4: build_api_index() 内置函数 [--incremental] ⭐ NEW                 │
│         └── 只解析修改的 C 头文件 (使用缓存)                                │
│         └── 解析 C 代码 → Whoosh                                             │
│                                                                             │
│  Step 5: python scripts/build_index_v2.py ⭐ 已集成                        │
│         └── 构建四维知识图谱                                                 │
│                                                                             │
│  Output: data/build_report_all.json (包含缓存统计) ⭐ NEW                   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Excel Constraints Builder

```
═══════════════════════════════════════════════════════════════════════════════
                          Excel Constraints Builder
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────┐
│ build_excel_constraints.py  │
├─────────────────────────────┤
│                             │
│ Excel Parser:               │
│ ┌─────────────────────────┐ │
│ │ B6x_IO_MAP.xlsx         │─┼──→ io_map.json
│ └─────────────────────────┘ │
│                             │
│ ┌─────────────────────────┐ │
│ │ B6x_BLE-功耗参考.xlsx   │─┼──→ power_consumption.json
│ └─────────────────────────┘ │
│                             │
│ ┌─────────────────────────┐ │
│ │ B6x-Flash-Map.xlsx      │─┼──→ flash_map.json
│ └─────────────────────────┘ │
│                             │
│ ┌─────────────────────────┐ │
│ │ B6x_BLE芯片兼容性.xlsx   │─┼──→ ble_compatibility.json
│ └─────────────────────────┘ │
│                             │
│ ┌─────────────────────────┐ │
│ │ B6x_SRAM_Map.xlsx       │─┼──→ sram_map.json
│ └─────────────────────────┘ │
│                             │
│ → build_report.json         │
└─────────────────────────────┘
```

### Document Index Builder

```
═══════════════════════════════════════════════════════════════════════════════
                          Document Index Builder
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────┐
│ build_document_index.py     │
├─────────────────────────────┤
│                             │
│ Document Parser:            │
│ ┌─────────────────────────┐ │
│ │ SDK 文档                │ │
│ │ ├── *.pdf               │ │
│ │ ├── *.docx              │ │
│ │ └── *.excel             │ │
│ └─────────────────────────┘ │
│           ↓                  │
│   parsed_documents.json     │
│                             │
│ Whoosh Indexer:             │
│ ┌─────────────────────────┐ │
│ │ WHOOSH 全文索引          │ │
│ │ → data/whoosh_index/    │ │
│ └─────────────────────────┘ │
│                             │
│ → document_index_report.json│
└─────────────────────────────┘
```

### Whoosh Index (全文索引)

```
═══════════════════════════════════════════════════════════════════════════════
                         Whoosh Index (Whoosh 全文索引)
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────────────────────────────────────────────────────┐
│ data/whoosh_index/                                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  From Excel Constraints:                                                    │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │ • io_map.json              → 引脚复用配置索引                   │        │
│  │ • power_consumption.json   → 功耗模式索引                      │        │
│  │ • flash_map.json           → Flash 内存布局索引                 │        │
│  │ • sram_map.json            → SRAM 内存布局索引                  │        │
│  │ • ble_compatibility.json   → BLE 兼容性索引                     │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│                                                                             │
│  From Documents:                                                            │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │ • *.pdf, *.docx 文档      → SDK 文档全文索引                    │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│                                                                             │
│  From C Code (Tree-sitter):                                                │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │ drivers/api/*.h  ──┐                                             │        │
│  │ usb/api/*.h       ──┼──→ [函数解析] ──→ Whoosh                   │        │
│  │ ble/api/*.h       ──┘                                             │        │
│  │                                                                  │        │
│  │ 索引内容: 函数名、参数、返回值、Doxygen 注释、宏定义、枚举类型     │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│                                                                             │
│  Index Files:                                                              │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │ MAIN_*.seg              → 索引段文件                             │        │
│  │ _MAIN_*.toc             → 索引表文件                             │        │
│  │ MAIN_WRITELOCK          → 写锁文件                               │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Knowledge Graph Builder

```
═══════════════════════════════════════════════════════════════════════════════
                 python scripts/build_index_v2.py
           (Four-Dimensional Knowledge Graph Builder)
═══════════════════════════════════════════════════════════════════════════════

┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│  Phase 1: Hardware & Registers Domain                                       │
│  ─────────────────────────────────────                                      │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ SVD Parser + BLE User Guide                                          │   │
│  │ gen/SVD/DragonC.svd + doc/.../B6x_BLE芯片使用指南.docx                 │   │
│  │   ↓                                                                    │   │
│  │ data/domain/hardware/interrupts.json + pin_mux.json                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Excel Parser                                                          │   │
│  │ doc/SW_Spec/B6x_IO_MAP.xlsx  → pin_mux.json                           │   │
│  │ doc/SW_Spec/B6x_BLE-功耗参考.xlsx  → power_consumption.json          │   │
│  │ doc/SW_Spec/B6x-Flash-Map.xlsx  → memory_regions.json                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Linker Parser + Interrupt Parser                                     │   │
│  │ core/**/*.ld  → memory_regions.json (补充)                           │   │
│  │ core/**/startup*.s  → interrupts.json                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Phase 2: SOC Drivers Domain                                                │
│  ────────────────────────────                                              │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Tree-sitter C Parser                                                  │   │
│  │ drivers/api/*.h + drivers/src/*.c                                    │   │
│  │ usb/api/*.h + usb/src/*.c                                            │   │
│  │   ↓                                                                    │   │
│  │ apis.json, macros.json, enums.json                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Struct Extractor                                                      │   │
│  │ drivers/api/*.h + usb/api/*.h  → structs.json                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Phase 3: BLE Stack Domain                                                  │
│  ───────────────────────────                                               │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Tree-sitter C Parser (BLE)                                            │   │
│  │ ble/api/*.h + ble/src/*.c  → apis.json                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Profile Parser                                                         │   │
│  │ ble/prf/*  → profiles.json                                             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ BLE Error Code Parser                                                  │   │
│  │ ble/api/le_err.h  → error_codes.json (160 个错误码)                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Mesh Parsers                                                          │   │
│  │ mesh/api/*  → mesh_apis.json, mesh_error_codes.json                    │   │
│  │ mesh/model/api/*  → mesh_models.json                                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Phase 4: Applications Domain                                               │
│  ───────────────────────────────                                            │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Example Scanner                                                        │   │
│  │ examples/*, projects/*  → examples.json                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Phase 5: Cross-Domain Relations                                            │
│  ────────────────────────────                                               │
│                                                                             │
│  Relation Mapper:                                                            │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ r1: Hardware ↔ Drivers (寄存器 → API, 结构体 → 寄存器)                 │   │
│  │ r2: Drivers ↔ BLE (Profile → API 依赖)                                │   │
│  │ r3: Applications → All (示例 → API, 初始化序列)                        │   │
│  │ r4: API Declaration → Implementation Mapping (.h → .c)                 │   │
│  │ r5: Profile → API Dependencies (AST-based)                             │   │
│  │ r6: Power Consumption → API Mapping                                    │   │
│  │ r7: Transitive Relations Inference                                      │   │
│  │                                                                      │   │
│  │   ↓                                                                  │   │
│  │ data/relations.json                                                  │   │
│  │ data/relations/relations_hardware.json                               │   │
│  │ data/relations/relations_drivers.json                                │   │
│  │ data/relations/relations_ble.json                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Phase 6: Unified Knowledge Graph Export                                    │
│  ─────────────────────────────────────────                                  │
│                                                                             │
│  → data/knowledge_graph.json                                               │
│     (包含所有域 + 关系 + 统计信息)                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 数据统计

### Whoosh 索引 (737 条目)

| 类型 | 数量 | 说明 |
|------|------|------|
| 函数 | 280 | API 函数 |
| 宏 | 260 | 宏定义 |
| 枚举 | 172 | 枚举类型 |
| 文档 | 25 | SDK 文档 |

### 知识图谱 (892 条目, 119 关系)

| 域 | 条目数 | 说明 |
|---|---|---|
| Hardware | 442 | 368 寄存器 + 65 引脚 + 7 功耗模式 + 2 内存区域 |
| Drivers | 412 | 18 API + 38 结构体 + 293 宏 + 89 枚举 |
| BLE | 38 | BLE API + GATT Profile + Mesh |
| Applications | 0 | 待扩展 |

### 硬件约束

| 类型 | 数量 | 说明 |
|------|------|------|
| 寄存器 | 368 | SVD 解析 |
| 引脚 | 65 | IO_MAP.xlsx |
| 功耗模式 | 7 | BLE-功耗参考.xlsx |

---

## 最终生成的 data/ 目录

```
data/
├── whoosh_index/                (Whoosh 全文索引)
│   ├── MAIN_*.seg               (索引段文件)
│   ├── _MAIN_*.toc              (索引表文件)
│   └── MAIN_WRITELOCK           (写锁)
│
├── constraints/                 (硬件约束)
│   ├── io_map.json              (65 个引脚配置)
│   ├── power_consumption.json   (7 种功耗模式)
│   ├── flash_map.json           (Flash 布局)
│   ├── sram_map.json            (SRAM 布局)
│   ├── memory_boundaries.json   (内存边界)
│   ├── ble_compatibility.json   (BLE 兼容性)
│   └── build_report.json        (构建报告)
│
├── domain/                      (分域数据)
│   ├── hardware/
│   │   ├── pin_mux.json          (引脚复用)
│   │   ├── interrupts.json       (中断定义)
│   │   ├── memory_boundaries.json (内存边界)
│   │   ├── memory_regions.json    (内存区域)
│   │   ├── sram_regions.json      (SRAM 区域)
│   │   └── power_consumption.json
│   ├── drivers/
│   │   ├── apis.json             (171 个 API)
│   │   ├── structs.json          (38 个结构体)
│   │   ├── macros.json           (82 个宏)
│   │   └── enums.json            (89 个枚举)
│   └── ble/
│       ├── apis.json
│       ├── mesh_apis.json
│       ├── mesh_error_codes.json
│       └── mesh_models.json
│
├── relations/                  (跨域关系)
│   ├── relations.json           (所有关系)
│   ├── relations_hardware.json  (硬件相关关系)
│   ├── relations_drivers.json   (驱动相关关系)
│   └── relations_ble.json       (BLE 相关关系)
│
├── knowledge_graph.json        (统一知识图谱)
├── parsed_documents.json       (解析的文档)
├── document_index_report.json  (文档索引报告)
├── build_report_all.json       (总构建报告)
└── regeneration_summary.txt    (重新生成摘要)
```

---

## 使用方式

### 完整重建

```bash
# 全量构建
python scripts/build_all_indices.py

# 增量构建 (只处理修改的文件) ⭐ NEW
python scripts/build_all_indices.py --incremental

# 查看缓存统计 ⭐ NEW
python scripts/build_all_indices.py --incremental --verbose
```

### 选择性清理 + 重建 ⭐ NEW

```bash
# 只清理并重建 Whoosh 索引
python scripts/build_all_indices.py --clean whoosh

# 只清理并重建特定域
python scripts/build_all_indices.py --clean domain:drivers

# 清理所有并强制重建
python scripts/build_all_indices.py --clean all --force
```

### 单域重建

```bash
# 仅重建硬件域
python scripts/build_index_v2.py --domain hardware

# 仅重建驱动域
python scripts/build_index_v2.py --domain drivers

# 仅重建 BLE 域
python scripts/build_index_v2.py --domain ble

# 仅重建应用域
python scripts/build_index_v2.py --domain applications
```

### 单域重建

```bash
# 仅重建硬件域
python scripts/build_index_v2.py --domain hardware

# 仅重建驱动域
python scripts/build_index_v2.py --domain drivers

# 仅重建 BLE 域
python scripts/build_index_v2.py --domain ble

# 仅重建应用域
python scripts/build_index_v2.py --domain applications
```

### 查看统计信息

```bash
python scripts/build_index_v2.py --stats
```

---

## 数据流向

```
原始文件 → [解析器] → 中间数据 → [索引器] → 最终索引

doc/SW_Spec/*.xlsx ──→ Excel Parser ──→ constraints/*.json ──→ Whoosh

drivers/api/*.h ──→ Tree-sitter ──→ 解析结果 ──┐
                                                   ├──→ Whoosh Index
ble/api/*.h ─────→ Tree-sitter ──→ 解析结果 ──┘
                                                   │
                                                   └──→ domain/*.json
                                                         │
                                                         └──→ knowledge_graph.json
```

---

## 相关文档

- **[架构设计](architecture.md)** - 系统架构、数据流程
- **[构建指南](build-guide.md)** - 详细的构建步骤（开发者）
- **[README](../README.md)** - 快速开始指南
