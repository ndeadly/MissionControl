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

namespace ams::mitm::btm::condition {

    /*
     * hid learns which controllers are connected from btm's device condition list and is told to re-read it
     * through the event from AcquireDeviceConditionEvent. To be able to present controllers btm knows nothing
     * about (Bluetooth LE ones), the mitm hands hid its own event, relays btm's real signals to it, and
     * signals it itself whenever the set of such controllers changes.
     */

    bool IsInitialized();
    void SignalInitialized();

    os::SystemEvent *GetSystemEvent();
    os::SystemEvent *GetForwardEvent();

    // Tell hid that the device list changed
    void Signal();

    Result Initialize();

}
