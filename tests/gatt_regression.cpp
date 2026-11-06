// Compile the production GATT implementation unchanged against a timed IPC shim.
#include "ble_gatt.cpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace ams::ble;

enum class DiscoveryMode { AfterSettling, AfterMtu, AfterBond, Never };
static DiscoveryMode mode;
static Result register_result{0}, query_result{0}, mtu_result{0};
static bool bonded, mtu_reply;
static int unregister_count;
static u64 connected_at;
static std::vector<u64> query_times;
static std::vector<u16> read_uuids;
static std::vector<u32> mtu_connections;

static EventRecord Cache(u32 conn_id) {
    EventRecord e{};
    e.type = BtdrvBleEventType_ClientCacheSave;
    auto &c = e.info.client_cache_save;
    c.conn_id = conn_id; c.count = 6;
    c.attr_list[0] = {BtdrvGattAttributeType_Service, 1, 20, 0, true, {{GattServiceHid}}};
    c.attr_list[1] = {BtdrvGattAttributeType_Characteristic, 2, 0, BtdrvGattCharacteristicProperty_Notify, true, {{GattCharReport}}};
    c.attr_list[2] = {BtdrvGattAttributeType_Descriptor, 3, 0, 0, true, {{GattDescClientCharConfig}}};
    c.attr_list[3] = {BtdrvGattAttributeType_Characteristic, 4, 0, BtdrvGattCharacteristicProperty_Write, true, {{GattCharReport}}};
    c.attr_list[4] = {BtdrvGattAttributeType_Characteristic, 5, 0, 0, true, {{GattCharHidInformation}}};
    c.attr_list[5] = {BtdrvGattAttributeType_Characteristic, 6, 0, 0, true, {{GattCharReportMap}}};
    return e;
}

static EventRecord Mtu(u32 conn_id) {
    EventRecord e{};
    e.type = BtdrvBleEventType_ClientConfigureMtu;
    e.info.client_configure_mtu = {0, conn_id, 256};
    return e;
}

static void PublishCache(u32 conn_id) {
    // The first chunk has usable input, but output and authentication metadata
    // arrive later. Readiness must not snapshot only the first chunk.
    auto first = Cache(conn_id), second = first;
    first.info.client_cache_save.count = 3;
    auto &c = second.info.client_cache_save;
    c.count = 3;
    for (int i = 0; i < 3; ++i) c.attr_list[i] = c.attr_list[i + 3];
    events::Schedule(first, 20);
    events::Schedule(second, 80);
}

Result btdrvGetGattAttribute(ams::bluetooth::Address, u32 conn_id) {
    query_times.push_back(events::now_ms);
    // Match the observed successful IPC with no callbacks when called too early.
    if (R_SUCCEEDED(query_result) && events::now_ms >= connected_at + 1500 &&
        (mode == DiscoveryMode::AfterSettling || (mode == DiscoveryMode::AfterBond && bonded))) {
        PublishCache(conn_id);
    }
    return query_result;
}
Result btdrvConfigureAttMtu(u32 conn_id, u16 mtu) {
    assert(mtu == 0x100);
    mtu_connections.push_back(conn_id);
    if (R_SUCCEEDED(mtu_result)) {
        if (mode == DiscoveryMode::AfterMtu) PublishCache(conn_id);
        if (mtu_reply) events::Schedule(Mtu(conn_id), 100);
    }
    return mtu_result;
}
Result btdrvReadGattCharacteristic(u32, bool, const BtdrvGattId *, const BtdrvGattId *attr, int) {
    read_uuids.push_back(attr->uuid.value);
    return {0};
}
Result btdrvRegisterGattNotification(u32, bool, const BtdrvGattId *, const BtdrvGattId *) { return register_result; }
Result btdrvUnregisterGattNotification(u32, bool, const BtdrvGattId *, const BtdrvGattId *) { ++unregister_count; return {0}; }
Result btdrvWriteGattDescriptor(u32, bool, const BtdrvGattId *, const BtdrvGattId *, const BtdrvGattId *, const u8 *, size_t, int) { return {0}; }

static void Reset(DiscoveryMode next = DiscoveryMode::AfterSettling) {
    for (auto &slot : gatt::g_slots) slot = {};
    events::queue.clear();
    events::disconnected = false;
    events::radio = events::completed = true;
    events::failure = 0;
    connected_at = events::now_ms = 0;
    mode = next;
    bonded = false;
    mtu_reply = true;
    register_result = query_result = mtu_result = {0};
    unregister_count = 0;
    query_times.clear();
    read_uuids.clear();
    mtu_connections.clear();
}

