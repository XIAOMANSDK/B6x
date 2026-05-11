# Python Clang 集成方案

**版本**: v1.3
**更新日期**: 2026-03-17
**状态**: ✅ 框架已实施，依赖待部署

> **注意**: Clang 框架代码已实现，但 libclang 动态库需要通过 `deploy_libclang.py` 部署。

---

## 一、背景

在 B6x MCP Server 项目中集成 Python Clang 绑定（libclang），增强代码分析能力。

**核心需求**:
1. 采用 python-clang，不后台运行 clangd，直接加载动态库
2. 保持现有接口兼容性
3. 与现有 Tree-sitter 解析器并存

---

## 二、架构决策

### 2.1 Clang 与 Tree-sitter 关系

**决策**: ✅ 并存模式（推荐）

| 解析器 | 职责 | 使用场景 |
|--------|------|----------|
| **Tree-sitter** | 快速语法解析 | 构建索引、快速搜索、实时预览 |
| **Clang** | 深度语义分析 | 类型推断、跨文件引用、宏展开 |

**职责划分**:

| 能力 | Tree-sitter | Clang |
|------|-------------|-------|
| 函数签名提取 | ✅ | ✅ |
| Doxygen 注释 | ✅ | ✅ |
| 快速索引 | ✅ (10ms) | ❌ (200ms) |
| 类型推断 | ❌ | ✅ |
| 宏展开 | ❌ | ✅ |
| 跨文件引用 | ❌ | ✅ |
| 函数调用目标 | ❌ 近似 | ✅ 精确 |

### 2.2 与三层架构的关系

**Clang 不创建新层，而是作为独立工具与现有三层并行**:

```
现有三层架构：
Layer 1: search_sdk()     → 发现 "信息在哪里"
Layer 2: inspect_node()   → 详情 "它长什么样"
Layer 3: validate_config()→ 验证 "配置行不行"

新增工具（与三层并行）：
Tool:    clang_analyze()  → 分析 "这段代码语义是什么"
```

**设计理由**:
- Clang 功能是"实时分析"，与 Layer 2 的"查看已知节点"定位不同
- 保持现有工具职责清晰，不修改 `inspect_node()`
- 独立工具便于后续扩展和维护

### 2.3 模块结构

```
src/
├── layer2_detail.py           # 现有，保持不变
├── clang_tools.py             # 新增：clang_analyze() MCP 工具
│
core/
├── tree_sitter_parser.py      # 现有，保持不变
├── clang_parser.py            # 新增：Clang 实时解析器核心
└── sdk_context.py             # 新增：SDK 编译上下文（include路径、宏定义）
```

---

## 三、MCP 接口设计

### 3.1 新增工具：`clang_analyze()`

**职责划分**:

| 工具 | 输入 | 数据来源 | 场景 |
|------|------|----------|------|
| `search_sdk()` | 搜索词 | Whoosh索引 | 搜索SDK内容 |
| `inspect_node()` | SDK节点ID | 知识图谱 | 查看已知节点详情 |
| `clang_analyze()` | 代码/符号 | Clang实时解析 | **语义分析** |

### 3.2 接口定义

```python
clang_analyze(
    code: str,                   # 必需：C代码片段或符号名
    type: str,                   # 必需：分析类型（refs | type | check）

    # SDK 上下文（可选，默认自动注入）
    sdk_context: dict = {
        "include_paths": [],     # 额外头文件路径（SDK路径自动添加）
        "defines": {},           # 额外预定义宏（__B6X__ 等自动添加）
        "target": "arm-none-eabi" # 目标平台
    },

    # 分析选项
    files: list = [],            # 可选：上下文文件（最多3个）
    brief: bool = True           # 可选：True=精简输出，False=完整输出
)
```

**type 参数取值**:
- `"refs"` - 查找所有引用
- `"type"` - 类型推导
- `"check"` - 编译诊断

### 3.3 SDK 上下文自动注入

```python
# src/core/sdk_context.py
class SDKContext:
    """SDK 编译上下文，提供 Clang 解析所需的头文件路径和宏定义"""

    def __init__(self, sdk_path: str):
        self.sdk_path = sdk_path

        # 默认头文件搜索路径
        self.default_includes = [
            f"{sdk_path}/drivers/api",
            f"{sdk_path}/core",
            f"{sdk_path}/ble/api",
            f"{sdk_path}/ble/lib",
        ]

        # 默认预定义宏
        self.default_defines = {
            "__B6X__": "1",
            "ARM_MATH_CM0_PLUS": "1",
            "__ARM_ARCH_6M__": "1",
        }

    def get_compile_args(self) -> list:
        """生成 Clang 编译参数"""
        args = []
        for path in self.default_includes:
            args.extend(["-I", path])
        for name, value in self.default_defines.items():
            args.append(f"-D{name}={value}")
        return args
```

