#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"

#include "bluetooth/bluetooth_events.hpp"
#include "controllermanager.hpp"

namespace ams::mitm::btdrv {

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        if (!bluetooth::core::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::core::Initialize(handle));

            if (hos::GetVersion() >= hos::Version_7_0_0)
                R_TRY(bluetooth::hid::report::InitializeFakeSharedMemory());

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

    /*
    Result BtdrvMitmService::CancelBond(BluetoothAddress address) {

        BTDRV_LOG_FMT("btdrv-mitm: CancelBond");

        R_ABORT_UNLESS(btdrvCancelBondFwd(this->forward_service.get(), &address));

        return ams::ResultSuccess();
    }
    */

    Result BtdrvMitmService::GetEventInfo(sf::Out<BluetoothEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        //BTDRV_LOG_FMT("btdrv-mitm: GetEventInfo [%02d]", out_type.GetValue());
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

    Result BtdrvMitmService::WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer) {

        //BTDRV_LOG_FMT("btdrv-mitm: WriteHidData (caller: %s)", this->client_info.program_id == ncm::SystemProgramId::Hid ? "HID" : "other");

        BTDRV_LOG_FMT("btdrv-mitm: WriteHidData");

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto controller = locateController(&address);
            if (controller && !controller->isSwitchController()) {
                 BTDRV_LOG_FMT("btdrv-mitm: WriteHidData - Non-Switch controller");

                // TODO: convert hid data format where possible and call btdrvWriteHidDataFwd

                return ams::ResultSuccess();
            }
        }

        R_TRY(btdrvWriteHidDataFwd(this->forward_service.get(), 
            &address,
            reinterpret_cast<const BluetoothHidData *>(buffer.GetPointer()) 
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::SetHidReport(BluetoothAddress address, BluetoothHhReportType type, const sf::InPointerBuffer &buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: SetHidReport");

        R_TRY(btdrvSetHidReportFwd(this->forward_service.get(), 
            &address, 
            type, 
            reinterpret_cast<const BluetoothHidData *>(buffer.GetPointer())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidReport(BluetoothAddress address, BluetoothHhReportType type, u8 id) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReport");

        R_TRY(btdrvGetHidReportFwd(this->forward_service.get(), &address, type, id));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetPairedDeviceInfo(BluetoothAddress address, const sf::OutPointerBuffer &out_buffer) {
        //BTDRV_LOG_FMT("btdrv-mitm: GetPairedDeviceInfo");
        
        R_TRY(btdrvGetPairedDeviceInfoFwd(this->forward_service.get(),
            &address, 
            reinterpret_cast<BluetoothDevicesSettings *>(out_buffer.GetPointer())
        ));

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            //BTDRV_LOG_FMT("Caller is BTM");

            auto controller = locateController(&address);
            if (controller && !controller->isSwitchController()) {
                BluetoothDevicesSettings *device = reinterpret_cast<BluetoothDevicesSettings *>(out_buffer.GetPointer());
                std::strncpy(device->name, "Lic Pro Controller", sizeof(BluetoothLocalName) - 1);
                BTDRV_LOG_FMT("!!! Modified controller name");
            } else {
                BTDRV_LOG_FMT("!!! Controller not found");
            }
        }
        else {
            BTDRV_LOG_FMT("!!! Caller is not BTM");
        }

        BTDRV_LOG_DATA_MSG(out_buffer.GetPointer(), sizeof(BluetoothDevicesSettings), "btdrv-mitm: GetPairedDeviceInfo");

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

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        R_TRY(bluetooth::hid::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /*
    Result BtdrvMitmService::SetTsi(BluetoothAddress address, u8 tsi) {

        BTDRV_LOG_FMT("btdrv-mitm: SetTsi");

        R_TRY(btdrvSetTsiFwd(this->forward_service.get(), &address, tsi));

        return ams::ResultSuccess();
    }
    */


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
    Result _GetHidReportEventInfoDeprecated(Service *srv, sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        R_TRY(bluetooth::hid::report::GetEventInfo(out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(this->forward_service.get(), out_type, out_buffer);
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
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

    Result BtdrvMitmService::GetBleManagedEventInfoDeprecated(sf::Out<BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return GetBleManagedEventInfo(out_type, out_buffer);
    }
    
    Result BtdrvMitmService::GetBleManagedEventInfo(sf::Out<BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        
        BTDRV_LOG_FMT("btdrv-mitm: GetBleManagedEventInfo");
        
        R_TRY(bluetooth::ble::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    void BtdrvMitmService::RedirectSystemEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectSystemEvents");

        g_redirectEvents = redirect;
    }

}
