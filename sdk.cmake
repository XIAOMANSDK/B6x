# ##############################################################################
# sdk.cmake - B6x SDK 公共配置文件
# ##############################################################################
# 文件说明:
#   本文件是 B6x SDK 的核心 CMake 配置文件，包含：
#   - 工具链配置 (ARM GCC)
#   - 编译/链接选项
#   - SDK 库文件管理
#   - 公共函数定义
#
# 使用方法:
#   在项目 CMakeLists.txt 中添加: include("${SDK_ROOT}/sdk.cmake")
# ##############################################################################

# ##############################################################################
# 防止重复包含
# ##############################################################################
if(SDK_CMAKE_INCLUDED)
    return()
endif()
set(SDK_CMAKE_INCLUDED TRUE)


# ##############################################################################
# 第 1 部分: 中文路径兼容性配置
# ##############################################################################
# 说明: 解决 Windows 下中文路径编码问题
# ##############################################################################

# 1.1 强制编译器和链接器使用响应文件 (避免命令行长度限制和编码问题)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS   1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES  1)

# 1.2 Ninja 生成器专用: 强制使用响应文件
if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")
endif()

# 1.3 设置 GCC 编译器输入/输出编码 (解决源码中文注释报错)
add_compile_options(
    -finput-charset=UTF-8    # 源文件编码
    -fexec-charset=GBK       # 执行字符集编码
)

# 1.4 链接器响应文件配置 (解决命令行编码问题)
set(CMAKE_C_LINKER_RESPONSE_FILE_FLAG       "@")
set(CMAKE_C_LINKER_RESPONSE_FILE_LINK_FLAG  "@")


# ##############################################################################
# 第 2 部分: 构建类型与 SDK 路径设置
# ##############################################################################
# 说明: 配置构建类型和 SDK 根目录
# ##############################################################################

# 2.1 重置 CMake 默认编译标志 (避免与自定义选项冲突)
set(CMAKE_C_FLAGS_DEBUG          "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE        "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL     "" CACHE STRING "" FORCE)

# 2.2 设置 ELF 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/elf)

# 2.3 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# 2.4 验证构建类型有效性
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if(NOT CMAKE_BUILD_TYPE_LOWER MATCHES "^(debug|release)$")
    message(WARNING "Unknown CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}, using Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# 2.5 设置 SDK 根目录
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "SDK Root Path")
endif()


# ##############################################################################
# 第 3 部分: 交叉编译配置
# ##############################################################################
# 说明: 配置 ARM Cortex-M0+ 交叉编译环境
# ##############################################################################

set(CMAKE_SYSTEM_NAME             Generic)        # 嵌入式系统无操作系统
set(CMAKE_SYSTEM_PROCESSOR        ARM)            # ARM 处理器
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) # 避免链接测试
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)             # 生成 compile_commands.json


# ##############################################################################
# 第 4 部分: 工具链定义
# ##############################################################################
# 说明: ARM GCC 工具链配置
# ##############################################################################

# 4.1 工具链前缀
set(TOOLCHAIN_PREFIX  "arm-none-eabi-")
string(REGEX REPLACE "-$" "" TOOLCHAIN_TRIPLE "${TOOLCHAIN_PREFIX}")

# 4.2 工具链组件
set(CMAKE_C_COMPILER    ${TOOLCHAIN_PREFIX}gcc)       # C 编译器
set(CMAKE_ASM_COMPILER  ${CMAKE_C_COMPILER})          # 汇编编译器
set(CMAKE_CXX_COMPILER  ${TOOLCHAIN_PREFIX}g++)       # C++ 编译器
set(CMAKE_OBJCOPY       ${TOOLCHAIN_PREFIX}objcopy)   # 目标文件转换
set(CMAKE_SIZE_UTIL     ${TOOLCHAIN_PREFIX}size)      # 段大小统计
set(CMAKE_OBJDUMP       ${TOOLCHAIN_PREFIX}objdump)   # 反汇编工具

