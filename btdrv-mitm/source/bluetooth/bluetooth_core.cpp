#include "bluetooth_core.hpp"

#include <atomic>
#include <mutex>
#include <cstring>
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::core {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        BluetoothEventType g_currentEventType;

        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btSystemEventUser;

        void EventThreadFunc(void *arg) {
            while (true) {
                os::WaitSystemEvent(&g_btSystemEvent);
                HandleEvent();
            }
        }

    }

    bool IsInitialized(void) {
        return g_isInitialized;
    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btSystemEventUser;
    }

    Result Initialize(Handle eventHandle) {
        os::AttachReadableHandleToSystemEvent(&g_btSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);

        R_TRY(os::CreateSystemEvent(&g_btSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btSystemEventUser, os::EventClearMode_AutoClear, true));

        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            EventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
        ));

        os::StartThread(&g_eventHandlerThread); 

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroyThread(&g_eventHandlerThread);

        os::DestroySystemEvent(&g_btSystemEventUser);
        os::DestroySystemEvent(&g_btSystemEventFwd);

        g_isInitialized = false;
    }

    Result GetEventInfo(BluetoothEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock);
        
        *type = g_currentEventType;
        std::memcpy(buffer, g_eventDataBuffer, size);

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {

        std::scoped_lock lk(g_eventDataLock);
        {
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

            BTDRV_LOG_FMT("[%02d] Bluetooth Core Event", g_currentEventType);

            BluetoothEventData *event_data = reinterpret_cast<BluetoothEventData *>(g_eventDataBuffer);

            size_t data_size;
            switch (g_currentEventType) {
                case BluetoothEvent_DeviceFound:
                    data_size = sizeof(event_data->deviceFound);
                    // Todo: try changing name and cod to look like switch pro controller
                    //snprintf(event_data->deviceFound.name, sizeof(BluetoothName), "Pro Controller");
                    //event_data->deviceFound._unk2 = 0xffffffcb;
                    break;
                case BluetoothEvent_DiscoveryStateChanged:
                    data_size = sizeof(event_data->discoveryState);
                    break;
                case BluetoothEvent_PinRequest:
                    data_size = sizeof(event_data->pinReply);
                    break;
                case BluetoothEvent_SspRequest:
                    data_size = sizeof(event_data->sspReply);
                    break;
                case BluetoothEvent_BondStateChanged:
                    data_size = sizeof(event_data->bondState.v2);
                    break;
                default:
                    data_size = sizeof(g_eventDataBuffer);
                    break;
            }

            BTDRV_LOG_DATA(event_data, data_size);
        }
        
        // Signal our forwarder events
        if (!g_redirectEvents || g_preparingForSleep)
            os::SignalSystemEvent(&g_btSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btSystemEventUser);
    }

}
