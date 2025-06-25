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

#include "once.h"

#define OOPS_ENABLE_TRACE 1

// 定义TRACE_SCOPE
#define OOPS_TRACE_SCOPE_NAMED(name) ::oops::TraceScope name{::oops::MakeTraceScope([]{})}

#if OOPS_ENABLE_TRACE
#define OOPS_TRACE_SCOPE() OOPS_TRACE_SCOPE_NAMED(__trace_scope##line)
#else
#define OOPS_TRACE_SCOPE()
#endif

#ifndef TRACE_SCOPE
#define TRACE_SCOPE() OOPS_TRACE_SCOPE()
#endif

// 定义TRACE
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE(label) ::oops::Trace(#label, __FILE__, __LINE__, []{})
#else
#define OOPS_TRACE(label)
#endif

#ifndef TRACE
#define TRACE(label) OOPS_TRACE(label)
#endif

// 定义TRACE_OUTPUT
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE_OUTPUT_IMPL(out, label) ::oops::TraceStat::Get().Output(out, label)
#else
#define OOPS_TRACE_OUTPUT_IMPL(out, label) 
#endif

#define OOPS_TRACE_OUTPUT(out, label) OOPS_TRACE_OUTPUT_IMPL(out, #label)

#ifndef TRACE_OUTPUT
#define TRACE_OUTPUT(out, label) OOPS_TRACE_OUTPUT(out, label)
#endif

#define OOPS_TRACE_PRINT() OOPS_TRACE_OUTPUT_IMPL(::std::cout, nullptr)

#ifndef TRACE_PRINT
#define TRACE_PRINT() OOPS_TRACE_PRINT()
#endif

#define OOPS_LOGGER ::std::cout

namespace oops {
namespace metric {
constexpr uint8_t TIME{1};
constexpr uint8_t CPU_TIME{1 << 1};
constexpr uint8_t RSS{1 << 2};
constexpr uint8_t HWM{1 << 3};
}

std::string RepeatStr(const std::string &str, size_t n) {
    if (n == 0 || str.empty()) {
        return {};
    }
    std::string ret;
    ret.reserve(n * str.size());
    for (size_t i{0}; i < n; ++i) {
        ret.append(str);
    }
    return ret;
}

std::string operator*(size_t n, const std::string &str) {
    return RepeatStr(str, n);
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

class Record {
public:
    class Sample;

private:
    class Data {
        friend class Record::Sample;

    public:
        Data &operator+=(const Data &rhs) {
            count_ += rhs.count_;
            time_ += rhs.time_;
            return *this;
        }

        Data &operator-=(const Data &rhs) {
            count_ -= rhs.count_;
            time_ -= rhs.time_;
            return *this;
        }

        size_t GetCount() const {
            return count_;
        }

        template <typename Out>
        void Output(Out &out, const Data &itv) const {
            out << "time: " << std::setprecision(3) << time_ << "s (" << (100 * time_ / itv.time_) << "%)";
        }

    private:
        size_t count_{0};
        double time_{0};
    };

public:
    // 采样结果，存储每个时刻的指标值
    class Sample {
    public:
        static auto Get() {
            Sample sample;
            sample.time_point_ = std::chrono::system_clock::now();
            return sample;
        }
        
        Data operator-(const Sample &rhs) const {
            Data data;
            auto count{std::chrono::duration_cast<std::chrono::microseconds>(time_point_ - rhs.time_point_).count()};
            data.count_ = 1;
            data.time_ = static_cast<double>(count) / 1e6;
            return data;
        }

    private:
        std::chrono::system_clock::time_point time_point_{};
    };

    Record(const char *label, const char *file, int line) : label_{label}, file_{file}, line_{line}, id_{id_incr_++} {}

    Record &operator+=(const Record &rhs) {
        data_ += rhs.data_;
        return *this;
    }

    Record &operator+=(const Data &rhs) {
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
    template <typename Out>
    void OutputLoc(Out &out) const {
        out << file_ << ":" << line_;
    }

    size_t GetCount() const {
        return data_.GetCount();
    }

    template <typename Out>
    void Output(Out &out, const Record &sum_record) const {
        static TraceConfig &trace_conf{TraceConfig::Get()};
        if (label_ == nullptr) {
            out << "[other] ";
        } else {
            if (trace_conf.GetOnlyId()) {
                out << "[" << id_ << "] ";
            } else {
                out << "[" << label_ << "-" << id_ << "] ";
            }
        }
        data_.Output(out, sum_record.data_);
        if (label_ != nullptr && !trace_conf.GetOnlyId()) {
            out << " loc: " << file_ << ":" << line_ << " ";
        }
    }

private:
    static inline size_t id_incr_{0};

    // 标识符
    size_t id_;
    const char *label_;
    const char *file_;
    int line_;

    // 统计数据成员
    Data data_;
};

// Trace数据的统计结果，当前全局粒度，后续预期线程粒度
class TraceStat {
    class RecordNode {
    public:
        static inline constexpr const char INDENT[]{"|   "};
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
        
        template <typename Out>
        void Output(Out &out, const Record &sum_record, size_t depth = 0) const {
            out << depth * std::string{INDENT};
            record_.Output(out, sum_record);
            out << std::endl;

            Record other_record{record_};
            other_record.SetLabel(nullptr);
            for (auto it{children_.rbegin()}; it != children_.rend(); ++it) {
                (*it)->Output(out, sum_record, depth + 1);
                other_record -= *((*it)->GetRecord());
            }
            if (!children_.empty()) {
                out << (depth + 1) * std::string{INDENT};
                other_record.Output(out, sum_record);
                out << std::endl;
            }
        }

    private:
        Record record_;
        size_t depth_;
        std::vector<RecordNode *> children_;
    };

public:
    static auto& Get() {
        static TraceStat prof_stat;
        return prof_stat;
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

    // 维护TraceScope
    void StartTraceScope(size_t count) {
        count_stack_.push(count);
        scope_stack_.push(Record::Sample::Get());
    }

    void EndTraceScope() {
        count_stack_.pop();
        scope_stack_.pop();
    }

    void UpdateRecord(Record &record) {
        Record::Sample sample{Record::Sample::Get()};
        std::swap(sample, scope_stack_.top());
        record += (scope_stack_.top() - sample);
        if (record.GetCount() != count_stack_.top()) {
            ONCE() {
                OOPS_LOGGER << "WARNING: TRACE count " << record.GetCount() << " is not equal to TRACE_SCOPE count " << scope_count_ << ". Please use a separate TRACE_SCOPE inside each loop or conditional block. (" << record.GetFile() << ":" << record.GetLine() << ")" << std::endl;
            }
        }
    }

    template <typename Out>
    void Output(Out &out) const {
        Record sum{"", "", 0};
        for (auto n : node_stack_) {
            sum += *(n->GetRecord());
        }
        for (auto n : node_stack_) {
            n->Output(out, sum);
        }
        out << std::endl;
    }
    
    template <typename Out>
    void Output(Out &out, const char *label) const {
        if (label == nullptr) {
            Output(out);
            return;
        }
        auto label_it{node_map_.find(label)};
        if (label_it != node_map_.end()) {
            label_it->second.Output(out, *(label_it->second.GetRecord()));
        }
        out << std::endl;
    }

private:
    size_t scope_count_{};
    std::stack<size_t> count_stack_;
    std::stack<Record::Sample> scope_stack_;                // 维护Scope层次关系，保留每层Scope的初始Sample
    std::vector<RecordNode *> node_stack_;                  // 没有置入父结点的Record，打印时需要遍历
    std::unordered_map<std::string, RecordNode> node_map_;  // 索引指定名字的Record
};

// Scope为粒度，每层统计作用域
template <typename F>
class TraceScope {
public:
    TraceScope() {
        static TraceStat &stat{TraceStat::Get()};
        stat.StartTraceScope(++count_);
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
    static Record *local_record{nullptr};
    if (local_record == nullptr) {
        local_record = stat.CreateRecord(label, file, line);
    }
    stat.UpdateRecord(*local_record);
}
}
