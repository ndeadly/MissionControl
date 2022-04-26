/*
 * Copyright (c) 2020-2022 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"
#include "bluetooth/bluetooth_core.hpp"
#include "bluetooth/bluetooth_hid.hpp"
#include "bluetooth/bluetooth_ble.hpp"
#include "../mcmitm_initialization.hpp"
#include "../controllers/controller_management.hpp"
#include <switch.h>

namespace ams::mitm::bluetooth {

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {
        if (!ams::bluetooth::core::IsInitialized()) {
            // Forward to the real function to obtain the system event handle
            os::NativeHandle handle = os::InvalidNativeHandle;
            R_TRY(btdrvInitializeBluetoothFwd(m_forward_service.get(), &handle));

            // Attach the real system event handle to our own event
            ams::bluetooth::core::GetSystemEvent()->AttachReadableHandle(handle, false, os::EventClearMode_ManualClear);

            // Return our forwarder event handle to the caller instead
            out_handle.SetValue(ams::bluetooth::core::GetForwardEvent()->GetReadableHandle(), false);

            // Initialise the hid report circular buffer
            R_TRY(ams::bluetooth::hid::report::InitializeReportBuffer());

            // Signal that the interface is initialised
            ams::bluetooth::core::SignalInitialized();
        } else {
            out_handle.SetValue(ams::bluetooth::core::GetUserForwardEvent()->GetReadableHandle(), false);
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::EnableBluetooth() {
        R_TRY(btdrvEnableBluetoothFwd(m_forward_service.get()));
        ams::bluetooth::core::SignalEnabled();

        // Wait until mc.mitm module initialisation has completed before returning
        ams::mitm::WaitInitialized();

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetEventInfo(sf::Out<ams::bluetooth::EventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return ams::bluetooth::core::GetEventInfo(m_client_info.program_id, out_type.GetPointer(), out_buffer.GetPointer(), out_buffer.GetSize());
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {
        if (!ams::bluetooth::hid::IsInitialized()) {
            // Forward to the real function to obtain the system event handle
            os::NativeHandle handle = os::InvalidNativeHandle;
            R_TRY(btdrvInitializeHidFwd(m_forward_service.get(), &handle, version));

            // Attach the real system event handle to our own event
            ams::bluetooth::hid::GetSystemEvent()->AttachReadableHandle(handle, false, os::EventClearMode_ManualClear);

            // Return our forwarder event handle to the caller instead
            out_handle.SetValue(ams::bluetooth::hid::GetForwardEvent()->GetReadableHandle(), false);

            // Signal that the interface is initialised
            ams::bluetooth::hid::SignalInitialized();
        }
        else {
            out_handle.SetValue(ams::bluetooth::hid::GetUserForwardEvent()->GetReadableHandle(), false);
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::WriteHidData(ams::bluetooth::Address address, const sf::InPointerBuffer &buffer) {
        auto report = reinterpret_cast<const ams::bluetooth::HidReport *>(buffer.GetPointer());

        if (m_client_info.program_id == ncm::SystemProgramId::Hid) {
            auto device = controller::LocateHandler(&address);
            if (device) {
                device->HandleOutputDataReport(report);
            }
        }
        else {
            R_TRY(btdrvWriteHidDataFwd(m_forward_service.get(), &address, report));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<ams::bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return ams::bluetooth::hid::GetEventInfo(out_type.GetPointer(), out_buffer.GetPointer(), out_buffer.GetSize());
    }

    Result BtdrvMitmService::SetTsi(ams::bluetooth::Address address, u8 tsi) {
        auto device = controller::LocateHandler(&address);
        if (!device || device->SupportsSetTsiCommand())
            return sm::mitm::ResultShouldForwardToSession();

        if (hos::GetVersion() < hos::Version_9_0_0) {
            const struct {
                uint32_t type;
                ams::bluetooth::Address address;
                uint8_t pad[2];
                uint32_t status;
            } event_data = {tsi == 0xff ? 1u : 0u, address, {0, 0}, 0};

            ams::bluetooth::hid::SignalFakeEvent(BtdrvHidEventTypeOld_Ext, &event_data, sizeof(event_data));
        }
        else if (hos::GetVersion() < hos::Version_12_0_0) {
            const struct {
                uint32_t type;
                uint32_t status;
                ams::bluetooth::Address address;
                uint8_t pad[2];
            } event_data = {tsi == 0xff ? 1u : 0u, 0, address, {0, 0}};

            ams::bluetooth::hid::SignalFakeEvent(BtdrvHidEventTypeOld_Ext, &event_data, sizeof(event_data));
        }
        else {
            const struct {
                ams::bluetooth::Address address;
                uint8_t flag;
                uint8_t tsi;
            } event_data = { address, 1, tsi };

            ams::bluetooth::core::SignalFakeEvent(BtdrvEventType_Tsi, &event_data, sizeof(event_data));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle) {
        return RegisterHidReportEvent(out_handle);
    }

    /* 4.0.0+ */
    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {
        if (!ams::bluetooth::hid::report::IsInitialized()) {
            // Forward to the real function to obtain the system event handle
            os::NativeHandle handle = os::InvalidNativeHandle;
            R_TRY(btdrvRegisterHidReportEventFwd(m_forward_service.get(), &handle));
            
            // Attach the real system event handle to our own event
            ams::bluetooth::hid::report::GetSystemEvent()->AttachReadableHandle(handle, false, os::EventClearMode_AutoClear);
            
            // Return our forwarder event handle to the caller instead
            out_handle.SetValue(ams::bluetooth::hid::report::GetForwardEvent()->GetReadableHandle(), false);

            // Signal that the interface is initialised
            ams::bluetooth::hid::report::SignalInitialized();
        }
        else {
            out_handle.SetValue(ams::bluetooth::hid::report::GetUserForwardEvent()->GetReadableHandle(), false);
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 6.2.0 */
    Result _GetHidReportEventInfoDeprecated(Service *srv, sf::Out<ams::bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        AMS_UNUSED(srv);

        return ams::bluetooth::hid::report::GetEventInfo(out_type.GetPointer(), out_buffer.GetPointer(), out_buffer.GetSize());
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<ams::bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(m_forward_service.get(), out_type, out_buffer);
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<ams::bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(m_forward_service.get(), out_type, out_buffer);
    }

    /* 7.0.0+ */
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {
        os::NativeHandle handle = os::InvalidNativeHandle;
        R_TRY(btdrvGetHidReportEventInfoFwd(m_forward_service.get(), &handle));
        R_TRY(ams::bluetooth::hid::report::MapRemoteSharedMemory(handle));
        out_handle.SetValue(ams::bluetooth::hid::report::GetFakeSharedMemory()->GetHandle(), false);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {
        if (!ams::bluetooth::ble::IsInitialized()) {
            // Forward to the real function to obtain the system event handle
            os::NativeHandle handle = os::InvalidNativeHandle;
            R_TRY(btdrvInitializeBleFwd(m_forward_service.get(), &handle));

            // Attach the real system event handle to our own event
            ams::bluetooth::ble::GetSystemEvent()->AttachReadableHandle(handle, false, os::EventClearMode_ManualClear);

            // Return our forwarder event handle to the caller instead
            out_handle.SetValue(ams::bluetooth::ble::GetForwardEvent()->GetReadableHandle(), false);

            // Signal that the interface is initialised
            ams::bluetooth::ble::SignalInitialized();
        }
        else {
            out_handle.SetValue(ams::bluetooth::ble::GetUserForwardEvent()->GetReadableHandle(), false);
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetBleManagedEventInfoDeprecated(sf::Out<ams::bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return GetBleManagedEventInfo(out_type, out_buffer);
    }

    Result BtdrvMitmService::GetBleManagedEventInfo(sf::Out<ams::bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return ams::bluetooth::ble::GetEventInfo(out_type.GetPointer(), out_buffer.GetPointer(), out_buffer.GetSize());
    }

    Result BtdrvMitmService::GetRealSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(ams::bluetooth::hid::report::GetRealSharedMemory()->GetHandle(), false);
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetFakeSharedMemory(sf::OutCopyHandle out_handle) {
        out_handle.SetValue(ams::bluetooth::hid::report::GetFakeSharedMemory()->GetHandle(), false);
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

    void BtdrvMitmService::SignalHidReportRead() {
        ams::bluetooth::hid::report::SignalReportRead();
    }

}
