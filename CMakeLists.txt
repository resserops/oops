cmake_minimum_required(VERSION 3.16.5)

project(oops VERSION 0.1.0)

# option
option(ENABLE_ASAN "Enable ASan for memory error detection" OFF)
option(ENABLE_TEST "Build test suites for each module" OFF)

set(oops_dir ${CMAKE_CURRENT_LIST_DIR})
set(oops_3rd_dir ${oops_dir}/third_party)
set(oops_3rd_cmake_dir ${oops_3rd_dir}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-std=c++17 -Werror)
set(EXECUTABLE_OUTPUT_PATH ${oops_dir}/build/bin)

if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    else()
        message(WARNING "ASan is not supported by this compiler")
    endif()
endif()

if(ENABLE_TEST)
    include(${oops_3rd_cmake_dir}/googletest.cmake)
endif()

add_subdirectory(module)
