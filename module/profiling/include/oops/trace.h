#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "oops/trace_detail.h"

#include "oops/format.h"
#include "oops/once.h"
#include "oops/str.h"

// 定义静态TRACE等级
#define OOPS_TRACE_LEVEL_VERBOSE 0
#define OOPS_TRACE_LEVEL_DEBUG   1
#define OOPS_TRACE_LEVEL_INFO    2
#define OOPS_TRACE_LEVEL_OFF     3

#ifndef OOPS_TRACE_LEVEL
#define OOPS_TRACE_LEVEL OOPS_TRACE_LEVEL_INFO // 默认等级INFO
#endif

// 定义TRACE_SCOPE
#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_VERBOSE
#define OOPS_TRACE_SCOPE_VERBOSE() auto oops_trace_scope__(::oops::MakeTraceScope<OOPS_TRACE_LEVEL_VERBOSE>([] {}))
#elif OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE_SCOPE_VERBOSE() ::oops::EmptyScope oops_trace_scope__
#else
#define OOPS_TRACE_SCOPE_VERBOSE() (void)0
#endif

#if OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_DEBUG
#define OOPS_TRACE_SCOPE_DEBUG() auto oops_trace_scope__(::oops::MakeTraceScope<OOPS_TRACE_LEVEL_DEBUG>([] {}))
#elif OOPS_TRACE_LEVEL <= OOPS_TRACE_LEVEL_INFO
#define OOPS_TRACE_SCOPE_DEBUG() ::oops::EmptyScope oops_trace_scope__
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
    oops_trace_scope__.Trace([] {}, label, __FILE__, __LINE__, ::oops::detail::ParseTraceVaArgs(__VA_ARGS__))
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

    std::thread::id thread_id_;
    TimeInterval root_itv;
    TimeInterval entry_itv;
    std::vector<Record> records;
};
std::ostream &operator<<(std::ostream &out, const RecordTable &sample);

class ParallelTraceStore;

// 存储Trace产生的Record，组织依赖关系
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
    TraceStore() = default;
    TraceStore(ParallelTraceStore *p_store);

    void Clear();

    void SetLocation(const char *label, const char *file, int line);
    void TraceScopeBegin(const void *location_id);
    void TraceScopeEnd();

    void Trace(const void *location_id, size_t mask);
    void Trace(const void *location_id, const detail::TraceVaArgs &va_args);

    RecordTable GetRecordTable(const char *label) const;
    RecordTable GetRecordTable() const;

private:
    int GetOrCreateNode(int parent_i, const void *location_id);
    void SetupSameLevelNode(const void *location_id);

    TimeInterval GetRootItv() const;
    TimeInterval GetRecordTableImpl(int node_i, size_t depth, RecordTable &record_table) const;
    Record GetRecord(const Node &node, size_t depth) const;
    Record GetRecord(const TimeInterval &itv, size_t depth) const;

    std::thread::id thread_id_{std::this_thread::get_id()};
    std::vector<Node> nodes_{Node{nullptr}};
    std::vector<int> node_stack_{0};
    std::stack<TimePoint> scope_stack_; // 维护当前Scope前一个TimePoint，每次Trace，读取并刷新TimePoint
};

struct ParallelRecordTable {
    void Output(std::ostream &out) const;
    std::vector<RecordTable> record_tables;
};
std::ostream &operator<<(std::ostream &out, const RecordTable &sample);

// 存储全局Record，每个线程拥有一个TraceStore
class ParallelTraceStore {
public:
    // 获取默认全局实例
    static auto &Get() {
        static ParallelTraceStore &stat{GetImpl()};
        return stat;
    }

    void Clear() {
        for (auto &local : local_trace_stores_) {
            local.Clear();
        }
    };

    void SetLocation(const char *label, const char *file, int line) { GetLocalImpl().SetLocation(label, file, line); }
    void TraceScopeBegin(const void *location_id) { GetLocalImpl().TraceScopeBegin(location_id); }
    void TraceScopeEnd() { GetLocalImpl().TraceScopeEnd(); }

    void Trace(const void *location_id, size_t mask) { GetLocalImpl().Trace(location_id, mask); }
    void Trace(const void *location_id, const detail::TraceVaArgs &va_args) {
        GetLocalImpl().Trace(location_id, va_args);
    }

    ParallelRecordTable GetRecordTable() const;

private:
    // TODO(resserops): 由于thread local是全局作用域，当前ParallelTraceStore定义为单例，后续通过local manager支持多实例
    ParallelTraceStore() = default;
    static ParallelTraceStore &GetImpl();

    TraceStore &GetLocal() {
        thread_local TraceStore &stat{GetLocalImpl()};
        return stat;
    }
    TraceStore &GetLocalImpl();
    TraceStore &CreateLocalTraceStore();

    std::deque<TraceStore> local_trace_stores_;
    std::mutex mutex_;
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
            ++count_;
            ParallelTraceStore::Get().TraceScopeBegin(&location_);
        }
    }

    ~TraceScope() {
        if (is_active_) {
            ParallelTraceStore::Get().TraceScopeEnd();
        }
    }

    // 做成类成员函数以便在编译器找到部分SCOPE和TRACE不匹配的问题
    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, size_t mask) {
        static unsigned char location{};
        thread_local unsigned char count{0};
        if (is_active_) {
            TraceImpl<Lambda>(++count, label, file, line);
            ParallelTraceStore::Get().Trace(&location, mask);
        }
    }

    template <typename Lambda>
    void Trace(Lambda &&, const char *label, const char *file, int line, const detail::TraceVaArgs &va_args) {
        static unsigned char location{};
        thread_local unsigned char count{0};
        if (is_active_) {
            TraceImpl<Lambda>(++count, label, file, line);
            ParallelTraceStore::Get().Trace(&location, va_args);
        }
    }

private:
    template <typename>
    void TraceImpl(unsigned char count, const char *label, const char *file, int line) {
        if (count != count_) {
            ThrowCountMismatchException(count_, count, label, file, line);
        }
        thread_local bool location_set{false};
        if (!location_set) {
            ParallelTraceStore::Get().SetLocation(label, file, line);
            location_set = true;
        }
    }

    static inline unsigned char location_{};
    static thread_local inline unsigned char count_{0};
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
