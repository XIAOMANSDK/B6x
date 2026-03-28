# Config 目录文件作用说明

**目录**: `xm_b6_mcp/config/`
**更新时间**: 2026-03-17
**版本**: v3.5

---

## 概述

`config/` 目录包含四个 YAML 配置文件，用于增强 B6x MCP Server 的智能分析能力：

1. **api_register_mapping.yaml** - API 函数与硬件寄存器的映射
2. **dependency_overrides.yaml** - API 依赖关系的人工覆盖
3. **flash_map.yaml** - Flash 内存布局配置
4. **sram_map.yaml** - SRAM 内存布局配置

这些文件提供了**人工知识注入**的机制，当自动分析不足以获得准确结果时使用。

---

## 文件 1: api_register_mapping.yaml

**作用**: 建立 API 函数与硬件寄存器之间的映射关系

### 为什么需要？

自动分析（通过 Tree-sitter 解析源码）虽然可以找到大部分关系，但某些复杂场景仍需人工标注：

| 场景 | 自动分析的局限 | 人工配置的优势 |
|------|----------------|----------------|
| 寄存器配置 | 宏定义导致间接访问，无法追踪 | 明确指定所有相关寄存器 |
| 位字段操作 | 位运算逻辑复杂，难以推断 | 精确到 bit 级别的映射 |
| 时钟依赖 | 时钟树跨越多个文件 | 明确标注时钟依赖 |
| 副作用 | 隐式读写寄存器 | 完整列出所有副作用 |

### 配置格式

```yaml
mappings:
  - api: B6x_UART_Init              # API 函数名
    registers:                      # 访问的寄存器列表
      - UART_CR                     # 控制寄存器
      - UART_BRR                    # 波特率寄存器
      - UART_LCR                    # 线控制寄存器
    fields:                         # 访问的位字段
      - DIV_MANTISSA                # 波特率整数部分
      - DIV_FRACTION                # 波特率小数部分
    access_type: configure          # 访问类型: read/write/configure
    confidence: 1.0                 # 置信度 (0.0-1.0)，人工配置总是 1.0
    note: Manual mapping for UART initialization
```

### 访问类型 (access_type)

| 类型 | 说明 | 示例 |
|------|------|------|
| `configure` | 配置型 API，初始化时调用 | `B6x_UART_Init` |
| `read` | 只读寄存器访问 | `B6x_GPIO_GetPin` |
| `write` | 只写寄存器访问 | `B6x_GPIO_SetPin` |

### 置信度 (confidence)

```python
# 人工配置总是最高置信度
confidence: 1.0    # 人工配置 (api_register_mapping.yaml)
confidence: 0.9    # 实现代码解析
confidence: 0.7    # 外设名称匹配
confidence: 0.5    # 参数类型推断
```

### 使用场景

#### 场景 1: 精确寄存器映射

**问题**: `B6x_UART_SetBaudRate` 通过宏操作位字段，自动分析无法追踪

```yaml
- api: B6x_UART_SetBaudRate
  registers:
    - UART_BRR
  fields:
    - DIV_MANTISSA    # 波特率分频整数部分
    - DIV_FRACTION    # 波特率分频小数部分
  access_type: write
  confidence: 1.0
  note: Baud rate configuration
```

#### 场景 2: 跨文件时钟依赖

**问题**: `RCC_EnablePeriphClock` 修改全局 RCC 寄存器，与外设 API 不在同一文件

```yaml
- api: RCC_EnablePeriphClock
  registers:
    - RCC_CLK_EN_ST    # 全局时钟使能寄存器
  access_type: write
  confidence: 1.0
  note: Enable peripheral clock
```

### 代码集成

**文件**: `src/core/api_register_mapper.py`

```python
class APIRegisterMapper:
    def __init__(self, config_path: str = None):
        self.manual_mappings = self._load_manual_mappings(config_path)

    def _load_manual_mappings(self, config_path):
        """加载人工配置的映射关系"""
        if config_path and Path(config_path).exists():
            with open(config_path) as f:
                data = yaml.safe_load(f)
                return {m['api']: m for m in data['mappings']}
        return {}

    def map_api_to_registers(self, api_name: str) -> APIRegisterMapping:
        # 优先使用人工配置 (confidence = 1.0)
        if api_name in self.manual_mappings:
            return APIRegisterMapping(**self.manual_mappings[api_name])

        # 回退到自动分析
        return self._analyze_automatically(api_name)
```

