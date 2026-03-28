cmake_minimum_required(VERSION 3.20.0)

# 如果 SDK_ROOT 未定义，使用相对路径设置
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(spiLCD C ASM)

set(LOCAL_SRC_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(STARTUP_LCD_SRC "${CMAKE_CURRENT_SOURCE_DIR}/startup_gnu_lcd.S")

set(SRC_LIST
    ${STARTUP_LCD_SRC}
    ${LOCAL_SRC_DIR}/main.c
    ${LOCAL_SRC_DIR}/lcd.c
    ${LOCAL_SRC_DIR}/eyes/eyes.c
    ${LOCAL_SRC_DIR}/slide/slide.c
    ${LOCAL_SRC_DIR}/robot/robot.c
    ${SDK_MODULES_SRC_DIR}/debug.c
)

# 设置 __MODULE__ 宏定义
set_module_definitions(SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${LOCAL_SRC_DIR}"
    "${LOCAL_SRC_DIR}/eyes"
    "${LOCAL_SRC_DIR}/slide"
    "${LOCAL_SRC_DIR}/robot"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DRVS_LIB}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -include "${LOCAL_SRC_DIR}/cfg.h"
    ${COMMON_COMPILE_OPTIONS}
    -Wno-missing-braces  # Disable warning for large 2D array initializers in eye data headers
)

# 使用专用链接脚本（更大的 Flash 空间用于图像数据）
set(LCD_LINK_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/link_xip_lcd.ld")
setup_target_link_options(${PROJECT_NAME} ${LCD_LINK_SCRIPT})

generate_project_output(${PROJECT_NAME})
