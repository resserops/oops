# scnlib-4.0.1 (e937be1a)
set(lib_dir ${oops_3rd_dir}/scnlib)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

if(TARGET FastFloat::fast_float)
    set(SCN_USE_EXTERNAL_FAST_FLOAT ON)
endif()

add_subdirectory(${lib_dir})
if(TARGET scn)
    set_target_properties(scn PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_options(scn PRIVATE -w)  # 静默屏蔽-Werror报错
endif()
