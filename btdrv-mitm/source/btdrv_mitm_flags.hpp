#pragma once
#include <atomic>

namespace ams {

    extern std::atomic<bool> g_redirectCoreEvents;
    extern std::atomic<bool> g_redirectHidEvents;
    extern std::atomic<bool> g_redirectHidReportEvents;
    extern std::atomic<bool> g_redirectBleEvents;

}
