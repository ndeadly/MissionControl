#pragma once

#include <atomic>
#include <memory>

namespace mc::bluetooth::hid {

    const constexpr size_t event_buffer_size = 0x480;
    const constexpr size_t thread_stack_size = 0x4000;

    extern std::atomic<bool> exitFlag;

    void Initialise(void);
    void Cleanup(void);
    void PrepareForSleep(void);
    void PrepareForWake(void);
    void OnWake(void);

}
