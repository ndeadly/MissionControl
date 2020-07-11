#include "btdrv_mitm_flags.hpp"

namespace ams {

    std::atomic<bool> g_preparingForSleep       = false;
    std::atomic<bool> g_redirectEvents          = false;
    std::atomic<bool> g_redirectHidReportEvents = false;

}
