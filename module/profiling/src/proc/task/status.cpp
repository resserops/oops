#include "oops/proc/task/status.h"

#include <fstream>
#include <ostream>

#include "fmt/format.h"

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace task {
namespace status {
namespace {
KeyValueParser<Info, Field, meta::TypeList<std::size_t>> kvparser{
    {{Field::VM_PEAK, "VmPeak", &Info::vm_peak},
     {Field::VM_SIZE, "VmSize", &Info::vm_size},
     {Field::VM_LCK, "VmLck", &Info::vm_lck},
     {Field::VM_PIN, "VmPin", &Info::vm_pin},
     {Field::VM_HWM, "VmHWM", &Info::vm_hwm},
     {Field::VM_RSS, "VmRSS", &Info::vm_rss},
     {Field::RSS_ANON, "RssAnon", &Info::rss_anon},
     {Field::RSS_FILE, "RssFile", &Info::rss_file},
     {Field::RSS_SHMEM, "RssShmem", &Info::rss_shmem},
     {Field::VM_DATA, "VmData", &Info::vm_data},
     {Field::VM_STK, "VmStk", &Info::vm_stk},
     {Field::VM_EXE, "VmExe", &Info::vm_exe},
     {Field::VM_LIB, "VmLib", &Info::vm_lib},
     {Field::VM_PTE, "VmPTE", &Info::vm_pte},
     {Field::VM_SWAP, "VmSwap", &Info::vm_swap}}};
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
