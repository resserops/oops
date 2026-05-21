# glob-0.0.1 (46fb9fc9)
set(lib_dir ${oops_3rd_dir}/glob)
if(NOT EXISTS "${lib_dir}/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init --recursive ${lib_dir})
endif()

# 使用header only版本替代构建版本，避免glob cmake依赖的cpm环境
add_library(glob INTERFACE)
target_include_directories(glob INTERFACE ${lib_dir}/single_include)
target_compile_options(glob INTERFACE -w)