---

## 文件 2: dependency_overrides.yaml

**作用**: 定义 API 之间的依赖关系和调用顺序

### 为什么需要？

自动依赖提取基于 Doxygen 注释和代码分析，但某些隐式依赖无法自动推断：

| 依赖类型 | 自动分析的局限 | 人工配置的优势 |
|----------|----------------|----------------|
| 时钟依赖 | 注释未提及时钟 | 明确标注 `requires_clock: true` |
| GPIO 配置 | GPIO 在其他模块配置 | 标注 `requires_gpio: true` |
| DMA 通道 | DMA 配置独立于外设 | 标注 `requires_dma: true` |
| 调用顺序 | 无法确定初始化顺序 | 提供 `call_sequence` |

### 配置格式

```yaml
dependencies:
  - api: B6x_UART_Init                # API 函数名
    pre_requisites:                   # 前置依赖 API
      - RCC_EnablePeriphClock         # 必须先使能时钟
      - GPIO_Init                     # 必须先配置 GPIO
    call_sequence:                    # 推荐调用顺序
      - RCC_EnablePeriphClock(RCC_PERIPH_UART1)
      - GPIO_Init(...)
      - B6x_UART_Init(...)
    notes:                            # 使用说明
      - Ensure UART clock is enabled before calling this function
      - GPIO pins must be configured with correct alternate function
    requires_clock: true              # 是否需要外设时钟
    requires_gpio: true               # 是否需要 GPIO 配置
    requires_dma: false               # 是否需要 DMA 配置
    requires_interrupt: false         # 是否需要中断配置
    see_also:                         # 相关 API
      - B6x_UART_DeInit
      - B6x_UART_Send
```

### 依赖类型标志

| 标志 | 说明 | 典型场景 |
|------|------|----------|
| `requires_clock` | 需要外设时钟 | 所有外设初始化 |
| `requires_gpio` | 需要 GPIO 配置 | UART, SPI, I2C |
| `requires_dma` | 需要 DMA 配置 | 高速数据传输 |
| `requires_interrupt` | 需要中断配置 | 事件驱动外设 |

### 使用场景

#### 场景 1: 完整初始化序列

**问题**: `B6x_UART_Init` 依赖前置步骤，但文档未明确说明

```yaml
- api: B6x_UART_Init
  call_sequence:
    - RCC_EnablePeriphClock(RCC_PERIPH_UART1)    # 步骤 1
    - GPIO_Init(PA9, GPIO_MODE_AF)                # 步骤 2
    - GPIO_Init(PA10, GPIO_MODE_AF)               # 步骤 3
    - B6x_UART_Init(...)                          # 步骤 4
  notes:
    - Ensure UART clock is enabled before calling this function
    - GPIO pins must be configured with correct alternate function
  pre_requisites:
    - RCC_EnablePeriphClock
    - GPIO_Init
  requires_clock: true
  requires_gpio: true
```

#### 场景 2: SPI + DMA 复杂依赖

**问题**: SPI DMA 模式需要多个模块协同

```yaml
- api: B6x_SPI_Transmit_DMA
  call_sequence:
    - RCC_EnablePeriphClock(RCC_PERIPH_SPI1)
    - RCC_EnablePeriphClock(RCC_PERIPH_DMA1)
    - DMA_Init(DMA1_Channel3, ...)
    - NVIC_EnableIRQ(DMA1_Channel3_IRQn)
    - B6x_SPI_EnableDMA(SPI1, SPI_DMA_TX)
    - B6x_SPI_Transmit_DMA(...)
  pre_requisites:
    - RCC_EnablePeriphClock
    - DMA_Init
    - NVIC_EnableIRQ
  requires_clock: true
  requires_dma: true
  requires_interrupt: true
```

### 代码集成

**文件**: `src/core/dependency_extractor.py`

