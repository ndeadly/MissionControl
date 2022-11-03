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
#include "forced_pro_controller.hpp"
#include "../mcmitm_config.hpp"

namespace ams::controller {

    ForcedProController::ForcedProController(const bluetooth::Address *address, HardwareID id)
    : SwitchController(address, id) {}


    Result ForcedProController::HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        const bluetooth::HidReport *report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            report = &event_info->data_report.v9.report;
        } else if (hos::GetVersion() >= hos::Version_7_0_0) {
            report = reinterpret_cast<const bluetooth::HidReport *>(&event_info->data_report.v7.report);
        } else {
            report = reinterpret_cast<const bluetooth::HidReport *>(&event_info->data_report.v1.report);
        }

        if (!m_future_responses.empty()) {
            if ((m_future_responses.back()->GetType() == BtdrvHidEventType_Data) && (m_future_responses.back()->GetUserData() == report->data[0])) {
                m_future_responses.back()->SetData(*event_info);
            }
        }

        std::scoped_lock lk(m_input_mutex);

        this->UpdateControllerState(report);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        if (input_report->id == 0x21) {
            auto response = &input_report->type0x21.hid_command_response;
            switch (response->id) {
                case HidCommand_SerialFlashRead:
                    if (response->data.serial_flash_read.address == 0x6050) {
                        if (ams::mitm::GetSystemLanguage() == 10) {
                            uint8_t data[] = {0xff, 0xd7, 0x00, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7};
                            std::memcpy(response->data.serial_flash_read.data, data, sizeof(data));
                        }
                        else {
                            uint8_t data[] = {0x32, 0x32, 0x32, 0xe6, 0xe6, 0xe6, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46};
                            std::memcpy(response->data.serial_flash_read.data, data, sizeof(data));
                        }
                    }
                    break;
                case HidCommand_GetDeviceInfo:
                    response->data.get_device_info.type = 0x03;
                    response->data.get_device_info._unk2 = 0x02;
                    break;
            }
        }

        this->ApplyButtonCombos(&input_report->buttons);

        return bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report);
    }

    Result ForcedProController::HandleOutputDataReport(const bluetooth::HidReport *report) {
        auto output_report = reinterpret_cast<const SwitchOutputReport *>(&report->data);
        if (output_report->id == 0x01 && (output_report->type0x01.hid_command.id == HidCommand_SerialFlashWrite || output_report->type0x01.hid_command.id == HidCommand_SerialFlashSectorErase)) {
            SwitchInputReport input_report = {
                .id = 0x21,
                .timer = 1,
                .conn_info = 1,
                .battery = BATTERY_MAX,
                .vibrator = 0,
            };

            input_report.type0x21.hid_command_response = {
                .ack = 0x80,
                .id = output_report->type0x01.hid_command.id,
                .data = {
                    .serial_flash_write = {
                        .status = 0x1 //Force write protected response just to be safe
                    }
                }
            };

            return bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report);
        }
        return this->WriteDataReport(report);
    }

}
