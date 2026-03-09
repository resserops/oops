set(googletest_dir ${oops_3rd_dir}/googletest)
if(NOT EXISTS "${googletest_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${googletest_dir})
endif()

add_subdirectory(${googletest_dir})
