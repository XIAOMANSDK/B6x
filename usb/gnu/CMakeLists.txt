cmake_minimum_required(VERSION 3.20.0)

# 同时生成多个库
option(ENABLE_MULTI_LIB "Enable Multiple library libudbd.a libudbd_lite.a" ON)

# libudbd_lite.a
set(SUFFIX_LITE _lite)

include("${CMAKE_SOURCE_DIR}/../../sdk.cmake")

project(usbd C)

# 指定源文件
set(SRC_LIST
    "${SDK_USB_DIR}/usbd/usbd_api.c"
    "${SDK_USB_DIR}/usbd/usbd_old.c"
    "${SDK_USB_DIR}/usbd/usbd_std.c"
    "${SDK_USB_DIR}/usbd/usbd_isr.c"
)

message(STATUS "Project Name     : ${PROJECT_NAME}")
message(STATUS "Project BINARY   : ${PROJECT_BINARY_DIR}")
message(STATUS "Project Path     : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Cmake Source Path: ${CMAKE_SOURCE_DIR}")
message(STATUS "ENABLE_MULTI_LIB : ${ENABLE_MULTI_LIB}")

# 设置源文件的 __MODULE__ 宏
# 设置 __MODULE__ 宏定义
set_module_definitions(SRC_LIST)

# 去除lib中时间戳信息
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcD <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> qD <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -D <TARGET>")

# 生成静态库
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

# 头文件搜索路径定义宏
macro(add_common_includes target)
  target_include_directories(${target} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${SDK_USB_DIR}/api"
    "${SDK_USB_DIR}/usbd"
  )
endmacro()
add_common_includes(${PROJECT_NAME})
# usbDFU
# O0 7108
# O1 4896
# O2 4752
# Os 4574
# O3 5124
# Oz 4574
set(ADDITIONAL_COMPILE_OPTIONS
    -O2
    -save-temps # 保存中间文件 .i .s .o
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMMON_COMPILE_OPTIONS}
    ${ADDITIONAL_COMPILE_OPTIONS}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_USB_DIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        "${SDK_USB_DIR}/lib/"
)

if(ENABLE_MULTI_LIB)
# 生成 libudbd_lite.a (CFG_LITELIB)
add_library(${PROJECT_NAME}${SUFFIX_LITE} STATIC ${SRC_LIST})
add_common_includes(${PROJECT_NAME}${SUFFIX_LITE})
target_compile_options(${PROJECT_NAME}${SUFFIX_LITE} PRIVATE
    -DUSBD_LITELIB
    ${COMMON_COMPILE_OPTIONS}
    ${ADDITIONAL_COMPILE_OPTIONS}
)

add_custom_command(TARGET ${PROJECT_NAME}${SUFFIX_LITE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_USB_DIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}${SUFFIX_LITE}>" "${SDK_USB_DIR}/lib/"
)
endif()