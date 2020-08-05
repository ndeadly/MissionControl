#include <cstring>
#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"

#include "bluetooth/bluetooth_events.hpp"
#include "controllers/controllermanager.hpp"

#include "btdrv_mitm_logging.hpp"

namespace ams::mitm::btdrv {

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        if (!bluetooth::core::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::core::Initialize(handle));
            R_TRY(bluetooth::hid::report::InitializeReportBuffer());           

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetForwardEvent())); 
        } else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBluetooth(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBluetooth");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetEventInfo(sf::Out<bluetooth::EventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetEventInfo");

        R_TRY(bluetooth::core::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(),
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeHid");

        if (!bluetooth::hid::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeHidFwd(this->forward_service.get(), &handle, version));
            R_TRY(bluetooth::hid::Initialize(handle));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetForwardEvent())); 
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }


    Result BtdrvMitmService::WriteHidData(bluetooth::Address address, const sf::InPointerBuffer &buffer) {

        auto requestData = reinterpret_cast<const bluetooth::HidReport *>(buffer.GetPointer());

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto device = controller::locateController(&address);
            if (device) {
                requestData = device->handleOutgoingReport(requestData);
            }
        }

        // Above handler sets size to 0 if we shouldn't send any data to the controller
        if (requestData->size > 0) {
            R_TRY(btdrvWriteHidDataFwd(this->forward_service.get(), 
                &address,
                requestData
            ));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetPairedDeviceInfo(sf::Out<bluetooth::DeviceSettings> out, bluetooth::Address address) {

        BTDRV_LOG_FMT("btdrv-mitm: GetPairedDeviceInfo");

        auto device = reinterpret_cast<BluetoothDevicesSettings *>(out.GetPointer());

        R_TRY(btdrvGetPairedDeviceInfoFwd(this->forward_service.get(), &address, device));

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            if (!controller::IsValidSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::proControllerName, sizeof(BluetoothLocalName) - 1);
            }
        }

        return ams::ResultSuccess();
    }
        
    Result BtdrvMitmService::FinalizeHid(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeHid");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        R_TRY(bluetooth::hid::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle) {
        return RegisterHidReportEvent(out_handle);
    }

    /* 4.0.0+ */
    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: RegisterHidReportEvent");

        if (!bluetooth::hid::report::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::hid::report::Initialize(handle));
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetForwardEvent()));
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 6.2.0 */
    Result _GetHidReportEventInfoDeprecated(Service *srv, sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        //BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo (deprecated)");

        R_TRY(bluetooth::hid::report::GetEventInfo(out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(this->forward_service.get(), out_type, out_buffer);
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(this->forward_service.get(), out_type, out_buffer);
    }

    /* 7.0.0+ */
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        Handle handle = INVALID_HANDLE;
        R_TRY(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));
        R_TRY(bluetooth::hid::report::MapRemoteSharedMemory(handle));
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBle");

        if (!bluetooth::ble::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeBleFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::ble::Initialize(handle));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetForwardEvent())); 
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBle(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBle");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBleFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetBleManagedEventInfoDeprecated(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return GetBleManagedEventInfo(out_type, out_buffer);
    }
    
    Result BtdrvMitmService::GetBleManagedEventInfo(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        
        BTDRV_LOG_FMT("btdrv-mitm: GetBleManagedEventInfo");
        
        R_TRY(bluetooth::ble::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    void BtdrvMitmService::RedirectCoreEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectCoreEvents [%s]", redirect ? "on" : "off");

        g_redirectCoreEvents = redirect;
    }

    void BtdrvMitmService::RedirectHidEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectHidEvents [%s]", redirect ? "on" : "off");

        g_redirectHidEvents = redirect;
    }

    void BtdrvMitmService::RedirectHidReportEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectHidReportEvents [%s]", redirect ? "on" : "off");

        g_redirectHidReportEvents = redirect;
    }

    void BtdrvMitmService::RedirectBleEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectBleEvents [%s]", redirect ? "on" : "off");

        g_redirectBleEvents = redirect;
    }

    Result BtdrvMitmService::GetRealSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(bluetooth::hid::report::GetRealSharedMemory()->handle);
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetFakeSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        return ams::ResultSuccess();
    }


}
