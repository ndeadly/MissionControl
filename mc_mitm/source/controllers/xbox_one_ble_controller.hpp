/*
 * Copyright (c) 2020-2026 ndeadly
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
#pragma once
#include "xbox_one_controller.hpp"

namespace ams::controller {

    /*
     * Xbox controller connected over Bluetooth LE (firmware 5.x and newer, Series X|S).
     *
     * The HID-over-GATT input report uses the same layout as the classic Bluetooth one minus the leading report
     * id, so input is re-framed and handed to the regular XboxOneController mapping. Output (rumble) goes back as
     * a GATT write to the HID output Report characteristic instead of over an L2CAP HID channel.
     */
    class XboxOneBleController : public XboxOneController {

        public:
            XboxOneBleController(bluetooth::Address address, u32 conn_id, const BtdrvGattId &service_id, bool service_primary, const BtdrvGattId &output_report_id, bool has_output_report = true)
            : XboxOneController(address, {0x045e, 0x0b13})
            , m_conn_id(conn_id)
            , m_service_id(service_id)
            , m_service_primary(service_primary)
            , m_output_report_id(output_report_id)
            , m_has_output_report(has_output_report) { }

            // Payload of a notification from the HID input Report characteristic
            Result HandleInputReport(const u8 *data, size_t size);
            Result HandleOutputDataReport(const u8 *report_buffer, size_t size) override;

            void WaitIdle();

        protected:
            void ParseInputReport(const u8 *report_buffer, size_t size) override;
            Result SetVibration(const SwitchMotorData *motor_data) override;
            Result WriteDataReport(const void *report_buffer, size_t size) override;

        private:
            os::SdkMutex m_report_mutex;
            os::SdkMutex m_rumble_mutex;
            u32 m_conn_id;
            BtdrvGattId m_service_id;
            bool m_service_primary;
            BtdrvGattId m_output_report_id;
            bool m_has_output_report;

            u8 m_last_rumble_strong = 0;
            u8 m_last_rumble_weak = 0;
            s64 m_last_rumble_tick = 0;

            bluetooth::HidReportEventInfo m_event_info;
    };

}
