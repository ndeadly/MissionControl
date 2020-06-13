#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "../circularbuffer.hpp"

extern SharedMemory g_realBtShmem;
extern SharedMemory g_fakeBtShmem;

extern ams::bluetooth::CircularBuffer *g_realCircBuff;
extern ams::bluetooth::CircularBuffer *g_fakeCircBuff;

extern ams::os::SystemEventType g_btHidReportSystemEvent;
extern ams::os::SystemEventType g_btHidReportSystemEventFwd;
extern ams::os::SystemEventType g_btHidReportSystemEventUser;

ams::Result InitializeBluetoothHidReportEvents(void);
ams::Result InitializeBluetoothHidReportFakeSharedMemory(void);
ams::Result StartBluetoothHidReportEventThread(void);
