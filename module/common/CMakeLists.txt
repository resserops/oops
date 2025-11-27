# 构建接口库
add_library(oops_common_i INTERFACE)
target_include_directories(oops_common_i INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cpp")
add_library(oops_common_o OBJECT ${SRC})
target_link_libraries(oops_common_o PRIVATE oops_common_i)
set_property(TARGET oops_common_o PROPERTY POSITION_INDEPENDENT_CODE ON)

# 构建静态库
add_library(oops_common_s STATIC)
target_link_libraries(oops_common_s PRIVATE oops_common_o)
set_target_properties(oops_common_s PROPERTIES OUTPUT_NAME oops_common)

# 构建动态库
add_library(oops_common_d SHARED)
target_link_libraries(oops_common_d PRIVATE oops_common_o)
set_target_properties(oops_common_d PROPERTIES OUTPUT_NAME oops_common)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_common_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_common_test_o PRIVATE oops_common_i gtest)

    # 构建测试程序
    add_executable(oops_common_test)
    set_target_properties(oops_common_test PROPERTIES OUTPUT_NAME test_common)
    target_link_libraries(oops_common_test PRIVATE oops_common_s oops_common_test_o gtest_main pthread)

    # 构建全量测试程序
    target_link_libraries(oops_test PRIVATE oops_common_s oops_common_test_o)
endif()
