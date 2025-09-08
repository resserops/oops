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
#include <bitset>

#include "oops/once.h"
#include "oops/str.h"
#include "oops/format.h"
#include "oops/range.h"

#include "oops/trace_def.h"

namespace oops {
class TraceConfig {
public:
    static auto& Get() {
        static TraceConfig trace_conf;
        return trace_conf;
    }

    void SetAnonymous(bool anonymous = true) { anonymous_ = anonymous; }
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
    static RecordStore& Get() {
        static RecordStore &stat{GetImpl()};
        return stat;
    }

    void Clear();
    void SetLocation(const char *label, const char *file, int line);
    
    void StartTraceScope(const void *location_id);
    void EndTraceScope();
    void UpdateRecord(const void *location_id);

    RecordTable GetRecordTable(const char *label) const;
    RecordTable GetRecordTable() const;

private:
    static RecordStore& GetImpl();
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

class TraceScopeBase {
protected:
    void Throw(unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line) {
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

    template <typename F2>
    void Trace(const char *label, const char *file, int line, F2 &&) {  // 做成类成员函数更优
        static unsigned char count{0};
        if (++count != count_) {
            Throw(count_, count, label, file, line);
        }
        ONCE() {
            RecordStore::Get().SetLocation(label, file, line);
        }
        RecordStore::Get().UpdateRecord(&count);
    }

private:
    static inline unsigned char count_{0};
};

template <typename F>
auto MakeTraceScope(F &&) {
    return TraceScope<F>{};
}
}
