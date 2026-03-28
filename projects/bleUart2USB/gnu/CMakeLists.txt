cmake_minimum_required(VERSION 3.20.0)

# 如果 SDK_ROOT 未定义，使用相对路径设置
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(bleUart2USB C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
# 添加 .c 文件
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)
aux_source_directory("${SDK_BLE_APP_DIR}" SRC_LIST)

# 按需添加其它源文件
list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/leds.c
    ${SDK_MODULES_SRC_DIR}/sftmr.c
    ${SDK_MODULES_SRC_DIR}/uart1Rb.c
    ${SDK_MODULES_SRC_DIR}/debug.c
    ${SDK_BLE_PRF_DIR}/prf_diss.c
    ${SDK_BLE_PRF_DIR}/prf_sess.c
    ${SDK_USB_CLASS_DIR}/cdc/usbd_cdc.c
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

# 头文件搜索路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${LOCAL_SRC_DIR}"
    ${SDK_BLE_INCLUDE}
    "${SDK_USB_DIR}/api"
    "${SDK_USB_CLASS_DIR}/cdc"
)

# 链接静态库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DRVS_LIB}
    ${BLE_LIB}
    ${USBD_LIB}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -include "${LOCAL_SRC_DIR}/cfg.h"
    ${COMMON_COMPILE_OPTIONS}
)

setup_target_link_options(${PROJECT_NAME} ${PROCESSED_LINKER_SCRIPT})

generate_project_output(${PROJECT_NAME})
