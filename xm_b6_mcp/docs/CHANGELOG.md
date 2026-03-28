# B6x MCP Server - 更新日志

---

## v3.5.0 - 2026-03-16

### 📚 文档重构：简化用户指南

**目标**：精简文档，聚焦用户使用，移除冗余内容。

**变更内容**：

1. **README.md 重构**：
   - 从 839 行精简到 174 行
   - 聚焦快速开始、核心工具、使用示例
   - 移除重复的技术细节，指向 architecture.md

2. **删除冗余文档**：
   - `edge_case_implementation.md` - 已完成的实施报告
   - `build-flow-diagram.md` - 与 build-guide.md 高度重复
   - `knowledge-graph-structure.md` - 内容已包含在 architecture.md
   - `libclang-deployment.md` - 内容已包含在 clang-integration.md

3. **文档重命名**：
   - `clang-integration-proposal.md` → `clang-integration.md`（已实施）

4. **文件移动**：
   - `docs/requirements.txt` → 根目录
   - `docs/requirements-dev.txt` → 根目录

**精简后文档结构**：

```
docs/
├── architecture.md           # 系统架构设计
├── build-guide.md            # 构建指南
├── clang-integration.md      # Clang 集成方案
├── config_directory_guide.md # 配置文件说明
├── tools-usage-guide.md      # 工具使用指南
└── CHANGELOG.md              # 版本历史
```

**文档减少**：从 10 个文档精简到 6 个（减少 40%）

---

## v3.4.1 - 2026-03-02

### 📚 文档更新：构建脚本与源码文档完善

**变更内容**：

新增完整的 `scripts/` 和 `src/` 目录文档：

| 目录 | 文件数 | 文档状态 |
|------|--------|----------|
| `scripts/` | 6 个脚本 | ✅ 已完善 |
| `src/` | 核心源码 | ✅ 新增文档 |

**更新内容**：

1. **README.md**：
   - 新增 "构建脚本说明" 章节
   - 新增 "源码结构" 章节
   - 添加所有 6 个脚本的详细使用说明
   - 包含命令示例、参数说明、输出文件
   - 更新版本历史

2. **docs/build-guide.md**：
   - 扩展脚本表格，包含所有 6 个脚本
   - 新增 `build_logger.py` 详细说明
   - 新增 `generate_data_quality_report.py` 详细说明
   - 更新输出文件结构（新增 `data_quality_report.json`、`logs/`）
   - 更新文档版本到 v3.4.1

3. **docs/architecture.md**：
   - 新增 5.4 节 "源码目录结构 (src/)"
   - 添加目录结构图
   - 核心文件说明表格
   - 设计模式说明
   - 代码示例和类型定义

**新增脚本文档**：

### build_logger.py

增强的日志工具模块，提供：
- 彩色控制台输出 (Windows/Linux/macOS 兼容)
- UTF-8 编码的文件日志
- 文件处理进度跟踪
- 构建摘要生成

```python
from build_logger import BuildLogger
logger = BuildLogger("my_script")
logger.file_start("uart.h")
logger.file_success("uart.h", "15 APIs indexed")
logger.summary()
```

### generate_data_quality_report.py

数据质量分析工具：
- 分析约束文件数据来源 (Excel 解析 vs 默认生成)
- 计算域数据覆盖率
- 生成质量评分 (EXCELLENT/GOOD/FAIR/POOR)
- 输出 JSON 和控制台报告

```bash
python scripts/generate_data_quality_report.py
# 输出: data/data_quality_report.json
```

**文档结构**：

```
docs/
├── architecture.md           # 系统架构 (已更新) ⭐
├── build-guide.md           # 构建指南 (已更新) ⭐
├── config_directory_guide.md # 配置说明 (已更新) ⭐
├── CHANGELOG.md             # 更新日志 (当前文件) ⭐
└── ...
```

**新增源码文档**：

### src/ 目录结构

新增 `docs/architecture.md` 5.4 节，包含：

| 文件 | 功能 | 设计模式 |
|------|------|----------|
| `main.py` | MCP 服务器入口 | - |
| `layer1_discovery.py` | 发现层 (search_sdk) | Composer Pattern |
| `layer2_detail.py` | 详情层 (inspect_node) | Strategy Table Pattern |
| `layer3_validation.py` | 验证层 (validate_config) | Chain of Responsibility |
| `domain_config.py` | 四维域配置 | - |

