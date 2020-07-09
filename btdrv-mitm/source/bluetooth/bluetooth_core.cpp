#include "bluetooth_core.hpp"

#include <atomic>
#include <mutex>
#include <cstring>

#include "../btdrv_mitm_flags.hpp"
#include "../controllermanager.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::core {

    namespace {

        //const constexpr char* g_licProControllerName = "Lic Pro Controller";
        const constexpr char* g_licProControllerName = "Pro Controller";

        std::atomic<bool> g_isInitialized(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        EventType g_currentEventType;

        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btSystemEventUser;
        os::EventType       g_dataReadEvent;

        /*
        void _LogEvent(EventType type, EventData *eventData) {
        
            size_t dataSize;
            switch (type) {
                case BluetoothEvent_DeviceFound:
                    dataSize = sizeof(eventData->deviceFound);
                    break;
                case BluetoothEvent_DiscoveryStateChanged:
                    dataSize = sizeof(eventData->discoveryState);
                    break;
                case BluetoothEvent_PinRequest:
                    dataSize = sizeof(eventData->pinReply);
                    break;
                case BluetoothEvent_SspRequest:
                    dataSize = sizeof(eventData->sspReply);
                    break;
                case BluetoothEvent_BondStateChanged:
                    if (hos::GetVersion() < hos::Version_9_0_0)
                        dataSize = sizeof(eventData->bondState);
                    else
                        dataSize = sizeof(eventData->bondState.v2);                  
                    break;
                default:
                    dataSize = sizeof(g_eventDataBuffer);
                    break;
            }

            //BTDRV_LOG_DATA(eventData, dataSize);
            BTDRV_LOG_DATA_MSG(eventData, dataSize, "[%02d] Bluetooth core event", type);
        }
        */

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
        //os::AttachReadableHandleToSystemEvent(&g_btSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);
        os::AttachReadableHandleToSystemEvent(&g_btSystemEvent, eventHandle, false, os::EventClearMode_ManualClear);

        R_TRY(os::CreateSystemEvent(&g_btSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btSystemEventUser, os::EventClearMode_AutoClear, true)); 
        os::InitializeEvent(&g_dataReadEvent, false, os::EventClearMode_AutoClear);

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::FinalizeEvent(&g_dataReadEvent);
        os::DestroySystemEvent(&g_btSystemEventUser);
        os::DestroySystemEvent(&g_btSystemEventFwd);

        g_isInitialized = false;
    }

    void handleDeviceFoundEvent(EventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->deviceFound.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->deviceFound.name)) {
            std::strncpy(eventData->deviceFound.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
    }

    void handlePinRequesEvent(EventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->pinReply.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->pinReply.name)) {
            std::strncpy(eventData->pinReply.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
    }

    void handleSspRequesEvent(EventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->sspReply.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->sspReply.name)) {
            std::strncpy(eventData->sspReply.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
    }

    Result GetEventInfo(ncm::ProgramId program_id, EventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock);

        *type = g_currentEventType;
        std::memcpy(buffer, g_eventDataBuffer, size);

        auto eventData = reinterpret_cast<EventData *>(buffer);

        if (program_id == ncm::SystemProgramId::Btm) {
            
            switch (g_currentEventType) {
                case BluetoothEvent_DeviceFound:
                    handleDeviceFoundEvent(eventData);
                    break;
                case BluetoothEvent_DiscoveryStateChanged:
                    break;
                case BluetoothEvent_PinRequest:
                    handlePinRequesEvent(eventData);
                    break;
                case BluetoothEvent_SspRequest:
                    handleSspRequesEvent(eventData);
                    break;
                case BluetoothEvent_BondStateChanged:
                    break;
                default:
                    break;
            }
        }

        //_LogEvent(g_currentEventType, eventData);

        os::SignalEvent(&g_dataReadEvent);

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_eventDataLock);
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));
        }

        BTDRV_LOG_FMT("[%02d] Core Event", g_currentEventType);

        if (!g_redirectEvents) {
            if (g_currentEventType != BluetoothEvent_PinRequest) {
                os::SignalSystemEvent(&g_btSystemEventFwd);
                os::WaitEvent(&g_dataReadEvent);
            }
            else {
                // Todo: set this to what it should be if we ever enable bluetooth to actually read the pincode parameter
                PinCode pincode = {};

                // Fuck BTM, we're sending the pin response. What it doesn't know won't hurt it
                auto eventData = reinterpret_cast<EventData *>(g_eventDataBuffer);
                R_ABORT_UNLESS(btdrvRespondToPinRequest(&eventData->pinReply.address, false, &pincode, sizeof(Address)));
            }
        }

        if (g_btSystemEventUser.state) {
            os::SignalSystemEvent(&g_btSystemEventUser);
            //os::TimedWaitEvent(&g_dataReadEvent, TimeSpan::FromMilliSeconds(500));
        }
    }

}
