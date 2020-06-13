#include "btdrv_mitm_flags.hpp"

std::atomic<bool> g_preparingForSleep   = false;
std::atomic<bool> g_redirectEvents      = false;