static void AssertSubscribed(u32 slot, u32 conn_id) {
    gatt::HidService hid{};
    assert(R_SUCCEEDED(gatt::Subscribe(slot, conn_id, &hid)));
    assert(hid.has_output_report && hid.output_report.handle == 4);
    assert(read_uuids.size() >= 2);
    assert(read_uuids[read_uuids.size() - 2] == GattCharHidInformation);
    assert(read_uuids.back() == GattCharReportMap);
}

int main() {
    const ams::bluetooth::Address a{{1}}, b{{2}};
    // No preloaded attribute events: the fake driver only produces them in
    // response to a request issued after connection setup has progressed.
    Reset();
    assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
    assert(query_times.size() == 1 && query_times[0] >= 2000);
    assert(mtu_connections == std::vector<u32>{260});
    AssertSubscribed(0, 260);
    gatt::Unsubscribe(0, 260);
    assert(unregister_count == 1);

    Reset(DiscoveryMode::AfterMtu);
    assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
    AssertSubscribed(0, 260);

    // Critical regression: empty pre-bond table must not prevent CreateBond.
    // Emulate the coordinator completing pairing, then receiving attributes.
    Reset(DiscoveryMode::AfterBond);
    assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
    assert(gatt::g_slots[0].table.count == 0);
    bonded = true;
    AssertSubscribed(0, 260);
    assert(query_times.size() == 2);
    assert(gatt::g_slots[0].table.count == 6);

    // An MTU timeout/refusal is optional, not proof of an unusable HID link.
    for (bool timeout : {true, false}) {
        Reset();
        mtu_reply = false;
        if (!timeout) mtu_result = {7};
        assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
        AssertSubscribed(0, 260);
        assert(events::now_ms == (timeout ? 7500u : 2500u));
    }

    // A different controller's MTU response must not finish this setup wait.
    Reset(DiscoveryMode::Never);
    mtu_reply = false;
    events::Schedule(Mtu(3), 2100);
    assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
    assert(events::now_ms == 7500);
    gatt::HidService hid{};
    assert(gatt::Subscribe(0, 260, &hid).value == 2);
    assert(events::now_ms <= 13000); // post-bond failure is still bounded

    // Cancellation is honored during both settling and the MTU wait.
    for (int kind : {events::Disconnect, events::RadioOff}) {
        for (u64 when : {500u, 2050u, 2300u}) {
            Reset();
            events::Schedule(EventRecord{kind, {}}, when);
            assert(gatt::Discover(0, 260, a).value == 4);
            assert(events::now_ms == when);
            if (when < 2000) assert(query_times.empty() && mtu_connections.empty());
        }
    }

    Reset();
    assert(R_SUCCEEDED(gatt::Discover(0, 260, a)));
    AssertSubscribed(0, 260);
    gatt::ResetSlot(0);
    assert(gatt::HasCachedAddress(0, a));
    // No new discovery events on reconnect: reuse the address cache in another slot.
    mode = DiscoveryMode::Never;
    assert(R_SUCCEEDED(gatt::Discover(1, 774, a)));
    AssertSubscribed(1, 774);
    auto late = Cache(260); late.info.client_cache_save.attr_list[0].id.uuid.value = 0;
    gatt::OnEvent(late);
    AssertSubscribed(1, 774);
    for (int i = 0; i < 100; ++i) gatt::OnEvent(Cache(774));
    assert(gatt::TakeRediscovered(1));
    assert(gatt::g_slots[1].table.count == 6);
    // Polling rediscovery between chunks must not clear all previous handles.
    auto chunk = Cache(774);
    chunk.info.client_cache_save.count = 1;
    chunk.info.client_cache_save.attr_list[0] = chunk.info.client_cache_save.attr_list[3];
    gatt::OnEvent(chunk);
    assert(gatt::TakeRediscovered(1));
    assert(gatt::g_slots[1].table.count == 6);
    AssertSubscribed(1, 774);

    register_result = {7};
    assert(gatt::Subscribe(1, 774, &hid).value == 7);
    register_result = {0};
    events::completed = false;
    assert(gatt::Subscribe(1, 774, &hid).value == 2);
    events::completed = true;
    events::disconnected = true;
    assert(gatt::Subscribe(1, 774, &hid).value == 4);
    assert(gatt::Discover(1, 774, a).value == 4);
    events::disconnected = false;
    assert(R_SUCCEEDED(gatt::Discover(1, 1028, b)));
    assert(gatt::g_slots[1].table.count == 0); // new address cannot inherit attributes
    query_result = {7};
    assert(gatt::Subscribe(1, 1028, &hid).value == 7);
    assert(gatt::Discover(4, 1028, b).value == 1);

    std::cout << "GATT delayed discovery/pairing order/MTU/cache/cancellation regressions passed\n";
}
