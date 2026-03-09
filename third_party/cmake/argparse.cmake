set(argparse_dir ${oops_3rd_dir}/argparse)
if(NOT EXISTS "${argparse_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${argparse_dir})
endif()

add_subdirectory(${argparse_dir})
