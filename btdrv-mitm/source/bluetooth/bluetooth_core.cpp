#include "bluetooth_core.hpp"

#include <atomic>
#include <mutex>
#include <cstring>
#include "../btdrv_mitm_flags.hpp"
#include "../controllermanager.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::core {

    namespace {

        const constexpr char* g_licProControllerName = "Lic Pro Controller";

        std::atomic<bool> g_isInitialized(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        BluetoothEventType g_currentEventType;

        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btSystemEventUser;

        void _LogEvent(BluetoothEventType type, BluetoothEventData *eventData) {
        
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

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroySystemEvent(&g_btSystemEventUser);
        os::DestroySystemEvent(&g_btSystemEventFwd);

        g_isInitialized = false;
    }

    void handleDeviceFoundEvent(BluetoothEventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->deviceFound.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->deviceFound.name)) {
            std::strncpy(eventData->deviceFound.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
        else {
            BTDRV_LOG_FMT("handleDeviceFoundEvent: [%02x%02x%02x] | %s",
                eventData->deviceFound.cod.cod[0],
                eventData->deviceFound.cod.cod[1],
                eventData->deviceFound.cod.cod[2],
                eventData->deviceFound.name
            );
        }
    }

    void handlePinRequesEvent(BluetoothEventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->pinReply.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->pinReply.name)) {
            std::strncpy(eventData->pinReply.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
        else {
            BTDRV_LOG_FMT("handleDeviceFoundEvent: [%02x%02x%02x] | %s",
                eventData->pinReply.cod.cod[0],
                eventData->pinReply.cod.cod[1],
                eventData->pinReply.cod.cod[2],
                eventData->pinReply.name
            );
        }
    }

    void handleSspRequesEvent(BluetoothEventData *eventData) {
        if (ams::mitm::btdrv::IsController(&eventData->sspReply.cod) && !ams::mitm::btdrv::IsValidSwitchControllerName(eventData->sspReply.name)) {
            std::strncpy(eventData->sspReply.name, g_licProControllerName, sizeof(BluetoothName) - 1);
            eventData->pinReply.cod = {0x00, 0x25, 0x08};
        }
        else {
            BTDRV_LOG_FMT("handleDeviceFoundEvent: [%02x%02x%02x] | %s",
                eventData->sspReply.cod.cod[0],
                eventData->sspReply.cod.cod[1],
                eventData->sspReply.cod.cod[2],
                eventData->sspReply.name
            );
        }
    }

    Result GetEventInfo(ncm::ProgramId program_id, BluetoothEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock);
        {
            *type = g_currentEventType;
            std::memcpy(buffer, g_eventDataBuffer, size);

            BluetoothEventData *eventData = reinterpret_cast<BluetoothEventData *>(buffer);

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
            } else {
                BTDRV_LOG_FMT("Not BTM: [%02x]", program_id);
            }

            _LogEvent(g_currentEventType, eventData);

        }
        
        // Might not be required if original event is autoclear?
        //os::ClearSystemEvent(&g_btSystemEvent);

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {

        std::scoped_lock lk(g_eventDataLock);
        
        R_ABORT_UNLESS(btdrvGetEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        //BTDRV_LOG_FMT("[%02d] Bluetooth Core Event", g_currentEventType);
        //BTDRV_LOG_DATA(g_eventDataBuffer, sizeof(g_eventDataBuffer));

        //BluetoothEventData *eventData = reinterpret_cast<BluetoothEventData *>(g_eventDataBuffer);

        //_LogEvent(g_currentEventType, eventData);

        /*
        if (g_currentEventType == BluetoothEvent_DeviceFound) {

            if (ams::mitm::btdrv::IsController(&eventData->deviceFound.cod)) {
                if (std::strncmp(eventData->deviceFound.name, "Nintendo RVL-CNT-01",    sizeof(BluetoothName)) == 0 ||
                    std::strncmp(eventData->deviceFound.name, "Nintendo RVL-CNT-01-UC", sizeof(BluetoothName)) == 0) 
                {
                    BTDRV_LOG_FMT("!!!!! Calling CreateBond");
                    btdrvCreateBond(&eventData->deviceFound.address, BluetoothTransport_Auto);
                    return;
                }
            }
        }
        */

        /*
        if (g_currentEventType == BluetoothEvent_PinRequest) {
            BTDRV_LOG_FMT("!!!!! Calling PinReply");
            BluetoothPinCode pincode = {};
            btdrvRespondToPinRequest(&eventData->pinReply.address, false, &pincode, sizeof(BluetoothAddress));
            return;
        }
        */

        // Signal our forwarder events
        //if (!g_redirectEvents || g_preparingForSleep)
        if (!g_redirectEvents)
            os::SignalSystemEvent(&g_btSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btSystemEventUser);
    }

}
