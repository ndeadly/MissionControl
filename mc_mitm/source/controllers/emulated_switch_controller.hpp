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
#pragma once
#include "switch_controller.hpp"
#include "virtual_spi_flash.hpp"

namespace ams::controller {

    inline uint8_t ScaleRumbleAmplitude(float amp, uint8_t lower, uint8_t upper) {
        return amp > 0.0 ? static_cast<uint8_t>(amp * (upper - lower) + lower) : 0;
    }

    class EmulatedSwitchController : public SwitchController {

        public:
            EmulatedSwitchController(const bluetooth::Address *address, HardwareID id);
            virtual ~EmulatedSwitchController() {};

            virtual Result Initialize();
            bool IsOfficialController() { return false; }

            Result HandleOutputDataReport(const bluetooth::HidReport *report) override;

        protected:
            void ClearControllerState();
            virtual Result SetVibration(const SwitchRumbleData *rumble_data) { AMS_UNUSED(rumble_data); return ams::ResultSuccess(); }
            virtual Result CancelVibration() { return ams::ResultSuccess(); }
            virtual Result SetPlayerLed(uint8_t led_mask) { AMS_UNUSED(led_mask); return ams::ResultSuccess(); }

            void UpdateControllerState(const bluetooth::HidReport *report) override;
            virtual void ProcessInputData(const bluetooth::HidReport *report) { AMS_UNUSED(report); }

            Result HandleRumbleData(const SwitchRumbleDataEncoded *encoded);
            Result HandleSubcommand(const SwitchSubcommand *subcmd);

            Result SubCmdRequestDeviceInfo(const SwitchSubcommand *subcmd);
            Result SubCmdSetInputReportMode(const SwitchSubcommand *subcmd);
            Result SubCmdTriggersElapsedTime(const SwitchSubcommand *subcmd);
            Result SubCmdResetPairingInfo(const SwitchSubcommand *subcmd);
            Result SubCmdSetShipPowerState(const SwitchSubcommand *subcmd);
            Result SubCmdSpiFlashRead(const SwitchSubcommand *subcmd);
            Result SubCmdSpiFlashWrite(const SwitchSubcommand *subcmd);
            Result SubCmdSpiSectorErase(const SwitchSubcommand *subcmd);
            Result SubCmd0x24(const SwitchSubcommand *subcmd);
            Result SubCmd0x25(const SwitchSubcommand *subcmd);
            Result SubCmdSetMcuConfig(const SwitchSubcommand *subcmd);
            Result SubCmdSetMcuState(const SwitchSubcommand *subcmd);
            Result SubCmdSetPlayerLeds(const SwitchSubcommand *subcmd);
            Result SubCmdGetPlayerLeds(const SwitchSubcommand *subcmd);
            Result SubCmdSetHomeLed(const SwitchSubcommand *subcmd);
            Result SubCmdEnableImu(const SwitchSubcommand *subcmd);
            Result SubCmdSetImuSensitivity(const SwitchSubcommand *subcmd);
            Result SubCmdEnableVibration(const SwitchSubcommand *subcmd);

            Result FakeSubCmdResponse(const SwitchSubcommandResponse *response);

            bool m_charging;
            bool m_ext_power;
            uint8_t m_battery;
            uint8_t m_led_pattern;

            SwitchButtonData m_buttons;
            SwitchAnalogStick m_left_stick;
            SwitchAnalogStick m_right_stick;
            Switch6AxisData m_motion_data[3];

            uint16_t m_gyro_sensitivity;
            uint16_t m_acc_sensitivity;

            uint8_t m_input_report_mode;

            bool m_enable_rumble;
            bool m_enable_motion;

            VirtualSpiFlash m_virtual_memory;
    };

}
