#include "oops/trace.h"

#include "oops/system_info.h"

namespace oops {
// TraceConfig
TraceConfig& TraceConfig::GetImpl() {
    static TraceConfig instance;
    return instance;
}

// Metrics
Memory Memory::Get() {
    Memory memory;
    auto status_info{status::Get(status::Field::VM_RSS | status::Field::VM_HWM | status::Field::VM_SWAP)};
    memory.rss = *status_info.vm_rss;
    memory.hwm = *status_info.vm_hwm;
    memory.swap = *status_info.vm_swap;
    return memory;
}

std::ostream &operator <<(std::ostream &out, const Sample &sample) {
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
    for (const auto &log : table_) {
        std::string time_ratio_str{ToStr(FFloatPoint{100 * log.GetTime() / root_itv.GetTime()}) + "%"};
        std::string time_str{ToStr(FFloatPoint{log.GetTime()}.SetPrecision(3))};
        ftable.AppendRow(StrRepeat("  ", log.depth) + log.GetLabelStr(), log.count, time_str, time_ratio_str, log.GetRssGiB(), log.GetHwmGiB(), log.GetSwapGiB(), log.GetLocationStr());
    }
    out << ftable;
}

// LocationStore
class LocationStore {
public:
    static LocationStore& Get() {
        static LocationStore instance;
        return instance;
    }
    
    LocationStore(const LocationStore&) = delete;
    LocationStore& operator=(const LocationStore&) = delete;
    LocationStore(LocationStore&&) = delete;
    LocationStore& operator=(LocationStore&&) = delete;

    void SetLocation(const void *key, Location &&value)  {
        assert(!Contains(key)); // Location重复设置，两处location对应的prev_node相同，TRACE_SCOPE if { TRACE } else { TRACE }场景
        locations_.push_back(std::move(value));
        Location &location{locations_.back()};
        assert(location.anonymous_id == -1);
        location.anonymous_id = locations_.size() - 1;
        location_map_.emplace(key, &location);
    }

    const Location &GetLocation(const void *key) const {
        assert(Contains(key));
        return *location_map_.at(key);
    }

    bool Contains(const void *key) const {
        return location_map_.find(key) != location_map_.end();
    }

private:
    LocationStore() = default;
    ~LocationStore() = default;

    std::unordered_map<const void *, Location *> location_map_; // 上个位置的标识void *，LOCATION信息绑定，永久存在不可清除，前提条件，每个scope的到代码块执行顺序是固定，串行的
    std::deque<Location> locations_;    // 保证地址稳定，只允许后端插入
};

// RecordStore::RecordNode
void RecordStore::RecordNode::UpdateTime(const TimeInterval &time_inverval) {
    data_ += time_inverval;
    ++count_;
}

void RecordStore::RecordNode::UpdateMemory(const Memory &memory) {
    memory_ = memory;
}

Location RecordStore::RecordNode::GetLocation() const {
    Location location{LocationStore::Get().GetLocation(location_id_)};
    if (TraceConfig::Get().GetAnonymous()) {
        location.label = nullptr;
        location.file = nullptr;
        location.line = -1;
    }
    return location;
}

bool RecordStore::RecordNode::Empty() const {
    return count_ == 0;
}

// RecordStore
RecordStore& RecordStore::GetImpl() {
    static RecordStore instance;
    return instance;
}

void RecordStore::Clear() { // Clear完后又执行到执行过的scope，是否会有问题？
    this->~RecordStore();
    new (this) RecordStore{};
}

int RecordStore::GetOrCreateNode(int parent_i, const void *location_id) {
    RecordNode &parent{nodes_[parent_i]};
    for (int node_i : parent.children_) {
        RecordNode &node{nodes_[node_i]};
        if (node.location_id_ == location_id) {
            return node_i;
        }
    }
    size_t node_i{nodes_.size()};
    nodes_.emplace_back(location_id);
    nodes_[parent_i].children_.push_back(node_i); // vector::emplace_back后，parent引用可能失效
    return node_i;
}

void RecordStore::SetLocation(const char *label, const char *file, int line) {
    const void *location_id{nodes_[node_stack_.back()].location_id_}; // 同层上个被创建节点的标识符
    LocationStore::Get().SetLocation(location_id, {label, file, line});
}

void RecordStore::StartTraceScope(const void *location_id) {
    // 压入初始时间点
    scope_stack_.push(TimePoint::Get());

    // 创建新节点
    assert(!node_stack_.empty());
    int parent_i{node_stack_.back()};
    int node_i{GetOrCreateNode(parent_i, location_id)};
    node_stack_.push_back(node_i);    
}

void RecordStore::EndTraceScope() {
    scope_stack_.pop();
    node_stack_.pop_back();
}

Record RecordStore::GetRecord(const RecordNode &node, size_t depth) const {
    Record record{GetRecord(node.data_, depth)};
    static_cast<Memory &>(record) = node.memory_;
    static_cast<Location &>(record) = node.GetLocation();
    record.count = node.count_;
    return record;
}

Record RecordStore::GetRecord(const TimeInterval &itv, size_t depth) const {
    Record record;
    record.depth = depth;
    record.count = 1;
    static_cast<TimeInterval &>(record) = itv;
    return record;
}

RecordTable RecordStore::GetRecordTable(const char *) const {
    return GetRecordTable();
}

TimeInterval RecordStore::GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const {
    const RecordNode &node{nodes_[node_i]};
    auto traverse_children = [&] (size_t depth) {
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
        record_table.Append(GetRecord(node.data_ - traverse_children(depth + 1), depth + 1)); 
    }
    return node.data_;
}

RecordTable RecordStore::GetRecordTable() const {
    RecordTable table;
    table.root_itv = table.entry_itv = GetRootItv();
    for (int node_i : nodes_[0].children_) {
        GetRecordTableImpl(node_i, 0, table);
    }
    return table;
}

TimeInterval RecordStore::GetRootItv() const {
    TimeInterval root_itv;
    for (int node_i : nodes_[0].children_) {
        root_itv += nodes_[node_i].data_;
    }
    return root_itv;
}

void TraceScopeBase::ThrowCountMismatchException(unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line) {
    std::ostringstream oss;
    oss << "TRACE " << label << " missing TRACE_SCOPE declaration in SAME block scope. ";
    if (scope_count < trace_count) {
        oss << "TRACE_SCOPE count " << static_cast<int>(scope_count) << " < TRACE count " << static_cast<int>(trace_count) << ". Possible cause: TRACE_SCOPE for { TRACE }. ";
    } else {
        oss << "TRACE_SCOPE count " << static_cast<int>(scope_count) << " > TRACE count " << static_cast<int>(trace_count) << ". Possible cause: TRACE_SCOPE if { TRACE }. ";
    }
    oss << "(" << file << ":" << line << ")";
    throw std::runtime_error(oss.str());  
}
}
