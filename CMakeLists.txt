cmake_minimum_required(VERSION 3.16.5)

project(oops VERSION 0.1.0)

# option
option(ENABLE_TEST "Build googletest framework and enable test suites for each module" OFF)

set(oops_dir ${CMAKE_CURRENT_LIST_DIR})
set(oops_3rd_dir ${oops_dir}/third_party)
set(oops_3rd_cmake_dir ${oops_3rd_dir}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror -O3")
set(EXECUTABLE_OUTPUT_PATH ${oops_dir}/build/bin)

if(ENABLE_TEST)
    include(${oops_3rd_cmake_dir}/googletest.cmake)
endif()

add_subdirectory(module)
