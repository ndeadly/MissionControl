/*
 * Copyright (c) 2020-2025 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    struct Dualshock3ButtonData {
        u8 select     : 1;
        u8 L3         : 1;
        u8 R3         : 1;
        u8 start      : 1;
        u8 dpad_up    : 1;
        u8 dpad_right : 1;
        u8 dpad_down  : 1;
        u8 dpad_left  : 1;

        u8 L2         : 1;
        u8 R2         : 1;
        u8 L1         : 1;
        u8 R1         : 1;
        u8 triangle   : 1;
        u8 circle     : 1;
        u8 cross      : 1;
        u8 square     : 1;

        u8 ps         : 1;
        u8            : 0;
    } PACKED;

    struct Dualshock3RumbleData {
        u8 amp_motor_left;
        u8 amp_motor_right;
    } PACKED;

    struct Dualshock3InputReport0x01 {
        u8 unk0;
        Dualshock3ButtonData buttons;
        u8 unk1;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 unk2[4];
        u8 pressure_dpad_up;
        u8 pressure_dpad_right;
        u8 pressure_dpad_down;
        u8 pressure_dpad_left;
        u8 left_trigger;
        u8 right_trigger;
        u8 unk3[2];
        u8 pressure_triangle;
        u8 pressure_circle;
        u8 pressure_cross;
        u8 pressure_square;
        u8 unk4[3];
        u8 charge;
        u8 battery;
        u8 connection;
        u8 unk5[9];
        u16 accel_x;
        u16 accel_y;
        u16 accel_z;
        u16 gyro_x;
    } PACKED;

    struct Dualshock3OutputReport0x01 {
        struct {
            u8 data[35];
        };
    } PACKED;

    struct Dualshock3ReportData {
        u8 id;
        union {
            Dualshock3InputReport0x01 input0x01;
            Dualshock3OutputReport0x01 output0x01;
        };
    } PACKED;

    class Dualshock3Controller final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x054c, 0x0268},   // Official Dualshock3
            };

            static const UsbHsInterfaceFilter *GetUsbInterfaceFilter();
            static bool UsbIdentify(UsbHsInterface *iface);
            static Result UsbPair(UsbHsInterface *iface);

        public:
            Dualshock3Controller(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            Result Initialize(void);
            Result SetVibration(const SwitchMotorData *motor_data);
            Result CancelVibration();
            Result SetPlayerLed(u8 led_mask);

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const Dualshock3ReportData *src);

            Result SendEnablePayload(void);
            Result PushRumbleLedState();

            u8 m_led_mask;
            Dualshock3RumbleData m_rumble_state;
    };

}
