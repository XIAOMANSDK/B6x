# 自动发现并添加所有示例项目
file(GLOB EXAMPLE_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT EXAMPLE_SUBDIRS)

# 遍历所有示例目录
set(added_count 0)
foreach(example_dir ${EXAMPLE_SUBDIRS})
    # 跳过隐藏目录和 CMake 文件
    if(example_dir MATCHES "^\\.|^CMakeLists\\.txt$")
        continue()
    endif()

    set(example_path "${CMAKE_CURRENT_SOURCE_DIR}/${example_dir}")
    set(example_cmake "${example_path}/gnu/CMakeLists.txt")

    # 只处理包含 gnu/CMakeLists.txt 的目录
    if(IS_DIRECTORY "${example_path}" AND EXISTS "${example_cmake}")
        add_subdirectory("${example_dir}/gnu" "build/${example_dir}")
        math(EXPR added_count "${added_count} + 1")
    endif()
endforeach()

message(STATUS "Added ${added_count} examples")
