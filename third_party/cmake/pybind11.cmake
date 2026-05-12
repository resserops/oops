# pybind11-3.0.4 d03662f0
set(lib_dir ${oops_3rd_dir}/pybind11)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
