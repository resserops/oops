#include "oops/proc/cpuinfo.h"

#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <utility>

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace cpuinfo {
namespace {
KeyValueParser<
    Entry, Field, meta::TypeList<std::size_t, double, std::string, bool, KiBs<std::size_t>, std::vector<std::string>>>
    kvparser{
        {{Field::PROCESSOR, "processor", &Entry::processor},
         {Field::VENDOR_ID, "vendor_id", &Entry::vendor_id},
         {Field::CPU_FAMILY, "cpu family", &Entry::cpu_family},
         {Field::MODEL, "model", &Entry::model},
         {Field::MODEL_NAME, "model name", &Entry::model_name},
         {Field::STEPPING, "stepping", &Entry::stepping},
         {Field::MICROCODE, "microcode", &Entry::microcode, "", "{:x}", "{:#x}"},
         {Field::CPU_MHZ, "cpu MHz", &Entry::cpu_mhz},
         {Field::CACHE_SIZE, "cache size", &Entry::cache_size, "KB"},
         {Field::PHYSICAL_ID, "physical id", &Entry::physical_id},
         {Field::SIBLINGS, "siblings", &Entry::siblings},
         {Field::CORE_ID, "core id", &Entry::core_id},
         {Field::CPU_CORES, "cpu cores", &Entry::cpu_cores},
         {Field::APICID, "apicid", &Entry::apicid},
         {Field::INITIAL_APICID, "initial apicid", &Entry::initial_apicid},
         {Field::FPU, "fpu", &Entry::fpu, "", "yes/no", "yes/no"},
         {Field::FPU_EXCEPTION, "fpu_exception", &Entry::fpu_exception, "", "yes/no", "yes/no"},
         {Field::CPUID_LEVEL, "cpuid level", &Entry::cpuid_level},
         {Field::WP, "wp", &Entry::wp, "", "yes/no", "yes/no"},
         {Field::FLAGS, "flags", &Entry::flags},
         {Field::BUGS, "bugs", &Entry::bugs},
         {Field::BOGOMIPS, "bogomips", &Entry::bogomips},
         {Field::CLFLUSH_SIZE, "clflush size", &Entry::clflush_size},
         {Field::CACHE_ALIGNMENT, "cache_alignment", &Entry::cache_alignment},
         {Field::ADDRESS_SIZES, "address sizes", &Entry::address_sizes},
         {Field::POWER_MANAGEMENT, "power management", &Entry::power_management}},
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
        processor.parsed |= kvparser.Parse(is, processor, field_mask);
        info.table.push_back(std::move(processor));
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &processor : info.table) {
        kvparser.Format(os, processor, processor.parsed);
        os << '\n';
    }
    return os;
}
} // namespace cpuinfo
} // namespace proc
} // namespace oops
