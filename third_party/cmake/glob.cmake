# glob-0.0.1 (46fb9fc9)
set(lib_dir ${oops_3rd_dir}/glob)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