**源码目录结构**：
```
src/
├── main.py                    # MCP 服务器入口点
├── layer1_discovery.py        # Layer 1: 发现层
├── layer2_detail.py           # Layer 2: 详情层
├── layer3_validation.py       # Layer 3: 验证层
├── domain_config.py           # 四维域配置
├── common/                    # 通用工具模块
│   ├── config_schema.py       # 配置 Schema
│   ├── node_id.py             # Node ID 标准化
│   └── errors.py              # 错误类型
├── core/                      # 核心解析模块 (30+)
└── modules/                   # 模块工具 (兼容层)
```

---

## v3.2.4 - 2026-02-28

### 🗂️ 项目结构优化

**变更内容**：
- 移动开发工具到 `scripts/` 目录
  - `src/tools/demo_integration.py` → `scripts/demo_integration.py`
  - `src/tools/generate_dependency_overrides.py` → `scripts/generate_dependency_overrides.py`
- 删除空的 `src/tools/` 目录
- 新增工具使用文档：`docs/tools-usage-guide.md`
- 新增测试报告：`docs/tools-testing-report.md`

**理由**：
- 统一所有维护脚本到 `scripts/` 目录
- `src/` 目录只包含运行时代码
- 便于发现和使用开发工具

**新增文档**：
- 完整的工具使用指南（339 行）
- 包含使用场景、输出示例、故障排除
- 测试报告和验证结果

### 🔧 工具修复

**问题：** 移动后工具的导入路径和路径计算不正确

**修复内容：**
1. `demo_integration.py`
   - 修复 Python 模块导入路径
   - 从 `sys.path.insert(0, str(Path(__file__).parent.parent.parent))`
   - 改为 `sys.path.insert(0, str(Path(__file__).parent.parent / 'src'))`

2. `generate_dependency_overrides.py`
   - 修复 SDK 路径计算
   - 修复输出路径计算
   - 正确识别 `mcp_root` 和 `sdk_root`

**测试结果：**
- ✅ `demo_integration.py` - Layer 1 (ConfigLoader) 完全正常
  - 成功查询 UART 依赖关系
  - 成功查找 GPIO 相关 API
  - 成功获取 I2C 调用序列
  - 成功验证配置完整性
- ✅ `generate_dependency_overrides.py` - 完全正常
  - 成功提取 33 个初始化函数
  - 成功加载 24 个手动定义
  - 成功生成 37 个 YAML 条目
  - 输出文件：20KB

---

## v3.2.3 - 2026-02-28

### 🐛 Bug 修复：DocEngine 和 MacroEngine 实现缺失

**问题描述**：
- `DocEngine.search()` 返回硬编码的 mock 数据，未使用 Whoosh 文档索引
- `MacroEngine.search()` 返回硬编码的 mock 数据，未使用 Whoosh 宏索引
- `WhooshIndexBuilder` 缺少 `search()` 方法，导致搜索引擎无法调用

**根本原因**：
1. **未实现的搜索引擎**：
   - `DocEngine` 和 `MacroEngine` 的 `search()` 方法只有 "For now, return mock results" 占位符
   - 未初始化 Whoosh 索引连接
   - 代码注释明确标注 "In production, this would use Whoosh index"

2. **缺失的 Whoosh 方法**：
   - `WhooshIndexBuilder` 类没有 `search()` 方法
   - `APIEngine` 尝试调用不存在的方法

3. **缺少 entry_type 过滤**：
   - `WhooshSearcher.search()` 不支持按 entry_type 过滤
   - 无法区分文档、宏、枚举等不同类型的条目

**实施的修复**：

| 文件 | 修改 | 说明 |
|------|------|------|
| `src/core/whoosh_indexer.py` | 添加 `filter_entry_type` 参数 | 支持按条目类型过滤 |
| `src/core/whoosh_indexer.py` | 添加 `WhooshIndexBuilder.search()` | 委托给 WhooshSearcher |
| `src/layer1_discovery.py` | 更新 `DocEngine` 类 | 实现 Whoosh 索引集成 |
| `src/layer1_discovery.py` | 更新 `MacroEngine` 类 | 实现 Whoosh 索引集成 |
| `src/layer1_discovery.py` | 添加 `WhooshIndexBuilder` 导入 | 支持新类型 |

