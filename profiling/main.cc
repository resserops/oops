#include "trace.h"

void func() {
    TRACE_SCOPE();
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    TRACE(func1);
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    TRACE(func2);
}

int main() {
    {
        TRACE_SCOPE();
        for (size_t i{0}; i < 10000; ++i) {
            TRACE_SCOPE();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            TRACE(a1);
            func();
            TRACE(a2);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            TRACE(a3);
        }
        TRACE_PRINT();
        TRACE(total);
    }
    std::cout << "\n";
    TRACE_PRINT();
    TRACE_OUTPUT(::std::cout, a2);
    return 0;
}