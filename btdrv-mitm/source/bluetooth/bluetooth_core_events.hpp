#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::bluetooth::core {

    void HandleEvent(void);

    os::SystemEventType *GetSystemEvent(void);
    os::SystemEventType *GetForwardEvent(void);
    os::SystemEventType *GetUserForwardEvent(void);

    Result InitializeEvents(void);
    Result StartEventHandlerThread(void);

}
