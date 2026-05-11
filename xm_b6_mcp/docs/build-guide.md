# 构建与集成指南

本文档面向开发者和维护者，介绍 B6x MCP 服务器的构建流程和操作步骤。

**详细原理说明请参阅**: [架构文档](architecture.md)

## 目录

1. [环境要求](#1-环境要求)
2. [构建脚本说明](#2-构建脚本说明)
3. [知识图谱构建](#3-知识图谱构建)
4. [文档索引构建](#4-文档索引构建)
5. [故障排查](#5-故障排查)

---

## 1. 环境要求

### 1.1 Python 环境

**Python 版本**: Python 3.8+

**安装依赖**:
```bash
cd xm_b6_mcp
pip install -r requirements.txt
```

**核心依赖**:
- `mcp` - MCP 服务器框架
- `tree-sitter` - C 代码解析
- `tree-sitter-c` - C 语言语法
- `whoosh` - 全文搜索引擎
- `pydantic` - 数据验证
- `openpyxl` - Excel 解析
- `python-docx` - Word 文档解析
- `pypdf2` - PDF 文档解析

### 1.2 SDK 路径配置

默认 SDK 路径为 `../sdk6`（相对于 xm_b6_mcp 目录）。

**自定义路径**（如需要）:
```bash
# Linux/macOS
export B6X_SDK_PATH="D:/svn/bxx_DragonC1/sdk6"

# Windows
set B6X_SDK_PATH=D:/svn/bxx_DragonC1/sdk6
```

---

## 2. 构建脚本说明

### 2.1 构建脚本概览

| 脚本 | 功能 | 运行时间 | 使用场景 |
|------|------|----------|----------|
| `build_all_indices.py` | 主构建脚本 (Excel + Whoosh + KG) | 2-5 分钟 (全量)<br>30-60 秒 (增量) | 首次构建或 SDK 更新后 |
| `build_document_index.py` | 文档索引构建 | 1-2 分钟 | 只更新文档索引 |
| `build_excel_constraints.py` | Excel 约束解析 | 10-20 秒 | 只更新约束文件 |
| `build_index_v2.py` | 知识图谱构建 | 20-30 秒 | 只更新知识图谱 |
| `build_logger.py` | 日志工具模块 | - | 被其他脚本调用 |
| `generate_data_quality_report.py` | 数据质量报告 | 5-10 秒 | 验证数据完整性 |

**增量构建参数**:
- `--incremental` - 只处理修改的文件 (推荐)
- `--force` - 强制全量重建 (忽略缓存)
- `--clean <TARGETS>` - 选择性清理后重建

### 2.2 脚本详细说明

#### build_all_indices.py - 主构建脚本

完整的索引构建脚本，协调所有子脚本的执行。

```bash
# 完整重建 (2-5 分钟)
python scripts/build_all_indices.py

# 增量构建 (30-60 秒，推荐)
python scripts/build_all_indices.py --incremental

# 选择性清理 + 重建
python scripts/build_all_indices.py --clean whoosh
python scripts/build_all_indices.py --clean domain:drivers

# 强制全量重建 (忽略缓存)
python scripts/build_all_indices.py --force

# 显示详细信息
python scripts/build_all_indices.py --verbose
```

**构建流程**:
1. **清理阶段** (可选): 清理指定目标
2. **Excel 约束构建**: 解析 Excel 文件 → `data/constraints/`
3. **文档索引构建**: 解析文档 → `data/whoosh_index/`
4. **API 索引构建**: Tree-sitter 解析 C 代码 → Whoosh
5. **知识图谱构建**: 整合所有域 → `data/knowledge_graph.json`
6. **生成报告**: `data/build_report_all.json`

#### build_document_index.py - 文档索引构建

解析 SDK 文档 (Word/PDF/Excel/Markdown) 并索引到 Whoosh 搜索引擎。

```bash
# 全量构建
python scripts/build_document_index.py

# 增量构建 (只处理修改的文档)
python scripts/build_document_index.py --incremental

# 强制重建
python scripts/build_document_index.py --force

# 显示详细信息
python scripts/build_document_index.py --verbose
```

**处理流程**:
1. 扫描 `doc/` 目录中的文档文件
2. 按文件类型解析 (Word/PDF/Excel/Markdown)
3. 分类到 5 个域 (Hardware/Drivers/BLE/Applications/Toolchain)
4. 索引到 Whoosh 搜索引擎
5. 生成统计报告

**输出**:
- `data/parsed_documents.json` - 解析后的文档数据
- `data/whoosh_index/` - Whoosh 搜索索引
- `data/document_index_report.json` - 索引报告

#### build_excel_constraints.py - Excel 约束解析

解析 `doc/SW_Spec/` 目录中的 Excel 规格文件，生成硬件约束 JSON 文件。

```bash
python scripts/build_excel_constraints.py
```

**输出文件** (`data/constraints/`):
| 文件 | 说明 |
|------|------|
| `io_map.json` | 引脚复用配置 (65 引脚) |
| `flash_map.json` | Flash 内存映射 |
| `sram_map.json` | SRAM 分配表 |
| `memory_boundaries.json` | 内存边界约束 |
| `power_consumption.json` | 功耗数据 |
| `phone_compatibility_issues.json` | 手机兼容性问题 |

**数据来源**:
- `B6X IO Map.xlsx` → `io_map.json`
- `B6X Memory.xlsx` → `flash_map.json`, `sram_map.json`
- `B6X Power.xlsx` → `power_consumption.json`
- `B6X Phone Compatibility.xlsx` → `phone_compatibility_issues.json`

#### build_index_v2.py - 知识图谱构建

构建四维知识图谱，整合所有 SDK 域数据。

```bash
# 完整构建
python scripts/build_index_v2.py

# 只构建特定域
python scripts/build_index_v2.py --domain hardware
python scripts/build_index_v2.py --domain drivers
python scripts/build_index_v2.py --domain ble
python scripts/build_index_v2.py --domain applications

# 显示统计信息
python scripts/build_index_v2.py --stats

# 测试模式 (验证构建结果)
python scripts/build_index_v2.py --test
```

**域构建顺序** (依赖关系):
1. **Hardware** - 寄存器、引脚复用、内存映射、中断向量
2. **Drivers** - APIs、结构体、宏定义、依赖关系
3. **BLE** - BLE APIs、GATT Profiles、错误码
4. **Applications** - 示例代码、调用链、配置

**输出**:
- `data/knowledge_graph.json` - 统一知识图谱
- `data/domain/{domain}/` - 各域独立数据
- `data/relations/` - 跨域关系映射

#### build_logger.py - 日志工具模块

增强的日志工具模块，为其他构建脚本提供统一的日志功能。

**特性**:
- 彩色控制台输出 (Windows/Linux/macOS)
- 文件日志记录 (UTF-8 编码)
- 进度跟踪和文件处理统计
- 诊断信息和异常追踪
- 构建摘要生成

**使用示例**:
```python
from build_logger import BuildLogger

# 创建日志器
logger = BuildLogger("my_script")

# 日志方法
logger.info("Processing files...")
logger.warning("File format deprecated")
logger.error("Failed to parse", details={"file": "test.xlsx"})

# 文件处理日志
logger.file_start("uart.h")
logger.file_success("uart.h", "15 APIs indexed")
logger.file_error("invalid.h", Exception("Parse error"))

# 输出构建摘要
logger.summary()
```

**日志文件位置**: `data/logs/{script_name}_{timestamp}.log`

#### generate_data_quality_report.py - 数据质量报告

分析生成的数据文件，生成数据质量报告。

```bash
python scripts/generate_data_quality_report.py
```

**分析内容**:
- **约束文件分析**: 检查数据来源 (Excel 解析 vs 默认生成)
- **域数据覆盖**: 各域文件数量和大小
- **关系文件**: 跨域关系数量统计
- **质量评分**: 基于默认数据占比计算

**输出**:
- **控制台报告** (人类可读):
  ```
  OVERALL SUMMARY
  ----------------------------------------------------------------------
  Total Constraint Files:  7
  Total Domain Files:      12
  Quality Score:           EXCELLENT
  Default Data Percentage: 0%

  DATA SOURCE BREAKDOWN
  ----------------------------------------------------------------------
  excel_parsed              7 files (100.0%)
  ```

- **JSON 报告** (`data/data_quality_report.json`):
  ```json
  {
    "generated_at": "2026-03-02T...",
    "analysis": {
      "overall": {
        "quality_score": "EXCELLENT",
        "default_data_percentage": 0
      }
    }
  }
  ```

**质量评分标准**:
| 评分 | 默认数据占比 | 说明 |
|------|-------------|------|
| EXCELLENT | 0% | 所有数据来自实际解析 |
| GOOD | < 20% | 少量默认数据 |
| FAIR | 20-50% | 部分默认数据，需验证 |
| POOR | > 50% | 大量默认数据，需补充 |


### 2.2 完整构建

**一键构建所有索引**（包括知识图谱）:
```bash
cd xm_b6_mcp
python scripts/build_all_indices.py
```

**增量构建** (SDK 小幅更新后) ⭐ NEW:
```bash
# 只处理修改的文件 (30-60 秒)
python scripts/build_all_indices.py --incremental

# 查看缓存统计
python scripts/build_all_indices.py --incremental --verbose
```

**选择性清理 + 重建** ⭐ NEW:
```bash
# 只清理并重建 Whoosh 索引
python scripts/build_all_indices.py --clean whoosh

# 只清理并重建特定域
python scripts/build_all_indices.py --clean domain:drivers

# 清理所有并强制重建
python scripts/build_all_indices.py --clean all --force
```

此脚本会自动完成：
1. 清理旧索引 (如果指定 `--clean`)
2. Excel 约束构建
3. 文档索引构建 (使用缓存，如果启用 `--incremental`)
4. API 索引构建 (使用缓存，如果启用 `--incremental`)
5. 四维知识图谱构建

---

## 3. 知识图谱构建

知识图谱构建已集成到主构建脚本 `build_all_indices.py` 中。

**单独构建知识图谱** (如需要):
```bash
python scripts/build_index_v2.py
```

**可选参数**:
```bash
# 只构建特定域
python scripts/build_index_v2.py --domain hardware

# 显示统计信息
python scripts/build_index_v2.py --stats
```

**输出位置**:
- `data/knowledge_graph.json` - 统一知识图谱
- `data/domain/{hardware,drivers,ble,applications}/` - 各域数据
- `data/relations/` - 跨域关系

---


## 4. 快速参考

### 4.1 输出文件结构

```
data/
├── knowledge_graph.json          # 统一知识图谱
├── whoosh_index/                 # Whoosh 文档搜索索引
├── parsed_documents.json         # 文档解析结果
├── build_report_all.json         # 构建报告
├── document_index_report.json    # 文档索引报告
├── data_quality_report.json      # 数据质量报告 ⭐ NEW
├── ble_error_codes.json          # BLE 错误码
├── logs/                         # 构建日志目录 ⭐ NEW
│   └── {script}_{timestamp}.log  # 各脚本的日志文件
├── constraints/                  # Excel 硬件约束
│   ├── io_map.json               # 引脚配置
│   ├── power_consumption.json    # 功耗数据
│   ├── flash_map.json            # Flash 映射
│   ├── sram_map.json             # SRAM 映射
│   ├── memory_boundaries.json    # 内存边界
│   ├── ble_compatibility.json    # BLE 兼容性
│   ├── phone_compatibility_issues.json
│   └── build_report.json
├── domain/                       # 各域数据
│   ├── hardware/
│   │   ├── pin_mux.json          # 引脚复用
│   │   ├── interrupts.json       # 中断定义
│   │   ├── memory_boundaries.json # 内存边界
│   │   ├── memory_regions.json   # 内存区域
│   │   ├── sram_regions.json     # SRAM 区域
│   │   └── power_consumption.json
│   ├── drivers/
│   │   ├── apis.json             # 171 个 API
│   │   ├── structs.json          # 38 个结构体
│   │   ├── macros.json           # 82 个宏定义
│   │   ├── enums.json            # 81 个枚举定义
│   │   ├── dependencies.json     # API 依赖关系
│   │   └── call_graph.json       # 调用图
│   ├── ble/
│   │   ├── apis.json             # 130 个 BLE API
│   │   ├── profiles.json         # GATT Profile
│   │   ├── error_codes.json      # 错误码
│   │   ├── mesh_apis.json        # Mesh API
│   │   └── mesh_models.json      # Mesh Model
│   └── applications/             # 示例项目数据
└── relations/                    # 跨域关系
    ├── relations.json            # 所有关系
    ├── relations_hardware.json   # Domain 1 关系
    ├── relations_drivers.json    # Domain 2 关系
    └── relations_ble.json        # Domain 3 关系
```

### 4.2 构建统计

| 指标 | 数值 |
|------|------|
| 知识图谱条目 | 892 |
| 跨域关系 | 119 |
| 寄存器定义 | 368 |
| 引脚配置 | 65 |
| 宏定义 | 293 |
| 枚举定义 | 81 |
| 结构体定义 | 38 |
| BLE 错误码 | 160 |
| 文档索引 | ~25 |

---

## 5. 相关文档

- [架构文档](architecture.md) - 系统架构和原理说明
- [Clang 集成](clang-integration.md) - Clang 语义分析方案
- [工具使用指南](tools-usage-guide.md) - 开发工具使用说明
- [README](../README.md) - 项目总览

---

**文档版本**: v3.5.0
**最后更新**: 2026-03-16
