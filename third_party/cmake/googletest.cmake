if(NOT TARGET gtest)
    execute_process(COMMAND git submodule update --init --recursive ${oops_3rd_dir}/googletest)
    add_subdirectory(${oops_3rd_dir}/googletest)
endif()
