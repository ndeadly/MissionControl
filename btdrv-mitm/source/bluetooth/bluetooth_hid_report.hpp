/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

    SharedMemory *GetRealSharedMemory(void);
    SharedMemory *GetFakeSharedMemory(void);

    os::SystemEventType *GetSystemEvent(void);
    os::SystemEventType *GetForwardEvent(void);
    os::SystemEventType *GetUserForwardEvent(void);

    Result Initialize(Handle eventHandle, Service *forwardService, os::ThreadId mainThreadId);
    void Finalize(void);

    Result MapRemoteSharedMemory(Handle handle);
    Result InitializeReportBuffer(void);

    Result WriteHidReportBuffer(const bluetooth::Address *address, const bluetooth::HidReport *report);
    Result SendHidReport(const bluetooth::Address *address, const bluetooth::HidReport *report);

    Result GetEventInfo(bluetooth::HidEventType *type, u8* buffer, size_t size);
    void HandleEvent(void);

}
