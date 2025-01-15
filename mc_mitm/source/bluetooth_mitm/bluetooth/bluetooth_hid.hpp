/*
 * Copyright (c) 2020-2025 ndeadly
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
#include "bluetooth_types.hpp"

namespace ams::bluetooth::hid {

    bool IsInitialized();
    void SignalInitialized();
    void WaitInitialized();

    os::SystemEvent *GetSystemEvent();
    os::SystemEvent *GetForwardEvent();
    os::SystemEvent *GetUserForwardEvent();

    void SignalFakeEvent(bluetooth::HidEventType type, const void *data, size_t size);
    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size);
    void HandleEvent();

}
