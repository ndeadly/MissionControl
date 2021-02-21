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

namespace ams::bluetooth::hid::report {

    bool IsInitialized(void);
    void WaitInitialized(void);

    SharedMemory *GetRealSharedMemory(void);
    SharedMemory *GetFakeSharedMemory(void);

    os::SystemEvent *GetSystemEvent(void);
    os::SystemEvent *GetForwardEvent(void);
    os::SystemEvent *GetUserForwardEvent(void);

    Result Initialize(Handle event_handle, Service *forward_service, os::ThreadId main_thread_id);
    void Finalize(void);

    Result MapRemoteSharedMemory(Handle handle);
    Result InitializeReportBuffer(void);

    Result WriteHidReportBuffer(const bluetooth::Address *address, const bluetooth::HidReport *report);
    Result SendHidReport(const bluetooth::Address *address, const bluetooth::HidReport *report);

    Result GetEventInfo(bluetooth::HidEventType *type, uint8_t* buffer, size_t size);
    void HandleEvent(void);

}
