#pragma once
#include <switch.h>
#include <stratosphere.hpp>

extern ams::os::SystemEventType g_btBleSystemEvent;
extern ams::os::SystemEventType g_btBleSystemEventFwd;
extern ams::os::SystemEventType g_btBleSystemEventUser;

ams::Result InitializeBluetoothBleEvents(void);
ams::Result StartBluetoothBleEventThread(void);
    