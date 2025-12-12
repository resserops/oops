# 构建接口库
add_library(oops_common_i INTERFACE)
target_include_directories(oops_common_i INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 构建对象库
file(GLOB_RECURSE SRC "src/*.cpp")
add_library(oops_common_o OBJECT ${SRC})
set_target_properties(oops_common_o PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(oops_common_o PRIVATE oops_common_i)

# 构建静态库
add_library(oops_common_s STATIC)
set_target_properties(oops_common_s PROPERTIES OUTPUT_NAME oops_common)
target_link_libraries(oops_common_s PRIVATE oops_common_o)
target_link_libraries(oops_common_s INTERFACE oops_common_i)

# 构建动态库
add_library(oops_common_d SHARED)
set_target_properties(oops_common_d PROPERTIES OUTPUT_NAME oops_common)
target_link_libraries(oops_common_d PRIVATE oops_common_o)
target_link_libraries(oops_common_d INTERFACE oops_common_i)

if(ENABLE_TEST)
    # 构建测试对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_common_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_common_test_o PRIVATE oops_common_i gtest)

    # 构建模块测试程序
    add_executable(oops_common_test)
    set_target_properties(oops_common_test PROPERTIES OUTPUT_NAME test_common)
    # 需要依赖oops_common_s而不是o，s包含依赖的其它s或o
    target_link_libraries(oops_common_test PRIVATE oops_common_test_o oops_common_s gtest_main pthread)

    # 构建全量测试程序
    target_link_libraries(oops_test PRIVATE oops_common_test_o oops_common_s)
endif()
