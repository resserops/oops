#include "oops/trace.h"

namespace oops {
void RecordTable::Output(std::ostream &out) const {
    FTable ftable;
    ftable.AppendRow("Lable", "Count", "Time (s)", "Time (%)", "Location");
    for (const auto &log : table_) {
        std::string time_ratio_str{ToStr(FFloatPoint{100 * log.GetTime() / root_itv.GetTime()}) + "%"};
        std::string time_str{ToStr(FFloatPoint{log.GetTime()}.SetPrecision(3))};
        ftable.AppendRow(StrRepeat("  ", log.depth) + log.GetLabelStr(), log.count, time_str, time_ratio_str, log.GetLocationStr());
    }
    out << ftable;
}

TraceStat& TraceStat::Get() {
    static TraceStat prof_stat;
    return prof_stat;
}

void TraceStat::Clear() {
    this->~TraceStat();
    new (this) TraceStat{};
}

int TraceStat::GetOrCreateNode(int parent_i, void *location_id) {
    RecordNode &parent{nodes_[parent_i]};
    for (int node_i : parent.children_) {
        RecordNode &node{nodes_[node_i]};
        if (node.location_id_ == location_id) {
            return node_i;
        }
    }
    size_t node_i{nodes_.size()};
    size_t node_idx_in_level{parent.children_.size()};
    nodes_.emplace_back(location_id);
    nodes_[parent_i].children_.push_back(node_i); // vector::emplace_back后，parent引用可能失效
    return node_i;
}

void TraceStat::SetLocation(const char *label, const char *file, int line) {
    void *location_id{nodes_[node_stack_.back()].location_id_};
    auto ret{location_map_.insert({location_id, {label, file, line}})};
    assert(ret.second);
}

void TraceStat::CreateRecord(void *location_id) {
    // 创建新节点
    assert(!node_stack_.empty());
    int parent_i{node_stack_.back()};
    int node_i{GetOrCreateNode(parent_i, location_id)};
    node_stack_.push_back(node_i);    
}

void TraceStat::UpdateRecord(void *location_id) {
    // 更新旧节点
    TimePoint sample{TimePoint::Get()};
    std::swap(sample, scope_stack_.top());
    nodes_[node_stack_.back()].UpdateTime(scope_stack_.top() - sample);

    // 创建新节点
    assert(node_stack_.size() > 1);
    int parent_i{node_stack_[node_stack_.size() - 2]};
    int node_i{GetOrCreateNode(parent_i, location_id)};
    node_stack_.back() = node_i;
}

void TraceStat::StartTraceScope(size_t &count) {
    scope_stack_.push(TimePoint::Get());
}

void TraceStat::EndTraceScope() {
    scope_stack_.pop();
    node_stack_.pop_back();
}

Record TraceStat::GetRecord(const RecordNode &node, size_t depth) const {
    Record record{GetRecord(node.data_, depth)};
    static_cast<Memory &>(record) = node.memory_;
    static_cast<Location &>(record) = GetLocaion(node);
    record.count = node.count;
    return record;
}

Location TraceStat::GetLocaion(const RecordNode &node) const {
    Location location{location_map_.at(node.location_id_)};
    if (TraceConfig::Get().GetAnonymous()) {
        location.label = nullptr;
        location.file = nullptr;
        location.line = -1;
    }
    return location;
}

Record TraceStat::GetRecord(const TimeInterval &itv, size_t depth) const {
    Record record;
    record.depth = depth;
    record.count = 1;
    static_cast<TimeInterval &>(record) = itv;
    return record;
}

RecordTable TraceStat::GetRecordTable(const char *label) const {
    return GetRecordTable();
}

TimeInterval TraceStat::GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const {
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

RecordTable TraceStat::GetRecordTable() const {
    RecordTable table;
    table.root_itv = table.entry_itv = GetRootItv();
    for (int node_i : nodes_[0].children_) {
        GetRecordTableImpl(node_i, 0, table);
    }
    return table;
}

TimeInterval TraceStat::GetRootItv() const {
    TimeInterval root_itv;
    for (int node_i : nodes_[0].children_) {
        root_itv += nodes_[node_i].data_;
    }
    return root_itv;
}
}
