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
#include "switch_hid_command_processor.hpp"
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    void SwitchExtGripCommandProcessor::ProcessCommand(const SwitchExtGripCommand *command) {
        SwitchExtGripCommandResponse response = {};

        switch (command->id) {
            case SwitchExtGripCommandId::Inquiry:             this->ProcessInquiry(&response);             break;
            case SwitchExtGripCommandId::CreateHidConnection: this->ProcessCreateHidConnection(&response); break;
            case SwitchExtGripCommandId::DeleteHidConnection: this->ProcessDeleteHidConnection(&response); break;
            case SwitchExtGripCommandId::StartHidData:        this->ProcessStartHidData(&response);        break;
            case SwitchExtGripCommandId::StopHidData:         this->ProcessStopHidData(&response);         break;
            case SwitchExtGripCommandId::Command0x06:         this->ProcessCommand0x06(&response);         break;
            case SwitchExtGripCommandId::GetFirmwareVersion:  this->ProcessGetFirmwareVersion(&response);  break;
            case SwitchExtGripCommandId::Command0x08:         this->ProcessCommand0x08(&response);         break;
            case SwitchExtGripCommandId::LoopbackTest:        this->ProcessLoopbackTest(&response);        break;
            case SwitchExtGripCommandId::Command0x92:         this->ProcessCommand0x92(&response);         break;
            case SwitchExtGripCommandId::SetUartClockTrim:    this->ProcessSetUartClockTrim(&response);    break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        m_response = response;
    }

    void SwitchExtGripCommandProcessor::ProcessInquiry(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::Inquiry;
        response->inquiry = {
            .device_type = m_controller->m_device_info.device_type,
            .address     = m_controller->m_address
        };
    }

    void SwitchExtGripCommandProcessor::ProcessCreateHidConnection(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::CreateHidConnection;
    }

    void SwitchExtGripCommandProcessor::ProcessDeleteHidConnection(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::DeleteHidConnection;
    }

    void SwitchExtGripCommandProcessor::ProcessStartHidData(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::StartHidData;
    }

    void SwitchExtGripCommandProcessor::ProcessStopHidData(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::StopHidData;
    }

    void SwitchExtGripCommandProcessor::ProcessCommand0x06(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::Command0x06;
    }

    void SwitchExtGripCommandProcessor::ProcessGetFirmwareVersion(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::GetFirmwareVersion;
        response->get_firmware_version = {
            .version = { 0x02, 0x01, 0x0A, 0x00 }
        };
    }

    void SwitchExtGripCommandProcessor::ProcessCommand0x08(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::Command0x08;
        response->command0x08.unk = 0x01;
    }

    void SwitchExtGripCommandProcessor::ProcessLoopbackTest(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::LoopbackTest;
    }

    void SwitchExtGripCommandProcessor::ProcessCommand0x92(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::Command0x92;
    }

    void SwitchExtGripCommandProcessor::ProcessSetUartClockTrim(SwitchExtGripCommandResponse *response) {
        response->command_id = SwitchExtGripCommandId::SetUartClockTrim;
    }

}
