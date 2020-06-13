#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"

#include "bluetooth/bluetooth_events.hpp"
#include "controllermanager.hpp"

namespace ams::mitm::btdrv {

    namespace {

        bool g_bluetoothInitialized = false;
        bool g_hidInitialized       = false;
        bool g_hidReportInitialized = false;
        bool g_bleInitialized       = false;
       
    }

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        //if (this->client_info.program_id == ncm::SystemProgramId::Btm)
        if (!g_bluetoothInitialized) {

            Handle handle = INVALID_HANDLE;
   
            // Forward to the real bluetooth module with our event handle instead
            R_TRY(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(bluetooth::core::GetSystemEvent(),
                handle, 
                false, 
                os::EventClearMode_AutoClear
            );

            // Create forwarder eventsg
            //R_ABORT_UNLESS(bluetooth::core::InitializeEvents());
            R_TRY(bluetooth::core::InitializeEvents());

            //bluetooth::events::AttachWaitHolder(BtdrvEventType_BluetoothCore);
            
            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetForwardEvent())); 

            // Create and map fake bluetooth hid report shared memory
            R_TRY(bluetooth::hid::report::InitializeFakeSharedMemory());

            // Create thread for forwarding events
            R_ABORT_UNLESS(bluetooth::core::StartEventHandlerThread());
            //R_TRY(StartEventHandlerThread());

            g_bluetoothInitialized = true;

        } else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBluetooth(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBluetooth");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    /*
    Result BtdrvMitmService::CancelBond(BluetoothAddress address) {

        BTDRV_LOG_FMT("btdrv-mitm: CancelBond");

        R_ABORT_UNLESS(btdrvCancelBondFwd(this->forward_service.get(), &address));

        return ams::ResultSuccess();
    }
    */


    Result BtdrvMitmService::GetEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetEventInfo");

        BluetoothEventType event_type;

        R_ABORT_UNLESS(btdrvGetEventInfoFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        BTDRV_LOG_FMT("  event %02d", event_type);

        BluetoothEventData *event_data = reinterpret_cast<BluetoothEventData *>(out_buffer.GetPointer());

        size_t data_size;
        switch (event_type) {
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
                data_size = out_buffer.GetSize();
                break;
        }

        BTDRV_LOG_DATA(out_buffer.GetPointer(), data_size);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeHid");

        Handle handle = INVALID_HANDLE;

        if (!g_hidInitialized) {

            R_ABORT_UNLESS(btdrvInitializeHidFwd(this->forward_service.get(), &handle, version));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(bluetooth::hid::GetSystemEvent(),
                handle, 
                false, 
                os::EventClearMode_AutoClear
            );

            // Create forwarder events
            R_ABORT_UNLESS(bluetooth::hid::InitializeEvents());
            //bluetooth::events::AttachWaitHolder(BtdrvEventType_BluetoothHid);

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetForwardEvent())); 

            // Create thread for forwarding events
            R_ABORT_UNLESS(bluetooth::hid::StartEventHandlerThread());

            g_hidInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer) {

        //BTDRV_LOG_FMT("btdrv-mitm: WriteHidData (caller: %s)", this->client_info.program_id == ncm::SystemProgramId::Hid ? "HID" : "other");

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto controller = locateController(&address);

            if (!controller->isSwitchController()) {
                 BTDRV_LOG_FMT("btdrv-mitm: WriteHidData - Non-Switch controller");
            }
        }

        R_ABORT_UNLESS(btdrvWriteHidDataFwd(this->forward_service.get(), 
            &address,
            reinterpret_cast<const BluetoothHidData *>(buffer.GetPointer()) 
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeHid(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeHid");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidEventInfoFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        BTDRV_LOG_FMT("  event %02d", event_type);

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle) {
        return RegisterHidReportEvent(out_handle);
    }

    /* 4.0.0+ */
    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: RegisterHidReportEvent");

        if (!g_hidReportInitialized) {
            Handle handle = INVALID_HANDLE;

            R_ABORT_UNLESS(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(bluetooth::hid::report::GetSystemEvent(),
                handle, 
                false, 
                os::EventClearMode_AutoClear
            );

             // Create forwarder events
            R_ABORT_UNLESS(bluetooth::hid::report::InitializeEvents());

            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetForwardEvent()));

            // Create thread for forwarding events
            R_ABORT_UNLESS(bluetooth::hid::report::StartEventHandlerThread());

            g_hidReportInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {
        
        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoDeprecatedFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        return ams::ResultSuccess();
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoDeprecatedFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        return ams::ResultSuccess();
    }

    /* 7.0.0+ */
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));

        // Load and map the real bluetooth shared memory
        R_TRY(bluetooth::hid::report::MapRemoteSharedMemory(handle));
       
        // Return the handle of our fake shared memory to the caller instead
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBle");

        Handle handle = INVALID_HANDLE;

        if (!g_bleInitialized) {

            R_ABORT_UNLESS(btdrvInitializeBleFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(bluetooth::ble::GetSystemEvent(), handle, false, os::EventClearMode_AutoClear);

            R_ABORT_UNLESS(bluetooth::ble::InitializeEvents());
            //bluetooth::events::AttachWaitHolder(BtdrvEventType_BluetoothBle);

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetForwardEvent())); 

            R_ABORT_UNLESS(bluetooth::ble::StartEventHandlerThread());

            g_bleInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBle(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBle");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBleFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::RedirectSystemEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectSystemEvents");

        g_redirectEvents = redirect;

        return ams::ResultSuccess();
    }

}
