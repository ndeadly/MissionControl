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
#include "switch_mcu_command_processor.hpp"

namespace ams::controller {

    namespace {

        // constexpr u8 ComputeCrc8(const void *data, size_t size) {
        //     return utils::Crc8<7>::Calculate(data, size);
        // }

    }

    void SwitchMcuCommandProcessor::McuResume(bool enable) {
        if (!enable){
            m_mcu_mode = SwitchMcuMode::Suspended;
        } else if (m_mcu_mode == SwitchMcuMode::Suspended) {
            m_mcu_mode = SwitchMcuMode::Standby;
        }
    }

    void SwitchMcuCommandProcessor::McuConfigure(SwitchMcuMode mode) {
        if (m_mcu_mode == SwitchMcuMode::Standby){
            m_mcu_mode = mode;
        }
    }

    void SwitchMcuCommandProcessor::McuWrite() {

    }

    void SwitchMcuCommandProcessor::GetStatus(void *buffer) {
        if (m_mcu_mode == SwitchMcuMode::Suspended || m_mcu_mode == SwitchMcuMode::Busy) {
            const u8 data[] = {
                0x01, 0x00, 0xff, 0x00, 0x08, 0x00, 0x1b, 0x06,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0xf6
            };
            std::memcpy(buffer, data, sizeof(data));
        } else {
            const u8 data[] = {
                0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x1b, 0x01,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0xef
            };
            std::memcpy(buffer, data, sizeof(data));
        }
    }

    void SwitchMcuCommandProcessor::ProcessCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response) {
        switch (command->sub_command) {
            case SwitchMcuSubCommandId::SetMcuMode:
                this->ProcessSetMcuModeCommand(command, response);
                break;

            case SwitchMcuSubCommandId::GetMcuMode:
                this->ProcessGetMcuModeCommand(command, response);
                break;

            case SwitchMcuSubCommandId::ReadDeviceMode:
                this->ProcessReadDeviceModeCommand(command, response);
                break;

            // case SwitchMcuSubCommandId::WriteDeviceRegisters:
            //     this->ProcessWriteDeviceRegistersCommand(command, response);
            //     break;
            default: {
                // Send device not ready response for now
                response->command = SwitchMcuCommandId::EmptyAwaitingCmd;
                response->data.get_mcu_mode.mode = m_mcu_mode;
            }
        }
    }

    void SwitchMcuCommandProcessor::ProcessSetMcuModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response) {
        AMS_UNUSED(command);
        response->command = SwitchMcuCommandId::EmptyAwaitingCmd;
    }

    void SwitchMcuCommandProcessor::ProcessGetMcuModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response) {
        AMS_UNUSED(command);
        response->command = SwitchMcuCommandId::StateReport;
        response->data.get_mcu_mode = {
            .unknown_1 = 0x08,
            .unknown_2 = 0x1b,
            .mode      = m_mcu_mode
        };
    }

    void SwitchMcuCommandProcessor::ProcessReadDeviceModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response) {
        AMS_UNUSED(command);
        response->command = SwitchMcuCommandId::NfcState;
        response->data.read_device_mode = {
            .unknown_1 = 0x05,
            .unknown_2 = 0x09,
            .unknown_3 = 0x31,
            .is_ready  = 0x01
        };
    }

    // void SwitchMcuCommandProcessor::ProcessWriteDeviceRegistersCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response) {
    //     response->command = 
    // }

}