**修复后的数据流**：

```
┌─────────────────────────────────────────────┐
│  Whoosh 索引 (737 条目)                     │
│  ├─ 280 个函数 (functions)                  │
│  ├─ 260 个宏 (macros)                       │
│  ├─ 172 个枚举 (enums)                      │
│  └─ 25 个文档 (documents)                   │
└─────────────────────────────────────────────┘
         ↓                    ↓                    ↓
   DocEngine.search()   MacroEngine.search()   APIEngine.search()
   filter_entry_type=   filter_entry_type=    (已正常工作)
   'document'           'macro'
         ↓                    ↓
   返回实际文档内容      返回实际宏定义
   ✅ 不再返回 mock     ✅ 不再返回 mock
```

**新增功能**：

1. **entry_type 过滤支持**：
```python
# WhooshSearcher.search() 新增参数
search(
    query_str,
    limit=10,
    fields=None,
    filter_peripheral=None,
    filter_module=None,
    filter_entry_type=None  # ✨ NEW
)
```

2. **WhooshIndexBuilder.search() 方法**：
```python
# 新增方法
def search(
    self,
    query_str: str,
    limit: int = 10,
    fields: Optional[List[str]] = None,
    filter_peripheral: Optional[str] = None,
    filter_module: Optional[str] = None,
    filter_entry_type: Optional[str] = None  # ✨ NEW
) -> List[Dict[str, Any]]:
    """Search the index with optional entry type filter"""
    with WhooshSearcher(self.index) as searcher:
        return searcher.search(
            query_str,
            limit=limit,
            fields=fields,
            filter_peripheral=filter_peripheral,
            filter_module=filter_module,
            filter_entry_type=filter_entry_type
        )
```

3. **DocEngine 实现**：
```python
class DocEngine(SearchEngine):
    def __init__(self):
        super().__init__()
        self.whoosh: Optional[WhooshIndexBuilder] = None
        self._init_index()  # ✨ NEW

    async def search(self, query: str, max_results: int):
        # ✨ FIXED: 使用实际 Whoosh 索引
        results = self.whoosh.search(
            query,
            limit=max_results,
            fields=['title', 'content', 'brief', 'name'],
            filter_entry_type='document'  # ✨ 过滤文档类型
        )
        # ... 格式化并返回结果
```

4. **MacroEngine 实现**：
```python
class MacroEngine(SearchEngine):
    def __init__(self):
        super().__init__()
        self.whoosh: Optional[WhooshIndexBuilder] = None
        self._init_index()  # ✨ NEW

    async def search(self, query: str, max_results: int):
        # ✨ FIXED: 使用实际 Whoosh 索引
        results = self.whoosh.search(
            query,
            limit=max_results,
            fields=['name', 'brief', 'content'],
            filter_entry_type='macro'  # ✨ 过滤宏类型
        )
        # ... 格式化并返回结果
```

**测试结果**：

```bash
# DocEngine 测试
Searching for: 'UART'
  Found 3 results from actual indexed documents:
    1. B6X Datasheet V3.4
    2. B6X BLE芯片使用指南 V1.0.1
    3. B6X BLE芯片使用指南 V1.0.0

Searching for: 'GPIO'
  Found 3 results:
    1. B6X Api(部分)设计指导
    2. B6X IO Map
    3. B6X Datasheet V3.4

# MacroEngine 测试
Searching for: 'BRR_115200'
  Found 4 results (different clock frequencies):
    - BRR_DIV(115200, 32M)
    - BRR_DIV(115200, 48M)
    - BRR_DIV(115200, 64M)
    - BRR_DIV(115200, 16M)

Searching for: 'LCR_BITS_DFLT'
  Found 1 result:
    - LCR_BITS(8, 1, none) // 8 DataBits, 1 StopBits, None Parity

Searching for: 'SYS_CLK'
  Found 2 results:
    - SYS_CLK = (3)
    - SADC_CLK_DIV = (((SYS_CLK + 1) * 4) - 1) // div_4M
```

**索引统计验证**：

