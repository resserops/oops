# 构建负载程序
file(GLOB_RECURSE SRC "*.cpp")
add_executable(oops_profiling_load_cpu_stepped ${SRC})
set_target_properties(oops_profiling_load_cpu_stepped PROPERTIES OUTPUT_NAME load_cpu_stepped)
target_link_libraries(oops_profiling_load_cpu_stepped PRIVATE pthread argparse oops_profiling_s)
