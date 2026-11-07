#include "ble_pairing.cpp"
#include <cassert>
#include <iostream>
using namespace ams::ble;
static constexpr ams::bluetooth::Address Target = {{0x14,0xcb,0x65,0xe8,0x2b,0xe2}};
static unsigned removes, connects, cancels, bonds;
static u64 connected_at;
static bool remove_fails, connect_fails, respond, old_teardown, new_bond, bond_fails;
static Result Connect(bool returning = true) {
    u32 conn_id = InvalidConnId;
    return pairing::Connect(0, 3, Target, &conn_id, TimeSpan::FromSeconds(15), returning);
}
Result btdrvRemoveBond(ams::bluetooth::Address addr) {
    assert(AddressEqual(addr, Target)); ++removes;
    if (old_teardown) events::Schedule(events::OldTeardown, 200);
    return {remove_fails ? 5u : 0u};
}
Result btdrvConnectGattServer(u8 client, ams::bluetooth::Address addr, bool direct, u64 aruid) {
    assert(client == 3 && AddressEqual(addr, Target) && direct && aruid == 0);
    ++connects; connected_at = events::now_ms;
    if (respond) {
        EventRecord record;
        record.info.client_connection.address = Target;
        events::Schedule(BtdrvBleEventType_ClientConnection, 20, record);
    }
    return {connect_fails ? 6u : 0u};
}
Result btdrvCancelConnectGattServer(u8 client, ams::bluetooth::Address addr, bool direct) {
    assert(client == 3 && AddressEqual(addr, Target) && direct && events::radio);
    ++cancels; return {0};
}
Result btdrvCreateBond(ams::bluetooth::Address addr, u32 transport) {
    assert(AddressEqual(addr, Target) && transport == 2 && events::radio && !events::disconnected);
    ++bonds;
    if (new_bond) events::Schedule(events::Bond, 100);
    return {bond_fails ? 7u : 0u};
}
static void Reset() {
    events::queue.clear(); events::now_ms = 0;
    events::radio = true;
    events::SetSlotConnecting(0, Target);
    removes = connects = cancels = bonds = 0; connected_at = 0;
    remove_fails = connect_fails = old_teardown = bond_fails = false;
    respond = new_bond = true;
}
int main() {
    Reset(); old_teardown = true;
    assert(R_SUCCEEDED(Connect()));
    assert(removes == 1 && connects == 1 && cancels == 0 && connected_at == 500);
    assert(!events::disconnected && !events::bond);
    Reset(); assert(R_SUCCEEDED(Connect(false))); assert(connected_at == 0); // no added first-pair delay
    Reset();
    EventRecord late;
    late.info.client_connection.address = Target;
    events::Schedule(BtdrvBleEventType_ClientConnection, 100, late);
    u32 adopted = InvalidConnId;
    assert(R_SUCCEEDED(pairing::Connect(0, 3, Target, &adopted, TimeSpan::FromSeconds(15), true)));
    assert(adopted == 259 && connects == 0); // don't lose a late live transport
    Reset(); respond = false;
    EventRecord invalid;
    invalid.info.client_connection.address = Target;
    invalid.info.client_connection.conn_id = InvalidConnId;
    events::Schedule(BtdrvBleEventType_ClientConnection, 510, invalid);
    assert(R_FAILED(Connect()) && cancels == 1);
    Reset(); remove_fails = true; assert(R_FAILED(Connect())); assert(connects == 0);
    Reset(); connect_fails = true; assert(R_FAILED(Connect()));
    Reset(); events::Schedule(events::Sleep, 200); assert(R_FAILED(Connect()));
    assert(connects == 0 && cancels == 0 && events::now_ms == 200);
    Reset(); respond = false; assert(R_FAILED(Connect())); assert(cancels == 1 && events::now_ms == 15500);
    Reset(); respond = false; events::Schedule(events::Sleep, 700); assert(R_FAILED(Connect())); assert(cancels == 0);
    Reset(); respond = false;
    EventRecord wrong;
    wrong.info.client_connection.address = Target;
    wrong.info.client_connection.client_if = 4;
    events::Schedule(BtdrvBleEventType_ClientConnection, 510, wrong);
    assert(R_FAILED(Connect())); assert(cancels == 1); // another client's completion isn't ours
    Reset(); events::radio = false; assert(R_FAILED(Connect())); assert(removes == 0);
    Reset(); events::bond = true; new_bond = false;
    assert(!pairing::Pair(0, Target)); assert(bonds == 1 && events::now_ms == 10000); // stale indication
    Reset(); assert(pairing::Pair(0, Target)); assert(events::now_ms == 600);
    Reset(); bond_fails = true; assert(!pairing::Pair(0, Target)); assert(events::now_ms == 0);
    Reset();
    EventRecord refused;
    refused.info.client_connection.address = Target;
    refused.info.client_connection.status = 2;
    refused.info.client_connection.reason = 0x16;
    events::Schedule(BtdrvBleEventType_ClientConnection, 110, refused);
    assert(!pairing::Pair(0, Target)); assert(events::now_ms == 110);
    Reset(); events::Schedule(events::Sleep, 200); assert(!pairing::Pair(0, Target));
    Reset(); events::disconnected = true; assert(!pairing::Pair(0, Target)); assert(bonds == 0);
    Reset(); events::radio = false; assert(!pairing::Pair(0, Target)); assert(bonds == 0);
    std::cout << "BLE production reconnect/pairing regressions passed\n";
}
