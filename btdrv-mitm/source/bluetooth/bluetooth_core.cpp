#include "bluetooth_core.hpp"

#include <atomic>
#include <mutex>
#include <cstring>

#include "../btdrv_mitm_flags.hpp"
#include "../controllers/controllermanager.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::core {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        EventType g_currentEventType;

        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btSystemEventUser;
        os::EventType       g_dataReadEvent;

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
        if (controller::IsController(&eventData->deviceFound.cod) && !controller::IsValidSwitchControllerName(eventData->deviceFound.name)) {
            std::strncpy(eventData->deviceFound.name, controller::proControllerName, sizeof(BluetoothName) - 1);
        }
    }

    void handlePinRequestEvent(EventData *eventData) {
        if (controller::IsController(&eventData->pinReply.cod) && !controller::IsValidSwitchControllerName(eventData->pinReply.name)) {
            std::strncpy(eventData->pinReply.name, controller::proControllerName, sizeof(BluetoothName) - 1);
        }
    }

    void handleSspRequestEvent(EventData *eventData) {
        if (controller::IsController(&eventData->sspReply.cod) && !controller::IsValidSwitchControllerName(eventData->sspReply.name)) {
            std::strncpy(eventData->sspReply.name, controller::proControllerName, sizeof(BluetoothName) - 1);
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
                case BluetoothEvent_PinRequest:
                    handlePinRequestEvent(eventData);
                    break;
                case BluetoothEvent_SspRequest:
                    handleSspRequestEvent(eventData);
                    break;
                default:
                    break;
            }
        }

        os::SignalEvent(&g_dataReadEvent);

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_eventDataLock);
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));
        }

        BTDRV_LOG_FMT("[%02d] Core Event", g_currentEventType);

        if (!g_redirectCoreEvents) {
            if (g_currentEventType != BluetoothEvent_PinRequest) {
                os::SignalSystemEvent(&g_btSystemEventFwd);
                os::WaitEvent(&g_dataReadEvent);
            }
            else {
                auto eventData = reinterpret_cast<EventData *>(g_eventDataBuffer);

                bluetooth::PinCode pincode = {};
                u8 pin_length;

                // Reverse host address as pincode for wii devices
                if (strncmp(eventData->pinReply.name, "Nintendo RVL", 12) == 0) {
                    // Fetch host adapter properties
                    BluetoothAdapterProperty props;
                    R_ABORT_UNLESS(btdrvGetAdapterProperties(&props));
                    // Reverse host address
                    *reinterpret_cast<u64 *>(&pincode) = util::SwapBytes(*reinterpret_cast<u64 *>(&props.address)) >> 16;
                    pin_length = sizeof(bluetooth::Address);
                }
                else {
                    // This is what the bluetooth sysmodule hardcodes
                    *reinterpret_cast<u32 *>(&pincode) = 0x30303030;
                    pin_length = 4;
                }

                // Fuck BTM, we're sending the pin response ourselves if it won't.
                R_ABORT_UNLESS(btdrvRespondToPinRequest(&eventData->pinReply.address, false, &pincode, pin_length));
            }
        }

        if (g_btSystemEventUser.state)
            os::SignalSystemEvent(&g_btSystemEventUser);

    }

}
