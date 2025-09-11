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
#include <functional>

#include "oops/trace_def.h"
#include "oops/trace_detail.h"

#include "oops/once.h"
#include "oops/str.h"
#include "oops/format.h"
#include "oops/range.h"

namespace oops {
// TARCE mask参数配置
constexpr size_t MEM = 1;
constexpr size_t MEM_ONCE = 1ul << 1;

// 全局配置
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

// Record数据结构，用于存储及输出、打印
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
        records.push_back(log);
    }

    void Output(std::ostream &out) const;

    TimeInterval root_itv;
    TimeInterval entry_itv;
    std::vector<Record> records;
};
std::ostream &operator <<(std::ostream &out, const RecordTable &sample);

// 存储所有Trace产生的Record，组织依赖关系
class RecordStore {
    class Node {
        friend class RecordStore;

    public:
        Node(const void *location_id) : location_id_{location_id} {}

        void UpdateTime(const TimeInterval &time_inverval);
        void UpdateMemory(const Memory &memory);

        Location GetLocation() const;
        bool Empty() const;

    private:
        // 标识符
        const void *location_id_;
        size_t count_{};

        // 统计数据成员
        TimeInterval time_inverval_;
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
    void TraceScopeBegin(const void *location_id);
    void TraceScopeEnd();

    void Trace(const void *location_id, size_t mask);
    void Trace(const void *location_id, const detail::TraceVaArgs &va_args);

    RecordTable GetRecordTable(const char *label) const;
    RecordTable GetRecordTable() const;

private:
    static RecordStore& GetImpl();  // 避免跨编译单元产生多实例

    int GetOrCreateNode(int parent_i, const void *location_id);
    void SetupSameLevelNode(const void *location_id);

    TimeInterval GetRootItv() const;
    TimeInterval GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const;
    Record GetRecord(const Node &node, size_t depth) const;
    Record GetRecord(const TimeInterval &itv, size_t depth) const;

    std::vector<Node> nodes_{{nullptr, 0, 0}};
    std::vector<int> node_stack_{0};
    std::stack<TimePoint> scope_stack_;     // 维护当前Scope前一个TimePoint，每次Trace，读取并刷新TimePoint
};

class TraceScopeBase {
protected:
    void ThrowCountMismatchException(unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line);
};

// Scope为粒度，每层统计作用域
template <typename>
class TraceScope : public TraceScopeBase {
public:
    TraceScope() {
        RecordStore::Get().TraceScopeBegin(&(++count_));
    }

    ~TraceScope() {
        RecordStore::Get().TraceScopeEnd();
    }

    template <typename>
    void TraceImpl(unsigned char count, const char *label, const char *file, int line) {
        if (count != count_) {
            ThrowCountMismatchException(count_, count, label, file, line);
        }
        static bool location_set{false};
        if (!location_set) {
            RecordStore::Get().SetLocation(label, file, line);
            location_set = true;
        }
    }

    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, size_t mask) {  // 做成类成员函数以便在编译器找到部分SCOPE和TRACE不匹配的问题
        static unsigned char count{0};
        TraceImpl<Lambda>(++count, label, file, line);
        RecordStore::Get().Trace(&count, mask);
    }

    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, const detail::TraceVaArgs &va_args) {
        static unsigned char count{0};
        TraceImpl<Lambda>(++count, label, file, line);
        RecordStore::Get().Trace(&count, va_args);
    }

private:
    static inline unsigned char count_{0};
};

template <typename Lambda>
auto MakeTraceScope(Lambda &&) {
    return TraceScope<Lambda>{};
}
}
