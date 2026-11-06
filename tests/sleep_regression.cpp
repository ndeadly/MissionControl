#include "ble_sleep.cpp"
#include <vector>
#include <iostream>

using namespace ams;
using namespace ams::ble;
static std::vector<u32> disconnected;
static std::vector<u8> cancelled;
static int stop_calls;
static bool complete_disconnects, late_connection, reject_disconnect;

Result btdrvStopBleScan() {
    assert(events::preparing && !events::ended);
    ++stop_calls;
    return {};
}
Result btdrvCancelConnectGattServer(u8 client, Address, bool direct) {
    assert(events::preparing && direct);
    cancelled.push_back(client);
    return {};
}
Result btdrvDisconnectGattServer(u32 id) {
    assert(events::preparing);
    disconnected.push_back(id);
    return {reject_disconnect ? 1u : 0u};
}
static void Tick() {
    if (late_connection && os::now_us == 20000) events::links[2].conn_id = 1029;
    if (complete_disconnects && os::now_us >= 40000) {
        for (auto &link : events::links) {
            if (std::find(disconnected.begin(), disconnected.end(), link.conn_id) != disconnected.end()) link.active = false;
        }
    }
}
static void Reset() {
    events::links = {};
    events::owns_radio = true;
    events::preparing = events::ended = false;
    os::now_us = 0;
    os::sleep_hook = Tick;
    stop_calls = 0;
    disconnected.clear(); cancelled.clear();
    complete_disconnects = true;
    late_connection = reject_disconnect = false;
}
int main() {
    Reset();
    for (u32 i = 0; i < 4; ++i) events::links[i] = {true, u8(i + 3), 3 + 257 * i, {}};
    PrepareForSleep();
    assert(disconnected == std::vector<u32>({3,260,517,774}));
    assert(cancelled == std::vector<u8>({3,4,5,6}));
    assert(stop_calls == 1 && events::ended && os::now_us == 40000);
    // A pending connect completing after cancellation must also be closed.
    Reset();
    late_connection = true;
    events::links[2] = {true, 5, InvalidConnId, {}};
    PrepareForSleep();
    assert(disconnected == std::vector<u32>({1029}));
    assert(cancelled == std::vector<u8>({5}));
    assert(events::ended && os::now_us == 40000);
    Reset();
    events::links[MaxBleControllers] = {true, InvalidClientIf, 1028, {}};
    PrepareForSleep();
    assert(disconnected == std::vector<u32>({1028}));
    assert(cancelled.empty() && events::ended && os::now_us == 40000);
    // Missing driver acknowledgements/errors use a single shared deadline.
    Reset();
    complete_disconnects = false;
    reject_disconnect = true;
    for (u32 i = 0; i < 4; ++i) events::links[i] = {true, u8(i + 3), 3 + 257 * i, {}};
    PrepareForSleep();
    assert(disconnected.size() == 4 && events::ended && os::now_us == 1000000);
    Reset();
    PrepareForSleep();
    assert(disconnected.empty() && cancelled.empty() && events::ended && os::now_us == 0);
    Reset();
    events::owns_radio = false;
    PrepareForSleep();
    assert(stop_calls == 0 && events::ended);
    std::cout << "BLE production sleep teardown regressions passed\n";
}
