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
#pragma once
#include "switch_controller.hpp"
#include "virtual_spi_flash.hpp"

namespace ams::controller {

    class EmulatedSwitchController : public SwitchController {

        public:
            EmulatedSwitchController(const bluetooth::Address *address, HardwareID id);
            virtual ~EmulatedSwitchController() {};

            virtual Result Initialize();
            bool IsOfficialController() { return false; }

            Result HandleOutputDataReport(const bluetooth::HidReport *report) override;

        protected:
            void ClearControllerState();
            virtual Result SetVibration(const SwitchRumbleData *rumble_data) { AMS_UNUSED(rumble_data); R_SUCCEED(); }
            virtual Result CancelVibration() { R_SUCCEED(); }
            virtual Result SetPlayerLed(u8 led_mask) { AMS_UNUSED(led_mask); R_SUCCEED(); }

            void UpdateControllerState(const bluetooth::HidReport *report) override;
            virtual void ProcessInputData(const bluetooth::HidReport *report) { AMS_UNUSED(report); }

            Result HandleRumbleData(const SwitchRumbleDataEncoded *encoded);
            Result HandleHidCommand(const SwitchHidCommand *command);
            Result HandleNfcIrData(const u8 *nfc_ir);

            Result HandleHidCommandGetDeviceInfo(const SwitchHidCommand *command);
            Result HandleHidCommandSetDataFormat(const SwitchHidCommand *command);
            Result HandleHidCommandLRButtonDetection(const SwitchHidCommand *command);
            Result HandleHidCommandClearPairingInfo(const SwitchHidCommand *command);
            Result HandleHidCommandShipment(const SwitchHidCommand *command);
            Result HandleHidCommandSerialFlashRead(const SwitchHidCommand *command);
            Result HandleHidCommandSerialFlashWrite(const SwitchHidCommand *command);
            Result HandleHidCommandSerialFlashSectorErase(const SwitchHidCommand *command);
            Result HandleHidCommandMcuWrite(const SwitchHidCommand *command);
            Result HandleHidCommandMcuResume(const SwitchHidCommand *command);
            Result HandleHidCommandMcuPollingEnable(const SwitchHidCommand *command);
            Result HandleHidCommandMcuPollingDisable(const SwitchHidCommand *command);
            Result HandleHidCommandSetIndicatorLed(const SwitchHidCommand *command);
            Result HandleHidCommandGetIndicatorLed(const SwitchHidCommand *command);
            Result HandleHidCommandSetNotificationLed(const SwitchHidCommand *command);
            Result HandleHidCommandSensorSleep(const SwitchHidCommand *command);
            Result HandleHidCommandSensorConfig(const SwitchHidCommand *command);
            Result HandleHidCommandMotorEnable(const SwitchHidCommand *command);

            Result FakeHidCommandResponse(const SwitchHidCommandResponse *response);
            Result FakeNfcIrResponse(const SwitchNfcIrResponse *response);

            bool m_charging;
            bool m_ext_power;
            u8 m_battery;
            u8 m_led_pattern;

            SwitchButtonData m_buttons;
            SwitchAnalogStick m_left_stick;
            SwitchAnalogStick m_right_stick;
            Switch6AxisData m_motion_data[3];

            u16 m_gyro_sensitivity;
            u16 m_acc_sensitivity;

            u8 m_input_report_mode;

            bool m_enable_rumble;
            bool m_enable_motion;

            float m_trigger_threshold;

            VirtualSpiFlash m_virtual_memory;
    };

}
