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

    void HandleEvent(void);

}
