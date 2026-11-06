#pragma once
namespace ams::ble::gatt { void OnEvent(const EventRecord &); }
namespace ams::ble::events {
inline bool disconnected = false, radio = true, completed = true;
inline u32 failure = 0;
inline u64 now_ms = 0;
struct ScheduledRecord { u64 at_ms; EventRecord event; };
inline std::deque<ScheduledRecord> queue;
constexpr int InvalidType = -1;
constexpr int Disconnect = -2, RadioOff = -3;
inline void Schedule(const EventRecord &event, u64 delay_ms = 0) {
    queue.push_back({now_ms + delay_ms, event});
    std::stable_sort(queue.begin(), queue.end(), [](const auto &a, const auto &b) { return a.at_ms < b.at_ms; });
}
inline bool IsSlotDisconnected(u32) { return disconnected; }
inline bool IsRadioEnabled() { return radio; }
inline u32 GetGattFailure(u32) { return failure; }
inline u32 GetGattResultCount(u32) { return 0; }
inline bool WaitForGattResult(u32, u32, TimeSpan) { return completed && !disconnected; }
template<typename F> bool WaitFor(int want, TimeSpan timeout, EventRecord *out, F predicate) {
    const auto deadline = now_ms + timeout.milliseconds;
    for (;;) {
        while (!queue.empty() && queue.front().at_ms <= now_ms) {
            auto record = queue.front().event; queue.pop_front();
            if (record.type == Disconnect) disconnected = true;
            if (record.type == RadioOff) radio = false;
            gatt::OnEvent(record);
            if ((want == InvalidType || record.type == want) && predicate(record)) {
                if (out) *out = record;
                return true;
            }
        }
        // State changes can wake an InvalidType predicate with no queued event.
        if (want == InvalidType && predicate(EventRecord{InvalidType, {}})) return true;
        if (queue.empty() || queue.front().at_ms > deadline) {
            now_ms = deadline;
            return false;
        }
        now_ms = std::max(now_ms, queue.front().at_ms);
    }
}
inline void Pump(TimeSpan timeout) { WaitFor(InvalidType, timeout, nullptr, [](const EventRecord &) { return false; }); }
}
