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
#include "switch_hid_commands.hpp"

namespace ams::controller {

    class EmulatedSwitchController;

    class SwitchHidCommandProcessor {
        private:
            EmulatedSwitchController *m_controller;
            SwitchHidCommandResponse m_response;

        public:
            SwitchHidCommandProcessor(EmulatedSwitchController *controller) : m_controller(controller), m_response{} { }

            void ProcessCommand(const SwitchHidCommand *command);
            SwitchHidCommandResponse GetResponse() { return m_response; }

        private:
            void ProcessPairingOut(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessGetDeviceInfo(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSetDataFormat(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessLRButtonDetection(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessClearPairingInfo(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessShipment(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSerialFlashRead(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSerialFlashWrite(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSerialFlashSectorErase(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessMcuWrite(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessMcuResume(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessMcuPollingEnable(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessMcuPollingDisable(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSetIndicatorLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessGetIndicatorLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSetNotificationLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSensorSleep(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessSensorConfig(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
            void ProcessMotorEnable(const SwitchHidCommand *command, SwitchHidCommandResponse *response);
    };

}
