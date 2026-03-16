# fmt-12.1.0 (407c905e)
set(lib_dir ${oops_3rd_dir}/fmt)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