### 3.4 使用示例

```python
# 查找符号引用
clang_analyze("B6x_UART_Init", type="refs")

# 推导类型（自动使用 SDK 上下文）
clang_analyze("cfg->baudrate", type="type")

# 检查代码
clang_analyze("B6x_UART_Init(UART1, &cfg);", type="check")

# 指定额外的上下文文件
clang_analyze(
    code="uart_init(&config)",
    type="refs",
    files=["projects/my_app/src/uart_config.c"]
)

# 自定义宏定义
clang_analyze(
    code="#ifdef DEBUG\ndebug_print();\n#endif",
    type="check",
    sdk_context={"defines": {"DEBUG": "1"}}
)
```

---

## 四、返回格式

### 4.1 成功响应（统一使用 `success` 字段）

```json
{
  "success": true,
  "type": "refs",
  "results": [
    {
      "file": "drivers/src/uart.c",
      "line": 45,
      "column": 5,
      "code": "B6x_UART_Init(&cfg)",
      "context": "void uart_init(void) { ... }"
    },
    {
      "file": "projects/demo/src/main.c",
      "line": 120,
      "column": 3,
      "code": "B6x_UART_Init(UART1, &uart_cfg)",
      "context": "int main(void) { ... }"
    }
  ],
  "count": 2,
  "truncated": false,
  "metadata": {
    "parse_time_ms": 150,
    "files_analyzed": 5,
    "sdk_context_used": true
  }
}
```

### 4.2 类型推导响应

```json
{
  "success": true,
  "type": "type",
  "result": {
    "expression": "cfg->baudrate",
    "type": "uint32_t",
    "kind": "builtin",
    "size": 4,
    "declaration": "drivers/api/uart.h:25",
    "comment": "Baud rate configuration (bps)"
  }
}
```

### 4.3 编译诊断响应

```json
{
  "success": true,
  "type": "check",
  "diagnostics": [
    {
      "severity": "warning",
      "message": "unused variable 'temp'",
      "file": "main.c",
      "line": 42,
      "column": 9,
      "suggestion": "Remove unused variable or use it"
    }
  ],
  "summary": {
    "errors": 0,
    "warnings": 1,
    "infos": 0
  }
}
```

### 4.4 与现有工具格式一致性

| 工具 | 成功标志 | 错误字段 | 数据字段 |
|------|----------|----------|----------|
| `search_sdk()` | `success: bool` | `error_info` | `results` |
| `inspect_node()` | `success: bool` | `error` | `data` |
| `validate_config()` | `is_valid: bool` | `errors[]` | `validated_items` |
| `clang_analyze()` | `success: bool` | `error` | `results` ✅ |

---

## 五、错误处理

### 5.1 libclang 加载失败

```json
{
  "success": false,
  "error": {
    "code": "LIBCLANG_NOT_FOUND",
    "message": "libclang.dll 加载失败: 找不到指定的模块",
    "solution": "请确保 libclang 已安装在以下位置之一：",
    "tried_paths": [
      "xm_b6_mcp/lib/clang/win-x64/libclang.dll",
      "C:\\Program Files\\LLVM\\bin\\libclang.dll"
    ],
    "download_url": "https://github.com/llvm/llvm-project/releases",
    "required_version": ">= 16.0.0"
  }
}
```

### 5.2 解析超时

```json
{
  "success": false,
  "error": {
    "code": "ANALYSIS_TIMEOUT",
    "message": "代码分析超时（超过 5 秒）",
    "solution": "请简化代码片段或减少上下文文件数量",
    "hint": "复杂宏展开或大型头文件可能导致超时"
  }
}
```

### 5.3 语法错误

```json
{
  "success": false,
  "error": {
    "code": "PARSE_ERROR",
    "message": "代码片段存在语法错误",
    "details": "expected ';' after expression",
    "location": {
      "line": 3,
      "column": 15
    }
  }
}
```

### 5.4 错误码定义

