#pragma once
#include <bitset>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include <sys/types.h>

#include "oops/enum_bitset.h"
#include "oops/storage.h"

namespace oops {
namespace proc {
namespace task {
namespace status {
enum class Field : uint8_t {
    NAME,
    UMASK,
    STATE,
    TGID,
    NGID,
    PID,
    PPID,
    TRACER_PID,
    UID,
    GID,
    FD_SIZE,
    GROUPS,
    NS_TGID,
    NS_PID,
    NS_PGID,
    NS_SID,
    KTHREAD,
    VM_PEAK,
    VM_SIZE,
    VM_LCK,
    VM_PIN,
    VM_HWM,
    VM_RSS,
    RSS_ANON,
    RSS_FILE,
    RSS_SHMEM,
    VM_DATA,
    VM_STK,
    VM_EXE,
    VM_LIB,
    VM_PTE,
    VM_SWAP,
    HUGETLB_PAGES,
    CORE_DUMPING,
    THP_ENABLED,
    UNTAG_MASK,
    THREADS,
    SIG_Q,
    SIG_PND,
    SHD_PND,
    SIG_BLK,
    SIG_IGNORED,
    SIG_CGT,
    CAP_INH,
    CAP_PRM,
    CAP_EFF,
    CAP_BND,
    CAP_AMB,
    NO_NEW_PRIVS,
    SECCOMP,
    SECCOMP_FILTERS,
    SPECULATION_STORE_BYPASS,
    SPECULATION_INDIRECT_BRANCH,
    CPUS_ALLOWED,
    CPUS_ALLOWED_LIST,
    MEMS_ALLOWED,
    MEMS_ALLOWED_LIST,
    VOLUNTARY_CTXT_SWITCHES,
    NONVOLUNTARY_CTXT_SWITCHES,
    X86_THREAD_FEATURES,
    X86_THREAD_FEATURES_LOCKED,
    COUNT
};
using FieldMask = EnumBitset<Field>;
using oops::operator|;

struct Info {
    // 基础属性
    std::string name{};  // 进程执行的命令，超过TASK_COMM_LEN的字符会被截断
    mode_t umask{};      // 4位8进制，文件权限掩码，(mode & ~umask) & 0777 = 进程创建文件的权限
    std::string state{}; // 进程状态

    pid_t tgid{};       // 线程组ID（进程ID）
    pid_t ngid{};       // NUMA组ID，0表示不属于任何NUMA组
    pid_t pid{};        // 线程ID
    pid_t ppid{};       // 父进程的PID
    pid_t tracer_pid{}; // 跟踪本进程的Tracer进程PID，0表示未被跟踪

    struct Uid {            // 用户ID
        uid_t real{};       // 真实UID：启动进程的用户
        uid_t effective{};  // 有效UID：内核权限检查实际使用
        uid_t saved_set{};  // 保存UID：有效UID的备份，用于降权后恢复
        uid_t filesystem{}; // 文件系统UID：VFS使用
    } uid{};

    struct Gid {            // 用户组ID
        gid_t real{};       // 真实GID：启动进程的用户组
        gid_t effective{};  // 有效GID：内核权限检查实际使用
        gid_t saved_set{};  // 保存GID：有效UID的备份，用于降权后恢复
        gid_t filesystem{}; // 文件系统GID：VFS使用
    } gid{};

    std::size_t fd_size{};       // 已分配的文件描述符表格容量
    std::vector<gid_t> groups{}; // 附加组GID列表，进程除了主组外，额外所属的其它用户组GID

    // 命名空间ID
    // 保存从外（访问者）到内（进程）每层PID命名空间中各类ID的值
    std::vector<pid_t> ns_tgid{}; // 线程组ID（进程ID）
    std::vector<pid_t> ns_pid{};  // 线程ID
    std::vector<pid_t> ns_pgid{}; // 进程组ID
    std::vector<pid_t> ns_sid{};  // 会话ID

    // 线程属性
    bool kthread{}; // 是否是内核线程

