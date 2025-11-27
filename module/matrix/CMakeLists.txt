# 构建接口库
add_library(oops_matrix_i INTERFACE)
target_include_directories(oops_matrix_i INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(oops_matrix_i INTERFACE oops_common_i)

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cpp")
add_library(oops_matrix_o OBJECT ${SRC})
target_link_libraries(oops_matrix_o PRIVATE oops_matrix_i)
set_property(TARGET oops_matrix_o PROPERTY POSITION_INDEPENDENT_CODE ON)

# 构建静态库
add_library(oops_matrix_s STATIC)
target_link_libraries(oops_matrix_s PRIVATE oops_matrix_o)
target_link_libraries(oops_matrix_s PRIVATE oops_common_s)
set_target_properties(oops_matrix_s PROPERTIES OUTPUT_NAME oops_matrix)

# 构建动态库
add_library(oops_matrix_d SHARED)
target_link_libraries(oops_matrix_d PRIVATE oops_matrix_o)
target_link_libraries(oops_matrix_d PRIVATE PUBLIC oops_common_d)
set_target_properties(oops_matrix_d PROPERTIES OUTPUT_NAME oops_matrix)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_matrix_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_matrix_test_o PRIVATE oops_matrix_i gtest)

    # 构建测试程序
    add_executable(oops_matrix_test)
    set_target_properties(oops_matrix_test PROPERTIES OUTPUT_NAME test_matrix)
    target_link_libraries(oops_matrix_test PRIVATE oops_matrix_s oops_matrix_test_o gtest_main pthread)

    # 构建大测试程序
    target_link_libraries(oops_test PRIVATE oops_matrix_s oops_matrix_test_o)
endif()
