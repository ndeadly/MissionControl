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
#include "switch_controller.hpp"
#include "switch_rumble_handler.hpp"
#include "switch_sixaxis_processor.hpp"
#include "switch_hid_command_processor.hpp"
#include "switch_mcu_command_processor.hpp"
#include "switch_ext_grip_command_processor.hpp"
#include "switch_virtual_controller_memory.hpp"

namespace ams::controller {

    constexpr u8 FullKeyBtFirmwareVersionMajor  = 0x04;
    constexpr u8 FullKeyBtFirmwareVersionMinor  = 0x21;
    constexpr u8 FullKeyMcuFirmwareVersionMajor = 0x03;
    constexpr u8 FullKeyMcuFirmwareVersionMinor = 0x05;

    class EmulatedSwitchController : public SwitchController {
        friend class SwitchHidCommandProcessor;
        friend class SwitchMcuCommandProcessor;
        friend class SwitchExtGripCommandProcessor;

        public:
            EmulatedSwitchController(bluetooth::Address address, HardwareID id);
            virtual ~EmulatedSwitchController() {};

            virtual Result Initialize();
            virtual bool IsOfficialController() override { return false; }

            Result HandleOutputDataReport(const bluetooth::HidReport *report) override;

        protected:
            void Reset();
            virtual Result SetVibration(const SwitchMotorData *motor_data) { AMS_UNUSED(motor_data); R_SUCCEED(); }
            virtual Result CancelVibration() { R_SUCCEED(); }
            virtual Result SetPlayerLed(u8 led_mask) { AMS_UNUSED(led_mask); R_SUCCEED(); }

            Result BufferDataReport(const bluetooth::HidReport *report);

            void UpdateControllerState(const bluetooth::HidReport *report) override;
            virtual void ProcessInputData(const bluetooth::HidReport *report) { AMS_UNUSED(report); }

            size_t FillCommandInputReport(u8 *report_buffer);
            size_t FillMcuUpdateInputReport(u8 *report_buffer);
            size_t FillBasicInputReport(u8 *report_buffer);
            size_t FillMcuInputReport(u8 *report_buffer);
            size_t FillAttachmentInputReport(u8 *report_buffer);
            size_t FillGenericInputReport(u8 *report_buffer);
            size_t FillExtGripInputReport(u8 *report_buffer);

            Result HandleCommandOutputReport(const u8 *report_buffer);
            Result HandleMcuUpdateOutputReport(const u8 *report_buffer);
            Result HandleBasicOutputReport(const u8 *report_buffer);
            Result HandleMcuOutputReport(const u8 *report_buffer);
            Result HandleAttachmentOutputReport(const u8 *report_buffer);
            Result HandleExtGripOutputReport(const u8 *report_buffer);

            Result HandleMotorData(const SwitchEncodedMotorData *enc_motor_data);

        protected:
            u8 m_latency_timer;
            SwitchPowerInfo m_power_info;
            SwitchPlayerIndicator m_player_indicator;

            SwitchMotorStatus m_motor_status;

            SwitchHidReportId m_input_report_mode;

            SwitchButtons m_buttons;
            SwitchAnalogStick m_left_stick;
            SwitchAnalogStick m_right_stick;

            float m_trigger_threshold;

            bool m_enable_motion;
            SwitchSixAxisProcessor m_sixaxis_processor;

            bool m_enable_rumble;
            SwitchRumbleHandler m_rumble_handler;

            SwitchHidCommandProcessor m_hid_command_processor;
            SwitchMcuCommandProcessor m_mcu_command_processor;
            SwitchExtGripCommandProcessor m_ext_grip_command_processor;

            SwitchVirtualControllerMemory m_virtual_memory;
    };

}