# 4.3 查找并验证工具链
find_program(TOOLCHAIN_PATH ${CMAKE_C_COMPILER})
if(NOT TOOLCHAIN_PATH)
    message(FATAL_ERROR
        "Error: ${CMAKE_C_COMPILER} not found in PATH\n"
        "Please ensure ARM GCC toolchain is properly installed"
    )
endif()
get_filename_component(TOOLCHAIN_DIR ${TOOLCHAIN_PATH} DIRECTORY)

# 4.4 验证工具链组件完整性
foreach(comp IN LISTS CMAKE_C_COMPILER CMAKE_OBJCOPY CMAKE_SIZE_UTIL CMAKE_OBJDUMP)
    find_program(_comp_path ${comp})
    if(NOT _comp_path)
        message(WARNING "Warning: Toolchain component not found: ${comp}")
    endif()
    unset(_comp_path CACHE)
endforeach()

# 4.5 获取 GCC 版本号
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)


# ##############################################################################
# 第 5 部分: SDK 目录结构
# ##############################################################################
# 说明: 定义 SDK 各组件的路径
# ##############################################################################

# 5.1 核心目录
set(SDK_CORE_DIR         "${SDK_ROOT}/core")
set(SDK_CORE_GNU_DIR     "${SDK_CORE_DIR}/gnu")

# 5.2 BLE 蓝牙目录
set(SDK_BLE_DIR          "${SDK_ROOT}/ble")
set(SDK_BLE_APP_DIR      "${SDK_ROOT}/ble/app")
set(SDK_BLE_PRF_DIR      "${SDK_ROOT}/ble/prf")
set(SDK_BLE_INCLUDE
    "${SDK_ROOT}/ble/api"
    "${SDK_BLE_APP_DIR}"
    "${SDK_BLE_PRF_DIR}"
)

# 5.3 USB 目录
set(SDK_USB_DIR          "${SDK_ROOT}/usb")
set(SDK_USB_CLASS_DIR    "${SDK_USB_DIR}/class")

# 5.4 驱动和模块目录
set(SDK_DRIVERS_DIR      "${SDK_ROOT}/drivers")
set(SDK_MODULES_DIR      "${SDK_ROOT}/modules")
set(SDK_MODULES_SRC_DIR  "${SDK_ROOT}/modules/src")

# 5.5 Mesh 目录
set(SDK_MESH_DIR         "${SDK_ROOT}/mesh")


# ##############################################################################
# 第 6 部分: 启动文件
# ##############################################################################
# 说明: ARM Cortex-M 启动汇编代码
# ##############################################################################

set(STARTUP_SRC          "${SDK_CORE_GNU_DIR}/startup_gnu.S")        # Flash 启动
set(MESH_STARTUP_SRC     "${SDK_CORE_GNU_DIR}/startup_gnu_mesh.S")   # Flash 启动, Mesh 版本
set(SRAM_STARTUP_SRC     "${SDK_CORE_GNU_DIR}/startup_sram_gnu.S")   # SRAM 启动


# ##############################################################################
# 第 7 部分: 链接脚本
# ##############################################################################
# 说明: 内存布局配置文件
# ##############################################################################

set(SRAM_LINK_SCRIPT     "${SDK_CORE_GNU_DIR}/link_sram.ld")         # SRAM 运行
set(LINK_SCRIPT          "${SDK_CORE_GNU_DIR}/link_xip.ld")          # XIP 执行
set(BLE_LINK_SCRIPT      "${SDK_CORE_GNU_DIR}/link_xip_ble.ld")      # BLE 标准版
set(BLE_MESH_LINK_SCRIPT "${SDK_CORE_GNU_DIR}/link_xip_mesh.ld")     # BLE Mesh
set(BLE_LITE_LINK_SCRIPT "${SDK_CORE_GNU_DIR}/link_xip_blelite.ld")  # BLE 精简版


# ##############################################################################
# 第 8 部分: 工具链库路径
# ##############################################################################
# 说明: GCC 工具链的库文件和头文件路径
# ##############################################################################

