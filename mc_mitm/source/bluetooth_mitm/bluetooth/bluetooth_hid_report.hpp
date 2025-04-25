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

namespace ams::bluetooth::hid::report {

    bool IsInitialized();
    void WaitInitialized();
    void SignalInitialized();

    void ForwardHidReportEvent();
    void ConsumeHidReportEvent();

    os::SharedMemory *GetRealSharedMemory();
    os::SharedMemory *GetFakeSharedMemory();

    os::SystemEvent *GetSystemEvent();
    os::SystemEvent *GetForwardEvent();
    os::SystemEvent *GetUserForwardEvent();

    Result Initialize();
    void Finalize();

    Result MapRemoteSharedMemory(os::NativeHandle handle);
    Result InitializeReportBuffer();

    Result WriteHidDataReport(const bluetooth::Address address, const bluetooth::HidReport *report);
    Result WriteHidSetReport(const bluetooth::Address address, u32 status);
    Result WriteHidGetReport(const bluetooth::Address address, const bluetooth::HidReport *report);

    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size);
    void HandleEvent();

}
