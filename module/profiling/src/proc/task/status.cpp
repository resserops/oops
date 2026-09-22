#include "oops/proc/task/status.h"

#include <cstddef>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/key_value_parser.h"

namespace oops {
// Uid特化
template <>
bool ParseField(std::string_view s, proc::task::status::Info::Uid &uid, std::string_view) {
    auto res{scn::scan<uid_t, uid_t, uid_t, uid_t>(s, "{} {} {} {}")};
    if (!res) {
        return false;
    }
    std::tie(uid.real, uid.effective, uid.saved_set, uid.filesystem) = res->values();
    return true;
}

template <>
std::string FormatField(const proc::task::status::Info::Uid &uid, std::string_view) {
    return fmt::format("{} {} {} {}", uid.real, uid.effective, uid.saved_set, uid.filesystem);
}

// Gid特化
template <>
bool ParseField(std::string_view s, proc::task::status::Info::Gid &gid, std::string_view) {
    auto res{scn::scan<gid_t, gid_t, gid_t, gid_t>(s, "{} {} {} {}")};
    if (!res) {
        return false;
    }
    std::tie(gid.real, gid.effective, gid.saved_set, gid.filesystem) = res->values();
    return true;
}

template <>
std::string FormatField(const proc::task::status::Info::Gid &gid, std::string_view) {
    return fmt::format("{} {} {} {}", gid.real, gid.effective, gid.saved_set, gid.filesystem);
}

// SigQ特化
template <>
bool ParseField(std::string_view s, proc::task::status::Info::SigQ &sig_q, std::string_view) {
    auto res{scn::scan<std::size_t, std::size_t>(s, "{}/{}")};
    if (!res) {
        return false;
    }
    std::tie(sig_q.count, sig_q.limit) = res->values();
    return true;
}

template <>
std::string FormatField(const proc::task::status::Info::SigQ &sig_q, std::string_view) {
    return fmt::format("{}/{}", sig_q.count, sig_q.limit);
}

namespace proc {
namespace task {
namespace status {
namespace {
using TL = meta::TypeList<
    std::string, std::size_t, std::bitset<64>, pid_t, mode_t, bool, KiBs, Info::Uid, Info::Gid, Info::SigQ,
    std::vector<gid_t>, std::vector<pid_t>, std::vector<std::string>, std::vector<bool>>;
KeyValueParser<Info, Field, TL> kvparser{
    {{Field::NAME, "Name", &Info::name},
     {Field::UMASK, "Umask", &Info::umask, "{:o}", "{:04o}"},
     {Field::STATE, "State", &Info::state},
     {Field::TGID, "Tgid", &Info::tgid},
     {Field::NGID, "Ngid", &Info::ngid},
     {Field::PID, "Pid", &Info::pid},
     {Field::PPID, "PPid", &Info::ppid},
     {Field::TRACER_PID, "TracerPid", &Info::tracer_pid},
     {Field::UID, "Uid", &Info::uid},
     {Field::GID, "Gid", &Info::gid},
     {Field::FD_SIZE, "FDSize", &Info::fd_size},
     {Field::GROUPS, "Groups", &Info::groups},
     {Field::NS_TGID, "NStgid", &Info::ns_tgid},
     {Field::NS_PID, "NSpid", &Info::ns_pid},
     {Field::NS_PGID, "NSpgid", &Info::ns_pgid},
     {Field::NS_SID, "NSsid", &Info::ns_sid},
     {Field::KTHREAD, "Kthread", &Info::kthread},
     {Field::VM_PEAK, "VmPeak", &Info::vm_peak, "{} kB"},
     {Field::VM_SIZE, "VmSize", &Info::vm_size, "{} kB"},
     {Field::VM_LCK, "VmLck", &Info::vm_lck, "{} kB"},
     {Field::VM_PIN, "VmPin", &Info::vm_pin, "{} kB"},
     {Field::VM_HWM, "VmHWM", &Info::vm_hwm, "{} kB"},
     {Field::VM_RSS, "VmRSS", &Info::vm_rss, "{} kB"},
     {Field::RSS_ANON, "RssAnon", &Info::rss_anon, "{} kB"},
     {Field::RSS_FILE, "RssFile", &Info::rss_file, "{} kB"},
     {Field::RSS_SHMEM, "RssShmem", &Info::rss_shmem, "{} kB"},
     {Field::VM_DATA, "VmData", &Info::vm_data, "{} kB"},
     {Field::VM_STK, "VmStk", &Info::vm_stk, "{} kB"},
     {Field::VM_EXE, "VmExe", &Info::vm_exe, "{} kB"},
     {Field::VM_LIB, "VmLib", &Info::vm_lib, "{} kB"},
     {Field::VM_PTE, "VmPTE", &Info::vm_pte, "{} kB"},
     {Field::VM_SWAP, "VmSwap", &Info::vm_swap, "{} kB"},
     {Field::HUGETLB_PAGES, "HugetlbPages", &Info::hugetlb_pages, "{} kB"},
     {Field::CORE_DUMPING, "CoreDumping", &Info::core_dumping},
     {Field::THP_ENABLED, "THP_enabled", &Info::thp_enabled},
     {Field::UNTAG_MASK, "untag_mask", &Info::untag_mask, "{:x}", "{:#x}"},
     {Field::THREADS, "Threads", &Info::threads},
     {Field::SIG_Q, "SigQ", &Info::sig_q},
     {Field::SIG_PND, "SigPnd", &Info::sig_pnd, "{:x}", "{:016x}"},
     {Field::SHD_PND, "ShdPnd", &Info::shd_pnd, "{:x}", "{:016x}"},
     {Field::SIG_BLK, "SigBlk", &Info::sig_blk, "{:x}", "{:016x}"},
     {Field::SIG_IGNORED, "SigIgn", &Info::sig_ign, "{:x}", "{:016x}"},
     {Field::SIG_CGT, "SigCgt", &Info::sig_cgt, "{:x}", "{:016x}"},
     {Field::CAP_INH, "CapInh", &Info::cap_inh, "{:x}", "{:016x}"},
     {Field::CAP_PRM, "CapPrm", &Info::cap_prm, "{:x}", "{:016x}"},
     {Field::CAP_EFF, "CapEff", &Info::cap_eff, "{:x}", "{:016x}"},
     {Field::CAP_BND, "CapBnd", &Info::cap_bnd, "{:x}", "{:016x}"},
     {Field::CAP_AMB, "CapAmb", &Info::cap_amb, "{:x}", "{:016x}"},
     {Field::NO_NEW_PRIVS, "NoNewPrivs", &Info::no_new_privs},
     {Field::SECCOMP, "Seccomp", &Info::seccomp},
     {Field::SECCOMP_FILTERS, "Seccomp_filters", &Info::seccomp_filters},
     {Field::SPECULATION_STORE_BYPASS, "Speculation_Store_Bypass", &Info::speculation_store_bypass},
     {Field::SPECULATION_INDIRECT_BRANCH, "SpeculationIndirectBranch", &Info::speculation_indirect_branch},
     {Field::CPUS_ALLOWED, "Cpus_allowed", &Info::cpus_allowed, "%*pb"},
     {Field::CPUS_ALLOWED_LIST, "Cpus_allowed_list", &Info::cpus_allowed_list},
     {Field::MEMS_ALLOWED, "Mems_allowed", &Info::mems_allowed, "%*pb"},
     {Field::MEMS_ALLOWED_LIST, "Mems_allowed_list", &Info::mems_allowed_list},
     {Field::VOLUNTARY_CTXT_SWITCHES, "voluntary_ctxt_switches", &Info::voluntary_ctxt_switches},
     {Field::NONVOLUNTARY_CTXT_SWITCHES, "nonvoluntary_ctxt_switches", &Info::nonvoluntary_ctxt_switches},
     {Field::X86_THREAD_FEATURES, "x86_Thread_features", &Info::x86_thread_features},
     {Field::X86_THREAD_FEATURES_LOCKED, "x86_Thread_features_locked", &Info::x86_thread_features_locked}}};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    info.parsed |= kvparser.Parse(is, info, field_mask);
    return info;
}

Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }
Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/status", pid));
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, pid_t tid) { return Get(pid, tid, ~FieldMask{}); }
Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/task/{}/status", pid, tid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace status
} // namespace task
} // namespace proc
} // namespace oops