```
Whoosh Index Statistics:
  Total documents: 737

Entry type distribution:
  - entry_type='function': 280 results
  - entry_type='macro': 260 results
  - entry_type='enum': 172 results
  - entry_type='document': 25 results

Macro sources:
  - ble/api/gap.h: 95 macros
  - ble/api/att.h: 54 macros
  - ble/api/blelib.h: 31 macros
  - drivers/api/i2c.h: 13 macros
  - drivers/api/uart.h: 9 macros (BRR_*, LCR_*)
  - drivers/api/core.h: 8 macros (CFG_PMU_*, CFG_WKUP_*)
  - ... (20+ files total)
```

**搜索引擎状态（v3.2.3）**：

| 搜索引擎 | 状态 | 数据源 | 测试 |
|---------|------|--------|------|
| APIEngine | ✅ 正常 | Whoosh (280 functions) | ✅ 通过 |
| DocEngine | ✅ **已修复** | Whoosh (25 documents) | ✅ 通过 |
| MacroEngine | ✅ **已修复** | Whoosh (260 macros) | ✅ 通过 |
| RegisterEngine | ✅ 正常 | SVD Parser | ✅ 通过 |
| ExampleEngine | ✅ 正常 | ExampleScanner | ✅ 通过 |

**向后兼容性**：
- ✅ 所有现有功能保持不变
- ✅ `filter_entry_type` 参数可选，默认行为不变
- ✅ Mock 数据已移除，仅使用实际索引数据
- ✅ 搜索结果格式保持一致

**相关文件**：
- 修改：`src/core/whoosh_indexer.py`、`src/layer1_discovery.py`
- 测试：所有搜索引擎现已使用实际 Whoosh 索引数据
- 文档：更新架构图反映完整的搜索引擎实现

**代码质量改进**：
- ✅ 移除所有 "For now, return mock results" 占位符
- ✅ 实现完整的 `_init_index()` 方法
- ✅ 添加 `_extract_*_summary()` 辅助方法
- ✅ 统一错误处理和日志记录

---

## v3.2.2 - 2026-02-27

### ✨ 新功能：宏和枚举索引

**功能增强**：扩展 Whoosh API 索引以支持宏定义和枚举类型。

**新增功能**：

1. **宏索引**：
   - 索引宏名称、值、类型
   - 支持 `register`、`bitmask`、`constant`、`function_macro` 类型
   - 存储宏值表达式
   - 提取 Doxygen 注释

2. **枚举索引**：
   - 索引枚举类型名称、底层类型
   - 存储所有枚举值（JSON 格式）
   - 包含每个枚举值的名称、数值、注释
   - 支持快速查找枚举常量

**实现细节**：

| 修改文件 | 说明 |
|----------|------|
| `src/core/whoosh_indexer.py` | 扩展 `build_from_parse_results()` 方法 |
| `scripts/test_macro_enum_index.py` | 新增测试脚本 |

**新增参数**：

```python
build_from_parse_results(
    parse_results,
    module="driver",
    index_macros=True,   # 新增：是否索引宏
    index_enums=True     # 新增：是否索引枚举
)
```

**索引数据结构**：

**宏条目**：
```python
IndexEntry(
    entry_type='macro',
    name='UART0_BASE',
    macro_value='0x40000000',
    macro_type='register',
    brief='UART0 base address'
)
```

**枚举条目**：
```python
IndexEntry(
    entry_type='enum',
    name='uart_port',
    enum_underlying_type='int',
    enum_values=[
        {'name': 'UART1_PORT', 'value': 0, 'brief': 'UART port 1'},
        {'name': 'UART2_PORT', 'value': 1, 'brief': 'UART port 2'}
    ]
)
```

**测试结果**：

```bash
# 解析统计
gpio.h:    1 functions, 0 macros, 3 enums
core.h:    7 functions, 8 macros, 4 enums
uart.h:   11 functions, 9 macros, 5 enums

Total:     19 functions, 17 macros, 12 enums
Indexed:   48 entries to Whoosh ✅

# 搜索验证
Total macros in index: 17
  - CFG_PMU_DFLT_CNTL = (CFG_PMU_CLK_STB(0x3F))
  - BRR_115200 = ...
  - LCR_BITS_DFLT = ...

Total enums in index: 12
  - uart_port (with values)
  - gpio_mode (with values)
  ...
```

**搜索示例**：

```python
# 搜索宏定义
search_sdk("UART0_BASE", "all")
# 返回宏定义，包括地址值和类型

# 搜索枚举常量
search_sdk("UART1_PORT", "all")
# 返回枚举值及其含义

# 搜索所有类型
search_sdk("GPIO", "all")
# 返回：函数 + 宏 + 枚举 + 文档
```

