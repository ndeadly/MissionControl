#include "bluetooth_hid_events.hpp"
#include "../controllermanager.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

ams::os::ThreadType g_bt_hid_event_task_thread;
alignas(ams::os::ThreadStackAlignment) u8 g_bt_hid_event_task_stack[0x2000];
u8 g_bt_hid_event_data_buffer[0x480];
HidEventType g_current_bt_hid_event_type;

ams::os::SystemEventType g_btHidSystemEvent;
ams::os::SystemEventType g_btHidSystemEventFwd;
ams::os::SystemEventType g_btHidSystemEventUser;


void handleConnectionStateEvent(HidEventData *eventData) {
    switch (eventData->connectionState.state) {
        case HidConnectionState_Connected:
            ams::mitm::btdrv::attachDeviceHandler(&eventData->connectionState.address);
            BTDRV_LOG_FMT("device connected");
            break;
        case HidConnectionState_Disconnected:
            ams::mitm::btdrv::removeDeviceHandler(&eventData->connectionState.address);
            BTDRV_LOG_FMT("device disconnected");
            break;
        default:
            break;
    }
    BTDRV_LOG_DATA(&eventData->connectionState.address, sizeof(BluetoothAddress));
}


void BluetoothHidEventThreadFunc(void *arg) {
    HidEventData *eventData = reinterpret_cast<HidEventData *>(g_bt_hid_event_data_buffer);

    while (true) {
        // Wait for real bluetooth event 
        ams::os::WaitSystemEvent(&g_btHidSystemEvent);

        BTDRV_LOG_FMT("hid event fired");

        R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_current_bt_hid_event_type, g_bt_hid_event_data_buffer, sizeof(g_bt_hid_event_data_buffer)));

        switch (g_current_bt_hid_event_type) {
            case HidEvent_ConnectionState:
                handleConnectionStateEvent(eventData);
                break;
            default:
                break;
        }
                
        // Signal our forwarder events
        //os::SignalSystemEvent(&g_btHidSystemEventFwd);

        if (!g_redirectEvents || g_preparingForSleep) {
            ams::os::SignalSystemEvent(&g_btHidSystemEventFwd);
        }
        else {
            ams::os::SignalSystemEvent(&g_btHidSystemEventUser);
        }
    }
}

ams::Result InitializeBluetoothHidEvents(void) {
    R_TRY(ams::os::CreateSystemEvent(&g_btHidSystemEventFwd, ams::os::EventClearMode_AutoClear, true));
    R_TRY(ams::os::CreateSystemEvent(&g_btHidSystemEventUser, ams::os::EventClearMode_AutoClear, true));

    return ams::ResultSuccess();
}

ams::Result StartBluetoothHidEventThread(void) {
    R_TRY(ams::os::CreateThread(&g_bt_hid_event_task_thread, 
        BluetoothHidEventThreadFunc, 
        nullptr, 
        g_bt_hid_event_task_stack, 
        sizeof(g_bt_hid_event_task_stack), 
        9
        //38 // priority of btm sysmodule + 1
    ));

    ams::os::StartThread(&g_bt_hid_event_task_thread); 

    return ams::ResultSuccess();
}
