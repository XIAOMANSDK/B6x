# ##############################################################################
# CMakeLists.txt - BLE Mesh Sig Model Library
# ##############################################################################
# @file    CMakeLists.txt
# @brief   Build BLE Mesh Sig Model static library (libsig_model.a)
# ##############################################################################

cmake_minimum_required(VERSION 3.20.0)

if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()
include(${SDK_ROOT}/sdk.cmake)

project(sig_model C)

set(MESH_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

# ##############################################################################
# 源文件 (自动搜索 model 目录下所有 .c 文件)
# ##############################################################################

file(GLOB_RECURSE SIG_MODEL_SOURCES "${MESH_DIR}/model/*.c")

# 输出 Sig Model 源文件列表
#list(LENGTH SIG_MODEL_SOURCES src_count)
#message(STATUS "Sig Model Sources (${src_count} files):")
#set(idx 1)
#foreach(file ${SIG_MODEL_SOURCES})
#    message(STATUS "  ${idx}. ${file}")
#    math(EXPR idx "${idx} + 1")
#endforeach()

# ##############################################################################
# 头文件目录
# ##############################################################################

set(SIG_MODEL_INCLUDE_DIRS
    ${COMMON_INCLUDE_DIRS}
    ${SDK_BLE_DIR}/api
    ${MESH_DIR}/api
    ${MESH_DIR}/model/api
)

# ##############################################################################
# 库文件配置 (去除时间戳)
# ##############################################################################

set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> qcD <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> qD  <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -D <TARGET>")

# ##############################################################################
# 创建静态库
# ##############################################################################

add_library(${PROJECT_NAME} STATIC ${SIG_MODEL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${SIG_MODEL_INCLUDE_DIRS})

target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMMON_COMPILE_OPTIONS}
)

set_module_definitions(SIG_MODEL_SOURCES)

# ##############################################################################
# 输出库文件
# ##############################################################################

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${MESH_DIR}/lib/
    COMMENT "Copying libsig_model.a to ${MESH_DIR}/lib/"
)
