#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::bluetooth::ble {

    bool IsInitialized(void);

    os::SystemEventType *GetSystemEvent(void);
    os::SystemEventType *GetForwardEvent(void);
    os::SystemEventType *GetUserForwardEvent(void);

    Result Initialize(Handle eventHandle);
    void Finalize(void);

    void HandleEvent(void);
    
}
