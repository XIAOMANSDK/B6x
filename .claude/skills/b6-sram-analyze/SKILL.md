---
name: b6-sram-analyze
description: |
  B6x 工程 SRAM/内存分析工具。

  TRIGGER when: 用户要求分析 SRAM 使用、内存使用分析、RAM 使用情况、MAP 文件分析、
  检查内存冲突、内存布局分析、"内存不够了"、"RAM 不够用"、"栈溢出"、"stack overflow"、
  "heap 不够"、"内存溢出"、"SRAM 使用率"、"剩余内存"、"内存占用"、
  BLE 库内存占用分析、优化内存使用、减少 RAM 占用。

  DO NOT TRIGGER when: 用户要求编译项目（用 /b6-build）、烧录调试（用 /b6-auto-debug）、
  代码审查（用 /b6-code-review）、硬件配置验证（用 /b6-validate-hardware）、
  BLE 协议问题、引脚配置问题。

user-invocable: true
disable-model-invocation: true
allowed-tools: Read, Grep, Glob
---

# B6 SRAM Analyzer

分析 B6x 工程 MAP 文件 SRAM 使用情况，检测冲突并提供优化建议。

## 触发

`/b6-sram-analyze [project_path]`

**参数**:
- `project_path`: 项目路径（可选，默认当前目录）

---

## B6x SRAM 布局

| 区域 | 起始地址 | 大小 | 用途 |
|------|----------|------|------|
| RAM | 0x20003000 | 20KB | 主 SRAM (Vector + User + Heap + Stack) |
| EM | 0x20008000 | 8KB | BLE Exchange Memory |

**总计**: 28 KB

---

## 执行流程

### Step 1: 定位 MAP 文件

根据构建系统查找：

| 构建系统 | MAP 文件路径 |
|----------|-------------|
| Keil | `<project>/mdk/output/*.map` |
| GCC | `<project>/gnu/build/map/*.map` |

**失败条件**: 未找到 MAP 文件时，提示用户先编译项目。

### Step 2: 解析 MAP 文件

#### Keil MAP 文件模式

```
Execution Region RW_IRAM1 (Base: 0x20003000)
    Base Addr    Size         Type   Attr
    0x20003000   0x00000098   Data   RW

Total RW Size (ZI + RW + ROM) = 4896 (0x1320)

Code (inc. data)   RO Data    RW Data    ZI Data      Debug   Object Name
     120          8          0          4         1536       988   startup.o
```

关键提取点:
- `Execution Region RW_IRAM*`: 获取区域地址和大小
- `Total RW Size`: 总 RW 大小
- `ZI Data` 列: 各模块的零初始化数据

#### GCC MAP 文件模式

```
MEMORY MAP
.
text    0x00000000    0x1234
data    0x20003000    0x5678
bss     0x20003500    0x9abc
```

关键提取点:
- `.bss` 段: 未初始化全局/静态变量
- `.data` 段: 已初始化全局/静态变量
- `_Heap_Begin` / `_Heap_End`: Heap 边界
- `_Stack_Top`: Stack 顶部

### Step 3: 计算使用率

```
SRAM 使用率 = Total RW Size / 28 KB × 100%
剩余空间 = 28 KB - Total RW Size
```

### Step 4: 检测冲突

| 检查项 | ⚠️ 警告 | 🔴 错误 |
|--------|---------|---------|
| SRAM 使用率 | > 90% | > 95% |
| Stack | < 1 KB | < 512 B |
| Heap | < 2 KB | < 1 KB |
| 区域重叠 | - | 任何 |
| 边界溢出 | - | 超出 0x20008000 |

### Step 5: 分析 TOP 10 模块

按 ZI Data 排序，识别内存大户。

---

## 输出格式

使用以下格式生成报告（以实际数据替代示例值）:

```markdown
# SRAM 使用分析报告 - {project_name}

## 内存布局

| 区域       | 起始地址    | 结束地址    | 大小    | 使用率 | 状态 |
|------------|-------------|-------------|---------|--------|------|
| Vector     | 0x20003000  | 0x20003098  | 152 B   | 0.5%   | ✅   |
| User RAM   | 0x20003098  | 0x20004E00  | 7.5 KB  | 26.8%  | ✅   |
| Heap       | 0x20004E00  | 0x20007A00  | 11 KB   | 39.3%  | ✅   |
| Stack      | 0x20007A00  | 0x20008000  | 1.5 KB  | 5.4%   | ✅   |

## BLE 库内存占用

| 库          | ZI Data | Total RAM |
|-------------|---------|-----------|
| ble6.lib    | 770 B   | 770 B     |

## 模块 TOP 10 (ZI Data)

| 排名 | 模块         | ZI Data | 占比   |
|------|--------------|---------|--------|
| 1    | startup.o    | 1536 B  | 8.2%   |
| 2    | app_task.o   | 512 B   | 2.7%   |
| 3    | uart.o       | 256 B   | 1.4%   |

## 总计

| 指标         | 值        | 状态 |
|--------------|-----------|------|
| Total RW     | 18.22 KB  |      |
| 使用率       | 65.1%     | ✅   |
| 剩余空间     | 9.78 KB   | ✅   |

**状态: ✅ PASS**
```

### 异常输出

发现问题时，在报告末尾附加:

```
⚠️ WARNING:
1. SRAM 使用率 92% (> 90%) — 建议减少 ZI 变量/buffer 大小
2. Stack 仅 768 B (< 1 KB) — 在 startup.s 增加 Stack_Size

🔴 FAIL:
1. 区域重叠: Heap 与 Stack 重叠 — 检查链接脚本分区配置
2. SRAM 使用率 97% (> 95%) — 减少 Heap 或 User RAM 分配
```

### 未找到 MAP 文件

```
❌ 未找到 MAP 文件
请先编译项目:
  Keil: 打开 mdk/*.uvprojx 并编译
  GCC:  cd <project_path>/gnu && cmake -B build -G Ninja && cmake --build build
```

---

## 优化建议

| 问题 | 原因 | 建议 |
|------|------|------|
| SRAM > 90% | ZI 变量过多 | 减少 buffer 大小，使用更小的数据类型 |
| Stack < 1 KB | 启动文件配置 | 在 startup.s 增加 `Stack_Size` |
| Heap < 2 KB | BLE 连接数过多 | 减少最大连接数，使用 BLE_LITE 库 |
| 区域重叠 | 链接脚本错误 | 检查 link_xip.ld 分区配置 |
| startup.o 过大 | Vector 表/Stack | 正常现象，无需优化 |

---

## 常见内存大户

| 模块 | 典型 ZI Data | 说明 |
|------|-------------|------|
| startup.o | 1-2 KB | Vector 表 + Stack 预留 |
| uart.o | 256-512 B | 接收缓冲区 |
| app_task.o | 256-1 KB | 任务栈/消息队列 |
| ble 相关 | 512-2 KB | BLE 协议栈缓冲 |
