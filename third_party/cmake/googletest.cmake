# googletest-1.17.0 (52eb8108)
set(lib_dir ${oops_3rd_dir}/googletest)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

add_subdirectory(${lib_dir})