**数据统计**：

| 类型 | 索引条目 | 数据内容 |
|------|----------|----------|
| 函数 | 19 | 名称、参数、返回值、注释 |
| 宏 | 17 | 名称、值、类型、注释 |
| 枚举 | 12 | 名称、底层类型、枚举值列表 |
| **总计** | **48** | 完整的 C 代码定义索引 |

**性能影响**：

| 指标 | v3.2.1 | v3.2.2 | 变化 |
|------|--------|--------|------|
| 索引条目数 | 21 (函数) | 48 (函数+宏+枚举) | +129% |
| 构建时间 | ~2-5s | ~2-5s | 无变化 |
| 索引大小 | ~5KB | ~12KB | +140% |
| 搜索响应 | <10ms | <10ms | 无影响 |

**向后兼容性**：
- ✅ 所有现有功能保持不变
- ✅ `index_macros` 和 `index_enums` 参数默认为 `True`
- ✅ 可以通过参数禁用宏/枚举索引

**相关文件**：
- 修改：`src/core/whoosh_indexer.py`
- 新增：`scripts/test_macro_enum_index.py`
- 文档：更新架构图和数据流

---

## v3.2.1 - 2026-02-27

### 🐛 Bug 修复：Tree-sitter → Whoosh API 索引连接

**问题描述**：
- `search_sdk()` 工具的 `scope="api"` 搜索返回空结果
- Whoosh 索引中缺少 C 代码的 API 函数数据
- `build_from_parse_results()` 方法存在但从未被调用

**根本原因**：
1. **Tree-sitter 解析器缺陷**：
   - `_extract_functions()` 只遍历根节点直接子节点
   - 函数声明嵌套在 `preproc_ifdef` 块内导致递归失败
   - `FunctionDeclaration` dataclass 初始化错误（缺少必需参数）

2. **Schema 字段不匹配**：
   - `APIDocument.to_dict()` 返回 `api_id` 字段
   - Whoosh Schema 期望 `entry_id` 字段

3. **构建流程缺失**：
   - `build_all_indices.py` 缺少 API 索引构建步骤

**实施的修复**：

| 文件 | 修改 | 说明 |
|------|------|------|
| `scripts/build_all_indices.py` | 添加 `build_api_index()` 函数 | 连接 Tree-sitter → Whoosh |
| `src/core/tree_sitter_parser.py` | 修复 `_extract_functions()` | 递归 AST 遍历 |
| `src/core/tree_sitter_parser.py` | 修复 `_extract_function_declaration()` | 正确初始化 dataclass |
| `src/core/whoosh_indexer.py` | 修复 `APIDocument.to_dict()` | 字段映射 `api_id` → `entry_id` |

**修复后的数据流**：
```
C 源码
   ↓
Tree-sitter 解析（递归遍历 AST）
   ↓
┌─────────────────────────────────────┐
│  Whoosh 索引（API 数据）             │
│  - 函数名称、返回类型、参数           │
│  - Doxygen 注释                      │
│  - 文件路径、行号                    │
└─────────────────────────────────────┘
   ↓
search_sdk(query, scope="api") ✅ 现在可以工作了！
```

**测试结果**：
```bash
# 解析结果
ke_api.h: 10 functions extracted
uart.h: 11 functions extracted

# Whoosh 索引
INFO - Added 21/21 documents to index
INFO - Indexed 21 API entries
```

**影响范围**：
- ✅ `search_sdk("UART", scope="api")` 现在返回 API 函数结果
- ✅ API 搜索与文档搜索、寄存器搜索统一在 Whoosh 引擎中
- ✅ `build_all_indices.py` 现在包含完整的 API 索引构建步骤

**向后兼容性**：
- ✅ 所有现有工具保持兼容
- ✅ 知识图谱构建流程不受影响
- ✅ 文档索引功能保持不变

**相关文件**：
- 修改：`scripts/build_all_indices.py`、`src/core/tree_sitter_parser.py`、`src/core/whoosh_indexer.py`
- 新增：`scripts/test_api_index.py`（测试脚本）
- 文档：更新架构图反映新的数据流

---

## v3.2.0 - 2026-02-27

### 🌟 三层统一架构

