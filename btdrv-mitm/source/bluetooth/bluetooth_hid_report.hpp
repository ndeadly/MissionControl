#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::bluetooth::hid::report {

    SharedMemory *GetRealSharedMemory(void);
    SharedMemory *GetFakeSharedMemory(void);

    os::SystemEventType *GetSystemEvent(void);
    os::SystemEventType *GetForwardEvent(void);
    os::SystemEventType *GetUserForwardEvent(void);

    Result MapRemoteSharedMemory(Handle handle);
    Result InitializeFakeSharedMemory(void);

    Result InitializeEvents(void);
    Result StartEventHandlerThread(void);

}
