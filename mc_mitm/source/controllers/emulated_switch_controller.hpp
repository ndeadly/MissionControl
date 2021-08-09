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
#pragma once
#include "switch_controller.hpp"

namespace ams::controller {

    inline uint8_t ScaleRumbleAmplitude(float amp, uint8_t lower, uint8_t upper) {
        return amp > 0.0 ? static_cast<uint8_t>(amp * (upper - lower) + lower) : 0;
    }

    class EmulatedSwitchController : public SwitchController {

        public:
            EmulatedSwitchController(const bluetooth::Address *address, HardwareID id);
            virtual ~EmulatedSwitchController();

            virtual Result Initialize(void);
            bool IsOfficialController(void) { return false; }
            
            Result HandleIncomingReport(const bluetooth::HidReport *report);
            Result HandleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            void ClearControllerState(void);
            virtual void UpdateControllerState(const bluetooth::HidReport *report) { }
            virtual Result SetVibration(const SwitchRumbleData *rumble_data) { return ams::ResultSuccess(); }
            virtual Result CancelVibration(void) { return ams::ResultSuccess(); }
            virtual Result SetPlayerLed(uint8_t led_mask) { return ams::ResultSuccess(); }

            Result HandleSubCmdReport(const bluetooth::HidReport *report);
            Result HandleRumbleReport(const bluetooth::HidReport *report);

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

            Result FakeSubCmdResponse(const SwitchSubcommandResponse *response);

            Result VirtualSpiFlashRead(int offset, void *data, size_t size);
            Result VirtualSpiFlashWrite(int offset, const void *data, size_t size);
            Result VirtualSpiFlashSectorErase(int offset);

            bool m_charging;
            uint8_t m_battery;
            SwitchButtonData m_buttons;
            SwitchAnalogStick m_left_stick;
            SwitchAnalogStick m_right_stick;
            Switch6AxisData m_motion_data[3];

            ProControllerColours m_colours;
            bool m_enable_rumble;

            fs::FileHandle m_spi_flash_file;

    };

}
