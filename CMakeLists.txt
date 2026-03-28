cmake_minimum_required(VERSION 3.20.0)

# 1. 定义 SDK 根目录
set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "SDK root path")

# 2. 核心构建选项
option(BUILD_EXAMPLES "编译examples" ON)
option(BUILD_PROJECTS "编译projects" ON)

set(TARGET_NAME "" CACHE STRING "指定要编译的单个目标名称（为空则编译所有配置）")
#set(TARGET_NAME "bleUart" CACHE STRING "指定要编译的单个目标名称（为空则编译所有配置）")

# 3. 包含工具链及 SDK 基础配置
include("${SDK_ROOT}/sdk.cmake")

# 4. 核心工程定义
project(B6x_SDK C ASM)

# 5. 输出构建分析日志
message(STATUS "----------------------------------------")
message(STATUS "Build Configuration:")
message(STATUS "  SDK Root       : ${SDK_ROOT}")
message(STATUS "  Build Type     : ${CMAKE_BUILD_TYPE}")

if(TARGET_NAME)
    message(STATUS "  Build Mode     : Single Target")
    message(STATUS "  Target Name    : ${TARGET_NAME}")
else()
    message(STATUS "  Build Mode     : Batch Build")
    message(STATUS "  Build Examples : ${BUILD_EXAMPLES}")
    message(STATUS "  Build Projects : ${BUILD_PROJECTS}")
endif()
message(STATUS "----------------------------------------")

# 6. 工程路由判断
if(TARGET_NAME)
    # 单独编译指定目标
    if(EXISTS "${SDK_ROOT}/examples/${TARGET_NAME}")
        add_subdirectory("examples/${TARGET_NAME}/gnu")
    elseif(EXISTS "${SDK_ROOT}/projects/${TARGET_NAME}")
        add_subdirectory("projects/${TARGET_NAME}/gnu")
    else()
        message(FATAL_ERROR "Target '${TARGET_NAME}' not found in examples/ or projects/ directory.")
    endif()
else()
    # 批量编译目录
    if(BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
    if(BUILD_PROJECTS)
        add_subdirectory(projects)
    endif()
endif()
