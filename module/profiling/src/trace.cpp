#include "oops/trace.h"

#include "oops/system_info.h"

namespace oops {
// TraceConfig
TraceConfig &TraceConfig::GetImpl() {
    static TraceConfig instance;
    return instance;
}

// Metrics
std::string Location::GetLabelStr() const {
    if (anonymous_id < 0) {
        return "other";
    }
    if (label == nullptr) {
        return std::string{"trace_"} + std::to_string(anonymous_id);
    }
    return std::string{label} + " (" + std::to_string(anonymous_id) + ")";
}

std::string Location::GetLocationStr() const {
    if (file == nullptr) {
        return {};
    }
    return std::string{file} + ":" + std::to_string(line);
}

TimePoint TimePoint::Get() {
    TimePoint trace_point;
    trace_point.time_point = std::chrono::high_resolution_clock::now();
    return trace_point;
}

TimeInterval &TimeInterval::operator+=(const TimeInterval &rhs) {
    time += rhs.time;
    return *this;
}

TimeInterval TimeInterval::operator-(const TimeInterval &rhs) const {
    TimeInterval itv;
    itv.time = time - rhs.time;
    return itv;
}

TimeInterval operator-(const TimePoint &lhs, const TimePoint &rhs) { return {lhs.time_point - rhs.time_point}; }

Memory Memory::Get() {
    using namespace proc;
    Memory memory;
    auto status_info{status::Get(status::Field::VM_RSS | status::Field::VM_HWM | status::Field::VM_SWAP)};
    memory.rss = *status_info.vm_rss;
    memory.hwm = *status_info.vm_hwm;
    memory.swap = *status_info.vm_swap;
    return memory;
}

std::ostream &operator<<(std::ostream &out, const Sample &sample) {
    out << sample.GetLabelStr() << " " << sample.GetLocationStr() << std::endl;
    out << "Time: " << sample.GetTime() << " s" << std::endl;
    out << "RSS: " << sample.GetRssGiB() << " GiB" << std::endl;
    out << "HWM: " << sample.GetHwmGiB() << " GiB" << std::endl;
    out << "Swap: " << sample.GetSwapGiB() << " GiB" << std::endl;
    return out;
}

void RecordTable::Output(std::ostream &out) const {
    FTable ftable;
    ftable.AppendRow("Lable", "Count", "Time (s)", "Time (%)", "RSS (G)", "HWM (G)", "Swap (G)", "Location");
    for (const auto &record : records) {
        std::string time_ratio_str{ToStr(FFloatPoint{100 * record.GetTime() / root_itv.GetTime()}) + "%"};
        std::string time_str{ToStr(FFloatPoint{record.GetTime()}.SetPrecision(3))};
        ftable.AppendRow(
            Repeat("  ", record.depth) + record.GetLabelStr(), record.count, time_str, time_ratio_str,
            record.GetRssGiB(), record.GetHwmGiB(), record.GetSwapGiB(), record.GetLocationStr());
    }
    out << ftable;
}

std::ostream &operator<<(std::ostream &out, const RecordTable &record_table) {
    record_table.Output(out);
    return out;
}

// LocationStore
class LocationStore {
public:
    static LocationStore &Get() {
        static LocationStore instance;
        return instance;
    }

    LocationStore(const LocationStore &) = delete;
    LocationStore &operator=(const LocationStore &) = delete;
    LocationStore(LocationStore &&) = delete;
    LocationStore &operator=(LocationStore &&) = delete;

    void SetLocation(const void *key, Location &&value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (Contains(key)) { // 多线程可能重复set_location
            return;
        }
        locations_.push_back(std::move(value));
        Location &location{locations_.back()};
        assert(location.anonymous_id == -1);
        location.anonymous_id = locations_.size() - 1;
        location_map_.emplace(key, &location);
    }

    const Location &GetLocation(const void *key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(Contains(key));
        return *location_map_.at(key);
    }

private:
    LocationStore() = default;
    ~LocationStore() = default;
    bool Contains(const void *key) const { return location_map_.find(key) != location_map_.end(); }

