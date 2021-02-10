/*
 * Copyright (c) 2020-2021 ndeadly
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

namespace ams::bluetooth::core {

    bool IsInitialized(void);
    void WaitInitialized(void);
    void SignalEnabled(void);
    void WaitEnabled(void);

    os::SystemEvent *GetSystemEvent(void);
    os::SystemEvent *GetForwardEvent(void);
    os::SystemEvent *GetUserForwardEvent(void);

    Result Initialize(Handle event_handle);
    void Finalize(void);

    Result GetEventInfo(bluetooth::EventType *type, uint8_t* buffer, size_t size);
    void HandleEvent(void);
   
}
