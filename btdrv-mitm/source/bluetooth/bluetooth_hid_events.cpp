#include "bluetooth_hid_events.hpp"
#include "../controllermanager.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid {

    namespace {

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];
        u8 g_eventDataBuffer[0x480];
        HidEventType g_currentEventType;

        os::SystemEventType g_btHidSystemEvent;
        os::SystemEventType g_btHidSystemEventFwd;
        os::SystemEventType g_btHidSystemEventUser;

    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btHidSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btHidSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btHidSystemEventUser;
    }

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

    void HandleEvent(void) {
        BTDRV_LOG_FMT("hid event fired");

        HidEventData *eventData = reinterpret_cast<HidEventData *>(g_eventDataBuffer);

        R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        switch (g_currentEventType) {
            case HidEvent_ConnectionState:
                handleConnectionStateEvent(eventData);
                break;
            default:
                break;
        }
                
        // Signal our forwarder events
        //os::SignalSystemEvent(&g_btHidSystemEventFwd);

        if (!g_redirectEvents || g_preparingForSleep) {
            os::SignalSystemEvent(&g_btHidSystemEventFwd);
        }
        else {
            os::SignalSystemEvent(&g_btHidSystemEventUser);
        }
    }

    void BluetoothHidEventThreadFunc(void *arg) {
        while (true) {
            // Wait for real bluetooth event 
            os::WaitSystemEvent(&g_btHidSystemEvent);

            HandleEvent();
        }
    }

    Result InitializeEvents(void) {
        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventUser, os::EventClearMode_AutoClear, true));

        return ams::ResultSuccess();
    }

    Result StartEventHandlerThread(void) {
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            BluetoothHidEventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
            //38 // priority of btm sysmodule + 1
        ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

}
