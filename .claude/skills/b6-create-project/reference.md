# BLE 开发参考

> B6x SDK BLE 快速参考指南，用于 `/b6-create-project` skill
>
> **重要**：本文档中的路径、API 名称、库配置必须通过 MCP 工具（search_sdk/inspect_node）实时验证，
> 不可凭此文档或记忆直接使用。SDK 版本更新可能导致不兼容。

## 目录

1. [BLE 库选择规则](#1-ble-库选择规则)
2. [标准初始化流程](#2-标准初始化流程)
3. [Profile 注册顺序](#3-profile-注册顺序)
4. [关键消息处理](#4-关键消息处理)
5. [广播数据格式](#5-广播数据格式)
6. [常见错误速查](#6-常见错误速查)
7. [关键文件路径](#7-关键文件路径)
8. [GCC/CMake 项目模板](#8-gccccmake-项目模板)
10. [spec.md 模板](#10-specmd-模板)
11. [plan.md 模板](#11-planmd-模板)

---

## 1. BLE 库选择规则

| 库 | 宏配置 | 连接数 | 角色 | RAM |
|---|--------|-------|------|-----|
| **ble6_lite.lib** | `BLE_LITELIB=1` | 1 | 仅 Slave | ~6KB |
| **ble6.lib** | 默认 | 3 | Master+Slave | ~14.6KB |
| **ble6_8act_6con.lib** | `BLE_LARGELIB=1` | 6 | Master+Slave | ~16.2KB |

**警告**: lite 库不支持 Master 角色（`BLE_NB_MASTER` 必须为 0）！

### cfg.h 配置示例

```c
// lite 库配置（单从设备，低功耗）
#define BLE_LITELIB     (1)
#define BLE_NB_SLAVE    (1)
#define BLE_NB_MASTER   (0)   // lite 库必须为 0

// 标准库配置（ANCS Client 等需要 Master 的应用）
#define BLE_LITELIB     (0)
#define BLE_NB_SLAVE    (1)
#define BLE_NB_MASTER   (2)
```

---

## 2. 标准初始化流程

### main.c 模板

```c
static void sysInit(void)
{
    iwdt_disable();
    rcc_ble_en();                        // ★ 使能 BLE 时钟
    rcc_adc_en();
    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    uart1Rb_Init();
    debug("Start(rsn:%X)...\r\n", rsn);
    app_init(rsn);                       // ★ BLE 应用初始化
}

int main(void)
{
    sysInit();
    devInit();
    GLOBAL_INT_START();

    while (1)
    {
        ble_schedule();                  // ★ 必须在主循环调用
        sftmr_schedule();
        user_procedure();
    }
}
```

### app_init 内部流程

```c
void app_init(uint16_t rsn)
{
    ble_heap(&heap);      // 配置堆内存
    ble_init();           // 初始化协议栈
    ble_app();            // 初始化应用任务 → gapm_reset()
    rfmdm_init();         // RF 调制解调器
    NVIC_EnableIRQ(BLE_IRQn);
}
```

---

## 3. Profile 注册顺序

**文件**: `app.c` 中的 `app_prf_create()`

```c
__WEAK void app_prf_create(void)
{
    // 1. GAP Service (0x1800) - 必须第一个
    gap_svc_init(GAP_START_HDL, GAP_ATT_CFG);

    // 2. GATT Service (0x1801) - Slave 需要
    #if (BLE_NB_SLAVE)
    gatt_svc_init(GAP_START_HDL);
    #endif

    // 3. 标准服务
    #if (PRF_DISS)
    diss_svc_init();       // Device Information
    #endif

    #if (PRF_BASS)
    bass_svc_init();       // Battery Service
    #endif

    #if (PRF_HIDS)
    hids_prf_init();       // HID Service
    #endif

    // 4. 自定义 Profile 在最后
}
```

---

## 4. 关键消息处理

### 连接相关消息

| 消息 | 触发时机 | 必要处理 |
|------|---------|---------|
| `GAPC_CONNECTION_IND` | 连接建立成功 | 保存 conidx，更新状态 |
| `GAPC_DISCONNECT_IND` | 连接断开 | **重启广播** |
| `GAPC_PARAM_UPDATE_REQ_IND` | 对端请求更新参数 | **必须响应** `gapc_param_update_rsp()` |

### 断开后重启广播

```c
APP_MSG_HANDLER(gapc_disconnect_ind)
{
    uint8_t conidx = TASK_IDX(src_id);
    DEBUG("Disconnect(cid:%d, reason:0x%02X)", conidx, param->reason);

    // 重启广播
    app_init_action(ACTV_CREATE);
}
```

### 连接参数更新响应

```c
APP_MSG_HANDLER(gapc_param_update_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    // 必须响应！
    gapc_param_update_rsp(conidx, true, param->intv_min, param->intv_max);
}
```

---

## 5. 广播数据格式

### 长度限制

- 广播数据: ≤ 31 字节
- 扫描响应数据: ≤ 31 字节

### 格式规范

```c
// 格式: [长度][类型][数据]
#define APP_ADV_DATA  "\x03\x03\xFF\x00\x03\x19\xC1\x03\x09\x09B6x-HID"
// 解析:
// \x03\x03\xFF\x00     - 3字节, 类型0x03(16-bit UUID), 数据0x00FF
// \x03\x19\xC1\x03     - 4字节, 类型0x19(Appearance), 数据0x03C1(键盘)
// \x09\x09B6x-HID      - 9字节, 类型0x09(Complete Name)
```

### 常用 AD Type

| 值 | 类型 | 说明 |
|---|------|------|
| 0x01 | Flags | 发现模式标志 |
| 0x03 | Complete List 16-bit UUID | 16位 UUID 列表 |
| 0x08 | Shortened Name | 简称（名称过长时用） |
| 0x09 | Complete Name | 完整名称 |
| 0x19 | Appearance | 设备外观 |

---

## 6. 常见错误速查

| 错误码 | 含义 | 原因 | 解决 |
|-------|------|------|------|
| 0x42 | NOT_SUPPORTED | lite 库使用 Master | 换标准库 |
| 0x4A | ADV_DATA_INVALID | 广播数据格式/长度错误 | 检查长度 ≤ 31 |
| 0x4B | INSUFF_RESOURCES | RAM 不足 | 减少连接数/Profile |
| 0x99 | CON_LIMIT_EXCEED | 连接数超限 | 检查库限制 |
| 0xFD | CCCD_IMPR_CONFIGURED | CCCD 配置错误 | 检查通知使能顺序 |

---

## 7. 关键文件路径

| 文件 | 用途 |
|------|------|
| `ble/api/bledef.h` | BLE 核心接口 |
| `ble/api/blelib.h` | 库配置宏 |
| `ble/api/gapm_api.h` | GAP Manager API |
| `ble/api/gapc_api.h` | GAP Controller API |
| `ble/api/le_err.h` | 错误码定义 |
| `ble/app/app.c` | 应用初始化示例 |
| `ble/app/app_gapc.c` | 连接事件处理示例 |

> 查找参考项目时，使用 `search_sdk(query, scope="examples")` 动态定位，
> 不要硬编码项目路径（路径可能随版本变化）。

---

## 8. GCC/CMake 项目模板

### 外设示例模板（TYPE_PERIPH）

参考 `examples/_blank/gnu/CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20.0)

# 如果 SDK_ROOT 未定义，使用相对路径设置
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project({PROJECT_NAME} C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)

list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/debug.c
    # 按需添加: ${SDK_MODULES_SRC_DIR}/sftmr.c 等
)

set_module_definitions(SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${LOCAL_SRC_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DRVS_LIB}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -include "${LOCAL_SRC_DIR}/cfg.h"
    ${COMMON_COMPILE_OPTIONS}
)

setup_target_link_options(${PROJECT_NAME} ${LINK_SCRIPT})

generate_project_output(${PROJECT_NAME})
```

### BLE 项目模板（TYPE_BLE / TYPE_MIXED）

参考 `projects/bleUart/gnu/CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20.0)

if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project({PROJECT_NAME} C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)
aux_source_directory("${SDK_BLE_APP_DIR}" SRC_LIST)

list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/debug.c
    ${SDK_MODULES_SRC_DIR}/sftmr.c
    ${SDK_MODULES_SRC_DIR}/uart1Rb.c
    # 按需添加 Profile 源文件：
    # ${SDK_BLE_PRF_DIR}/prf_diss.c
    # ${SDK_BLE_PRF_DIR}/prf_bass.c
    # ${SDK_BLE_PRF_DIR}/prf_sess.c
)

set_module_definitions(SRC_LIST)

# BLE 项目需要预处理链接脚本（cfg.h 中的宏影响内存布局）
set(LINKER_SCRIPT_IN ${BLE_LINK_SCRIPT})
set(PROCESSED_LINKER_SCRIPT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.ld")

add_custom_command(
  OUTPUT ${PROCESSED_LINKER_SCRIPT}
  COMMAND ${CMAKE_C_COMPILER}
    -E -P -x c
    -I "${LOCAL_SRC_DIR}"
    -I "${SDK_BLE_DIR}/api"
    -include "${LOCAL_SRC_DIR}/cfg.h"
    "${LINKER_SCRIPT_IN}"
    -o "${PROCESSED_LINKER_SCRIPT}"
  DEPENDS "${LINKER_SCRIPT_IN}"
  COMMENT "Preprocessing ${PROJECT_NAME} linker script"
  VERBATIM
)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${PROCESSED_LINKER_SCRIPT})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${LOCAL_SRC_DIR}"
    ${SDK_BLE_INCLUDE}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DRVS_LIB}
    ${BLE_LIB}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -include "${LOCAL_SRC_DIR}/cfg.h"
    ${COMMON_COMPILE_OPTIONS}
)

setup_target_link_options(${PROJECT_NAME} ${PROCESSED_LINKER_SCRIPT})

generate_project_output(${PROJECT_NAME})
```

### 关键 CMake 变量说明

| 变量 | 说明 |
|------|------|
| `${SDK_ROOT}` | SDK 根目录 |
| `${STARTUP_SRC}` | 启动文件（由 sdk.cmake 定义） |
| `${SDK_MODULES_SRC_DIR}` | `modules/` 目录 |
| `${SDK_BLE_APP_DIR}` | `ble/app/` 目录 |
| `${SDK_BLE_PRF_DIR}` | `ble/prf/` 目录 |
| `${SDK_BLE_INCLUDE}` | BLE 头文件路径集合 |
| `${COMMON_INCLUDE_DIRS}` | 通用头文件路径 |
| `${COMMON_COMPILE_OPTIONS}` | 通用编译选项 |
| `${DRVS_LIB}` | 驱动静态库 |
| `${BLE_LIB}` | BLE 协议栈库（由 cfg.h 宏选择） |
| `${BLE_LINK_SCRIPT}` | BLE 链接脚本模板 |
| `${LINK_SCRIPT}` | 外设项目链接脚本 |

---

## 10. spec.md 模板

```markdown
# 需求规格 - {项目名称}
- **类型**: BLE 应用 / 外设示例 / BLE+外设混合
- **路径**: `projects/{name}` 或 `examples/{name}`
- **构建系统**: Keil / GCC / Both

## 功能需求
- [ ] 功能 1: 描述

## 硬件配置
| 外设 | 引脚 | 用途 | 备注 |
|------|------|------|------|

## BLE 配置（仅 BLE 项目）
- **库**: ble6_lite.lib / ble6.lib / ble6_8act_6con.lib
- **BLE_LITELIB/BLE_LARGELIB**: 0/1 | **NB_SLAVE/NB_MASTER**: N/N
- **Profile**: DISS, BASS, HIDS...

## 参考项目
- `projects/{similar}` — 相似原因
```

---

## 11. plan.md 模板

### 设计原则（源自 writing-plans 方法论）

| 原则 | 说明 |
|------|------|
| **无占位符** | 每步包含完整代码，禁止 "TBD/TODO/后续补充" |
| **原子步骤** | 每步 2-5 分钟可完成的单一操作 |
| **精确路径** | 始终使用相对于 SDK 根目录的精确文件路径 |
| **可验证** | 每步有明确的预期结果或验证方式 |
| **DRY/YAGNI** | 不重复、不提前设计不需要的功能 |

---

### Header 模板

```markdown
# {项目名称} Implementation Plan

**Goal:** {一句话描述项目目标}

**Architecture:** {2-3 句描述架构方案：项目类型、关键组件、数据流}

**Tech Stack:** B6x SDK | {芯片型号} | {构建系统: Keil/GCC/Both} | {BLE库: lite/标准/大型}

**Reference Template:** `{参考项目路径}` | **Approach:** {minimal/optimal/balanced}

**Scope:** {TYPE_BLE / TYPE_PERIPH / TYPE_MIXED}
```

---

### File Structure 模板

任务拆分前，先锁定文件职责映射：

```markdown
## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `src/main.c` | Create | 系统初始化、主循环 |
| `src/cfg.h` | Create | [仅 BLE] BLE 库与连接数配置 |
| `src/app.c` | Create | [仅 BLE] BLE 应用初始化、Profile 注册 |
| `src/{feature}.c` | Create | 自定义功能模块 |
| `src/{feature}.h` | Create | 功能模块接口 |
| `mdk/{name}.uvprojx` | Create | [如选择] Keil 工程文件 |
| `gnu/CMakeLists.txt` | Create | [如选择] GCC 构建文件 |

### Directory Layout
{project}/
├── src/
│   ├── main.c           — entry point, sysInit + devInit + main loop
│   ├── cfg.h            — [BLE] BLE lib/slave/master config
│   ├── app.c            — [BLE] app_init, app_prf_create, gap handlers
│   └── {feature}.c      — custom peripheral/feature logic
├── mdk/                 — [如选择]
│   └── {name}.uvprojx   — Keil project
└── gnu/                 — [如选择]
    └── CMakeLists.txt   — GCC/CMake build
```

---

### Task 模板

每个 Task 对应一个组件，每个 Step 是原子操作。Step 必须包含完整代码。

```markdown
## Tasks

### Task 1: Project Scaffolding

**Files:**
- Create: `{project}/src/main.c`
- Create: `{project}/src/cfg.h` (仅 BLE/Mixed)

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p {project}/src {project}/mdk {project}/gnu
```

Expected: directories created

- [ ] **Step 2: Create cfg.h** (仅 BLE/Mixed)

```c
// cfg.h — BLE configuration
#ifndef __CFG_H__
#define __CFG_H__

#define BLE_LITELIB     ({0 or 1})
#define BLE_LARGELIB    ({0 or 1})
#define BLE_NB_SLAVE    ({N})
#define BLE_NB_MASTER   ({N})
// ... 其他项目特定宏

#endif
```

- [ ] **Step 3: Create main.c**

```c
// main.c — complete code here
#include "cfg.h"
// ... 完整实现，包含 sysInit/devInit/main
```

- [ ] **Step 4: Verify file structure**

```bash
find {project} -type f | sort
```

Expected: all files listed in File Structure present

---

### Task 2: BLE Application (仅 TYPE_BLE/TYPE_MIXED)

**Files:**
- Create: `{project}/src/app.c`
- Reference: `ble/app/app.c`, `ble/app/app_gapc.c`

- [ ] **Step 1: Create app.c with initialization**

```c
// app.c — complete app_init + ble_app + app_prf_create
// 完整代码...
```

- [ ] **Step 2: Add GAP message handlers**

```c
// GAPC_CONNECTION_IND handler — 完整代码
// GAPC_DISCONNECT_IND handler — 含重启广播逻辑
// GAPC_PARAM_UPDATE_REQ_IND handler — 含 gapc_param_update_rsp 响应
```

- [ ] **Step 3: Add advertising data**

```c
// 广播数据和扫描响应数据 — 完整的十六进制字符串
```

- [ ] **Step 4: Verify BLE initialization chain**

Checklist: app_init → ble_init → ble_app → app_prf_create → gapm_reset

---

### Task 3: Build System

**Files:**
- Create: `{project}/mdk/{name}.uvprojx` (如选择)
- Create: `{project}/gnu/CMakeLists.txt` (如选择)

- [ ] **Step 1: Create CMakeLists.txt** (如选择 GCC)

```cmake
# 完整的 CMakeLists.txt 内容
# 参考 reference.md 第 8 节模板
# 按项目类型选择 TYPE_PERIPH 或 TYPE_BLE 模板
```

- [ ] **Step 2: Create Keil project** (如选择 Keil)

使用参考模板的 .uvprojx 文件，修改：
- 项目名称
- 源文件列表
- 头文件路径
- 预处理宏（cfg.h 中的宏）

- [ ] **Step 3: Verify build configuration**

验证项：
- [ ] 头文件包含路径正确
- [ ] 源文件列表完整
- [ ] 链接脚本匹配项目类型
- [ ] 预处理宏与 cfg.h 一致

---

### Task N: Custom Feature Module

**Files:**
- Create: `{project}/src/{feature}.c`
- Create: `{project}/src/{feature}.h`
- Modify: `{project}/src/main.c` (如需在主循环调用)

- [ ] **Step 1: Create header file**

```c
// {feature}.h — 完整接口定义
```

- [ ] **Step 2: Create implementation**

```c
// {feature}.c — 完整实现
```

- [ ] **Step 3: Integrate into main.c**

```c
// 在 main.c 中添加初始化调用和主循环处理
```

- [ ] **Step 4: Verify integration**

验证项：
- [ ] 头文件包含正确
- [ ] 初始化调用顺序合理
- [ ] 编译无未定义符号
```

---

### Self-Review Checklist

计划完成后，执行以下自审（不派发子代理，自行检查）：

```markdown
## Self-Review

### 1. Spec Coverage
对照 spec.md 逐项检查：
- [ ] 每个功能需求都有对应 Task
- [ ] 每个外设都有初始化和集成步骤
- [ ] [仅 BLE] Profile 注册覆盖所有需求的服务

### 2. Placeholder Scan
搜索以下红旗模式，发现则修复：
- [ ] 无 TBD / TODO / "后续补充" / "implement later"
- [ ] 无 "添加适当的错误处理" / "handle edge cases"
- [ ] 无 "参考 Task N" (不重复代码)
- [ ] 每个 Step 有完整代码或命令（非仅描述）

### 3. Type Consistency
- [ ] 函数签名在声明和调用处一致
- [ ] 宏定义值在 cfg.h 与 CMakeLists.txt/uvprojx 中一致
- [ ] [仅 BLE] BLE_NB_SLAVE + BLE_NB_MASTER ≤ 库连接数限制
- [ ] [仅 BLE] BLE_LITELIB=1 时 BLE_NB_MASTER=0

### 4. Build System Consistency
- [ ] CMakeLists.txt 源文件列表与实际文件一致
- [ ] 头文件路径覆盖所有 #include
- [ ] 链接脚本类型匹配项目类型（BLE 用 BLE_LINK_SCRIPT）
```
