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

namespace oops {
constexpr size_t MEM = 1;

template <typename T, std::ostream &Stream = std::cout>
struct StreamOut {
    void operator() (const T &t) {
        std::cout << t;
    }
};


class TraceConfig {
public:
    static auto& Get() {
        static auto &instance{GetImpl()};
        return instance;
    }

    void SetAnonymous(bool anonymous = true) { anonymous_ = anonymous; }
    bool GetAnonymous() const { return anonymous_; }

private:
    static TraceConfig& GetImpl();

    bool anonymous_{false};
};

struct Location {   // 打点位置相关
    Location() = default;
    Location(const char *label, const char *file, int line) : label{label}, file{file}, line{line} {}

    std::string GetLabelStr() const {
        if (anonymous_id < 0) {
            return "other";
        }
        if (label == nullptr) {
            return std::string{"trace_"} + std::to_string(anonymous_id);
        }
        return std::string{label} + " (" + std::to_string(anonymous_id) + ")";
    }

    std::string GetLocationStr() const {
        if (file == nullptr) {
            return {};
        }
        return std::string{file} + ":" + std::to_string(line);
    }

    const char *label{};
    const char *file{};
    int line{-1};
    int anonymous_id{-1};
};

struct TimePoint {
    static TimePoint Get() {
        TimePoint trace_point;
        trace_point.time_point = std::chrono::high_resolution_clock::now();
        return trace_point;
    }
    
    std::chrono::high_resolution_clock::time_point time_point;
};

struct TimeInterval {
    TimeInterval &operator+=(const TimeInterval &rhs) {
        time += rhs.time;
        return *this;
    }
    TimeInterval operator-(const TimeInterval &rhs) const {
        TimeInterval itv;
        itv.time = time - rhs.time;
        return itv;
    }
    double GetTime() const {
        return std::chrono::duration<double>(time).count();
    }

    std::chrono::high_resolution_clock::duration time{};
};

inline TimeInterval operator-(const TimePoint &lhs, const TimePoint &rhs) {
    return {lhs.time_point - rhs.time_point};
}

struct Memory {
    static Memory Get();
    
    double GetRssGiB() const { return 1.0 * rss / 1024 / 1024; }
    double GetHwmGiB() const { return 1.0 * hwm / 1024 / 1024; }
    double GetSwapGiB() const { return 1.0 * swap / 1024 / 1024; }

    size_t rss{};
    size_t hwm{};
    size_t swap{};
};

struct Sample : public Location, TimeInterval, Memory {};
std::ostream &operator <<(std::ostream &out, const Sample &sample);

struct Record : public Sample {
    size_t count{}; // Node中获取
    size_t depth{}; // 递归时获取
};

struct RecordTable {
    void Append(const Record &log) {
        table_.push_back(log);
    }

    void Output(std::ostream &out) const;

    TimeInterval root_itv;
    TimeInterval entry_itv;
    std::vector<Record> table_;
};

class RecordStore {
    class RecordNode {
        friend class RecordStore;

    public:
        RecordNode(const void *location_id) : location_id_{location_id} {}

        void UpdateTime(const TimeInterval &time_inverval);
        void UpdateMemory(const Memory &memory);

        Location GetLocation() const;
        bool Empty() const;

    private:
        // 标识符
        const void *location_id_;
        size_t count_{};

        // 统计数据成员
        TimeInterval data_;
        Memory memory_;

        std::vector<int> children_;
    };

public:
    static auto& Get() {
        static RecordStore &stat{GetImpl()};
        return stat;
    }

    void Clear();
    void SetLocation(const char *label, const char *file, int line);
    
    void StartTraceScope(const void *location_id);
    void EndTraceScope();

    template<size_t Mask, typename SampleHandler>
    void UpdateRecord(const void *location_id) {
        // 更新旧节点
        RecordNode &node{nodes_[node_stack_.back()]};
        TimePoint time_point{TimePoint::Get()};
        // assert(LocationStore::Get().Contains(node.location_id_));   // 更新数据的节点对应的location_id没有设置过location，可能是TRACE_SCOPE for{ TRACE }场景，外层应该抛异常
        TimeInterval itv{time_point - scope_stack_.top()};
        node.UpdateTime(itv);
        scope_stack_.top() = time_point;
        
        if constexpr (Mask & MEM) {
            node.UpdateMemory(Memory::Get());
        }
        
        if constexpr (!std::is_same_v<SampleHandler, void>) {
            SampleHandler{}({node.GetLocation(), itv, node.memory_});
        }

        // 创建新节点
        assert(node_stack_.size() > 1);
        int parent_i{node_stack_[node_stack_.size() - 2]};
        int node_i{GetOrCreateNode(parent_i, location_id)};
        node_stack_.back() = node_i;
    }

    RecordTable GetRecordTable(const char *label) const;
    RecordTable GetRecordTable() const;

private:
    static RecordStore& GetImpl();  // 避免跨编译单元产生多实例

    int GetOrCreateNode(int parent_i, const void *location_id);
    TimeInterval GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const;
    Record GetRecord(const RecordNode &node, size_t depth) const;
    Record GetRecord(const TimeInterval &itv, size_t depth) const;
    TimeInterval GetRootItv() const;

    std::vector<RecordNode> nodes_{{nullptr, 0, 0}};
    std::vector<int> node_stack_{0};
    std::stack<TimePoint> scope_stack_;     // 维护当前Scope的第一个TimePoint，每次Trace，读取并刷新TimePoint
    std::stack<size_t> count_stack_;        // 维护当前Scope的计数，
};

template <size_t M = 0, typename SH = void>
struct TraceParams {
    using SampleHandler = SH;
    static constexpr size_t MASK{M};
};

class TraceScopeBase {
protected:
    void ThrowCountMismatchException(unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line);
};

// Scope为粒度，每层统计作用域
template <typename F>
class TraceScope : public TraceScopeBase {
public:
    TraceScope() {
        RecordStore::Get().StartTraceScope(&(++count_));
    }

    ~TraceScope() {
        RecordStore::Get().EndTraceScope();
    }

    template <typename TraceParams, typename F2>
    void Trace(const char *label, const char *file, int line, F2 &&) {  // 做成类成员函数以便在编译器找到部分SCOPE和TRACE不匹配的问题
        static unsigned char count{0};
        if (++count != count_) {
            ThrowCountMismatchException(count_, count, label, file, line);
        }
        ONCE() {
            RecordStore::Get().SetLocation(label, file, line);
        }
        RecordStore::Get().UpdateRecord<TraceParams::MASK, typename TraceParams::SampleHandler>(&count);
    }

private:
    static inline unsigned char count_{0};
};

template <typename F>
auto MakeTraceScope(F &&) {
    return TraceScope<F>{};
}
}
