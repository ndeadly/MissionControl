/*
 * Copyright (c) 2020-2026 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "ble_types.hpp"

namespace ams::ble::events {

    /*
     * btdrv delivers BLE events to a single consumer (btm). The btdrv mitm offers every event here first: a
     * copy is queued for the host thread, and the events btm must not see are withheld. btm aborts when handed
     * events for a scan or connection it didn't make, and makes neither while this host holds BLE, so what
     * reaches it is decided from the host's declared state rather than by matching ids: the stack emits
     * follow-up events before the host thread could have recorded them.
     */

    /* Hooks called from the btdrv event threads */
    bool OnBleEvent(bluetooth::BleEventType type, const bluetooth::BleEventInfo *info);
    bool OnCoreEvent(u32 type, const bluetooth::EventInfo *info);

    /* State declared by the host thread */
    void Activate();
    void TakeOwnership();
    void SetRegistering(bool registering);
    void SetScanning(bool scanning);
    void ExpectFilterEvent();

    void SetTarget(const bluetooth::Address *address);
    bool GetTarget(bluetooth::Address *out);
    void SetConnection(u32 conn_id);
    u32 GetConnection();
    bool IsDisconnected();
    u16 GetDisconnectReason();

    // Results of our GATT requests arrive on the LE HID stream; the host thread reports each one here so a
    // request is not issued while the previous one is still pending
    void NoteGattResult(u32 result);
    u32 GetGattResultCount();
    bool WaitForGattResult(u32 count_before, TimeSpan timeout);
    u32 GetGattFailure();       // last non-zero result since ClearGattFailure
    void ClearGattFailure();

    // A link that came up for an address the host tried to reach while no target is set (a cancelled request
    // completing anyway) is held for the host to take over
    bool TakeAdoptedConnection(bluetooth::Address *out_address, u32 *out_conn_id);
    bool HasAdoptedConnection();

    // The remembered controller, whose pairing requests are answered on its behalf
    void SetClaimedAddress(const bluetooth::Address *address);

    // btm runs classic inquiry while the Controllers screen is open; that is the window for pairing new pads
    void SetInquiryActive(bool active);
    bool IsPairingWindowOpen();

    // Set when the stack reports the bond with the target complete (core Connection event)
    bool TakeBondComplete();

    // The console's sleep state (from the psc power module): no radio activity is allowed while asleep
    void SetRadioEnabled(bool enabled);
    bool IsRadioEnabled();
    void WaitRadioEnabled();
    bool TakeRadioCycled();

    // Last GATT client btm registered, usable as ours since btm never connects with it on a retail console
    u8 GetSpareClientIf();

    /* Queue consumed by the host thread */
    u32 TakeRingDropped();
    bool Pop(EventRecord *out);
    void WaitForQueue(TimeSpan timeout);

    using Sink = void (*)(const EventRecord &);
    void SetSink(Sink sink);
    Sink GetSink();

    void Log(const EventRecord &record);
    void LogAddress(const char *label, const bluetooth::Address &address);

    // Drains the queue until an event of the requested type satisfying pred arrives or the timeout expires.
    // Every event seen is logged and applied to the sink. InvalidType matches any type.
    constexpr bluetooth::BleEventType InvalidType = static_cast<bluetooth::BleEventType>(0xFFFF);

    template<typename F>
    bool WaitFor(bluetooth::BleEventType want, TimeSpan timeout, EventRecord *out, F pred) {
        static EventRecord s_record;

        const auto deadline = os::GetSystemTick().ToTimeSpan() + timeout;
        for (;;) {
            while (Pop(&s_record)) {
                Log(s_record);
                if (auto sink = GetSink(); sink) {
                    sink(s_record);
                }

                if (((want == InvalidType) || (s_record.type == want)) && pred(s_record)) {
                    if (out) {
                        *out = s_record;
                    }
                    return true;
                }
            }

            const auto now = os::GetSystemTick().ToTimeSpan();
            if (now >= deadline) {
                return false;
            }

            WaitForQueue(deadline - now);
        }
    }

    inline void Pump(TimeSpan duration) {
        WaitFor(InvalidType, duration, nullptr, [](const EventRecord &) { return false; });
    }

}
