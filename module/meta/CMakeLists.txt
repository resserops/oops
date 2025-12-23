# 构建接口库
add_library(oops_meta_i INTERFACE)
target_include_directories(oops_meta_i INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(ENABLE_TEST)
    # 构建测试对象库，纯静态测试仅构建对象库
    file(GLOB_RECURSE TEST_SRC "test/*.cpp")
    add_library(oops_meta_test_o OBJECT ${TEST_SRC})
    target_link_libraries(oops_meta_test_o PRIVATE oops_meta_i)
endif()
