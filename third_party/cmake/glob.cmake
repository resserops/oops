# glob-0.0.1 (46fb9fc9)
set(lib_dir ${oops_3rd_dir}/glob)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
if(TARGET Glob)
    target_compile_options(Glob PRIVATE -w)  # 静默屏蔽-Werror报错
endif()
