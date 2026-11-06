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
#include "controller_management.hpp"
#include "../ble/ble_events.hpp"
#include <algorithm>
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        // HOGP strips report ids from the data; the input report we forward is report 0x01
        constexpr u8 InputReportId = 0x01;

    }

    Result XboxOneBleController::HandleInputReport(const u8 *data, size_t size) {
        // Only the 9.0.0+ event layout is filled in; controllers only started using BLE in 2021
        R_UNLESS(hos::GetVersion() >= hos::Version_9_0_0, sf::ResultNotSupported());

        std::scoped_lock lk(m_report_mutex);

        R_UNLESS(m_connected, svc::ResultInvalidState());
        // Xbox One BLE pads can omit the Series share byte. Other HID Report
        // characteristics (battery/guide) must never be decoded as stick data.
        constexpr size_t InputSize = sizeof(XboxOneInputReport0x01);
        R_UNLESS((size == InputSize) || (size == InputSize - 1), svc::ResultInvalidSize());
        auto &event = m_event_info.data_report.v9;

        event.res = 0;
        event.proto_mode = 0;
        event.addr = m_address;
        event.pad = 0;
        event.report.size = sizeof(XboxOneInputReport0x01) + 1;
        event.report.data[0] = InputReportId;
        event.report.data[sizeof(XboxOneInputReport0x01)] = 0;
        std::memcpy(&event.report.data[1], data, size);

        XboxOneController::HandleDataReportEvent(&m_event_info);
        R_SUCCEED();
    }

    Result XboxOneBleController::HandleOutputDataReport(const u8 *report_buffer, size_t size) {
        R_UNLESS(size >= 1, svc::ResultInvalidSize());

        const auto report_id = static_cast<SwitchHidReportId>(report_buffer[0]);
        if (report_id == SwitchHidReportId::BasicOutputReport) {
            std::scoped_lock lk(m_rumble_mutex);
            R_UNLESS(m_connected, svc::ResultInvalidState());
            R_RETURN(XboxOneController::HandleOutputDataReport(report_buffer, size));
        }

        std::scoped_lock lk(m_report_mutex, m_rumble_mutex);
        R_UNLESS(m_connected, svc::ResultInvalidState());
        R_RETURN(XboxOneController::HandleOutputDataReport(report_buffer, size));
    }

    void XboxOneBleController::Disconnect() {
        std::scoped_lock lk(m_report_mutex, m_rumble_mutex);
        m_connected = false;
    }

    void XboxOneBleController::ParseInputReport(const u8 *report_buffer, size_t size) {
        XboxOneController::ParseInputReport(report_buffer, size);

        // The Bluetooth LE firmware shuffles the last two button bytes compared to the classic layout the base
        // class decodes: View moved to bit 2 of the second button byte, and the third button byte that used to hold
        // View now carries the Share button of Series X|S pads.
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(report_buffer);
        if ((xbox_report->id == InputReportId) && (size >= sizeof(XboxOneInputReport0x01) + 1)) {
            const u8 *buttons = reinterpret_cast<const u8 *>(&xbox_report->input0x01.buttons);
            m_buttons.Assign(SwitchButton::Minus,   (buttons[1] & 0x04) != 0);
            m_buttons.Assign(SwitchButton::Capture, (buttons[2] & 0x01) != 0);
        }
    }

    Result XboxOneBleController::SetVibration(const SwitchMotorData *motor_data) {
        if (!m_has_output_report) {
            R_SUCCEED();
        }

        // hid includes rumble data in every output report it sends, at around 60Hz. Turning each one into a GATT
        // write saturates the LE link and starves the input notifications, so throttle updates and use a 250ms sustain.
        constexpr s64 MinRumbleIntervalNs                  = TimeSpan::FromMilliSeconds(32).GetNanoSeconds();
        constexpr s64 MultiControllerMinRumbleIntervalNs   = TimeSpan::FromMilliSeconds(64).GetNanoSeconds();
        constexpr s64 HeavyMultiControllerRumbleIntervalNs = TimeSpan::FromMilliSeconds(96).GetNanoSeconds();

        const u8 strong = static_cast<u8>(100 * std::max(motor_data->left_motor.low_band_amp, motor_data->right_motor.low_band_amp));
        const u8 weak   = static_cast<u8>(100 * std::max(motor_data->left_motor.high_band_amp, motor_data->right_motor.high_band_amp));

        const s64 now_tick = os::GetSystemTick().GetInt64Value();
        const TimeSpan elapsed_time = (m_last_rumble_tick > 0)
            ? os::Tick(now_tick - m_last_rumble_tick).ToTimeSpan()
            : TimeSpan::FromSeconds(1);

        const s64 elapsed_ms = elapsed_time.GetMilliSeconds();
        const s64 elapsed_ns = elapsed_time.GetNanoSeconds();

        const bool is_stop = (strong == 0) && (weak == 0);
        const bool was_stopped = (m_last_rumble_strong == 0) && (m_last_rumble_weak == 0);

        if (is_stop) {
            if (was_stopped) {
                R_SUCCEED();
            }
        } else {
            if ((strong == m_last_rumble_strong) && (weak == m_last_rumble_weak) && (elapsed_ms < 180)) {
                R_SUCCEED();
            }
            // Stop packets remain immediate, but stop/start bursts must not bypass
            // the write budget (menus can generate many short rumble pulses).
            const size_t total_controllers = controller::GetConnectedControllerCount();
            const s64 min_rumble_interval_ns = (total_controllers >= 4) ? HeavyMultiControllerRumbleIntervalNs
                                             : (total_controllers >= 2) ? MultiControllerMinRumbleIntervalNs
                                             : MinRumbleIntervalNs;
            if (elapsed_ns < min_rumble_interval_ns) R_SUCCEED();
        }

        XboxOneReportData report = {};
        report.id = 0x03;
        report.output0x03.enable             = 0x03;
        report.output0x03.magnitude_strong   = strong;
        report.output0x03.magnitude_weak     = weak;
        report.output0x03.pulse_sustain_10ms = is_stop ? 0 : 25;
        report.output0x03.pulse_release_10ms = 0;
        report.output0x03.loop_count         = 0;

        R_TRY(this->WriteDataReport(&report, sizeof(XboxOneOutputReport0x03) + 1));
        m_last_rumble_strong = strong;
        m_last_rumble_weak = weak;
        m_last_rumble_tick = now_tick;
        R_SUCCEED();
    }

    Result XboxOneBleController::WriteDataReport(const void *report_buffer, size_t size) {
        R_UNLESS(ble::events::IsRadioEnabled(), svc::ResultInvalidState());
        if (!m_has_output_report) {
            R_SUCCEED();
        }
        // Strip the report id: over GATT the characteristic itself identifies the report
        R_UNLESS(size >= 1, svc::ResultInvalidSize());
        const u8 *data = static_cast<const u8 *>(report_buffer);
        R_RETURN(btdrvWriteGattCharacteristic(m_conn_id, m_service_primary, &m_service_id, &m_output_report_id, &data[1], size - 1, BtdrvGattAuthReqType_NoMitm, false));
    }

}
