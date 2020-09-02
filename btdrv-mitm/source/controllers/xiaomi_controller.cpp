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
#include "xiaomi_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void XiaomiController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto xiaomi_report = reinterpret_cast<const XiaomiReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(xiaomi_report->id) {
            default:
                break;
        }

        out_report->size = sizeof(SwitchInputReport0x30) + 1;
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = m_battery | m_charging;
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

}
