#pragma once
#include <atomic>

namespace ams {

    extern std::atomic<bool> g_preparingForSleep;
    extern std::atomic<bool> g_redirectEvents;
    extern std::atomic<bool> g_redirectHidReportEvents;

}
