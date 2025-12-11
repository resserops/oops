if(ENABLE_TEST)
    add_executable(oops_test)
    set_target_properties(oops_test PROPERTIES OUTPUT_NAME test)
    target_link_libraries(oops_test PRIVATE gtest_main pthread)
endif()

add_subdirectory(common)
add_subdirectory(matrix)
add_subdirectory(profiling)
