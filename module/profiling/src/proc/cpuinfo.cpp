#include "oops/proc/cpuinfo.h"

#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <utility>

#include "oops/key_value_io.h"

namespace oops {
namespace proc {
namespace cpuinfo {
namespace {
KeyValueIO<Entry, Field> kvio{
    {{Field::PROCESSOR, "processor", CW<&Entry::processor>},
     {Field::VENDOR_ID, "vendor_id", CW<&Entry::vendor_id>},
     {Field::CPU_FAMILY, "cpu family", CW<&Entry::cpu_family>},
     {Field::MODEL, "model", CW<&Entry::model>},
     {Field::MODEL_NAME, "model name", CW<&Entry::model_name>},
     {Field::STEPPING, "stepping", CW<&Entry::stepping>},
     {Field::MICROCODE, "microcode", CW<&Entry::microcode>, "{:x}", "{:#x}"},
     {Field::CPU_MHZ, "cpu MHz", CW<&Entry::cpu_mhz>},
     {Field::CACHE_SIZE, "cache size", CW<&Entry::cache_size>, "{} KB"},
     {Field::PHYSICAL_ID, "physical id", CW<&Entry::physical_id>},
     {Field::SIBLINGS, "siblings", CW<&Entry::siblings>},
     {Field::CORE_ID, "core id", CW<&Entry::core_id>},
     {Field::CPU_CORES, "cpu cores", CW<&Entry::cpu_cores>},
     {Field::APICID, "apicid", CW<&Entry::apicid>},
     {Field::INITIAL_APICID, "initial apicid", CW<&Entry::initial_apicid>},
     {Field::FPU, "fpu", CW<&Entry::fpu>, "yes/no"},
     {Field::FPU_EXCEPTION, "fpu_exception", CW<&Entry::fpu_exception>, "yes/no"},
     {Field::CPUID_LEVEL, "cpuid level", CW<&Entry::cpuid_level>},
     {Field::WP, "wp", CW<&Entry::wp>, "yes/no"},
     {Field::FLAGS, "flags", CW<&Entry::flags>},
     {Field::BUGS, "bugs", CW<&Entry::bugs>},
     {Field::BOGOMIPS, "bogomips", CW<&Entry::bogomips>},
     {Field::CLFLUSH_SIZE, "clflush size", CW<&Entry::clflush_size>},
     {Field::CACHE_ALIGNMENT, "cache_alignment", CW<&Entry::cache_alignment>},
     {Field::ADDRESS_SIZES, "address sizes", CW<&Entry::address_sizes>},
     {Field::POWER_MANAGEMENT, "power management", CW<&Entry::power_management>}},
    ":",
    [](std::string_view s) { return s.empty(); }};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/cpuinfo");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    while (is.peek() != EOF) {
        Entry processor;
        processor.parsed |= kvio.Scan(is, processor, field_mask);
        info.table.push_back(std::move(processor));
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &processor : info.table) {
        kvio.Format(os, processor, processor.parsed);
        os << '\n';
    }
    return os;
}
} // namespace cpuinfo
} // namespace proc
} // namespace oops
