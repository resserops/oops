#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <iostream>
#include <stack>
#include <iomanip>
#include <sstream>

#include "oops/once.h"
#include "oops/str.h"
#include "oops/format.h"
#include "oops/range.h"

#include "oops/trace_def.h"

#define OOPS_LOGGER ::std::cout

namespace oops {
namespace metric {
constexpr uint8_t TIME{1};
constexpr uint8_t CPU_TIME{1 << 1};
constexpr uint8_t RSS{1 << 2};
constexpr uint8_t HWM{1 << 3};
}

class TraceConfig {
public:
    static auto& Get() {
        static TraceConfig trace_conf;
        return trace_conf;
    }

    void SetOnlyId(bool only_id) { only_id_ = only_id; }
    bool GetOnlyId() const { return only_id_; }

private:
    bool only_id_{false};
};

struct TraceInterval {
    TraceInterval &operator+=(const TraceInterval &rhs) {
        count += rhs.count;
        time += rhs.time;
        return *this;
    }
    TraceInterval operator-(const TraceInterval &rhs) const {
        TraceInterval itv;
        itv.count = count - rhs.count;
        itv.time = time - rhs.time;
        return itv;
    }

    void Output(std::ostream &out, const TraceInterval &itv) const {}
    size_t count{0};
    double time{.0};
};

struct TracePoint {
    static TracePoint Get() {
        TracePoint trace_point;
        trace_point.time_point = std::chrono::system_clock::now();
        return trace_point;
    }
    TraceInterval operator-(const TracePoint &rhs) const {
        TraceInterval data;
        auto us{std::chrono::duration_cast<std::chrono::microseconds>(time_point - rhs.time_point).count()};
        data.count = 1;
        data.time = static_cast<double>(us) / 1e6;
        return data;
    }
    
    std::chrono::system_clock::time_point time_point;
};

struct Record {
    std::string id{};
    const char *label{};
    const char *file{};
    int line{};
    size_t depth;
    size_t count;
    double time;
    double time_ratio2entry;
    double time_ratio2root;
};

struct RecordTable {
    void Append(const Record &log) {
        table_.push_back(log);
    }

    void Output(std::ostream &out) const {
        FTable ftable;
        ftable.AppendRow("Lable", "Count", "Time (s)", "Time (%)", "Location");
        for (const auto &log : table_) {
            std::string time_ratio = ToStr(FFloatPoint{100 * log.time_ratio2root}) + "%";
            auto time = FFloatPoint{log.time}.SetPrecision(3);
            if (log.label != nullptr) {
                std::ostringstream oss;
                oss << std::string(log.file) << ":" << log.line;
                ftable.AppendRow(StrRepeat("  ", log.depth) + log.label + ":" + log.id, log.count, time, time_ratio, oss.str());
            } else if (log.time >= 0.001) {
                ftable.AppendRow(StrRepeat("  ", log.depth) + "other", "", time, time_ratio, "");
            }
        }
        out << ftable;
    }

    std::vector<Record> table_;
};

struct Location {
    const char *label;
    const char *file;
    int line;
};

// Trace数据的统计结果，当前全局粒度，后续预期线程粒度
class TraceStat {
    struct RecordNode {
        RecordNode(void *location_id, int level, int idx_in_level) : location_id_{location_id}, level{level}, idx_in_level{idx_in_level} {}
        RecordNode operator+=(const TraceInterval &rhs) {
            data_ += rhs;
            return *this;
        }
        bool Empty() const {
            return data_.count == 0;
        }

        // 标识符
        void *location_id_;

        // 统计数据成员
        TraceInterval data_;

        // 树结构维护
        int level, idx_in_level;
        std::vector<int> children_;
    };

public:
    static auto& Get() {
        static TraceStat prof_stat;
        return prof_stat;
    }

    void Clear() {
        this->~TraceStat();
        new (this) TraceStat{};
    }

    inline size_t CombineHash(size_t a, size_t b) {
        constexpr size_t GOLDEN_RATIO = 0x9e3779b97f4a7c15ULL;
        return a ^ (b + GOLDEN_RATIO + (a << 6) + (a >> 2));
    }

    int GetOrCreateNode(int parent_i, void *location_id) {
        RecordNode &parent{nodes_[parent_i]};
        for (int node_i : parent.children_) {
            RecordNode &node{nodes_[node_i]};
            if (node.location_id_ == location_id) {
                return node_i;
            }
        }
        size_t node_i{nodes_.size()};
        size_t node_idx_in_level{parent.children_.size()};
        nodes_.emplace_back(location_id, parent.level + 1, node_idx_in_level);
        nodes_[parent_i].children_.push_back(node_i); // vector::emplace_back后，parent引用可能失效
        return node_i;
    }

    void SetLocation(const char *label, const char *file, int line) {
        void *location_id{nodes_[node_stack_.back()].location_id_};
        auto ret{location_map_.insert({location_id, {label, file, line}})};
        assert(ret.second);
    }

