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
    TraceInterval &operator-=(const TraceInterval &rhs) {
        count -= rhs.count;
        time -= rhs.time;
        return *this;
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

struct RecordLog {
    int id;
    const char *label;
    const char *file;
    int line;
    size_t depth;
    size_t count;
    double time;
    double time_ratio2entry;
    double time_ratio2root;
};

struct RecordTable {
    void Extend(const RecordTable &rhs) {
        // table_.insert(a.end(), b.begin(), b.end());
    }

    void Append(const RecordLog &log) {
        table_.push_back(log);
    }

    void Output(std::ostream &out) const {
        FTable ftable;
        ftable.AppendRow("Lable", "Count", "Time (s)", "Time (%)", "Location");
        for (const auto &log : table_) {
            std::string time_ratio = ToStr(FFloatPoint{100 * log.time_ratio2root}) + "%";
            auto time = FFloatPoint{log.time}.SetPrecision(3);
            if (log.id >= 0) {
                std::ostringstream oss;
                oss << std::string(log.file) << ":" << log.line;
                ftable.AppendRow(StrRepeat("  ", log.depth) + log.label + ":" + std::to_string(log.id), log.count, time, time_ratio, oss.str());
            } else if (log.time >= 0.001) {
                ftable.AppendRow(StrRepeat("  ", log.depth) + "other", "", time, time_ratio, "");
            }
        }
        out << ftable;
    }

    std::vector<RecordLog> table_;
};

class Record {
public:
    Record() = default;
    Record(const char *label, const char *file, int line) : label_{label}, file_{file}, line_{line}, id_{id_incr_++} {}

    Record &operator+=(const Record &rhs) {
        data_ += rhs.data_;
        return *this;
    }

    Record &operator+=(const TraceInterval &rhs) {
        data_ += rhs;
        return *this;
    }

    Record &operator-=(const Record &rhs) {
        data_ -= rhs.data_;
        return *this;
    }

    void SetLabel(const char *label) {
        label_ = label;
    }
    const char *GetFile() const {
        return file_;
    }
    int GetLine() const {
        return line_;
    }

    size_t GetCount() const {
        return data_.count;
    }

    const TraceInterval &GetTraceInterval() const {
        return data_;
    }

    RecordLog GetLog() const {
        RecordLog record_log;
        record_log.count = data_.count;
        record_log.id = id_;
        record_log.label = label_;
        record_log.file = file_;
        record_log.line = line_;
        record_log.time = data_.time;
        return record_log;
    }

    static inline int id_incr_{0};

    // 标识符
    int id_;
    const char *label_;
    const char *file_;
    int line_;

    // 统计数据成员
    TraceInterval data_;
};

// Trace数据的统计结果，当前全局粒度，后续预期线程粒度
class TraceStat {
    class RecordNode {
    public:
        RecordNode() = default;
        RecordNode(Record &&record, size_t depth) : record_{std::move(record)}, depth_{depth} {}

        void AddChild(RecordNode *node) {
            children_.push_back(node);
        }

        const Record *GetRecord() const {
            return &record_;
        }

        Record *GetRecord() {
            return &record_;
        }
        
        size_t GetDepth() const {
            return depth_;
        }

        TraceInterval GetRecordTable(const TraceInterval &root_itv, const TraceInterval &entry_itv, size_t depth, RecordTable &record_table) const {
            const auto &itv{record_.GetTraceInterval()};
            if (record_.file_ != nullptr) {
                RecordLog log{record_.GetLog()};
                log.depth = depth;
                log.time_ratio2root = itv.time / root_itv.time;
                log.time_ratio2entry = itv.time / entry_itv.time;
                record_table.Append(log);
                if (!children_.empty()) {
                    TraceInterval my_itv = itv;
                    for (auto &c : children_) {
                        my_itv -= c->GetRecordTable(root_itv, entry_itv, depth + 1, record_table);
                    }
                    RecordLog other_log;
                    other_log.id = -1;
                    other_log.label = "other";
                    other_log.time = my_itv.time;
                    other_log.depth = depth + 1;
                    other_log.time_ratio2root = my_itv.time / root_itv.time;
                    other_log.time_ratio2entry = my_itv.time / root_itv.time;
                    record_table.Append(other_log);
                }
                return itv;
            } else {
                // 跳过空结点
                if (!children_.empty()) {
                    TraceInterval my_itv;
                    for (auto &c : children_) {
                        my_itv += c->GetRecordTable(root_itv, entry_itv, depth, record_table);
                    }
                    return my_itv;
                }
                return itv;
            }
        }

        Record record_;
        size_t depth_;
        std::vector<RecordNode *> children_;
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

    // 维护Record
    Record* CreateRecord(const char *label, const char *file, int line) {
        assert(node_map_.find(label) == node_map_.end());

        // 找不到Record，创建节点
        size_t scope_depth{scope_stack_.size()};

        auto ret{node_map_.insert({label, {{label, file, line}, scope_depth}})};
        RecordNode &node{ret.first->second};

        // 栈中节点深度大于新节点的，加入新节点的子节点
        while (!node_stack_.empty() && node_stack_.back()->GetDepth() > scope_depth) {
            node.AddChild(node_stack_.back());
            node_stack_.pop_back();
        }
        node_stack_.push_back(&node);

        return node.GetRecord();
    }

    inline size_t CombineHash(size_t a, size_t b) {
        constexpr size_t GOLDEN_RATIO = 0x9e3779b97f4a7c15ULL;
        return a ^ (b + GOLDEN_RATIO + (a << 6) + (a >> 2));
    }

    void CreateRecord(size_t id) {
        if (hash_stack_.empty()) {
            hash_stack_.push_back(id);
        } else {
            size_t chash = CombineHash(hash_stack_.back(), id);
            hash_stack_.push_back(chash);
        }
        size_t hash = hash_stack_.back();

        auto ret{nodes_.insert({hash, RecordNode{}})};
        RecordNode &node = ret.first->second;
        if (ret.second) {
            node.depth_ = hash_stack_.size();
            // 新node，建立和父节点的连接关系
            if (!node_stack_.empty()) {
                node_stack_.back()->AddChild(&node);
            }
        } else {
            assert(node.depth_ == hash_stack_.size());
        }

        // 自己压栈
        node_stack_.push_back(&node);        
    }

    void UpdateRecord(size_t id, const char *label, const char *file, int line) {
        // 更新数据，填充当前结点
        node_stack_.back()->record_.label_ = label;
        node_stack_.back()->record_.file_ = file;
        node_stack_.back()->record_.line_ = line;
        UpdateRecord(node_stack_.back()->record_);

        // 更新下个结点并建立关系
        assert(!hash_stack_.empty()); // 至少有一个scope的结点
        size_t hash = CombineHash(hash_stack_.back(), id);
        hash_stack_.back() = hash;

        auto ret{nodes_.insert({hash, RecordNode{}})};
        RecordNode &node = ret.first->second;
        if (ret.second) {
            node.depth_ = hash_stack_.size();
            // 新node，建立和父节点的连接关系
            if (node_stack_.size() > 1) {
                node_stack_[node_stack_.size() - 2]->AddChild(&node);
            }
        } else {
            assert(node.depth_ == hash_stack_.size());
        }

        // 自己替换栈
        node_stack_.back() = &node;
    }

    // 维护TraceScope
    void StartTraceScope(size_t &count) {
        count_stack_.push(count);
        scope_stack_.push(TracePoint::Get());
    }

    void EndTraceScope() {
        count_stack_.pop();
        scope_stack_.pop();
        hash_stack_.pop_back();
        node_stack_.pop_back();
    }

    void UpdateRecord(Record &record) {
        TracePoint sample{TracePoint::Get()};
        std::swap(sample, scope_stack_.top());
        record += (scope_stack_.top() - sample);

        if (record.GetCount() != count_stack_.top()) {
            OOPS_LOGGER << "WARNING: TRACE count " << record.GetCount() << " is not equal to TRACE_SCOPE count " << count_stack_.top() << ". Please use a separate TRACE_SCOPE inside each loop or conditional block. (" << record.GetFile() << ":" << record.GetLine() << ")" << std::endl;
        }
    }

    RecordTable GetRecordTable(const char *label) const {
        if (label == nullptr) {
            return GetRecordTable();
        }
        auto node_it{node_map_.find(label)};
        if (node_it != node_map_.end()) {
            auto &entry_node{node_it->second};
            auto root_itv{GetRootItv()};
            RecordTable table;
            entry_node.GetRecordTable(root_itv, entry_node.GetRecord()->GetTraceInterval(), 0, table);
            return table;
        }
        return {};
    }

    RecordTable GetRecordTable() const {
        auto root_itv{GetRootItv()};
        std::cout << root_itv.count << std::endl;
        std::cout << root_itv.time << std::endl;
        RecordTable table;
        for (auto n : root_.children_) {
            if (n->record_.file_ == nullptr) {
                continue;
            }
            n->GetRecordTable(root_itv, root_itv, 0, table);
        }
        return table;
    }

    TraceInterval GetRootItv() const {
        TraceInterval root_itv;
        for (auto n : root_.children_) {
            root_itv += n->GetRecord()->GetTraceInterval();
        }
        return root_itv;
    }

private:
    std::vector<size_t> hash_stack_;// 方案2，先使用存在hash碰撞的方式
    std::unordered_map<size_t, RecordNode> nodes_;  // key为path hash，每个hash代表一个路径，每个独立路径，对应一个record
    RecordNode root_;

    std::stack<size_t> count_stack_;
    std::stack<TracePoint> scope_stack_;                    // 维护Scope层次关系，保留每层Scope的初始Sample
    std::vector<RecordNode *> node_stack_{&root_};                  // 没有置入父结点的Record，打印时需要遍历
    std::unordered_map<std::string, RecordNode> node_map_;  // 索引指定名字的Record
};

// 当前代码，不同上层record通过调用函数，trace到相同的代码，会一起统计，无法区分这个trace的归属
// 后续思路，每个trace的全链前缀都要有不同的record
// scope的时候建立索引关系，trace的时候刷值

// Scope为粒度，每层统计作用域
template <typename F>
class TraceScope {
public:
    TraceScope() {
        static TraceStat &stat{TraceStat::Get()};
        stat.StartTraceScope(++count_);
        stat.CreateRecord(size_t(&count_));
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
    static size_t unique;
    static TraceStat &stat{TraceStat::Get()};
    stat.UpdateRecord(size_t(&unique), label, file, line);
}
}
