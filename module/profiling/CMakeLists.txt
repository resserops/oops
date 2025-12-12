# 构建接口库
add_library(oops_profiling_i INTERFACE)
target_include_directories(oops_profiling_i INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(oops_profiling_i INTERFACE oops_common_i)

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cpp")
add_library(oops_profiling_o OBJECT ${SRC})
set_target_properties(oops_profiling_o PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(oops_profiling_o PRIVATE oops_profiling_i)

# 构建静态库
add_library(oops_profiling_s STATIC)
set_target_properties(oops_profiling_s PROPERTIES OUTPUT_NAME oops_profiling)
target_link_libraries(oops_profiling_s PRIVATE oops_profiling_o oops_common_s)
target_link_libraries(oops_profiling_s INTERFACE oops_profiling_i)

# 构建动态库
add_library(oops_profiling_d SHARED)
set_target_properties(oops_profiling_d PROPERTIES OUTPUT_NAME oops_profiling)
target_link_libraries(oops_profiling_d PRIVATE oops_profiling_o oops_common_d)
target_link_libraries(oops_profiling_d INTERFACE oops_profiling_i)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_profiling_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_profiling_test_o PRIVATE oops_profiling_i gtest)

    # 构建模块测试程序
    add_executable(oops_profiling_test)
    set_target_properties(oops_profiling_test PROPERTIES OUTPUT_NAME test_profiling)
    target_link_libraries(oops_profiling_test PRIVATE oops_profiling_test_o oops_profiling_s gtest_main pthread)

    # 构建全量测试程序
    target_link_libraries(oops_test PRIVATE oops_profiling_test_o oops_profiling_s)
endif()
