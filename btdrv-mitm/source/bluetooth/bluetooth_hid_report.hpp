#pragma once
#include <switch.h>
#include <stratosphere.hpp>

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
    Result InitializeFakeSharedMemory(void);

    Result WriteFakeHidData(const BluetoothAddress *address, const BluetoothHidData *data);

    Result GetEventInfo(HidEventType *type, u8* buffer, size_t size);
    void HandleEvent(void);

}
