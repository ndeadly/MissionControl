#pragma once
#include <switch.h>
#include <stratosphere.hpp>  

extern ams::os::SystemEventType g_btHidSystemEvent;
extern ams::os::SystemEventType g_btHidSystemEventFwd;
extern ams::os::SystemEventType g_btHidSystemEventUser;

ams::Result InitializeBluetoothHidEvents(void);
ams::Result StartBluetoothHidEventThread(void);
