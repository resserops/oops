#pragma once
#include <cstddef>
#include <iosfwd>

#include <sys/types.h>

namespace oops {
namespace proc {
namespace loadavg {
struct Info {
    double load1{};  // 近1分钟平均负载（处于R或D状态的任务平均数）
    double load5{};  // 近5分钟平均负载
    double load15{}; // 近15分钟平均负载

    struct SchedEntities {
        std::size_t running{}; // nr_running，处于R状态的调度实体（进程/线程）数
        std::size_t total{};   // nr_threads，调度实体总数
    } sched_entities{};
    pid_t last_pid{}; // 最近创建的进程PID

    bool parsed{}; // 整行是否解析成功，失败时字段可能部分填充、不可用
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(std::istream &is);

std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace loadavg
} // namespace proc
} // namespace oops
