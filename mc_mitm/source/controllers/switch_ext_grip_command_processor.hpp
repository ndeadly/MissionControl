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
#include "switch_ext_grip_commands.hpp"

namespace ams::controller {

        class EmulatedSwitchController;

        class SwitchExtGripCommandProcessor {
            private:
                EmulatedSwitchController *m_controller;
                SwitchExtGripCommandResponse m_response;

            public:
                SwitchExtGripCommandProcessor(EmulatedSwitchController *controller) : m_controller(controller), m_response{} { }

                void ProcessCommand(const SwitchExtGripCommand *command);
                SwitchExtGripCommandResponse GetResponse() { return m_response; }

            private:
                void ProcessInquiry(SwitchExtGripCommandResponse *response);
                void ProcessCreateHidConnection(SwitchExtGripCommandResponse *response);
                void ProcessDeleteHidConnection(SwitchExtGripCommandResponse *response);
                void ProcessStartHidData(SwitchExtGripCommandResponse *response);
                void ProcessStopHidData(SwitchExtGripCommandResponse *response);
                void ProcessCommand0x06(SwitchExtGripCommandResponse *response);
                void ProcessGetFirmwareVersion(SwitchExtGripCommandResponse *response);
                void ProcessCommand0x08(SwitchExtGripCommandResponse *response);
                void ProcessLoopbackTest(SwitchExtGripCommandResponse *response);
                void ProcessCommand0x92(SwitchExtGripCommandResponse *response);
                void ProcessSetUartClockTrim(SwitchExtGripCommandResponse *response);
        };

}
