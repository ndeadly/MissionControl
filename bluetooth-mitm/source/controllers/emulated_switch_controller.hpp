/*
 * Copyright (c) 2020 ndeadly
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
#include "switch_controller.hpp"

namespace ams::controller {

    class EmulatedSwitchController : public SwitchController {

        public:
            EmulatedSwitchController(const bluetooth::Address *address);
            
            Result HandleIncomingReport(const bluetooth::HidReport *report);
            Result HandleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            void ClearControllerState(void);
            virtual void UpdateControllerState(const bluetooth::HidReport *report) {};
            void ApplyButtonCombos(SwitchButtonData *buttons);

            virtual Result SetVibration(void) { return ams::ResultSuccess(); };
            virtual Result SetPlayerLed(uint8_t led_mask) { return ams::ResultSuccess(); };

            constexpr void PackStickData(SwitchStickData *stick, uint16_t x, uint16_t y) {
                *stick = (SwitchStickData){
                    static_cast<uint8_t>(x & 0xff), 
                    static_cast<uint8_t>((x >> 8) | ((y & 0xff) << 4)), 
                    static_cast<uint8_t>((y >> 4) & 0xff)
                };
            }

            Result HandleSubCmdReport(const bluetooth::HidReport *report);
            Result SubCmdRequestDeviceInfo(const bluetooth::HidReport *report);
            Result SubCmdSpiFlashRead(const bluetooth::HidReport *report);
            Result SubCmdSpiFlashWrite(const bluetooth::HidReport *report);
            Result SubCmdSpiSectorErase(const bluetooth::HidReport *report);
            Result SubCmdSetInputReportMode(const bluetooth::HidReport *report);
            Result SubCmdTriggersElapsedTime(const bluetooth::HidReport *report);
            Result SubCmdSetShipPowerState(const bluetooth::HidReport *report);
            Result SubCmdSetMcuConfig(const bluetooth::HidReport *report);
            Result SubCmdSetMcuState(const bluetooth::HidReport *report);
            Result SubCmdSetPlayerLeds(const bluetooth::HidReport *report);
            Result SubCmdSetHomeLed(const bluetooth::HidReport *report);
            Result SubCmdEnableImu(const bluetooth::HidReport *report);
            Result SubCmdEnableVibration(const bluetooth::HidReport *report);

            Result FakeSubCmdResponse(const uint8_t response[], size_t size);

            bool    m_charging;
            uint8_t m_battery;
            SwitchButtonData m_buttons;
            SwitchStickData  m_left_stick;
            SwitchStickData  m_right_stick;
            Switch6AxisData  m_motion_data[3];

            ProControllerColours m_colours;

            static bluetooth::HidReport s_input_report;
            static bluetooth::HidReport s_output_report;
    };

}
