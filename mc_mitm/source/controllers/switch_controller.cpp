/*
 * Copyright (c) 2020-2021 ndeadly
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
#include "../utils.hpp"
#include <string>

namespace ams::controller {

    namespace {

        const uint8_t led_player_mappings[] = {
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

    Result LedsMaskToPlayerNumber(uint8_t led_mask, uint8_t *player_number) {
        *player_number = led_player_mappings[led_mask & 0xf];
        if (*player_number == SwitchPlayerNumber_Unknown)
            return -1;

        return ams::ResultSuccess();
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

    bluetooth::HidReport SwitchController::s_input_report;
    bluetooth::HidReport SwitchController::s_output_report;

    Result SwitchController::Initialize(void) {
        if (this->HasSetTsiDisableFlag())
            m_settsi_supported = false;

        return ams::ResultSuccess(); 
    }

    bool SwitchController::HasSetTsiDisableFlag(void) {
        std::string flag_file = GetControllerDirectory(&m_address) + "/settsi_disable.flag";

        bool file_exists;
        if (R_SUCCEEDED(fs::HasFile(&file_exists, flag_file.c_str()))) {
            return file_exists;
        }

        return false;
    }

    Result SwitchController::HandleIncomingReport(const bluetooth::HidReport *report) {
        s_input_report.size = report->size;
	    std::memcpy(s_input_report.data, report->data, report->size);

        auto switch_report = reinterpret_cast<SwitchReportData *>(s_input_report.data);
        if (switch_report->id == 0x30) {
            this->ApplyButtonCombos(&switch_report->input0x30.buttons);
        }

        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, &s_input_report);
    }

    Result SwitchController::HandleOutgoingReport(const bluetooth::HidReport *report) {
        return bluetooth::hid::report::SendHidReport(&m_address, report);
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