# 8.1 获取工具链安装根目录
get_filename_component(TOOLCHAIN_ROOT ${TOOLCHAIN_DIR} DIRECTORY)

# 8.2 multilib 后缀 (Cortex-M0+ 无硬件浮点)
set(MULTILIB_SUFFIX "thumb/v6-m/nofp")

# 8.3 库文件目录
set(TOOLCHAIN_LIBGCC_DIR "${TOOLCHAIN_ROOT}/lib/gcc/${TOOLCHAIN_TRIPLE}/${GCC_VERSION}/${MULTILIB_SUFFIX}")
set(TOOLCHAIN_LIBC_DIR   "${TOOLCHAIN_ROOT}/${TOOLCHAIN_TRIPLE}/lib/${MULTILIB_SUFFIX}")

# 8.4 头文件目录
set(TOOLCHAIN_INCLUDE_DIR "${TOOLCHAIN_ROOT}/${TOOLCHAIN_TRIPLE}/include")

# 8.5 打印工具链信息
message(STATUS "----------------------------------------")
message(STATUS "Toolchain Information:")
message(STATUS "  CMake Version    : ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
message(STATUS "  GCC Version      : ${GCC_VERSION}")
message(STATUS "  Toolchain Dir    : ${TOOLCHAIN_DIR}")
message(STATUS "  Toolchain Root   : ${TOOLCHAIN_ROOT}")
message(STATUS "  libc Path        : ${TOOLCHAIN_LIBC_DIR}")
message(STATUS "  libgcc Path      : ${TOOLCHAIN_LIBGCC_DIR}")
message(STATUS "  Include Path     : ${TOOLCHAIN_INCLUDE_DIR}")
message(STATUS "  Output Dir       : ${CMAKE_BINARY_DIR}")
message(STATUS "----------------------------------------")


# ##############################################################################
# 第 9 部分: 公共头文件目录
# ##############################################################################
# 说明: 所有项目通用的头文件包含路径
# ##############################################################################

set(COMMON_INCLUDE_DIRS
    ${TOOLCHAIN_INCLUDE_DIR}
    ${SDK_CORE_DIR}
    ${SDK_CORE_DIR}/reg
    ${SDK_DRIVERS_DIR}/api
    ${SDK_MODULES_DIR}/api
)


# ##############################################################################
# 第 10 部分: 链接库目录标志
# ##############################################################################
# 说明: 传递给链接器的库搜索路径
# ##############################################################################

set(TOOLCHAIN_LIB_DIR_FLAGS
    -L${TOOLCHAIN_LIBGCC_DIR}
    -L${TOOLCHAIN_LIBC_DIR}
)


# ##############################################################################
# 第 11 部分: SDK 库文件管理
# ##############################################################################
# 说明: 将 SDK 库文件复制到 build/lib 目录，避免中文路径编码问题
# ##############################################################################

# 11.1 库文件复制函数
# 参数:
#   lib_name       - 库文件名 (如 libdrvs.a)
#   lib_source_dir - 库文件源目录
function(copy_sdk_lib_to_build lib_name lib_source_dir)
    set(LIB_BUILD_DIR "${CMAKE_BINARY_DIR}/lib")
    file(MAKE_DIRECTORY "${LIB_BUILD_DIR}")

    set(SRC_LIB "${lib_source_dir}/${lib_name}")
    set(DST_LIB "${LIB_BUILD_DIR}/${lib_name}")

    # 检查源文件是否存在
    if(NOT EXISTS "${SRC_LIB}")
        message(WARNING "Source library not found: ${SRC_LIB}")
        return()
    endif()

    # 计算源文件哈希值
    file(SHA256 "${SRC_LIB}" SRC_HASH)

    # 检查目标文件是否存在且哈希值相同
    if(EXISTS "${DST_LIB}")
        file(SHA256 "${DST_LIB}" DST_HASH)
        if(SRC_HASH STREQUAL DST_HASH)
            #message(STATUS "Library up-to-date: ${lib_name}")
            return()
        endif()
    endif()

    # 拷贝文件
    file(COPY "${SRC_LIB}" DESTINATION "${LIB_BUILD_DIR}")
    if(EXISTS "${DST_LIB}")
        message(STATUS "Updated library: ${lib_name} (source changed)")
    else()
        message(STATUS "Copied library: ${lib_name}")
    endif()
endfunction()

# 11.2 复制 SDK 库文件到 build/lib 目录
copy_sdk_lib_to_build("libdrvs.a"           "${SDK_DRIVERS_DIR}/lib")
copy_sdk_lib_to_build("libusbd.a"           "${SDK_USB_DIR}/lib")
copy_sdk_lib_to_build("libusbd_lite.a"      "${SDK_USB_DIR}/lib")
copy_sdk_lib_to_build("libble6.a"           "${SDK_BLE_DIR}/lib")
copy_sdk_lib_to_build("libble6_lite.a"      "${SDK_BLE_DIR}/lib")
copy_sdk_lib_to_build("libble6_8act_6con.a" "${SDK_BLE_DIR}/lib")
copy_sdk_lib_to_build("libble6_mesh.a"      "${SDK_MESH_DIR}/lib")
copy_sdk_lib_to_build("libsig_model.a"      "${SDK_MESH_DIR}/lib")

# 11.3 定义库文件路径变量 (使用复制后的路径)
# 驱动库
set(DRVS_LIB           "${CMAKE_BINARY_DIR}/lib/libdrvs.a")

# USB 库
set(USBD_LIB           "${CMAKE_BINARY_DIR}/lib/libusbd.a")
set(USBD_LIB_LITE      "${CMAKE_BINARY_DIR}/lib/libusbd_lite.a")

# BLE 库
set(BLE_LIB            "${CMAKE_BINARY_DIR}/lib/libble6.a")
set(BLE_LIB_LITE       "${CMAKE_BINARY_DIR}/lib/libble6_lite.a")
set(BLE_LIB_LARGE      "${CMAKE_BINARY_DIR}/lib/libble6_8act_6con.a")

# Mesh 库
set(BLE_LIB_MESH       "${CMAKE_BINARY_DIR}/lib/libble6_mesh.a")
set(BLE_LIB_SIG_MODEL  "${CMAKE_BINARY_DIR}/lib/libsig_model.a")


# ##############################################################################
# 第 12 部分: 公共编译选项
# ##############################################################################
# 说明: 所有项目通用的编译选项
# ##############################################################################

set(COMMON_COMPILE_OPTIONS
    # --- 架构选项 ---
    -mcpu=cortex-m0plus      # Cortex-M0+ 处理器
    -mthumb                  # Thumb 指令集
    -mthumb-interwork        # ARM/Thumb 混合调用

    # --- 优化选项 ---
    -Os                      # 优化代码大小

    # --- 警告选项 ---
    -Wall                    # 常见警告
    -Wextra                  # 额外警告

    # --- 代码生成选项 ---
    -ffunction-sections      # 每个函数独立段
    -fdata-sections          # 每个数据独立段
    -fno-builtin             # 禁用内建函数
    -fno-strict-aliasing     # 禁用严格别名
    -fshort-enums            # 短枚举类型

    # --- 语言标准 ---
    -std=gnu11               # GNU C11 标准
)

# 12.1 Debug 模式额外选项
if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    list(APPEND COMMON_COMPILE_OPTIONS
        -g3                     # 最大调试信息
        -fno-omit-frame-pointer # 保留帧指针
    )
endif()


# ##############################################################################
# 第 13 部分: 公共链接选项
# ##############################################################################
# 说明: 所有项目通用的链接选项
# ##############################################################################

set(COMMON_LINK_OPTIONS
    ${TOOLCHAIN_LIB_DIR_FLAGS}
    -lnosys                  # 空系统调用库
    -specs=nano.specs        # newlib-nano 规格
    -nostartfiles            # 不使用标准启动文件
    -msoft-float             # 软件浮点
    -Wl,--gc-sections        # 删除未使用段
    -Wl,--print-memory-usage # 打印内存使用
    -flto                    # 启用链接时优化
)

# 13.1 GCC 12+ 兼容性选项
if(GCC_VERSION VERSION_GREATER_EQUAL 12.0)
    list(APPEND COMMON_LINK_OPTIONS "-Wl,--no-warn-rwx-segments")
endif()

# 13.2 Release 模式选项
if(CMAKE_BUILD_TYPE_LOWER STREQUAL "release")
    list(APPEND COMMON_LINK_OPTIONS -Wl,--fatal-warnings)
endif()


# ##############################################################################
# 第 14 部分: 公共链接函数
# ##############################################################################
# 说明: 统一设置目标链接选项，包括 map 文件生成
# ##############################################################################

# ------------------------------------------------------------------------------
# 函数: setup_target_link_options
# 说明: 为目标设置完整的链接选项
# 参数:
#   target        - 目标名称
#   linker_script - 链接脚本路径
# 用法:
#   setup_target_link_options(${PROJECT_NAME} ${LINK_SCRIPT})
# ------------------------------------------------------------------------------
function(setup_target_link_options target linker_script)
    target_link_options(${target} PRIVATE
        ${COMMON_LINK_OPTIONS}
        -T ${linker_script}
        -Wl,-Map=map/${target}.map
    )
endfunction()


# ##############################################################################
# 第 15 部分: 公共辅助函数
# ##############################################################################
# 说明: 供各项目调用的辅助函数
# ##############################################################################

# ------------------------------------------------------------------------------
# 函数: set_module_definitions
# 说明: 为源文件列表设置 __MODULE__ 宏定义 (用于调试追踪)
# 参数:
#   src_list - 源文件列表变量名
# 用法:
#   set_module_definitions(SRC_LIST)
# ------------------------------------------------------------------------------
function(set_module_definitions src_list)
    foreach(src_file IN LISTS ${src_list})
        if(src_file MATCHES "\\.c$")
            get_filename_component(module_name ${src_file} NAME)
            set_source_files_properties(${src_file}
                PROPERTIES COMPILE_DEFINITIONS "__MODULE__=\"${module_name}\""
            )
        endif()
    endforeach()
endfunction()

# ------------------------------------------------------------------------------
# 函数: generate_project_output
# 说明: 生成项目输出文件 (hex, bin, asm)
# 输出目录结构:
#   build/
#   ├── elf/  - ELF 可执行文件
#   ├── hex/  - Intel HEX 文件
#   ├── bin/  - 二进制文件
#   ├── asm/  - 反汇编文件
#   └── map/  - Map 映射文件
# 参数:
#   target - 目标名称
# 用法:
#   在 add_executable 之后调用:
#   generate_project_output(${PROJECT_NAME})
# ------------------------------------------------------------------------------
function(generate_project_output target)
    # 设置目标输出文件后缀为 .elf
    set_target_properties(${target} PROPERTIES SUFFIX ".elf")

    # 创建输出目录
    file(MAKE_DIRECTORY
        "${CMAKE_BINARY_DIR}/hex"
        "${CMAKE_BINARY_DIR}/bin"
        "${CMAKE_BINARY_DIR}/asm"
        "${CMAKE_BINARY_DIR}/map"
    )

    # 使用相对路径避免中文路径编码问题
    # WORKING_DIRECTORY 设置为 CMAKE_BINARY_DIR，使用相对路径引用 elf 文件
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex   elf/${target}.elf hex/${target}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary elf/${target}.elf bin/${target}.bin
        COMMAND ${CMAKE_OBJDUMP} -d -S     elf/${target}.elf > asm/${target}.asm
        COMMAND ${CMAKE_SIZE_UTIL} -G      elf/${target}.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )
endfunction()


# ##############################################################################
# 文件结束
# ##############################################################################