    void CreateRecord(void *location_id) {
        assert(!node_stack_.empty());
        int parent_i{node_stack_.back()};
        int node_i{GetOrCreateNode(parent_i, location_id)};
        node_stack_.push_back(node_i);    
    }

    void UpdateRecord(void *location_id) {
        TracePoint sample{TracePoint::Get()};
        std::swap(sample, scope_stack_.top());
        nodes_[node_stack_.back()] += (scope_stack_.top() - sample);

        assert(node_stack_.size() > 1);
        int parent_i{node_stack_[node_stack_.size() - 2]};
        int node_i{GetOrCreateNode(parent_i, location_id)};
        node_stack_.back() = node_i;
    }

    // 维护TraceScope
    void StartTraceScope(size_t &count) {
        scope_stack_.push(TracePoint::Get());
    }

    void EndTraceScope() {
        scope_stack_.pop();
        node_stack_.pop_back();
    }

    struct SumIntervals {
        TraceInterval root;
        TraceInterval entry;
    };
    
    Record GetRecord(const RecordNode &node, const SumIntervals &sum_itvs, size_t depth) const {
        Record record{GetRecord(node.data_, sum_itvs, depth)};
        record.id = std::string{} + static_cast<char>('A' + node.level - 1) + std::to_string(node.idx_in_level);
        record.label = location_map_.at(node.location_id_).label;
        record.file = location_map_.at(node.location_id_).file;
        record.line = location_map_.at(node.location_id_).line;
        return record;
    }

    Record GetRecord(const TraceInterval &itv, const SumIntervals &sum_itvs, size_t depth) const {
        Record record;
        record.depth = depth;
        record.count = itv.count;
        record.time = itv.time;
        record.time_ratio2entry = itv.time / sum_itvs.entry.time;
        record.time_ratio2root = itv.time / sum_itvs.root.time;
        return record;
    }

    RecordTable GetRecordTable(const char *label) const {
        return GetRecordTable();
    }

    TraceInterval GetRecordTableImpl(int node_i, const SumIntervals &sum_itvs, size_t depth, RecordTable &record_table) const {
        const RecordNode &node{nodes_[node_i]};
        auto traverse_children = [&] (size_t depth) {
            TraceInterval child_itv;
            for (int child_i : node.children_) {
                child_itv += GetRecordTableImpl(child_i, sum_itvs, depth, record_table);
            }
            return child_itv;
        };
        if (node.Empty()) {
            return traverse_children(depth);
        }
        record_table.Append(GetRecord(node, sum_itvs, depth));
        if (!node.children_.empty()) {
            record_table.Append(GetRecord(node.data_ - traverse_children(depth + 1), sum_itvs, depth + 1)); 
        }
        return node.data_;
    }

    RecordTable GetRecordTable() const {
        auto root_itv{GetRootItv()};
        RecordTable table;
        for (int node_i : nodes_[0].children_) {
            GetRecordTableImpl(node_i, {root_itv, root_itv}, 0, table);
        }
        return table;
    }

    TraceInterval GetRootItv() const {
        TraceInterval root_itv;
        for (int node_i : nodes_[0].children_) {
            root_itv += nodes_[node_i].data_;
        }
        return root_itv;
    }

private:
    std::vector<RecordNode> nodes_{{nullptr, 0, 0}};
    std::vector<int> node_stack_{0};  // 方案2，先使用存在hash碰撞的方式
    std::unordered_map<void *, Location> location_map_; // location数量比node少，而且访问较独立不会刷新，冷数据，单独管理
    
    std::stack<TracePoint> scope_stack_;                    // 维护Scope层次关系，保留每层Scope的初始Sample
};

// 当前代码，不同上层record通过调用函数，trace到相同的代码，会一起统计，无法区分这个trace的归属
// 后续思路，每个trace的全链前缀都要有不同的record
// scope的时候建立索引关系，trace的时候刷值

template <typename T>
struct TypeId {
    static void *Get() {
        static char *addr{&c};
        return addr;
    }
    static inline char c;
};

// Scope为粒度，每层统计作用域
template <typename F>
class TraceScope {
public:
    TraceScope() {
        static TraceStat &stat{TraceStat::Get()};
        stat.StartTraceScope(++count_);
        stat.CreateRecord(&count_);
    }

    ~TraceScope() {
        static TraceStat &stat{TraceStat::Get()};
        stat.EndTraceScope();
    }

private:
    static inline size_t count_{0};
};

template <typename F>
auto MakeTraceScope(F &&f) {
    return TraceScope<F>{};
}

template <typename F>
void Trace(const char *label, const char *file, int line, F &&f) {
    static TraceStat &stat{TraceStat::Get()};
    static bool location_set{false};
    if (!location_set) {
        stat.SetLocation(label, file, line);
        location_set = true;
    }
    stat.UpdateRecord(&location_set);
}
}
