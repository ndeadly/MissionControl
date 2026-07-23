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
#include "switch_power_info.hpp"
#include "switch_buttons.hpp"
#include "switch_analog_stick.hpp"
#include "switch_motor_status.hpp"

#include "switch_hid_commands.hpp"
#include "switch_ext_grip_commands.hpp"

#include "switch_sixaxis_types.hpp"
#include "switch_hd_rumble_types.hpp"
#include "switch_mcu_types.hpp"
#include "switch_attachment_types.hpp"

namespace ams::controller {

    enum class SwitchHidReportId : u8 {
        CommandOutputReport    = 0x01,
        McuUpdateOutputReport  = 0x03,
        BasicOutputReport      = 0x10,
        McuOutputReport        = 0x11,
        AttachmentOutputReport = 0x12,
        CommandInputReport     = 0x21,
        McuUpdateInputReport   = 0x23,
        BasicInputReport       = 0x30,
        McuInputReport         = 0x31,
        AttachmentInputReport  = 0x32,
        Report0x33             = 0x33,
        GenericInputReport     = 0x3F,
        OtaEnableFwuReport     = 0x70,
        OtaSetupReadReport     = 0x71,
        OtaReadReport          = 0x72,
        OtaWriteReport         = 0x73,
        OtaEraseReport         = 0x74,
        OtaLaunchReport        = 0x75,
        ExtGripOutputReport    = 0x80,
        ExtGripInputReport     = 0x81,
        Report0x82             = 0x82,
    };

    struct SwitchHidCommandOutputReport {
        SwitchHidReportId report_id;
        u8 packet_number;
        SwitchEncodedMotorData motor_data;
        SwitchHidCommand command;
    };
    static_assert(sizeof(SwitchHidCommandOutputReport) == 0x31);

    struct SwitchHidMcuUpdateOutputReport {
        SwitchHidReportId report_id;
        u8 packet_number;
        SwitchEncodedMotorData motor_data;
        SwitchMcuUpdateDataOut mcu_update_data;
    };
    static_assert(sizeof(SwitchHidMcuUpdateOutputReport) == 0x13E);

    struct SwitchHidBasicOutputReport {
        SwitchHidReportId report_id;
        u8 packet_number;
        SwitchEncodedMotorData motor_data;
    };
    static_assert(sizeof(SwitchHidBasicOutputReport) == 0xA);

    struct SwitchHidMcuOutputReport {
        SwitchHidReportId report_id;
        u8 packet_number;
        SwitchEncodedMotorData motor_data;
        SwitchMcuDataOut mcu_data;
    };
    static_assert(sizeof(SwitchHidMcuOutputReport) == 0x30);

    struct SwitchHidAttachmentOutputReport {
        SwitchHidReportId report_id;
        u8 packet_number;
        SwitchEncodedMotorData motor_data;
        SwitchAttachmentDataOut attachment_data;
    };
    static_assert(sizeof(SwitchHidAttachmentOutputReport) == 0x30);

    struct SwitchExtGripOutputReport {
        SwitchHidReportId report_id;
        SwitchExtGripCommand command;
    };
    static_assert(sizeof(SwitchExtGripOutputReport) == 0x40);

    struct SwitchHidCommandInputReport {
        SwitchHidReportId report_id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonsType buttons;
        SwitchAnalogStickType left_analog_stick;
        SwitchAnalogStickType right_analog_stick;
        SwitchMotorStatusType motor_status;
        SwitchHidCommandResponse command_response;
    };
    static_assert(sizeof(SwitchHidCommandInputReport) == 0x31);

    struct SwitchMcuUpdateInputReport {
        SwitchHidReportId report_id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonsType buttons;
        SwitchAnalogStickType left_analog_stick;
        SwitchAnalogStickType right_analog_stick;
        SwitchMotorStatusType motor_status;
        SwitchMcuUpdateDataIn mcu_updata_data;
    };
    static_assert(sizeof(SwitchMcuUpdateInputReport) == 0x30);

    struct SwitchHidBasicInputReport {
        SwitchHidReportId report_id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonsType buttons;
        SwitchAnalogStickType left_analog_stick;
        SwitchAnalogStickType right_analog_stick;
        SwitchMotorStatusType motor_status;
        // SwitchSixAxisDataType six_axis_data;
        SwitchMotionData motion_data;
    };
    static_assert(sizeof(SwitchHidBasicInputReport) == 0x31);

    struct SwitchHidMcuInputReport {
        SwitchHidReportId report_id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonsType buttons;
        SwitchAnalogStickType left_analog_stick;
        SwitchAnalogStickType right_analog_stick;
        SwitchMotorStatusType motor_status;
        // SwitchSixAxisDataType six_axis_data;
        SwitchMotionData motion_data;
        SwitchMcuDataIn mcu_data;
    };
    static_assert(sizeof(SwitchHidMcuInputReport) == 0x16A);

    struct SwitchHidAttachmentInputReport {
        SwitchHidReportId report_id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonsType buttons;
        SwitchAnalogStickType left_analog_stick;
        SwitchAnalogStickType right_analog_stick;
        SwitchMotorStatusType motor_status;
        // SwitchSixAxisDataType six_axis_data;
        SwitchMotionData motion_data;
        SwitchAttachmentDataIn attachment_data;
    };
    static_assert(sizeof(SwitchHidAttachmentInputReport) == 0x16A);

    struct SwitchHidGenericInputReport {
        SwitchHidReportId report_id;
    //     ButtonsType<2> button_status;
    //     DirectionalPadType<0, 8> stick_hat_data;
        // u16 button_status;
        u8 button_status[2];
        u8 stick_hat_data;
        AnalogStickType<u16> left_analog_stick;
        AnalogStickType<u16> right_analog_stick;
    };
    static_assert(sizeof(SwitchHidGenericInputReport) == 0xC);

    struct SwitchExtGripInputReport {
        SwitchHidReportId report_id;
        SwitchExtGripCommandResponse command_response;
    };
    static_assert(sizeof(SwitchExtGripInputReport) == 0x40);

}
