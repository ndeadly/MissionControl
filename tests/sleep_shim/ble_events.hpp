#pragma once
#include "switch.h"
#include "stratosphere.hpp"
#include <array>
#include <algorithm>

namespace ams::ble {
    constexpr u32 MaxBleControllers = 4, InvalidConnId = UINT32_MAX;
    constexpr u8 InvalidClientIf = UINT8_MAX;
    using Address = std::array<u8, 6>;
}
namespace ams::ble::events {
    struct SleepLink { bool active; u8 client_if; u32 conn_id; Address address; };
    inline std::array<SleepLink, MaxBleControllers + 1> links;
    inline bool preparing = false, ended = false, owns_radio = true;
    inline bool BeginSleep() { preparing = true; ended = false; return owns_radio; }
    inline void EndSleep() { preparing = false; ended = true; }
    inline SleepLink GetSleepLink(u32 slot) { return links.at(slot); }
}
Result btdrvStopBleScan();
Result btdrvCancelConnectGattServer(u8, ams::ble::Address, bool);
Result btdrvDisconnectGattServer(u32);
