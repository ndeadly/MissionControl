#pragma once
#include <stratosphere.hpp>

#include "bluetooth_core_events.hpp"
#include "bluetooth_hid_events.hpp"
#include "bluetooth_hid_report_events.hpp"
#include "bluetooth_ble_events.hpp"

namespace ams::bluetooth::events {

    enum BtdrvEventType {
        BtdrvEventType_PscPm,
        BtdrvEventType_BluetoothCore,
        BtdrvEventType_BluetoothHid,
        BtdrvEventType_BluetoothBle,
    };

    void    AttachWaitHolder(BtdrvEventType type);
    Result  InitializeSystemEvents(void);
    Result  StartEventHandlerThread(void);

}