    static inline std::mutex mutex_;
    std::unordered_map<const void *, Location *> location_map_; // 替换成三方parallel数据结构
    std::deque<Location> locations_;                            // 保证地址稳定，只允许后端插入
};

// TraceStore::Node
void TraceStore::Node::UpdateTime(const TimeInterval &time_inverval) {
    time_inverval_ += time_inverval;
    ++count_;
}

void TraceStore::Node::UpdateMemory(const Memory &memory) { memory_ = memory; }

Location TraceStore::Node::GetLocation() const {
    Location location{LocationStore::Get().GetLocation(location_id_)};
    if (TraceConfig::Get().GetAnonymous()) {
        location.label = nullptr;
        location.file = nullptr;
        location.line = -1;
    }
    return location;
}

bool TraceStore::Node::Empty() const { return count_ == 0; }

// TraceStore
void TraceStore::Clear() { // Clear完后又执行到执行过的scope，是否会有问题？
    this->~TraceStore();
    new (this) TraceStore{};
}

int TraceStore::GetOrCreateNode(int parent_i, const void *location_id) {
    Node &parent{nodes_[parent_i]};
    for (int node_i : parent.children_) {
        Node &node{nodes_[node_i]};
        if (node.location_id_ == location_id) {
            return node_i;
        }
    }
    size_t node_i{nodes_.size()};
    nodes_.emplace_back(location_id);
    nodes_[parent_i].children_.push_back(node_i); // vector::emplace_back后，parent引用可能失效
    return node_i;
}

void TraceStore::SetLocation(const char *label, const char *file, int line) {
    const void *location_id{nodes_[node_stack_.back()].location_id_}; // 同层上个被创建节点的标识符
    LocationStore::Get().SetLocation(location_id, {label, file, line});
}

void TraceStore::TraceScopeBegin(const void *location_id) {
    // 压入初始时间点
    scope_stack_.push(TimePoint::Get());

    // 创建新一层节点
    assert(!node_stack_.empty());
    int parent_i{node_stack_.back()};
    int node_i{GetOrCreateNode(parent_i, location_id)};
    node_stack_.push_back(node_i);
}

void TraceStore::TraceScopeEnd() {
    scope_stack_.pop();
    node_stack_.pop_back();
}

void TraceStore::Trace(const void *location_id, size_t mask) {
    // 更新旧节点
    Node &node{nodes_[node_stack_.back()]};
    // 更新数据的节点对应的location_id没有设置过location，可能是TRACE_SCOPE for { TRACE }场景，外层应该抛异常
    // assert(LocationStore::Get().Contains(node.location_id_));

    // 更新时间
    TimePoint time_point{TimePoint::Get()};
    node.UpdateTime(time_point - scope_stack_.top());
    scope_stack_.top() = time_point;

    // 更新内存
    if (mask & MEM) {
        node.UpdateMemory(Memory::Get());
    }

    // 创建新节点
    SetupSameLevelNode(location_id);
}

void TraceStore::Trace(const void *location_id, const detail::TraceVaArgs &va_args) {
    // 更新旧节点
    Node &node{nodes_[node_stack_.back()]};
    // assert(LocationStore::Get().Contains(node.location_id_));

    // 更新时间
    TimePoint time_point{TimePoint::Get()};
    TimeInterval itv{time_point - scope_stack_.top()};
    node.UpdateTime(itv);
    scope_stack_.top() = time_point;

    // 更新内存
    if (va_args.mask & MEM) {
        node.UpdateMemory(Memory::Get());
    }

    // 构造并处理Sample
    va_args.sample_handler({node.GetLocation(), itv, node.memory_});

    // 创建新节点
    SetupSameLevelNode(location_id);
}

void TraceStore::SetupSameLevelNode(const void *location_id) {
    assert(node_stack_.size() > 1);
    int parent_i{node_stack_[node_stack_.size() - 2]};
    int node_i{GetOrCreateNode(parent_i, location_id)};
    node_stack_.back() = node_i;
}

Record TraceStore::GetRecord(const Node &node, size_t depth) const {
    Record record{GetRecord(node.time_inverval_, depth)};
    static_cast<Memory &>(record) = node.memory_;
    static_cast<Location &>(record) = node.GetLocation();
    record.count = node.count_;
    return record;
}

Record TraceStore::GetRecord(const TimeInterval &itv, size_t depth) const {
    Record record;
    record.depth = depth;
    record.count = 1;
    static_cast<TimeInterval &>(record) = itv;
    return record;
}

RecordTable TraceStore::GetRecordTable(const char *) const { return GetRecordTable(); }

TimeInterval TraceStore::GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const {
    const Node &node{nodes_[node_i]};
    auto traverse_children = [&](size_t depth) {
        TimeInterval child_itv;
        for (int child_i : node.children_) {
            child_itv += GetRecordTableImpl(child_i, depth, record_table);
        }
        return child_itv;
    };
    if (node.Empty()) {
        return traverse_children(depth);
    }
    record_table.Append(GetRecord(node, depth));
    if (!node.children_.empty()) {
        record_table.Append(GetRecord(node.time_inverval_ - traverse_children(depth + 1), depth + 1));
    }
    return node.time_inverval_;
}

RecordTable TraceStore::GetRecordTable() const {
    RecordTable table;
    table.thread_id_ = thread_id_;
    table.root_itv = table.entry_itv = GetRootItv();
    for (int node_i : nodes_[0].children_) {
        GetRecordTableImpl(node_i, 0, table);
    }
    return table;
}

TimeInterval TraceStore::GetRootItv() const {
    TimeInterval root_itv;
    for (int node_i : nodes_[0].children_) {
        root_itv += nodes_[node_i].time_inverval_;
    }
    return root_itv;
}

// ParallelTraceStore
ParallelTraceStore &ParallelTraceStore::GetImpl() {
    static ParallelTraceStore instance;
    return instance;
}

TraceStore &ParallelTraceStore::GetLocalImpl() {
    thread_local TraceStore &instance{CreateLocalTraceStore()};
    return instance;
}

TraceStore &ParallelTraceStore::CreateLocalTraceStore() {
    std::lock_guard<std::mutex> lock(mutex_);
    local_trace_stores_.emplace_back();
    return local_trace_stores_.back();
}

void ParallelRecordTable::Output(std::ostream &out) const {
    for (const auto &local : record_tables) {
        out << "Thread: " << local.thread_id_ << std::endl;
        out << local << std::endl;
    }
}

std::ostream &operator<<(std::ostream &out, const ParallelRecordTable &parallel_recored_table) {
    parallel_recored_table.Output(out);
    return out;
}

ParallelRecordTable ParallelTraceStore::GetRecordTable() const {
    ParallelRecordTable parallel_record_table;
    for (auto local_trace_store : local_trace_stores_) {
        parallel_record_table.record_tables.push_back(local_trace_store.GetRecordTable());
    }
    return parallel_record_table;
}

void TraceScopeBase::ThrowCountMismatchException(
    unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line) {
    std::ostringstream oss;
    oss << "TRACE " << label << " missing TRACE_SCOPE declaration in SAME block scope. ";
    if (scope_count < trace_count) {
        oss << "TRACE_SCOPE count " << static_cast<int>(scope_count) << " < TRACE count "
            << static_cast<int>(trace_count) << ". Possible cause: TRACE_SCOPE for { TRACE }. ";
    } else {
        oss << "TRACE_SCOPE count " << static_cast<int>(scope_count) << " > TRACE count "
            << static_cast<int>(trace_count) << ". Possible cause: TRACE_SCOPE if { TRACE }. ";
    }
    oss << "(" << file << ":" << line << ")";
    throw std::runtime_error(oss.str());
}
} // namespace oops