    // 内存统计
    KiBs vm_peak{};       // 虚拟内存峰值
    KiBs vm_size{};       // 虚拟内存
    KiBs vm_lck{};        // 锁定内存（不可交换到磁盘）
    KiBs vm_pin{};        // 固定内存（物理地址不可迁移）
    KiBs vm_hwm{};        // 物理内存峰值
    KiBs vm_rss{};        // 物理内存 = RssAnon + RssFile + RssShmem，不精确
    KiBs rss_anon{};      // 私有匿名映射内存，不精确
    KiBs rss_file{};      // 私有文件映射内存，不精确
    KiBs rss_shmem{};     // 共享映射（包括共享匿名和共享文件映射）内存
    KiBs vm_data{};       // 数据段，不精确
    KiBs vm_stk{};        // 堆栈段，不精确
    KiBs vm_exe{};        // 文本段，不精确
    KiBs vm_lib{};        // 共享库代码
    KiBs vm_pte{};        // 页表项
    KiBs vm_swap{};       // 换出的匿名私有映射
    KiBs hugetlb_pages{}; // 标准大页内存（不包括透明大页）

    // 杂项
    bool core_dumping{};         // 进程是否正在核心转储
    bool thp_enabled{};          // 是否开启透明大页
    std::uintptr_t untag_mask{}; // 16位16进制，地址标签掩码，用于抹除虚拟地址高位标签
    std::size_t threads{};       // 包含本线程的线程组（进程）中的线程总数

    // 信号
    struct SigQ {            // count/limit
        std::size_t count{}; // 真实UID下排队的信号数量
        std::size_t limit{}; // 真实UID下排队的最大信号数量
    } sig_q{};

    std::bitset<64> sig_pnd{}; // 线程级待处理信号掩码
    std::bitset<64> shd_pnd{}; // 进程级待处理信号掩码
    std::bitset<64> sig_blk{}; // 被阻塞信号掩码
    std::bitset<64> sig_ign{}; // 被忽略信号掩码
    std::bitset<64> sig_cgt{}; // 被捕获信号掩码

    // 能力
    std::bitset<64> cap_inh{}; // 可继承能力集
    std::bitset<64> cap_prm{}; // 许可能力集
    std::bitset<64> cap_eff{}; // 有效能力集
    std::bitset<64> cap_bnd{}; // 能力边界集
    std::bitset<64> cap_amb{}; // 环境能力集

    // 安全
    bool no_new_privs{};                       // 是否不允许提权
    std::size_t seccomp{};                     // 安全计算模式：0 = DISABLED，1 = STRICT，2 = FILTER
    std::size_t seccomp_filters{};             // 安全计算过滤器数量
    std::string speculation_store_bypass{};    // 进程对预测存储旁路漏洞安全防御状态
    std::string speculation_indirect_branch{}; // 进程对间接分支预测漏洞的安全防御状态

    // 亲和性
    std::vector<bool> cpus_allowed{}; // 动态位16进制，允许运行CPU掩码
    std::string cpus_allowed_list{};  // 同上，可读区间格式
    std::vector<bool> mems_allowed{}; // 动态位16进制，允许使用的内存节点掩码
    std::string mems_allowed_list{};  // 同上，可读区间格式

    // 上下文切换
    std::size_t voluntary_ctxt_switches{};    // 自愿上下文切换次数
    std::size_t nonvoluntary_ctxt_switches{}; // 非自愿上下文切换次数

    // x86硬件级安全特性
    std::vector<std::string> x86_thread_features{};        // 启用的硬件安全特性
    std::vector<std::string> x86_thread_features_locked{}; // 锁定（不允许修改）的硬件安全特性

    FieldMask parsed;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(std::istream &is, const FieldMask &field_mask);
[[nodiscard]] Info Get(pid_t pid);                                         // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, const FieldMask &field_mask);            // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid);                              // /proc/{pid}/task/{tid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask); // /proc/{pid}/task/{tid}

std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace status
} // namespace task
} // namespace proc
} // namespace oops
