#pragma once

#include <atomic>
#include <cstdint>

// Small, lock-free diagnostics helpers for cross-thread stats. Pure logic (unit-tested in
// tests/test_core.cpp): the gamepad poll thread records samples while the browser main thread
// reads them once per stats window, with no locks on the poll path.

namespace tsl {

// Raise *slot* to *value* if it is larger, atomically. Relaxed: these are diagnostic counters,
// not a synchronisation point.
inline void AtomicMax(std::atomic<uint32_t> &slot, uint32_t value) {
    uint32_t prev = slot.load(std::memory_order_relaxed);
    while (value > prev && !slot.compare_exchange_weak(prev, value, std::memory_order_relaxed)) {
        // prev was reloaded with the current value; loop until the max sticks or value <= prev.
    }
}

// "Worst value this window" with a session-wide high-water mark. Producers call Record() from any
// thread; a single reader calls TakeWindow() once per stats window (which also resets the window)
// and Max() for the session peak.
class MaxWindow {
public:
    void Record(uint32_t value) {
        AtomicMax(window_, value);
        AtomicMax(max_, value);
    }

    // The window's worst value, then reset the window to 0 for the next one. Max() is untouched.
    uint32_t TakeWindow() { return window_.exchange(0, std::memory_order_relaxed); }

    uint32_t Window() const { return window_.load(std::memory_order_relaxed); }
    uint32_t Max() const { return max_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint32_t> window_{0};
    std::atomic<uint32_t> max_{0};
};

}  // namespace tsl
