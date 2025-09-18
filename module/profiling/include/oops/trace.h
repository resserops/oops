#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "oops/trace_detail.h"

#include "oops/format.h"
#include "oops/once.h"
#include "oops/range.h"
#include "oops/str.h"

// 定义静态TRACE等级
#define OOPS_TRACE_LEVEL_VERBOSE 0
#define OOPS_TRACE_LEVEL_DEBUG 1
#define OOPS_TRACE_LEVEL_INFO 2
#define OOPS_TRACE_LEVEL_OFF 3

#ifndef OOPS_TRACE_LEVEL
#define OOPS_TRACE_LEVEL OOPS_TRACE_LEVEL_INFO // 默认等级INFO
#endif

// 定义TRACE_SCOPE
#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_VERBOSE
#define OOPS_TRACE_SCOPE_VERBOSE() auto oops_trace_scope__(::oops::MakeTraceScope<OOPS_TRACE_LEVEL_VERBOSE>([] {}))
#elif OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE_SCOPE_VERBOSE() EmptyScope oops_trace_scope__
#else
#define OOPS_TRACE_SCOPE_VERBOSE() (void)0
#endif

#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_DEBUG
#define OOPS_TRACE_SCOPE_DEBUG() auto oops_trace_scope__(::oops::MakeTraceScope<OOPS_TRACE_LEVEL_DEBUG>([] {}))
#elif OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE_SCOPE_DEBUG() EmptyScope oops_trace_scope__
#else
#define OOPS_TRACE_SCOPE_DEBUG() (void)0
#endif

#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE_SCOPE_INFO() auto oops_trace_scope__(::oops::MakeTraceScope<OOPS_TRACE_LEVEL_INFO>([] {}))
#else
#define OOPS_TRACE_SCOPE_INFO() (void)0
#endif

#define OOPS_TRACE_SCOPE(level) OOPS_TRACE_SCOPE_##level()

#ifndef TRACE_SCOPE
#define TRACE_SCOPE(level) OOPS_TRACE_SCOPE(level)
#endif

// 定义TRACE
#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE(label, ...) \
    oops_trace_scope__.Trace([] {}, #label, __FILE__, __LINE__, ::oops::detail::ParseTraceVaArgs(__VA_ARGS__))
#else
#define OOPS_TRACE(label, ...) (void)0
#endif

#ifndef TRACE
#define TRACE(label, ...) OOPS_TRACE(label, __VA_ARGS__)
#endif

namespace oops {
// TARCE mask参数配置
constexpr size_t MEM = 1;
constexpr size_t MEM_ONCE = 1ul << 1;

// 全局配置
class TraceConfig {
public:
    static auto &Get() {
        static auto &instance{GetImpl()};
        return instance;
    }

    void SetTraceLevel(unsigned char trace_level) { trace_level_ = trace_level; }
    unsigned char GetTraceLevel() const { return trace_level_; }
    void SetAnonymous(bool anonymous = true) { anonymous_ = anonymous; }
    bool GetAnonymous() const { return anonymous_; }

private:
    static TraceConfig &GetImpl();

    unsigned char trace_level_{OOPS_TRACE_LEVEL_INFO};
    bool anonymous_{false};
};

// Record数据结构，用于存储及输出、打印
struct Location { // 打点位置相关
    Location() = default;
    Location(const char *label, const char *file, int line) : label{label}, file{file}, line{line} {}

    std::string GetLabelStr() const;
    std::string GetLocationStr() const;

    const char *label{};
    const char *file{};
    int line{-1};
    int anonymous_id{-1};
};

struct TimePoint {
    static TimePoint Get();
    std::chrono::high_resolution_clock::time_point time_point{};
};

struct TimeInterval {
    TimeInterval &operator+=(const TimeInterval &rhs);
    TimeInterval operator-(const TimeInterval &rhs) const;
    double GetTime() const { return std::chrono::duration<double>(time).count(); }

    std::chrono::high_resolution_clock::duration time{};
};

TimeInterval operator-(const TimePoint &lhs, const TimePoint &rhs);

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
std::ostream &operator<<(std::ostream &out, const Sample &sample);

struct Record : public Sample {
    size_t count{}; // Node中获取
    size_t depth{}; // 递归时获取
};

struct RecordTable {
    void Append(const Record &log) { records.push_back(log); }
    void Output(std::ostream &out) const;

    TimeInterval root_itv;
    TimeInterval entry_itv;
    std::vector<Record> records;
};
std::ostream &operator<<(std::ostream &out, const RecordTable &sample);

// 存储所有Trace产生的Record，组织依赖关系
class TraceStore {
    class Node {
        friend class TraceStore;

    public:
        explicit Node(const void *location_id) : location_id_{location_id} {}

        void UpdateTime(const TimeInterval &time_inverval);
        void UpdateMemory(const Memory &memory);

        Location GetLocation() const;
        bool Empty() const;

    private:
        const void *location_id_{};
        size_t count_{};

        // 统计数据成员
        TimeInterval time_inverval_;
        Memory memory_;

        std::vector<int> children_;
    };

public:
    static auto &Get() {
        static TraceStore &stat{GetImpl()};
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
    static TraceStore &GetImpl(); // 避免跨编译单元产生多实例

    int GetOrCreateNode(int parent_i, const void *location_id);
    void SetupSameLevelNode(const void *location_id);

    TimeInterval GetRootItv() const;
    TimeInterval GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const;
    Record GetRecord(const Node &node, size_t depth) const;
    Record GetRecord(const TimeInterval &itv, size_t depth) const;

    std::vector<Node> nodes_{Node{nullptr}};
    std::vector<int> node_stack_{0};
    std::stack<TimePoint> scope_stack_; // 维护当前Scope前一个TimePoint，每次Trace，读取并刷新TimePoint
};

class TraceScopeBase {
protected:
    void ThrowCountMismatchException(
        unsigned char scope_count, unsigned char trace_count, const char *label, const char *file, int line);
};

template <unsigned char Level, typename>
class TraceScope : public TraceScopeBase {
public:
    TraceScope() : is_active_{TraceConfig::Get().GetTraceLevel() <= Level} {
        if (is_active_) {
            TraceStore::Get().TraceScopeBegin(&(++count_));
        }
    }
    ~TraceScope() {
        if (is_active_) {
            TraceStore::Get().TraceScopeEnd();
        }
    }

    // 做成类成员函数以便在编译器找到部分SCOPE和TRACE不匹配的问题
    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, size_t mask) {
        static unsigned char count{0};
        if (is_active_) {
            TraceImpl<Lambda>(++count, label, file, line);
            TraceStore::Get().Trace(&count, mask);
        }
    }

    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, const detail::TraceVaArgs &va_args) {
        static unsigned char count{0};
        if (is_active_) {
            TraceImpl<Lambda>(++count, label, file, line);
            TraceStore::Get().Trace(&count, va_args);
        }
    }

private:
    template <typename>
    void TraceImpl(unsigned char count, const char *label, const char *file, int line) {
        if (count != count_) {
            ThrowCountMismatchException(count_, count, label, file, line);
        }
        static bool location_set{false};
        if (!location_set) {
            TraceStore::Get().SetLocation(label, file, line);
            location_set = true;
        }
    }

    static inline unsigned char count_{0};
    bool is_active_{};
};

struct EmptyScope {
    template <typename... Args>
    void Trace(Args &&...) {}
};

template <unsigned char Level, typename Lambda>
auto MakeTraceScope(Lambda &&) {
    return TraceScope<Level, Lambda>{};
}
} // namespace oops
