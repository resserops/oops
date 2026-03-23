# scnlib-4.0.1 (e937be1a)
set(lib_dir ${oops_3rd_dir}/scnlib)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
if(TARGET scn)
    target_compile_options(scn PRIVATE -w)  # 静默屏蔽-Werror报错
endif()
