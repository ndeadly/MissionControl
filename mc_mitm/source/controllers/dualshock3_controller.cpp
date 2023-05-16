/*
 * Copyright (C) 2020-2023 ndeadly
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
#include "dualshock3_controller.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_core.hpp"
#include "../mcmitm_config.hpp"
#include <stratosphere.hpp>
#include <cstdlib>

namespace ams::controller {

    namespace {

        const char *ds3_device_name = "PLAYSTATION(R)3 Controller";
        constexpr u16 ds3_vendor_id = 0x054c;
        constexpr u16 ds3_product_id = 0x0268;

        enum Dualshock3LedMode {
            Dualshock3LedMode_Switch = 0,
            Dualshock3LedMode_Ps3 = 1,
            Dualshock3LedMode_Hybrid = 2,
        };

        const u8 enable_payload[] = { 0xf4, 0x42, 0x03, 0x00, 0x00 };
        const u8 led_config[] = { 0xff, 0x27, 0x10, 0x00, 0x32 };
        const u8 player_led_patterns[] = { 0b1000, 0b1100, 0b1110, 0b1111, 0b1001, 0b0101, 0b1101, 0b0110 };

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;
        constexpr float accel_scale_factor = 65535 / 16000.0f * 1000 / 113;

        alignas(os::MemoryPageSize) constinit u8 g_usb_buffer[0x1000];

        const UsbHsInterfaceFilter g_interface_filter = {
            .Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct | UsbHsInterfaceFilterFlags_bInterfaceClass,
            .idVendor = ds3_vendor_id,
            .idProduct = ds3_product_id,
            .bInterfaceClass = USB_CLASS_HID,
        };

        Result SetMasterAddress(UsbHsClientIfSession *if_session, const BtdrvAddress *address) {
            const struct {
                u8 unk1;
                u8 unk2;
                BtdrvAddress address;
            } data = {0x01, 0x00, *address};

            std::memcpy(&g_usb_buffer, &data, sizeof(data));

            u32 rx_size = 0;
            R_TRY(usbHsIfCtrlXfer(if_session,
                USB_ENDPOINT_OUT | (0x01 << 5) | 0x01,
                USB_REQUEST_SET_CONFIGURATION,
                0x3f5,
                0,
                sizeof(data),
                &g_usb_buffer,
                &rx_size
            ));

            R_SUCCEED();
        }

        Result GetSlaveAddress(UsbHsClientIfSession *if_session, BtdrvAddress *address) {
            u32 tx_size = 0;
            R_TRY(usbHsIfCtrlXfer(if_session,
                USB_ENDPOINT_IN | (0x01 << 5) | 0x01,
                USB_REQUEST_CLEAR_FEATURE,
                0x3f2,
                0,
                18,
                &g_usb_buffer,
                &tx_size
            ));

            *address = *reinterpret_cast<BtdrvAddress *>(&g_usb_buffer[4]);

            R_SUCCEED();
        }

        Result TrustDevice(const BtdrvAddress *address) {
            SetSysBluetoothDevicesSettings device = {};
            device.addr = *address;
            device.class_of_device = {0x00, 0x05, 0x08};
            device.link_key_present = false;
            device.trusted_services = 0x100000;
            device.vid = ds3_vendor_id;
            device.pid = ds3_product_id;
            device.sub_class = 0x08;
            device.attribute_mask = 0xff;

            if (hos::GetVersion() < hos::Version_13_0_0) {
                std::strncpy(device.name.name, ds3_device_name, sizeof(device.name));
            }
            else {
                std::strncpy(device.name2, ds3_device_name, sizeof(device.name2));
            }

            R_RETURN(btdrvAddPairedDeviceInfo(&device));
        }

        void SignalBondComplete(const BtdrvAddress *address) {
            if (hos::GetVersion() < hos::Version_9_0_0) {
                const struct {
                    BtdrvAddress addr;
                    u8 pad[2];
                    u32 status;
                    u32 type;
                } bond_event = { *address, {0}, 0,  BtdrvConnectionEventType_Suspended };
                
                bluetooth::core::SignalFakeEvent(BtdrvEventTypeOld_Connection, &bond_event, sizeof(bond_event));
            } else if (hos::GetVersion() < hos::Version_12_0_0) {
                const struct {
                    u32 status;
                    BtdrvAddress addr;
                    u8 pad[2];
                    u32 type;
                } bond_event = { 0, *address, {0}, BtdrvConnectionEventType_Suspended };

                bluetooth::core::SignalFakeEvent(BtdrvEventTypeOld_Connection, &bond_event, sizeof(bond_event));
            } else {
                const struct {
                    u32 type;
                    BtdrvAddress addr;
                    u8 reserved[0xfe];
                } bond_event = { BtdrvConnectionEventType_Suspended, *address, {0} };

                bluetooth::core::SignalFakeEvent(BtdrvEventType_Connection, &bond_event, sizeof(bond_event));
            }
        }

    }

    const UsbHsInterfaceFilter *Dualshock3Controller::GetUsbInterfaceFilter() {
        return &g_interface_filter;
    }

    bool Dualshock3Controller::UsbIdentify(UsbHsInterface *iface) {
        return (iface->device_desc.idVendor == ds3_vendor_id) && (iface->device_desc.idProduct == ds3_product_id);
    }

    Result Dualshock3Controller::UsbPair(UsbHsInterface *iface) {
        // Acquire usb:hs client interface session
        UsbHsClientIfSession if_session;
        R_TRY(usbHsAcquireUsbIf(&if_session, iface));

        // Close session on function exit
        ON_SCOPE_EXIT {
            if (usbHsIfIsActive(&if_session)) {
                usbHsIfClose(&if_session);
            }
        };

        // Fetch the console bluetooth address
        BtdrvAdapterProperty property;
        R_TRY(btdrvGetAdapterProperty(BtdrvAdapterPropertyType_Address, &property));

        // Set the console address as the master on the DS3
        BtdrvAddress master_address = *reinterpret_cast<BtdrvAddress *>(property.data);
        R_TRY(SetMasterAddress(&if_session, &master_address));

        // Get the address of the DS3
        BtdrvAddress slave_address;
        R_TRY(GetSlaveAddress(&if_session, &slave_address));

        // Add DS3 to list of trusted devices
        R_TRY(TrustDevice(&slave_address));
        
        // Signal fake bonding success event for btm
        SignalBondComplete(&slave_address);

        R_SUCCEED();
    }

    Result Dualshock3Controller::Initialize() {
        R_TRY(EmulatedSwitchController::Initialize());
        R_TRY(this->SendEnablePayload());

        R_SUCCEED();
    }

    Result Dualshock3Controller::SetVibration(const SwitchRumbleData *rumble_data) {
        m_rumble_state.amp_motor_left  = static_cast<u8>(255 * std::max(rumble_data[0].low_band_amp, rumble_data[1].low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<u8>(255 * std::max(rumble_data[0].high_band_amp, rumble_data[1].high_band_amp));
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock3Controller::CancelVibration() {
        m_rumble_state.amp_motor_left = 0;
        m_rumble_state.amp_motor_right = 0;
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock3Controller::SetPlayerLed(u8 led_mask) {
        u8 player_index;
        R_TRY(LedsMaskToPlayerNumber(led_mask, &player_index));

        auto config = mitm::GetGlobalConfig();
        switch(config->misc.dualshock3_led_mode) {
            case Dualshock3LedMode_Switch:
                m_led_mask = player_led_patterns[player_index];
                break;
            case Dualshock3LedMode_Ps3:
                m_led_mask = player_index < 4 ? 1 << player_index : ~(1 << player_index) & 0x0f;
                break;
            case Dualshock3LedMode_Hybrid:
                m_led_mask = led_mask;
                break;
            default:
                break;
        };

        R_RETURN(this->PushRumbleLedState());
    }

    void Dualshock3Controller::ProcessInputData(const bluetooth::HidReport *report) {
        auto ds3_report = reinterpret_cast<const Dualshock3ReportData *>(&report->data);

        switch(ds3_report->id) {
            case 0x01:
                this->MapInputReport0x01(ds3_report); break;
            default:
                break;
        }
    }

    void Dualshock3Controller::MapInputReport0x01(const Dualshock3ReportData *src) {
        m_charging = src->input0x01.charge == 0x02;
        m_battery = std::clamp<u8>(src->input0x01.battery, 0, 4) * 2;

        // Workaround for controller reporting battery empty and being disconnected under certain conditions
        if (m_battery == 0) {
            m_battery = 1;
        }

        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & UINT12_MAX
        );

        m_buttons.dpad_down  = src->input0x01.buttons.dpad_down;
        m_buttons.dpad_up    = src->input0x01.buttons.dpad_up;
        m_buttons.dpad_right = src->input0x01.buttons.dpad_right;
        m_buttons.dpad_left  = src->input0x01.buttons.dpad_left;

        m_buttons.A = src->input0x01.buttons.circle;
        m_buttons.B = src->input0x01.buttons.cross;
        m_buttons.X = src->input0x01.buttons.triangle;
        m_buttons.Y = src->input0x01.buttons.square;

        m_buttons.R  = src->input0x01.buttons.R1;
        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.L  = src->input0x01.buttons.L1;
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * UINT8_MAX);

        m_buttons.minus = src->input0x01.buttons.select;
        m_buttons.plus  = src->input0x01.buttons.start;

        m_buttons.lstick_press = src->input0x01.buttons.L3;
        m_buttons.rstick_press = src->input0x01.buttons.R3;

        m_buttons.home = src->input0x01.buttons.ps;

        if (m_enable_motion) {
            s16 acc_x = -static_cast<s16>(accel_scale_factor * (511 - util::SwapEndian(src->input0x01.accel_y)));
            s16 acc_y = -static_cast<s16>(accel_scale_factor * (util::SwapEndian(src->input0x01.accel_x) - 511));
            s16 acc_z =  static_cast<s16>(accel_scale_factor * (511 - util::SwapEndian(src->input0x01.accel_z)));

            m_motion_data[0].accel_x = acc_x;
            m_motion_data[0].accel_y = acc_y;
            m_motion_data[0].accel_z = acc_z;

            m_motion_data[1].accel_x = acc_x;
            m_motion_data[1].accel_y = acc_y;
            m_motion_data[1].accel_z = acc_z;

            m_motion_data[2].accel_x = acc_x;
            m_motion_data[2].accel_y = acc_y;
            m_motion_data[2].accel_z = acc_z;
        } else {
            std::memset(&m_motion_data, 0, sizeof(m_motion_data));
        }
    }

    Result Dualshock3Controller::SendEnablePayload() {
        m_output_report.size = sizeof(enable_payload);
        std::memcpy(m_output_report.data, enable_payload, m_output_report.size);

        R_RETURN(this->SetReport(BtdrvBluetoothHhReportType_Feature, &m_output_report));
    }

    Result Dualshock3Controller::PushRumbleLedState() {
        std::scoped_lock lk(m_output_mutex);

        Dualshock3ReportData report = {};
        report.id = 0x01;
        report.output0x01.data[1] = 10;
        report.output0x01.data[2] = m_rumble_state.amp_motor_right ? 1 : 0;
        report.output0x01.data[3] = 10;
        report.output0x01.data[4] = m_rumble_state.amp_motor_left;
        report.output0x01.data[9] = m_led_mask << 1;
        std::memcpy(&report.output0x01.data[10], led_config, sizeof(led_config));
        std::memcpy(&report.output0x01.data[15], led_config, sizeof(led_config));
        std::memcpy(&report.output0x01.data[20], led_config, sizeof(led_config));
        std::memcpy(&report.output0x01.data[25], led_config, sizeof(led_config));

        m_output_report.size = sizeof(report.output0x01) + sizeof(report.id);
        std::memcpy(m_output_report.data, &report, m_output_report.size);

        R_RETURN(this->SetReport(BtdrvBluetoothHhReportType_Output, &m_output_report));
    }

}