```python
class DependencyExtractor:
    def __init__(self, config_path: str = None):
        self.manual_overrides = self._load_overrides(config_path)

    def _load_overrides(self, config_path):
        """加载人工覆盖的依赖关系"""
        if config_path and Path(config_path).exists():
            with open(config_path) as f:
                data = yaml.safe_load(f)
                return {d['api']: d for d in data['dependencies']}
        return {}

    def get_api_dependencies(self, api_name: str) -> APIDependency:
        # 优先使用人工覆盖
        if api_name in self.manual_overrides:
            override = self.manual_overrides[api_name]
            return APIDependency(
                api_name=api_name,
                pre_requisites=override['pre_requisites'],
                requires_peripheral_clock=override['requires_clock'],
                requires_gpio_config=override['requires_gpio'],
                requires_dma_config=override['requires_dma'],
                requires_interrupt_config=override['requires_interrupt'],
                call_sequence=override['call_sequence'],
                notes=override['notes'],
                see_also=override['see_also'],
                dependency_source='manual'
            )

        # 回退到自动提取
        return self._extract_from_doxygen(api_name)
```

---

## 文件 3: flash_map.yaml

**作用**: 定义 B6x 系列芯片的 Flash 内存布局

### 为什么需要？

内存布局信息用于验证链接脚本和内存配置的正确性：

| 场景 | 自动分析的局限 | 人工配置的优势 |
|------|----------------|----------------|
| Flash 区域划分 | 链接脚本解析复杂 | 明确定义 DATA/CODE 区域 |
| 芯片变体差异 | B61/B62/B63/B66 Flash 大小不同 | 一次性配置所有变体 |
| 特殊页面用途 | Boot/OTA/用户区域 | 精确到页面级别的映射 |

### 配置格式

```yaml
variants:
  flash_128kb:
    description: "Flash 128KB variant for B6x series"
    total_size:
      bytes: 131072        # 128KB in bytes
      kb: 128
      human: "128KB"
    address_range:
      start: "0x000000"
      end: "0x01FFFF"
    geometry:
      page_size: 256       # bytes
      sector_size: 4096    # 4KB (16 pages)
      total_pages: 512
      total_sectors: 32
    regions:
      data_region:
        name: "DATA Region"
        size:
          bytes: 16384
          kb: 16
        special_pages:
          - name: "LoadInfo"
            page: 0
            description: "Boot configuration block"
```

### 芯片变体

| 变体 | Flash 大小 | 适用芯片 |
|------|-----------|----------|
| `flash_128kb` | 128KB | B61, B62 |
| `flash_256kb` | 256KB | B63, B66, B68 |

### 使用场景

#### 场景: 验证 OTA 升级区域

```yaml
# 在 flash_map.yaml 中定义 OTA 区域
ota_region:
  name: "OTA Region"
  size:
    kb: 64
  sectors:
    start: 16
    end: 31
```

---

## 文件 4: sram_map.yaml

**作用**: 定义 B6x 系列芯片的 SRAM 内存布局和分配模式

### 为什么需要？

SRAM 分配影响 BLE 协议栈和应用程序的内存规划：

| 场景 | 自动分析的局限 | 人工配置的优势 |
|------|----------------|----------------|
| SRAM 块划分 | 多个 SRAM 块物理分离 | 明确定义 SRAM1/2/3 |
| 缓存配置 | SRAM3 可配置为 Cache | 标注使用注意事项 |
| BLE 协议栈需求 | 协议栈内存需求不透明 | 提供推荐分配方案 |

### 配置格式

```yaml
chip_variants:
  b61:
    flash_size_kb: 128
    description: "B61 chip with 128KB Flash"
  b63:
    flash_size_kb: 256
    description: "B63 chip with 256KB Flash"

sram_blocks:
  sram1:
    name: "SRAM1"
    description: "Main SRAM block 1"
    address_range:
      start: "0x20003000"
      end: "0x20007FFF"
    size:
      bytes: 20480       # 20KB
      kb: 20
  sram2:
    name: "SRAM2"
    size:
      bytes: 8192        # 8KB
      kb: 8
  sram3:
    name: "SRAM3"
    description: "SRAM block 3 - typically used as Cache"
    size:
      bytes: 4096        # 4KB
      kb: 4
    notes:
      - "Typically configured as Cache"
      - "Can be configured as general purpose SRAM"

sram_summary:
  total_size:
    bytes: 32768         # 32KB total
    kb: 32
```

### SRAM 块说明

| 块 | 大小 | 地址范围 | 典型用途 |
|----|------|----------|----------|
| SRAM1 | 20KB | 0x20003000-0x20007FFF | 主应用程序 |
| SRAM2 | 8KB | 0x20008000-0x20009FFF | BLE 协议栈 |
| SRAM3 | 4KB | 0x2000A000-0x2000AFFF | Cache / 扩展 |

