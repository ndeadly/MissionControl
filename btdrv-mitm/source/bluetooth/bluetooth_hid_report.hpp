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

    Result Initialize(Handle eventHandle);
    void Finalize(void);

    Result MapRemoteSharedMemory(Handle handle);
    Result InitializeReportBuffer(void);

    Result WriteFakeHidData(const bluetooth::Address *address, const bluetooth::HidReport *data);

    Result GetEventInfo(bluetooth::HidEventType *type, u8* buffer, size_t size);
    void HandleEvent(void);

}