**重大更新**: 采用三层统一架构，简化 API，提升易用性和可维护性。

#### 核心工具

| 工具 | 功能 | 设计模式 |
|------|------|----------|
| `search_sdk()` | 统一搜索 SDK（API/文档/寄存器/示例/宏） | Composer Pattern |
| `inspect_node()` | 查看详细信息（定义/实现/依赖/调用链） | Strategy Table Pattern |
| `validate_config()` | 验证硬件配置（引脚/时钟/DMA/内存） | Chain of Responsibility Pattern |

#### 架构改进

- ✅ **统一接口**: 3 个核心工具替代原有多个工具
- ✅ **简化 Schema**: ~5KB（结构清晰，易于维护）
- ✅ **清晰工作流**: 发现 → 详情 → 验证
- ✅ **完整测试**: 84 个测试用例，100% 分支覆盖
- ✅ **设计模式**: Composer, Strategy Table, Chain of Responsibility

#### 支持的搜索域

| 域 | 说明 | 搜索引擎 |
|----|------|----------|
| `api` | 驱动 API 函数 | Whoosh |
| `docs` | SDK 文档内容 | Whoosh |
| `macros` | 宏定义和枚举 | Whoosh |
| `registers` | 硬件寄存器 | SVD Parser |
| `examples` | 示例代码 | Example Scanner |

#### 支持的视图类型

| 节点类型 | 可用视图 |
|----------|----------|
| API | summary, definition, implementation, dependencies, call_chain, usage_examples, full |
| Register | register_info, bit_fields, memory_map, full |
| Document | doc_content, doc_summary |

#### 验证器

| 优先级 | 验证器 | 级别 |
|--------|--------|------|
| 1 | 引脚冲突检测 | Error |
| 2 | 时钟树验证 | Error |
| 3 | DMA 分配验证 | Error |
| 4 | 中断优先级检查 | Warning |
| 5 | 内存使用检查 | Warning |
| 6 | 功耗估算 | Info |
| 7 | 兼容性检查 | Info |

#### 迁移指南

```python
# 新工具使用示例
search_sdk("UART initialization", "all")
inspect_node("B6x_UART_Init", "definition")
validate_config({"pins": {"PA9": "UART1_TX", "PA10": "UART1_RX"}})
```

#### 技术栈

- **Python**: 3.8+
- **MCP Framework**: Model Context Protocol
- **搜索引擎**: Whoosh (全文索引)
- **代码解析**: Tree-sitter (C 语言)
- **数据解析**: SVD, Excel, Word, PDF

**完整文档**:
- [架构文档](architecture.md)
- [能力演示](capabilities-demo.md)
- [构建指南](build-guide.md)

---

## v3.1.0 - 2026-02-27

### 新增功能

- **USB 外设支持** - Domain 2 扩展 USB 模块解析
- **蓝牙 Mesh 网络** - Domain 3 扩展 Mesh 模块解析
- **BLE 库配置** - Module C 智能库选择工具

---

## v3.0.0 - 2026-02-27

### 四维知识图谱

引入基于域的知识图谱组织：

| 域 | 功能 |
|---|------|
| Domain 1 | 硬件与寄存器（引脚复用、内存区域、中断） |
| Domain 2 | SOC 驱动（初始化 API、配置结构体） |
| Domain 3 | BLE 协议栈（GATT 服务、错误码、Profile） |
| Domain 4 | 应用与上下文（示例、调用链、初始化序列） |

### 新增解析器

- LinkerScriptParser - 链接脚本解析
- InterruptVectorParser - 中断向量表解析
- StructExtractor - C 结构体提取
- ProfileParser - BLE GATT 服务解析
- RelationMapper - 跨域关系映射
- ExcelHardwareParser - Excel 硬件约束解析

---

## v2.1.0 - 2025-02-26

### 新增功能

- Excel 硬件约束解析器
- BLE 错误码解析器
- 项目依赖分析器

---

## v2.0.0 - 2025-02-26

### 新增功能

- 示例目录扫描器
- API 使用清单生成
- API ↔ 示例映射

---

## v1.0.0 - 2025-02-26

### 初始版本

- MCP 服务器入口
- Tree-sitter + Whoosh 两阶段架构
- Module A：SDK 参考（17 个工具）
- SVD 寄存器解析器
- 知识图谱模型

---

**格式**: 基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
**版本**: 遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
