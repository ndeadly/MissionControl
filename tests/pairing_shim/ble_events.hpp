#pragma once
namespace ams::ble::events {
inline bool radio = true, disconnected = false, bond = false;
inline u16 reason = 0;
inline u32 conn_id = InvalidConnId;
inline u64 now_ms = 0;
inline bluetooth::Address address = {};
constexpr int InvalidType = -1, Bond = -2, Sleep = -3, OldTeardown = -4;
struct ScheduledRecord { u64 at_ms; EventRecord event; };
inline std::deque<ScheduledRecord> queue;
inline void Schedule(int type, u64 delay_ms, EventRecord record = {}) {
    record.type = type;
    queue.push_back({now_ms + delay_ms, record});
    std::stable_sort(queue.begin(), queue.end(), [](const auto &a, const auto &b) { return a.at_ms < b.at_ms; });
}
inline bool IsRadioEnabled() { return radio; }
inline bool IsSlotDisconnected(u32) { return disconnected; }
inline u32 GetSlotConnectionId(u32) { return disconnected ? InvalidConnId : conn_id; }
inline u16 GetSlotDisconnectReason(u32) { return reason; }
inline bool TakeSlotBondComplete(u32) { bool result = bond; bond = false; return result; }
inline void SetSlotConnecting(u32, const bluetooth::Address &addr) { address = addr; disconnected = bond = false; reason = 0; conn_id = InvalidConnId; }
template<typename F> bool WaitFor(int want, TimeSpan timeout, EventRecord *out, F predicate) {
    const auto deadline = now_ms + timeout.milliseconds;
    for (;;) {
        while (!queue.empty() && queue.front().at_ms <= now_ms) {
            auto record = queue.front().event; queue.pop_front();
            if (record.type == Bond) bond = true;
            if (record.type == Sleep) radio = false;
            if (record.type == OldTeardown) bond = disconnected = true;
            if (record.type == BtdrvBleEventType_ClientConnection && AddressEqual(record.info.client_connection.address, address)) {
                disconnected = record.info.client_connection.status != BleConnectionStatus_Connected;
                conn_id = record.info.client_connection.conn_id;
                reason = record.info.client_connection.reason;
            }
            if ((want == InvalidType || record.type == want) && predicate(record)) {
                if (out) *out = record;
                return true;
            }
        }
        if (want == InvalidType && predicate(EventRecord{InvalidType, {}})) {
            if (out) *out = {InvalidType, {}};
            return true;
        }
        if (queue.empty() || queue.front().at_ms > deadline) { now_ms = deadline; return false; }
        now_ms = std::max(now_ms, queue.front().at_ms);
    }
}
}
