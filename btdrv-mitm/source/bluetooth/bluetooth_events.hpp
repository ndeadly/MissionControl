#pragma once
#include <stratosphere.hpp>

#include "bluetooth_core.hpp"
#include "bluetooth_hid.hpp"
#include "bluetooth_hid_report.hpp"
#include "bluetooth_ble.hpp"

namespace ams::bluetooth::events {

    enum BtdrvEventType {
        BtdrvEventType_PscPm,
        BtdrvEventType_BluetoothCore,
        BtdrvEventType_BluetoothHid,
        BtdrvEventType_BluetoothBle,
    };

    Result Initialize(void);
    void Finalize(void);

}
