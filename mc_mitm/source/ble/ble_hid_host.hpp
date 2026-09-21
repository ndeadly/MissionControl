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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::ble {

    /*
     * HID-over-GATT host for Bluetooth LE controllers (Xbox controllers on firmware 5.x and newer).
     *
     * btm leaves BLE disabled on a retail console and never connects to LE devices, so this module acts as the
     * GATT client itself: it brings BLE up, finds the controller (a remembered one by connecting to it directly,
     * a new one by scanning for a gamepad in pairing mode), pairs when the link needs it, subscribes to the HID
     * input Report characteristic and presents the device to hid as a Pro Controller. Input reports arrive on
     * btdrv's LE HID event stream and are handed to the controller driver.
     */

    // True when the host runs. hid only learns of a controller btm doesn't manage through the device condition
    // list of 13.0.0 and later, so the rest of the mitm must make way for the host on those versions only.
    bool IsEnabled();

    void Launch();
    void WaitFinished();

}