### 使用场景

#### 场景: 验证内存配置

```python
# validate_config() 使用 sram_map.yaml 验证内存配置
config = {
    "memory": {
        "sram_used_kb": 24,
        "ble_stack_reserved_kb": 8
    }
}

# sram_map.yaml 提供的信息:
# - Total SRAM: 32KB
# - SRAM2 推荐用于 BLE 协议栈

if config["memory"]["sram_used_kb"] > 32:
    print("Error: SRAM usage exceeds available space")
```

---

## 实际应用示例

### 示例 1: validate_config() 使用依赖配置

```python
# 用户配置
config = {
    "uart": {
        "baudrate": 115200,
        "pins": ["PA9", "PA10"]
    }
}

# validate_config() 检查依赖
validation = await validate_config(config)

# dependency_overrides.yaml 提供的依赖信息:
# - B6x_UART_Init requires_clock=True
# - B6x_UART_Init requires_gpio=True

if validation.missing_dependencies:
    # 提示用户缺少配置
    print("Missing: RCC clock configuration")
    print("Missing: GPIO alternate function configuration")
```

### 示例 2: inspect_node() 使用寄存器映射

```python
# 查询 API 的寄存器信息
info = await inspect_node("B6x_UART_Init", "api")

# api_register_mapping.yaml 提供的映射:
# - registers: [UART_CR, UART_BRR, UART_LCR]

print(info.related_registers)
# 输出: ['UART_CR', 'UART_BRR', 'UART_LCR']
```

---

## 配置维护

### 添加新映射

**场景**: 发现新的依赖或映射关系

1. 编辑对应的 YAML 文件
2. 添加新的 entry
3. 重新运行 `build_index_v2.py` (可选，用于重新索引)

### 验证配置

```python
# 测试配置加载
from src.core.api_register_mapper import APIRegisterMapper
from src.core.dependency_extractor import DependencyExtractor

mapper = APIRegisterMapper("config/api_register_mapping.yaml")
extractor = DependencyExtractor("config/dependency_overrides.yaml")

# 查询特定 API
mapping = mapper.map_api_to_registers("B6x_UART_Init")
deps = extractor.get_api_dependencies("B6x_UART_Init")
```

---

## 最佳实践

### 1. 何时使用人工配置？

| 场景 | 推荐操作 |
|------|----------|
| 自动分析置信度 < 0.7 | 添加人工配置 |
| 关键 API (初始化类) | 添加完整依赖链 |
| 跨模块依赖 | 使用 `see_also` 链接 |
| 复杂位操作 | 明确列出所有字段 |

### 2. 置信度阈值

```python
# 代码中的置信度使用
if mapping.confidence >= 0.7:
    # 高置信度：直接使用
    return mapping
else:
    # 低置信度：提示用户验证
    logger.warning(f"Low confidence mapping for {api_name}")
    return mapping
```

### 3. 配置优先级

```
人工配置 (1.0) > 实现代码解析 (0.9) > 名称匹配 (0.7) > 参数推断 (0.5)
```

---

## 相关文件

```
xm_b6_mcp/
├── config/
│   ├── api_register_mapping.yaml      # API-寄存器映射
│   ├── dependency_overrides.yaml      # 依赖关系覆盖
│   ├── flash_map.yaml                 # Flash 内存布局
│   └── sram_map.yaml                  # SRAM 内存布局
├── src/core/
│   ├── api_register_mapper.py         # 映射解析器
│   └── dependency_extractor.py        # 依赖提取器
└── src/
    ├── layer2_detail.py               # inspect_node() 使用
    └── layer3_validation.py           # validate_config() 使用
```

---

## 总结

| 文件 | 主要作用 | 使用场景 | 置信度 |
|------|----------|----------|--------|
| `api_register_mapping.yaml` | API → 寄存器映射 | inspect_node() | 1.0 (人工) |
| `dependency_overrides.yaml` | API 依赖关系 | validate_config() | 1.0 (人工) |
| `flash_map.yaml` | Flash 内存布局 | validate_config() | 1.0 (人工) |
| `sram_map.yaml` | SRAM 内存布局 | validate_config() | 1.0 (人工) |

这些配置文件提供了**知识增强**机制，在自动分析不足时注入人工专家知识，确保 MCP Server 提供准确的开发建议。
