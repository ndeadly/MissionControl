/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"
#include "bluetooth/bluetooth_events.hpp"
#include "../controllers/controller_management.hpp"
#include <switch.h>
#include <cstring>

namespace ams::mitm::btdrv {

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {
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
        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetEventInfo(sf::Out<bluetooth::EventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        R_TRY(bluetooth::core::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(),
            static_cast<uint8_t *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {
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
        auto report = reinterpret_cast<const bluetooth::HidReport *>(buffer.GetPointer());

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto device = controller::LocateHandler(&address);
            if (device) {
                device->HandleOutgoingReport(report);
            }
        }
        else {
            R_TRY(btdrvWriteHidDataFwd(this->forward_service.get(), &address, report));
        }

        return ams::ResultSuccess();
    }
        
    Result BtdrvMitmService::FinalizeHid(void) {
        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        R_TRY(bluetooth::hid::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<uint8_t *>(out_buffer.GetPointer()),
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
        if (!bluetooth::hid::report::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::hid::report::Initialize(handle, this->forward_service.get(), os::GetThreadId(os::GetCurrentThread())));
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetForwardEvent()));
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 6.2.0 */
    Result _GetHidReportEventInfoDeprecated(Service *srv, sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        R_TRY(bluetooth::hid::report::GetEventInfo(out_type.GetPointer(), 
            static_cast<uint8_t *>(out_buffer.GetPointer()),
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
        Handle handle = INVALID_HANDLE;
        R_TRY(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));
        R_TRY(bluetooth::hid::report::MapRemoteSharedMemory(handle));
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {
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
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBleFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetBleManagedEventInfoDeprecated(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return GetBleManagedEventInfo(out_type, out_buffer);
    }
    
    Result BtdrvMitmService::GetBleManagedEventInfo(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {        
        R_TRY(bluetooth::ble::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<uint8_t *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetRealSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(bluetooth::hid::report::GetRealSharedMemory()->handle);
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetFakeSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        return ams::ResultSuccess();
    }

    void BtdrvMitmService::RedirectCoreEvents(bool redirect) {
        g_redirect_core_events = redirect;
    }

    void BtdrvMitmService::RedirectHidEvents(bool redirect) {
        g_redirect_hid_events = redirect;
    }

    void BtdrvMitmService::RedirectHidReportEvents(bool redirect) {
        g_redirect_hid_report_events = redirect;
    }

    void BtdrvMitmService::RedirectBleEvents(bool redirect) {
        g_redirect_ble_events = redirect;
    }

}