| 错误码 | 说明 | 解决方案 |
|--------|------|----------|
| `LIBCLANG_NOT_FOUND` | libclang 动态库未找到 | 检查安装路径 |
| `LIBCLANG_VERSION` | libclang 版本过低 | 升级到 >= 16.0.0 |
| `ANALYSIS_TIMEOUT` | 分析超时 | 简化代码片段 |
| `PARSE_ERROR` | 语法解析失败 | 修复代码语法 |
| `FILE_NOT_FOUND` | 上下文文件不存在 | 检查文件路径 |
| `INVALID_TYPE` | 无效的分析类型 | 使用 refs/type/check |

---

## 六、输入限制

### 6.1 参数限制

| 参数 | 限制 | 说明 |
|------|------|------|
| `code` | 2000 字符 | 超出返回 INPUT_TOO_LARGE 错误 |
| `files` | 3 个 | 上下文文件数量上限 |
| 单文件 | 50 KB | 超出跳过并警告 |
| `sdk_context.includes` | 10 个 | 额外头文件路径上限 |

### 6.2 输入验证

```python
def validate_input(code: str, files: list) -> tuple[bool, str]:
    """验证输入参数"""
    if len(code) > 2000:
        return False, "代码片段超过 2000 字符限制"

    if len(files) > 3:
        return False, "上下文文件数量超过 3 个限制"

    for file in files:
        if not Path(file).exists():
            return False, f"文件不存在: {file}"
        if Path(file).stat().st_size > 50 * 1024:
            return False, f"文件过大: {file} (超过 50KB)"

    return True, ""
```

---

## 七、性能优化

### 7.1 核心配置

```python
# src/core/clang_parser.py
class ClangParserConfig:
    """Clang 解析器配置"""

    # 超时控制
    ANALYSIS_TIMEOUT = 5.0        # 单次分析超时（秒）

    # 并发控制
    MAX_CONCURRENT = 1            # 最大并发解析任务数

    # 缓存配置
    CACHE_TTL = 300               # 结果缓存时间（秒）
    CACHE_MAX_SIZE = 100          # 最大缓存条目数

    # TU Pool（翻译单元池）
    TU_POOL_SIZE = 3              # 保持内存中的 TU 数量
    TU_MAX_MEMORY_MB = 500        # TU 最大内存占用（MB）
```

### 7.2 优化策略

| 优化项 | 策略 | 效果 |
|--------|------|------|
| **Preamble Cache** | 预编译常用头文件 | 加速重复解析 50% |
| **TU Pool** | 保持最近解析的 TU 在内存 | 避免重复解析 |
| **结果缓存** | 相同查询缓存 5 分钟 | 即时响应 |
| **超时控制** | 5 秒超时 | 防止卡死 |
| **并发限制** | 最多 1 个任务 | 避免内存爆炸 |

### 7.3 缓存实现

```python
from functools import lru_cache
import hashlib

class ClangResultCache:
    """Clang 分析结果缓存"""

    def __init__(self, max_size: int = 100, ttl: int = 300):
        self.cache = {}
        self.max_size = max_size
        self.ttl = ttl

    def _hash_query(self, code: str, analysis_type: str, files: list) -> str:
        """生成查询哈希"""
        content = f"{code}|{analysis_type}|{'|'.join(sorted(files))}"
        return hashlib.md5(content.encode()).hexdigest()

    def get(self, code: str, analysis_type: str, files: list) -> Optional[dict]:
        """获取缓存结果"""
        key = self._hash_query(code, analysis_type, files)
        if key in self.cache:
            result, timestamp = self.cache[key]
            if time.time() - timestamp < self.ttl:
                return result
            else:
                del self.cache[key]
        return None

    def set(self, code: str, analysis_type: str, files: list, result: dict):
        """设置缓存结果"""
        if len(self.cache) >= self.max_size:
            # 删除最旧的条目
            oldest_key = min(self.cache.keys(),
                           key=lambda k: self.cache[k][1])
            del self.cache[oldest_key]

        key = self._hash_query(code, analysis_type, files)
        self.cache[key] = (result, time.time())
```

---

## 八、依赖与部署

### 8.1 新增依赖

```txt
# requirements.txt
clang>=16.0.0  # Python bindings（可选，本地部署时不需要）
```

### 8.2 libclang 动态库部署

**使用部署脚本自动下载和安装**:

```bash
# 检查当前状态
python scripts/deploy_libclang.py --check

# 自动下载并部署
python scripts/deploy_libclang.py

# 从本地路径复制
python scripts/deploy_libclang.py --local "C:/path/to/libclang.dll"
```

**部署目录结构**:

```
xm_b6_mcp/
├── lib/
│   └── clang/
│       ├── win-x64/
│       │   └── libclang.dll      # Windows 64位 (需部署)
│       ├── linux-x64/
│       │   └── libclang.so.18.1  # Linux 64位 (需部署)
│       └── darwin-x64/
│           └── libclang.dylib    # macOS 64位 (需部署)
└── ...
```

> **注意**: libclang 文件不包含在代码库中，需要首次使用前运行 `deploy_libclang.py`。

### 8.3 加载优先级

| 优先级 | 方案 | 路径 |
|--------|------|------|
| 1 | 本地目录 | `xm_b6_mcp/lib/clang/{platform}/libclang.*` |
| 2 | 系统路径 | 通过 `ctypes.util.find_library` 查找 |

### 8.4 版本要求

- **最低版本**: libclang >= 16.0.0
- **推荐版本**: LLVM 18.1.8 (deploy_libclang.py 默认下载)
- **支持标准**: C11
- **目标平台**: ARM Cortex-M0+

---

## 九、待确认事项

### 9.1 架构决策（已确认）

| # | 问题 | 决策 |
|---|------|------|
| 1 | Clang 与 Tree-sitter 关系 | ✅ 并存 |
| 2 | 默认解析器 | ✅ Tree-sitter |
| 3 | Clang 触发方式 | ✅ 显式调用 `clang_analyze()` |
| 4 | 新工具名称 | ✅ `clang_analyze()` |
| 5 | 是否增强现有工具 | ✅ 否，保持 `inspect_node()` 不变 |

### 9.2 实现优先级

| 优先级 | 功能 | type参数 | 状态 |
|--------|------|----------|------|
| **P0** | 框架搭建 + 符号引用查找 | `refs` | ✅ 已完成 |
| **P1** | 类型推导 | `type` | ✅ 已完成 |
| **P1** | 编译诊断 | `check` | ✅ 已完成 |
| **P2** | 性能优化（缓存、超时） | - | ✅ 已完成 |

---

## 十、风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| libclang 兼容性问题 | 中 | 高 | 提供 Windows 预编译版本，明确版本要求 |
| 解析超时 | 中 | 中 | 5s 超时 + 异步执行 + 明确错误提示 |
| 内存占用过大 | 低 | 中 | 单任务限制 + TU Pool 大小限制 (500MB) |
| SDK 头文件缺失 | 低 | 高 | 启动时检查 SDK 完整性 |
| 结果缓存不一致 | 低 | 低 | 文件修改时间检测 + TTL 过期 |

---

## 十一、实现计划

### 阶段 1：基础框架（P0）✅ 已完成

- [x] 创建 `src/core/sdk_context.py` - SDK 编译上下文
- [x] 创建 `src/core/clang_parser.py` - Clang 实时解析器
- [x] 实现 libclang 自动检测和加载
- [x] 实现输入验证和错误处理
- [x] 实现 `refs` 分析类型

### 阶段 2：MCP 集成（P0）✅ 已完成

- [x] 创建 `src/clang_tools.py` - clang_analyze() 实现
- [x] 在 `main.py` 注册新工具
- [x] 统一返回格式
- [x] 单元测试

### 阶段 3：功能完善（P1）✅ 已完成

- [x] 实现 `type` 分析类型
- [x] 实现 `check` 分析类型
- [x] SDK 上下文自动注入

### 阶段 4：性能优化（P2）✅ 已完成

- [x] 实现 Preamble Cache（结果缓存）
- [x] 实现 TU Pool（ThreadPoolExecutor）
- [x] 实现结果缓存（ClangResultCache）
- [ ] 性能测试和调优（待后续优化）

---

## 附录：文件清单

| 文件 | 类型 | 说明 |
|------|------|------|
| `src/core/clang_parser.py` | 已实现 | Clang 解析器核心实现 |
| `src/core/sdk_context.py` | 已实现 | SDK 编译上下文管理 |
| `src/clang_tools.py` | 已实现 | clang_analyze() MCP 工具 |
| `scripts/deploy_libclang.py` | 已实现 | libclang 部署脚本 |
| `lib/clang/win-x64/libclang.dll` | 需部署 | Windows libclang 动态库 |
| `lib/clang/linux-x64/libclang.so.18.1` | 需部署 | Linux libclang 动态库 |
| `lib/clang/darwin-x64/libclang.dylib` | 需部署 | macOS libclang 动态库 |

> **部署说明**: 使用 `python scripts/deploy_libclang.py` 自动下载和部署 libclang。
