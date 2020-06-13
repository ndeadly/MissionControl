#pragma once
#include <switch.h>
#include <stratosphere.hpp>

extern ams::os::SystemEventType g_btSystemEvent;
extern ams::os::SystemEventType g_btSystemEventFwd;
extern ams::os::SystemEventType g_btSystemEventUser;

ams::Result InitializeBluetoothCoreEvents(void);
ams::Result StartBluetoothCoreEventThread(void);
