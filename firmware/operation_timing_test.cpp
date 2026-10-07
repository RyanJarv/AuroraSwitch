// Host timing tests cover zero duration, session maximum, and clock wrap.
#include "../support/operation_timing.hpp"
#include <cassert>

int main()
{
    volatile daisy_development::OperationTiming timing{};
    timing.Begin(100);
    assert(timing.completed == 0 && timing.elapsed_ms == 0);
    timing.Finish(123);
    assert(timing.completed == 1 && timing.elapsed_ms == 23 && timing.maximum_ms == 23);
    timing.Begin(200);
    assert(timing.completed == 0 && timing.maximum_ms == 23);
    timing.Finish(200);
    assert(timing.elapsed_ms == 0 && timing.maximum_ms == 23);
    timing.Begin(0xfffffff0U);
    timing.Finish(0x20);
    assert(timing.elapsed_ms == 48 && timing.maximum_ms == 48);
    timing.Begin(400);
    assert(timing.completed == 0 && timing.elapsed_ms == 0 && timing.maximum_ms == 48);
    // A never-returning operation remains incomplete; no invented duration.
}
