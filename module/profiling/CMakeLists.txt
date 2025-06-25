# 构建接口库
add_library(oops_profiling_i INTERFACE)
target_include_directories(oops_profiling_i INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(oops_profiling_i INTERFACE oops_common_i)

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cc")
add_library(oops_profiling_o OBJECT ${SRC})
target_link_libraries(oops_profiling_o PRIVATE oops_common_o)
set_property(TARGET oops_profiling_o PROPERTY POSITION_INDEPENDENT_CODE ON)

# 构建静态库
add_library(oops_profiling_s STATIC)
target_link_libraries(oops_profiling_s PRIVATE oops_profiling_o)
target_link_libraries(oops_profiling_s PRIVATE oops_common_s)
set_target_properties(oops_profiling_s PROPERTIES OUTPUT_NAME oops_profiling)

# 构建动态库
add_library(oops_profiling_d SHARED)
target_link_libraries(oops_profiling_d PRIVATE oops_profiling_o)
target_link_libraries(oops_profiling_d PRIVATE PUBLIC oops_common_d)
set_target_properties(oops_profiling_d PROPERTIES OUTPUT_NAME oops_profiling)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cc")
    add_library(oops_profiling_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_profiling_test_o PRIVATE oops_profiling_i)

    # 构建测试程序
    add_executable(oops_profiling_test)
    set_target_properties(oops_profiling_test PROPERTIES OUTPUT_NAME test_profiling)
    target_link_libraries(oops_profiling_test PRIVATE oops_profiling_s oops_profiling_test_o gtest_main pthread)

    # 构建大测试程序
    target_link_libraries(oops_test PRIVATE oops_profiling_s oops_profiling_test_o)
endif()
