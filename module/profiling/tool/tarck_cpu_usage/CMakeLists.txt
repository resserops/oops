# 构建工具程序
file(GLOB_RECURSE SRC "*.cpp")
add_executable(oops_profiling_track_cpu_usage ${SRC})
set_target_properties(oops_profiling_track_cpu_usage PROPERTIES OUTPUT_NAME track_cpu_usage)
target_link_libraries(oops_profiling_track_cpu_usage PRIVATE pthread argparse oops_profiling_s)
