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
#include "default_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    Result DefaultController::Initialize(void) {
        // Write empty Switch report into buffer to kick off connection 
        R_TRY(EmulatedSwitchController::Initialize());
        s_output_report.size = sizeof(SwitchInputReport0x30) + 1;
        auto switch_report = reinterpret_cast<SwitchReportData *>(s_output_report.data);
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = BATTERY_MAX;
        std::memset(switch_report->input0x30.motion, 0, sizeof(switch_report->input0x30.motion));
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
        R_TRY(bluetooth::hid::report::WriteHidReportBuffer(&m_address, &s_output_report));
        return ams::ResultSuccess();
    }

    Result DefaultController::HandleIncomingReport(const bluetooth::HidReport *report) { 
        // Drop all incoming data for now
        return ams::ResultSuccess(); 
    };

}
