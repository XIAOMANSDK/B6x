cmake_minimum_required(VERSION 3.20.0)

# Set SDK_ROOT using relative path if not defined
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(bleHid_Uart C ASM)

set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
# Add source files
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/../protocol" SRC_LIST)

# Add additional source files as needed
list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/leds.c
    ${SDK_MODULES_SRC_DIR}/sftmr.c
    ${SDK_MODULES_SRC_DIR}/debug.c
    ${SDK_BLE_APP_DIR}/app_actv.c
    ${SDK_BLE_APP_DIR}/app_msg.c
    ${SDK_BLE_APP_DIR}/app_gapm.c
    ${SDK_BLE_APP_DIR}/app_gapc.c
    ${SDK_BLE_PRF_DIR}/prf_diss.c
    ${SDK_BLE_PRF_DIR}/prf_bass.c
    ${SDK_BLE_PRF_DIR}/prf_hids.c
)

# Set __MODULE__ macro definitions
set_module_definitions(SRC_LIST)

set(LINKER_SCRIPT_IN ${BLE_LITE_LINK_SCRIPT})
set(PROCESSED_LINKER_SCRIPT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.ld")

# Add linker script preprocessing command
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

# Header file include paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    "${LOCAL_SRC_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
    ${SDK_BLE_INCLUDE}
)

# Link static libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DRVS_LIB}
    ${BLE_LIB_LITE}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -include "${LOCAL_SRC_DIR}/cfg.h"
    ${COMMON_COMPILE_OPTIONS}
)

setup_target_link_options(${PROJECT_NAME} ${PROCESSED_LINKER_SCRIPT})

generate_project_output(${PROJECT_NAME})
