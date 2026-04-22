---
name: b6-cmake-init
description: |
  为现有 B6x SDK Keil 项目生成 CMakeLists.txt，实现 Keil → GNU/CMake 迁移。

  TRIGGER when: 用户要求为现有项目生成 CMakeLists.txt、添加 GCC/CMake 支持、迁移 Keil 项目到 GNU 环境、
  创建 CMake 配置、转换 Keil 工程到 CMake、为项目添加 GCC 编译支持、支持 CI/CD 编译、
  "帮我生成 CMakeLists.txt"、"添加 CMake 支持"、"迁移到 GCC"、"让项目支持 GCC 编译"。

  DO NOT TRIGGER when: 用户要求创建全新项目（用 /b6-create-project）、编译现有项目（用 /b6-build）、
  烧录调试（用 /b6-auto-debug）、纯代码审查、BLE 协议问题、仅查看项目结构。

user-invocable: true
disable-model-invocation: true
allowed-tools: Read, Grep, Glob, Write, Bash
---

# B6 CMake Init

为现有 B6x SDK Keil 项目生成 CMakeLists.txt，支持从 Keil 项目迁移到 GNU/CMake 环境。

## 触发

`/b6-cmake-init <project_path>`

**参数**:
- `project_path`: 项目路径（必需，包含 mdk/*.uvprojx 的目录）

---

## 使用场景

1. **迁移现有 Keil 项目** 到 GNU 环境
2. **添加 GCC 支持** 到仅 Keil 的项目
3. **创建 CMakeLists.txt** 用于 CI/CD 自动化
4. **双构建系统支持** - 同时保留 Keil 和 GCC 编译能力

---

## 执行流程

### Step 1: 验证输入

```
检查项目路径是否存在
检查 mdk/*.uvprojx 是否存在
检查 src/ 目录是否存在
```

**失败条件**（立即退出）:
- 路径不存在
- 未找到 Keil 工程文件 (uvprojx)
- 未找到源文件目录

### Step 2: 解析 Keil 工程

从 uvprojx 文件提取:

| 信息 | XML 路径 | 用途 |
|------|----------|------|
| 项目名称 | `TargetOption/TargetName` | project() 名称 |
| 源文件 | `Groups/Group/Files` | SRC_LIST |
| Include 路径 | `VariousControls/IncludePath` | target_include_directories |
| 宏定义 | `VariousControls/Define` | target_compile_definitions |
| 链接脚本 | `LDads` | LINK_SCRIPT |

### Step 3: 判断项目类型

| 类型 | 判断条件 | 模板参考 |
|------|----------|----------|
| 纯外设 | 无 BLE 相关文件 | `examples/_blank/gnu/CMakeLists.txt` |
| BLE 应用 | 包含 ble 相关源文件 | `projects/bleHid/gnu/CMakeLists.txt` |
| Mesh | 包含 mesh 相关文件 | `mesh/gnu/CMakeLists.txt` |

### Step 4: 分析 BLE 配置（如适用）

从 `src/cfg.h` 或 `inc/cfg.h` 读取 BLE 库配置:

```c
// 实际宏名称（优先级从上到下匹配）
#define BLE_LITELIB   (1)   // → ble6_lite.lib  (仅 Slave, 1 连接)
#define BLE_LARGELIB  (1)   // → ble6_8act_6con.lib (6 连接)
// 两者均为 0   → ble6.lib (标准库, 3 连接)
```

识别 Profile 源文件:

| 文件 | Profile | 说明 |
|------|---------|------|
| `prf_diss.c` | Device Information | 设备信息 |
| `prf_bass.c` | Battery Service | 电池服务 |
| `prf_hids.c` | HID Service | HID 设备 |
| `prf_otas.c` | OTA Service | OTA 升级 |

### Step 5: 生成 CMakeLists.txt

根据项目类型选择模板，填充提取的配置。

### Step 6: 创建 gnu/ 目录

```
project/gnu/
└── CMakeLists.txt
```

---

## CMakeLists.txt 模板

### 纯外设项目

```cmake
cmake_minimum_required(VERSION 3.20.0)

# 如果 SDK_ROOT 未定义，使用相对路径设置
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(<PROJECT_NAME> C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)

list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/debug.c
)

# 设置 __MODULE__ 宏定义
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

### BLE 项目

```cmake
cmake_minimum_required(VERSION 3.20.0)

if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(<PROJECT_NAME> C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)
aux_source_directory("${SDK_BLE_APP_DIR}" SRC_LIST)

list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/leds.c
    ${SDK_MODULES_SRC_DIR}/sftmr.c
    ${SDK_MODULES_SRC_DIR}/uart1Rb.c
    ${SDK_MODULES_SRC_DIR}/debug.c
    # 添加 Profile 源文件 (根据 Keil 工程分析结果)
    ${SDK_BLE_PRF_DIR}/prf_diss.c
    ${SDK_BLE_PRF_DIR}/prf_bass.c
)

# 设置 __MODULE__ 宏定义
set_module_definitions(SRC_LIST)

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

---

## 输出格式

### 成功

```
================================================================================
                        CMake Initialization Complete
================================================================================

项目: bleMyProject
路径: projects/bleMyProject/gnu/CMakeLists.txt

--------------------------------------------------------------------------------
分析结果
--------------------------------------------------------------------------------

| 项目类型 | BLE 应用           |
|----------|--------------------|
| BLE 库   | ble6.lib           |
| Profiles | DISS, BASS, HIDS   |
| 源文件   | 12 个              |
| 启动文件 | startup_gnu.S      |
| 链接脚本 | link_xip_ble.ld    |

--------------------------------------------------------------------------------
生成文件
--------------------------------------------------------------------------------

| 文件                       | 操作 | 说明              |
|----------------------------|------|-------------------|
| gnu/CMakeLists.txt         | 新建 | CMake 配置        |
| gnu/                       | 新建 | GNU 工程目录      |

--------------------------------------------------------------------------------
后续步骤
--------------------------------------------------------------------------------

1. 编译验证:
   cd <project_path>/gnu
   cmake -B build -G Ninja
   cmake --build build

2. 调试配置 (可选):
   - 创建 .vscode/launch.json (Cortex-Debug)
   - 创建 .vscode/tasks.json (CMake 任务)

================================================================================
```

### 失败

```
================================================================================
                        CMake Initialization Failed
================================================================================

项目: projects/myProject
错误: 未找到 Keil 工程文件

请确认:
1. 项目目录包含 mdk/*.uvprojx 文件
2. 路径正确

================================================================================
```

---

## 目录结构

### 输入要求

```
project/
├── mdk/              # Keil 工程 (必需)
│   └── *.uvprojx
├── src/              # 源文件 (必需)
│   └── main.c
└── inc/              # 头文件 (可选)
    └── cfg.h
```

### 输出结构

```
project/
├── mdk/              # Keil 工程 (保留)
├── gnu/              # 新增 GNU 工程
│   └── CMakeLists.txt
├── src/
└── inc/
```

