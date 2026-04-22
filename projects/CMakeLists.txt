# 递归添加项目目录的函数
function(add_project_subdirectories parent_dir)
    file(GLOB SUBDIRS RELATIVE "${parent_dir}" "${parent_dir}/*")
    list(SORT SUBDIRS)

    foreach(subdir ${SUBDIRS})
        # 跳过隐藏目录和 CMake 文件
        if(subdir MATCHES "^\\.|^CMakeLists\\.txt$")
            continue()
        endif()

        set(subdir_path "${parent_dir}/${subdir}")
        set(gnu_cmake "${subdir_path}/gnu/CMakeLists.txt")

        if(IS_DIRECTORY "${subdir_path}")
            if(EXISTS "${gnu_cmake}")
                # 计算相对路径用于输出目录
                file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${subdir_path}")
                add_subdirectory("${rel_path}/gnu" "build/${rel_path}")
                # 使用全局属性计数
                get_property(count GLOBAL PROPERTY project_count)
                math(EXPR count "${count} + 1")
                set_property(GLOBAL PROPERTY project_count ${count})
            else()
                # 递归处理子目录
                add_project_subdirectories("${subdir_path}")
            endif()
        endif()
    endforeach()
endfunction()

# 自动发现并添加所有项目
set_property(GLOBAL PROPERTY project_count 0)
add_project_subdirectories("${CMAKE_CURRENT_SOURCE_DIR}")
get_property(total_added GLOBAL PROPERTY project_count)
message(STATUS "Added ${total_added} projects")
