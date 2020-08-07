#include "btdrv_mitm_flags.hpp"

namespace ams {

    std::atomic<bool> g_redirectCoreEvents      = false;
    std::atomic<bool> g_redirectHidEvents       = false;
    std::atomic<bool> g_redirectHidReportEvents = false;
    std::atomic<bool> g_redirectBleEvents       = false;

}
