cmake_minimum_required(VERSION 3.20.0)

# Set SDK_ROOT if not defined
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(sadcTest C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)

list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/debug.c
)

# Set __MODULE__ macro definitions
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
