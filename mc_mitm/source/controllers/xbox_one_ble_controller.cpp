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
#include "xbox_one_ble_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        // HOGP strips report ids from the data; the input report we forward is report 0x01
        constexpr u8 InputReportId = 0x01;

    }

    Result XboxOneBleController::HandleInputReport(const u8 *data, size_t size) {
        // Only the 9.0.0+ event layout is filled in; controllers only started using BLE in 2021
        R_UNLESS(hos::GetVersion() >= hos::Version_9_0_0, sf::ResultNotSupported());

        auto &event = m_event_info.data_report.v9;
        R_UNLESS(size + 1 <= sizeof(event.report.data), svc::ResultInvalidSize());

        event.res = 0;
        event.proto_mode = 0;
        event.addr = m_address;
        event.pad = 0;
        event.report.size = size + 1;
        event.report.data[0] = InputReportId;
        std::memcpy(&event.report.data[1], data, size);

        R_RETURN(XboxOneController::HandleDataReportEvent(&m_event_info));
    }

    void XboxOneBleController::ProcessInputData(const bluetooth::HidReport *report) {
        XboxOneController::ProcessInputData(report);

        // The Bluetooth LE firmware shuffles the last two button bytes compared to the classic layout the base
        // class decodes: View moved to bit 2 of the third button byte, and the trailing byte that used to hold
        // View now carries the Share button of Series X|S pads.
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(&report->data);
        if ((xbox_report->id == InputReportId) && (report->size >= sizeof(XboxOneInputReport0x01) + 1)) {
            const u8 *buttons = reinterpret_cast<const u8 *>(&xbox_report->input0x01.buttons);
            m_buttons.minus   = (buttons[2] & 0x04) != 0;
            m_buttons.capture = (buttons[3] & 0x01) != 0;
        }
    }

    Result XboxOneBleController::SetVibration(const SwitchMotorData *motor_data) {
        // hid includes rumble data in every output report it sends, at around 60Hz. Turning each one into a GATT
        // write saturates the LE link and starves the input notifications, so only send changes.
        const u8 strong = static_cast<u8>(100 * std::max(motor_data->left_motor.low_band_amp, motor_data->right_motor.low_band_amp));
        const u8 weak   = static_cast<u8>(100 * std::max(motor_data->left_motor.high_band_amp, motor_data->right_motor.high_band_amp));
        if ((strong == m_last_rumble_strong) && (weak == m_last_rumble_weak)) {
            R_SUCCEED();
        }

        m_last_rumble_strong = strong;
        m_last_rumble_weak = weak;

        R_RETURN(XboxOneController::SetVibration(motor_data));
    }

    Result XboxOneBleController::WriteDataReport(const bluetooth::HidReport *report) {
        // Strip the report id: over GATT the characteristic itself identifies the report
        R_UNLESS(report->size >= 1, svc::ResultInvalidSize());
        R_RETURN(btdrvWriteGattCharacteristic(m_conn_id, m_service_primary, &m_service_id, &m_output_report_id, &report->data[1], report->size - 1, BtdrvGattAuthReqType_NoMitm, false));
    }

}
