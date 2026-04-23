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
#include <stratosphere.hpp>
#include "switch_mcu_types.hpp"

namespace ams::controller {

    class SwitchMcuCommandProcessor {
        private:
            bool m_polling_enable;
            SwitchMcuMode m_mcu_mode;
            SwitchMcuResponse m_response;

        public:
            SwitchMcuCommandProcessor() : m_mcu_mode(SwitchMcuMode::Suspended), m_response{} { }

            void McuReset();
            void McuWrite();
            void McuResume(u8 mode);
            void McuPollingEnable(bool enable);
            // void McuConfigure(SwitchMcuMode mode);
            // void GetStatus(void *buffer);

            void ProcessCommand(const SwitchMcuCommand *command);
            SwitchMcuResponse GetResponse() { return m_response; }

        private:
            // void ProcessSetMcuModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response);
            // void ProcessGetMcuModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response);
            // void ProcessReadDeviceModeCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response);
            // void ProcessWriteDeviceRegistersCommand(const SwitchMcuCommand *command, SwitchMcuResponse *response);
    };

}
