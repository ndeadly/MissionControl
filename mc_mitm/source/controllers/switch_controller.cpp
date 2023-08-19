/*
 * Copyright (c) 2020-2023 ndeadly
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
#include "switch_controller.hpp"
#include "../mcmitm_config.hpp"
#include <string>

namespace ams::controller {

    namespace {

        const u8 led_player_mappings[] = {
            SwitchPlayerNumber_Unknown, //0000
            SwitchPlayerNumber_One,     //0001
            SwitchPlayerNumber_Unknown, //0010
            SwitchPlayerNumber_Two,     //0011
            SwitchPlayerNumber_Unknown, //0100
            SwitchPlayerNumber_Six,     //0101
            SwitchPlayerNumber_Eight,   //0110
            SwitchPlayerNumber_Three,   //0111
            SwitchPlayerNumber_One,     //1000
            SwitchPlayerNumber_Five,    //1001
            SwitchPlayerNumber_Six,     //1010
            SwitchPlayerNumber_Seven,   //1011
            SwitchPlayerNumber_Two,     //1100
            SwitchPlayerNumber_Seven,   //1101
            SwitchPlayerNumber_Three,   //1110
            SwitchPlayerNumber_Four,    //1111
        };

    }

    Result LedsMaskToPlayerNumber(u8 led_mask, u8 *player_number) {
        *player_number = led_player_mappings[(led_mask & 0xf) | (led_mask >> 4)];
        if (*player_number == SwitchPlayerNumber_Unknown) {
            return -1;
        }

        R_SUCCEED();
    }

    std::string GetControllerDirectory(const bluetooth::Address *address) {
        char path[0x100];
        util::SNPrintf(path, sizeof(path), "sdmc:/config/MissionControl/controllers/%02x%02x%02x%02x%02x%02x",
            address->address[0],
            address->address[1],
            address->address[2],
            address->address[3],
            address->address[4],
            address->address[5]
        );
        return path;
    }

    Result SwitchController::Initialize() {
        R_SUCCEED();
    }

    Result SwitchController::HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        const bluetooth::HidReport *report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            report = &event_info->data_report.v9.report;
        } else if (hos::GetVersion() >= hos::Version_7_0_0) {
            report = reinterpret_cast<const bluetooth::HidReport *>(&event_info->data_report.v7.report);
        } else {
            report = reinterpret_cast<const bluetooth::HidReport *>(&event_info->data_report.v1.report);
        }

        if (!m_future_responses.empty()) {
            if ((m_future_responses.front()->GetType() == BtdrvHidEventType_Data) && (m_future_responses.front()->GetUserData() == report->data[0])) {
                m_future_responses.front()->SetData(*event_info);
            }
        }

        std::scoped_lock lk(m_input_mutex);

        this->UpdateControllerState(report);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        if (input_report->id == 0x21) {
            if (input_report->type0x21.hid_command_response.id == HidCommand_SerialFlashRead) {
                if (input_report->type0x21.hid_command_response.data.serial_flash_read.address == 0x6050) {
                    if (ams::mitm::GetSystemLanguage() == 10) {
                        u8 data[] = {0xff, 0xd7, 0x00, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7};
                        std::memcpy(input_report->type0x21.hid_command_response.data.serial_flash_read.data, data, sizeof(data));
                    }
                }
            }
        }

        this->ApplyButtonCombos(&input_report->buttons); 

        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report));
    }

    Result SwitchController::HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        if (!m_future_responses.empty()) {
            if (m_future_responses.front()->GetType() == BtdrvHidEventType_SetReport) {
                m_future_responses.front()->SetData(*event_info);
            }

            R_SUCCEED();
        }

        R_RETURN(bluetooth::hid::report::WriteHidSetReport(m_address, event_info->set_report.res));
    }

    Result SwitchController::HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        if (!m_future_responses.empty()) {
            if (m_future_responses.front()->GetType() == BtdrvHidEventType_GetReport) {
                m_future_responses.front()->SetData(*event_info);
            }

            R_SUCCEED();
        }

        auto report = hos::GetVersion() >= hos::Version_9_0_0 ? &event_info->get_report.v9.report : reinterpret_cast<const bluetooth::HidReport *>(&event_info->get_report.v1.report);
        R_RETURN(bluetooth::hid::report::WriteHidGetReport(m_address, report));
    }

    Result SwitchController::HandleOutputDataReport(const bluetooth::HidReport *report) {
        R_RETURN(this->WriteDataReport(report));
    }

    Result SwitchController::WriteDataReport(const bluetooth::HidReport *report) {
        R_RETURN(btdrvWriteHidData(m_address, report));
    }

    Result SwitchController::WriteDataReport(const bluetooth::HidReport *report, u8 response_id, bluetooth::HidReport *out_report) {       
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_Data);
        response->SetUserData(response_id);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        R_TRY(btdrvWriteHidData(m_address, report));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();

        const bluetooth::HidReport *data_report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            data_report = &response_data.data_report.v9.report;
        } else if (hos::GetVersion() >= hos::Version_7_0_0) {
            data_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.data_report.v7.report);
        } else {
            data_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.data_report.v1.report);
        }

        out_report->size = data_report->size;
        std::memcpy(&out_report->data, &data_report->data, data_report->size);

        R_SUCCEED();
    }

    Result SwitchController::SetReport(BtdrvBluetoothHhReportType type, const bluetooth::HidReport *report) {
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_SetReport);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        R_TRY(btdrvSetHidReport(m_address, type, report));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();

        return response_data.set_report.res;
    }

    Result SwitchController::GetReport(u8 id, BtdrvBluetoothHhReportType type, bluetooth::HidReport *out_report) {
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_GetReport);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        R_TRY(btdrvGetHidReport(m_address, id, type));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();
        
        Result result;
        const bluetooth::HidReport *get_report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            result = response_data.get_report.v9.res;
            get_report = &response_data.get_report.v9.report;
        } else {
            result = response_data.get_report.v1.res;
            get_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.get_report.v1.report);
        }

        if (R_SUCCEEDED(result)) {
            out_report->size = get_report->size;
            std::memcpy(&out_report->data, &get_report->data, get_report->size);
        }

        return result;
    }

    void SwitchController::UpdateControllerState(const bluetooth::HidReport *report) {
        m_input_report.size = report->size;
        std::memcpy(m_input_report.data, report->data, report->size);
    }

    void SwitchController::ApplyButtonCombos(SwitchButtonData *buttons) {
        // Home combo = MINUS + DPAD_DOWN
        if (buttons->minus && buttons->dpad_down) {
            buttons->home = 1;
            buttons->minus = 0;
            buttons->dpad_down = 0;
        }

        // Capture combo = MINUS + DPAD_UP
        if (buttons->minus && buttons->dpad_up) {
            buttons->capture = 1;
            buttons->minus = 0;
            buttons->dpad_up = 0;
        }
    }

}
