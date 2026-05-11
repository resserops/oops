# 构建工具程序
file(GLOB_RECURSE SRC "*.cpp")
add_executable(oops_profiling_monitor ${SRC})
set_target_properties(oops_profiling_monitor PROPERTIES OUTPUT_NAME monitor)
target_link_libraries(oops_profiling_monitor PRIVATE pthread argparse Glob oops_profiling_s)
