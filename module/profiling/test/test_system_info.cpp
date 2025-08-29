#include <thread>

#include "gtest/gtest.h"

#include "oops/system_info.h"

using namespace oops;

TEST(ProfilingSystemInfo, Lscpu) {
    using namespace lscpu;
    auto info{Get()};
    std::cout << info.architecture << std::endl;
    std::cout << info.cpus << std::endl;
    std::cout << info.sockets << std::endl;
    std::cout << info.threads_per_core << std::endl;
    std::cout << info.cores_per_socket << std::endl;
    std::cout << info.model_name << std::endl;
    std::cout << info.numa_nodes << std::endl;
    std::cout << info.cpu_mhz << std::endl;
}