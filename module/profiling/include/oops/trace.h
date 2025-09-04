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
class TraceConfig {
public:
    static auto& Get() {
        static TraceConfig trace_conf;
        return trace_conf;
    }

    void SetAnonymous(bool anonymous) { anonymous_ = anonymous; }
    bool GetAnonymous() const { return anonymous_; }

private:
    bool anonymous_{false};
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

struct TimePoint {
    static TimePoint Get() {
        TimePoint trace_point;
        trace_point.time_point = std::chrono::high_resolution_clock::now();
        return trace_point;
    }

    TimeInterval operator-(const TimePoint &rhs) const {
        TimeInterval data;
        data.time = time_point - rhs.time_point;
        return data;
    }
    
    std::chrono::high_resolution_clock::time_point time_point;
};

struct Location {   // 打点位置相关
    Location() = default;
    Location(const char *label, const char *file, int line) : label{label}, file{file}, line{line}, anonymous_id{anonymous_id_incr++} {}

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

    inline static int anonymous_id_incr{}; 

    int line{-1};
    int anonymous_id{-1};
    const char *label{};
    const char *file{};
};

struct Memory {
    size_t rss{};
    size_t hwm{};
    size_t swap{};
};

struct Record : public Location, TimeInterval, Memory {
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

// Trace数据的统计结果，当前全局粒度，后续预期线程粒度
class TraceStat {
    struct RecordNode {
        RecordNode(void *location_id) : location_id_{location_id} {}
        void Update(const TimeInterval rhs) {
            data_ += rhs;
        }
        bool Empty() const {
            return count == 0;
        }
        void UpdateTime(TimeInterval time_inverval) {
            data_ += time_inverval;
            ++count;
        }
        void UpdateMemory(const Memory &memory) {
            memory_ = memory;
        }

        // 标识符
        void *location_id_;
        size_t count{};

        // 统计数据成员
        TimeInterval data_;
        Memory memory_;

        std::vector<int> children_;
    };

public:
    static TraceStat& Get();

    void Clear();
    void SetLocation(const char *label, const char *file, int line);
    void CreateRecord(void *location_id);
    void UpdateRecord(void *location_id);

    void StartTraceScope(size_t &count);
    void EndTraceScope();

    RecordTable GetRecordTable(const char *label) const;
    RecordTable GetRecordTable() const;

private:
    int GetOrCreateNode(int parent_i, void *location_id);
    TimeInterval GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const;
    Record GetRecord(const RecordNode &node, size_t depth) const;
    Record GetRecord(const TimeInterval &itv, size_t depth) const;
    TimeInterval GetRootItv() const;
    Location GetLocaion(const RecordNode &node) const;

    std::vector<RecordNode> nodes_{{nullptr, 0, 0}};
    std::vector<int> node_stack_{0};                    // 方案2，先使用存在hash碰撞的方式
    std::unordered_map<void *, Location> location_map_;
    
    std::stack<TimePoint> scope_stack_;                 // 维护Scope层次关系，保留每层Scope的初始TimePoint
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
auto MakeTraceScope(F &&) {
    return TraceScope<F>{};
}

template <typename F>
void Trace(const char *label, const char *file, int line, F &&) {
    static TraceStat &stat{TraceStat::Get()};
    static bool location_set{false};
    if (!location_set) {
        stat.SetLocation(label, file, line);
        location_set = true;
    }
    stat.UpdateRecord(&location_set);
}
}
