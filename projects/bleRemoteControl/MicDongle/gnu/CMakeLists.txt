cmake_minimum_required(VERSION 3.20.0)

option(ENABLE_AI_MOUSE "Enable AI Mouse" OFF)

# Set SDK_ROOT if not defined
if(NOT SDK_ROOT)
    set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
endif()

include("${SDK_ROOT}/sdk.cmake")
project(MicDongle C ASM)



set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
aux_source_directory("${LOCAL_SRC_DIR}" SRC_LIST)

# Add additional source files
list(APPEND SRC_LIST
    ${STARTUP_SRC}
    ${SDK_MODULES_SRC_DIR}/adpcm.c
    ${SDK_MODULES_SRC_DIR}/debug.c
    ${SDK_BLE_APP_DIR}/app_gapm.c
    ${SDK_USB_CLASS_DIR}/hid/usbd_hid.c
    ${SDK_USB_CLASS_DIR}/audio/usbd_audio.c
)

# 移除指定文件
foreach(src ${SRC_LIST})
  if(ENABLE_AI_MOUSE)
      if(src MATCHES "audio_hid.c")
          list(REMOVE_ITEM SRC_LIST "${src}")
      endif()
  else()
      if(src MATCHES "audio_hid_AiMouse.c")
          list(REMOVE_ITEM SRC_LIST "${src}")
      endif()
  endif()
endforeach()

# Set __MODULE__ macro definitions
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
    "${SDK_USB_DIR}/api"
    "${SDK_USB_CLASS_DIR}/hid"
    "${SDK_USB_CLASS_DIR}/audio"
)

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
