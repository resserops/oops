# fast_float-6.1.6 (00c8c7b0)
set(lib_dir ${oops_3rd_dir}/fast_float)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
