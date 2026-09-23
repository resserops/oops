#include "oops/proc/task/status.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/key_value_io.h"
#include "oops/str.h"

namespace oops {
namespace proc {
namespace task {
namespace status {
namespace {
bool ScanUid(std::string_view s, Info::Uid &uid) {
    auto res{scn::scan<uid_t, uid_t, uid_t, uid_t>(s, "{} {} {} {}")};
    if (!res) {
        return false;
    }
    std::tie(uid.real, uid.effective, uid.saved_set, uid.filesystem) = res->values();
    return true;
}

std::string FormatUid(const Info::Uid &uid) {
    return fmt::format("{} {} {} {}", uid.real, uid.effective, uid.saved_set, uid.filesystem);
}

bool ScanGid(std::string_view s, Info::Gid &gid) {
    auto res{scn::scan<gid_t, gid_t, gid_t, gid_t>(s, "{} {} {} {}")};
    if (!res) {
        return false;
    }
    std::tie(gid.real, gid.effective, gid.saved_set, gid.filesystem) = res->values();
    return true;
}

std::string FormatGid(const Info::Gid &gid) {
    return fmt::format("{} {} {} {}", gid.real, gid.effective, gid.saved_set, gid.filesystem);
}

bool ScanSigQ(std::string_view s, Info::SigQ &sig_q) {
    auto res{scn::scan<std::size_t, std::size_t>(s, "{}/{}")};
    if (!res) {
        return false;
    }
    std::tie(sig_q.count, sig_q.limit) = res->values();
    return true;
}

std::string FormatSigQ(const Info::SigQ &sig_q) { return fmt::format("{}/{}", sig_q.count, sig_q.limit); }

bool ScanMask(std::string_view s, std::bitset<64> &bs) {
    std::uint64_t v{};
    if (!ScanField(s, v, "{:x}")) {
        return false;
    }
    bs = std::bitset<64>{v};
    return true;
}

std::string FormatMask(const std::bitset<64> &bs) { return fmt::format("{:016x}", bs.to_ullong()); }

// 动态位掩码：Linux内核%pb格式，逗号分隔16进制组、高位组在前
bool ScanBitmap(std::string_view s, std::vector<bool> &v) {
    std::vector<std::uint32_t> chunks;
    std::size_t digits{0};
    for (auto token : Split(s, ',')) {
        token = Strip(token);
        std::uint32_t chunk{};
        if (token.empty() || (!chunks.empty() && token.size() != 8) || !ScanField(token, chunk, "{:x}")) {
            return false;
        }
        chunks.push_back(chunk);
        digits += token.size();
    }

    v.assign(digits * 4, false);
    for (std::size_t c{0}; c < chunks.size(); ++c) {
        for (std::size_t b{0}; b < 32; ++b) {
            if (chunks[c] & (1u << b)) {
                v[(chunks.size() - 1 - c) * 32 + b] = true; // 首个chunk为最高组
            }
        }
    }
    return true;
}

std::string FormatBitmap(const std::vector<bool> &v) {
    const std::size_t digits{(v.size() + 3) / 4};
    const std::size_t chunks{(digits + 7) / 8};
    std::string s;
    s.reserve(digits + chunks - 1);
    for (std::size_t c{chunks}; c-- > 0;) {
        std::uint32_t chunk{};
        for (std::size_t b{0}; b < 32 && c * 32 + b < v.size(); ++b) {
            if (v[c * 32 + b]) {
                chunk |= 1u << b;
            }
        }
        s += fmt::format("{:0{}x}", chunk, c == chunks - 1 ? digits - 8 * (chunks - 1) : 8);
        if (c > 0) {
            s += ',';
        }
    }
    return s;
}

KeyValueIO<Info, Field> kvio{
    {{Field::NAME, "Name", CW<&Info::name>},
     {Field::UMASK, "Umask", CW<&Info::umask>, "{:o}", "{:04o}"},
     {Field::STATE, "State", CW<&Info::state>},
     {Field::TGID, "Tgid", CW<&Info::tgid>},
     {Field::NGID, "Ngid", CW<&Info::ngid>},
     {Field::PID, "Pid", CW<&Info::pid>},
     {Field::PPID, "PPid", CW<&Info::ppid>},
     {Field::TRACER_PID, "TracerPid", CW<&Info::tracer_pid>},
     {Field::UID, "Uid", CW<&Info::uid>, CW<ScanUid>, CW<FormatUid>},
     {Field::GID, "Gid", CW<&Info::gid>, CW<ScanGid>, CW<FormatGid>},
     {Field::FD_SIZE, "FDSize", CW<&Info::fd_size>},
     {Field::GROUPS, "Groups", CW<&Info::groups>},
     {Field::NS_TGID, "NStgid", CW<&Info::ns_tgid>},
     {Field::NS_PID, "NSpid", CW<&Info::ns_pid>},
     {Field::NS_PGID, "NSpgid", CW<&Info::ns_pgid>},
     {Field::NS_SID, "NSsid", CW<&Info::ns_sid>},
     {Field::KTHREAD, "Kthread", CW<&Info::kthread>},
     {Field::VM_PEAK, "VmPeak", CW<&Info::vm_peak>, "{} kB"},
     {Field::VM_SIZE, "VmSize", CW<&Info::vm_size>, "{} kB"},
     {Field::VM_LCK, "VmLck", CW<&Info::vm_lck>, "{} kB"},
     {Field::VM_PIN, "VmPin", CW<&Info::vm_pin>, "{} kB"},
     {Field::VM_HWM, "VmHWM", CW<&Info::vm_hwm>, "{} kB"},
     {Field::VM_RSS, "VmRSS", CW<&Info::vm_rss>, "{} kB"},
     {Field::RSS_ANON, "RssAnon", CW<&Info::rss_anon>, "{} kB"},
     {Field::RSS_FILE, "RssFile", CW<&Info::rss_file>, "{} kB"},
     {Field::RSS_SHMEM, "RssShmem", CW<&Info::rss_shmem>, "{} kB"},
     {Field::VM_DATA, "VmData", CW<&Info::vm_data>, "{} kB"},
     {Field::VM_STK, "VmStk", CW<&Info::vm_stk>, "{} kB"},
     {Field::VM_EXE, "VmExe", CW<&Info::vm_exe>, "{} kB"},
     {Field::VM_LIB, "VmLib", CW<&Info::vm_lib>, "{} kB"},
     {Field::VM_PTE, "VmPTE", CW<&Info::vm_pte>, "{} kB"},
     {Field::VM_SWAP, "VmSwap", CW<&Info::vm_swap>, "{} kB"},
     {Field::HUGETLB_PAGES, "HugetlbPages", CW<&Info::hugetlb_pages>, "{} kB"},
     {Field::CORE_DUMPING, "CoreDumping", CW<&Info::core_dumping>},
     {Field::THP_ENABLED, "THP_enabled", CW<&Info::thp_enabled>},
     {Field::UNTAG_MASK, "untag_mask", CW<&Info::untag_mask>, "{:x}", "{:#x}"},
     {Field::THREADS, "Threads", CW<&Info::threads>},
     {Field::SIG_Q, "SigQ", CW<&Info::sig_q>, CW<ScanSigQ>, CW<FormatSigQ>},
     {Field::SIG_PND, "SigPnd", CW<&Info::sig_pnd>, CW<ScanMask>, CW<FormatMask>},
     {Field::SHD_PND, "ShdPnd", CW<&Info::shd_pnd>, CW<ScanMask>, CW<FormatMask>},
     {Field::SIG_BLK, "SigBlk", CW<&Info::sig_blk>, CW<ScanMask>, CW<FormatMask>},
     {Field::SIG_IGNORED, "SigIgn", CW<&Info::sig_ign>, CW<ScanMask>, CW<FormatMask>},
     {Field::SIG_CGT, "SigCgt", CW<&Info::sig_cgt>, CW<ScanMask>, CW<FormatMask>},
     {Field::CAP_INH, "CapInh", CW<&Info::cap_inh>, CW<ScanMask>, CW<FormatMask>},
     {Field::CAP_PRM, "CapPrm", CW<&Info::cap_prm>, CW<ScanMask>, CW<FormatMask>},
     {Field::CAP_EFF, "CapEff", CW<&Info::cap_eff>, CW<ScanMask>, CW<FormatMask>},
     {Field::CAP_BND, "CapBnd", CW<&Info::cap_bnd>, CW<ScanMask>, CW<FormatMask>},
     {Field::CAP_AMB, "CapAmb", CW<&Info::cap_amb>, CW<ScanMask>, CW<FormatMask>},
     {Field::NO_NEW_PRIVS, "NoNewPrivs", CW<&Info::no_new_privs>},
     {Field::SECCOMP, "Seccomp", CW<&Info::seccomp>},
     {Field::SECCOMP_FILTERS, "Seccomp_filters", CW<&Info::seccomp_filters>},
     {Field::SPECULATION_STORE_BYPASS, "Speculation_Store_Bypass", CW<&Info::speculation_store_bypass>},
     {Field::SPECULATION_INDIRECT_BRANCH, "SpeculationIndirectBranch", CW<&Info::speculation_indirect_branch>},
     {Field::CPUS_ALLOWED, "Cpus_allowed", CW<&Info::cpus_allowed>, CW<ScanBitmap>, CW<FormatBitmap>},
     {Field::CPUS_ALLOWED_LIST, "Cpus_allowed_list", CW<&Info::cpus_allowed_list>},
     {Field::MEMS_ALLOWED, "Mems_allowed", CW<&Info::mems_allowed>, CW<ScanBitmap>, CW<FormatBitmap>},
     {Field::MEMS_ALLOWED_LIST, "Mems_allowed_list", CW<&Info::mems_allowed_list>},
     {Field::VOLUNTARY_CTXT_SWITCHES, "voluntary_ctxt_switches", CW<&Info::voluntary_ctxt_switches>},
     {Field::NONVOLUNTARY_CTXT_SWITCHES, "nonvoluntary_ctxt_switches", CW<&Info::nonvoluntary_ctxt_switches>},
     {Field::X86_THREAD_FEATURES, "x86_Thread_features", CW<&Info::x86_thread_features>},
     {Field::X86_THREAD_FEATURES_LOCKED, "x86_Thread_features_locked", CW<&Info::x86_thread_features_locked>}}};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    info.parsed |= kvio.Scan(is, info, field_mask);
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
    kvio.Format(os, info, info.parsed);
    return os;
}
} // namespace status
} // namespace task
} // namespace proc
} // namespace oops
