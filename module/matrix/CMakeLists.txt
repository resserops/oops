# 构建接口库
add_library(oops_matrix_i INTERFACE)
target_include_directories(oops_matrix_i INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(oops_matrix_i INTERFACE oops_common_i oops_meta_i)

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cpp")
add_library(oops_matrix_o OBJECT ${SRC})
set_target_properties(oops_matrix_o PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(oops_matrix_o PRIVATE oops_matrix_i)

# 构建静态库
add_library(oops_matrix_s STATIC)
set_target_properties(oops_matrix_s PROPERTIES OUTPUT_NAME oops_matrix)
target_link_libraries(oops_matrix_s PRIVATE oops_matrix_o oops_common_s)
target_link_libraries(oops_matrix_s INTERFACE oops_matrix_i)

# 构建动态库
add_library(oops_matrix_d SHARED)
set_target_properties(oops_matrix_d PROPERTIES OUTPUT_NAME oops_matrix)
target_link_libraries(oops_matrix_d PRIVATE oops_matrix_o oops_common_d)
target_link_libraries(oops_matrix_d INTERFACE oops_matrix_i)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_matrix_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_matrix_test_o PRIVATE oops_matrix_i gtest)

    # 用例路径定义
    set(CASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/case")
    target_compile_definitions(oops_matrix_test_o PRIVATE "CASE_DIR=\"${CASE_DIR}\"")

    # 构建模块测试程序
    add_executable(oops_matrix_test)
    set_target_properties(oops_matrix_test PROPERTIES OUTPUT_NAME test_matrix)
    target_link_libraries(oops_matrix_test PRIVATE oops_matrix_test_o oops_matrix_s gtest_main pthread)

    # 构建全量测试程序
    target_link_libraries(oops_test PRIVATE oops_matrix_test_o oops_matrix_s)
endif()
