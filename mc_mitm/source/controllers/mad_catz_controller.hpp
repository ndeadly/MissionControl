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
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    // Additional notes for Mad Catz C.T.R.L. R:
    // (Retail C.T.R.L. Model 32266, and Micro C.T.R.L. R)
    // - Mathew 'Mad Cat' Carr - 2026/08/26 2026/11/15
    //
    // Should work with stock firmware as-new controllers, and controllers
    // with the official Mad Catz XInput firmware update 1.2.5 ('XInput update').
    // This is the retail boxed C.T.R.L. R controller,
    // NOT the pack-in M.O.J.O. version.
    // Sadly, the two are different. (M.O.J.O. controller is BLE.)
    // Volume down is mapped to Switch Capture button.
    // Volume up is mapped to Switch Home button.
    // Mad Catz Emblem/Guide button is -not- the Switch Home button.

    // The C.T.R.L. R pad has a physical mode which on the bottom.
    // The three modes are 'Smart', 'Mouse' and 'Desktop'.
    // These modes control whether the triggers are reported as
    // analogue values or digital button bits.
    // The C.T.R.L. R pad doesn't report this mode switch position over
    // Bluetooth. Even the official Mad Catz drivers on Windows require
    // the user to manually set the pad type in the system tray widget.
    // Certain Input Report fields only have valid values in
    // specific mode switch modes.
    // We will require the user to set the switch to 'Desktop' mode
    // where the LT RT analogue triggers read as digital on-off values,
    // which is what the Switch wants.
    // In this mode, partially pulled analogue triggers are interpreted
    // as digital buttons by the C.T.R.L. R pad.
    // (The C.T.R.L. R pad doesn't have GameCube-style
    // haptic click action on the triggers.)
    // Trigger analogue continuous values are only valid when the
    // mode switch is Smart. Smart mode causes other issues, like the
    // Back button becoming an instantaneous 'Android Back' system action.
    // It's best to just use Desktop mode for this project.

    // Several buttons' fields are instantaneous on-off tap actions,
    // which means the Switch won't react to them fast enough,
    // so we cannot use them in this project.

    struct MadCatzButtonData {
        u8 X      : 1;
        u8 A      : 1;
        u8 B      : 1;
        u8 Y      : 1;
        u8 LB     : 1;
        u8 RB     : 1;
        u8 LT     : 1; // Valid only in Desktop mode.
        u8 RT     : 1; // Valid only in Desktop mode.

        u8 back   : 1; // Valid only in Desktop mode.
        u8 start  : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8 emblem : 1; // Valid only in Desktop mode. Instantaneous.
        u8        : 0;
    } PACKED;

    // Input Report 0x01 contains buttons and sticks.
    // Original controller firmware reported analogue values as u16.
    // Controller firmware 1.2.5 reports analogue values as u8.
    enum MadCatzInputReport0x01Format {
        MadCatzInputReport0x01Format_FirmwareStock,
        MadCatzInputReport0x01Format_Firmware125,
    };

    template <typename T> requires std::integral<T>
    struct MadCatzInputReport0x01 {
        MadCatzButtonData buttons;
        DirectionalPadType<1, 0> dpad;
        AnalogStick<T> left_stick;
        AnalogStick<T> right_stick;
        AnalogTriggerType<T> left_trigger;
        AnalogTriggerType<T> right_trigger;
    } PACKED;

    // Input Report 0x02 contains media buttons and system events.
    // Tap and hold events for FF and rewind appear as unique
    // separate events, with the initial taps being instantaneous.
    struct MadCatzInputReport0x02 {
        u8 ff_tap      : 1; // Instantaneous.
        u8 rewind_tap  : 1; // Instantaneous.
        u8 volume_up   : 1;
        u8 volume_down : 1;
        u8 play_pause  : 1;
        u8 ff_hold     : 1;
        u8 rew_hold    : 1;
        u8 system_back : 1; // Valid only in Smart mode. Instantaneous.

        u8 menu_up     : 1; // Valid only in Mouse mode. Act as a media remote's d-pad.
        u8 menu_down   : 1; //  "
        u8 menu_left   : 1; //  "
        u8 menu_right  : 1; //  "
        u8 system_home : 1; // Valid only in Smart mode. Instantaneous.
        u8             : 0;
    } PACKED;

    struct MadCatzInputReport0x81 {
        struct {
            union {
                struct {
                    u8 A      : 1;
                    u8 B      : 1;
                    u8 X      : 1;
                    u8 Y      : 1;
                    u8 L1     : 1;
                    u8 R1     : 1;
                    u8 select : 1;
                    u8 start  : 1;

                    u8 L3     : 1;
                    u8 R3     : 1;
                    u8        : 0;
                };

                struct {
                    u8 A     : 1;
                    u8 B     : 1;
                    u8       : 1;
                    u8 X     : 1;
                    u8 Y     : 1;
                    u8       : 1;
                    u8 L1    : 1;
                    u8 R1    : 1;

                    u8       : 3;
                    u8 start : 1;
                    u8       : 1;
                    u8 L3    : 1;
                    u8 R3    : 1;
                    u8       : 0;
                } xinput;
            };
        } buttons;
        DirectionalPadType<1, 0> dpad;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        AnalogTriggerType<u8> left_trigger;
        AnalogTriggerType<u8> right_trigger;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x82 {
        struct {
            u8            : 2;
            u8 R1         : 1;
            u8 L1         : 1;
            u8 Y          : 1;
            u8 B          : 1;
            u8 X          : 1;
            u8 select     : 1;

            u8 dpad_up    : 1;
            u8 dpad_down  : 1;
            u8 dpad_left  : 1;
            u8 dpad_right : 1;
            u8            : 4;
        } buttons;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x83 {
        struct {
            u8 R2 : 1;
            u8 L2 : 1;
            u8 R3 : 1;
            u8 L3 : 1;
            u8    : 0;
        } buttons;
        AnalogStick<u8> left_stick;
        u8 reserved[2];
    } PACKED;

    struct MadCatzReportData {
        u8 id;
        union {
            MadCatzInputReport0x01<u16> input0x01stock;
            MadCatzInputReport0x01<u8>  input0x01updated;
            MadCatzInputReport0x02      input0x02;

            MadCatzInputReport0x81      input0x81;
            MadCatzInputReport0x82      input0x82;
            MadCatzInputReport0x83      input0x83;
        };
    } PACKED;

    class MadCatzController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0738, 0x5266},   // Mad Catz C.T.R.L. R
                {0x0738, 0x5262},   // Mad Catz Micro C.T.R.L. R

                {0x0738, 0x5250},   // Mad Catz C.T.R.L. R for Samsung
                {0x0738, 0x5269}    // Mad Catz L.Y.N.X. 3
            };

            MadCatzController(bluetooth::Address address, HardwareID id) : EmulatedSwitchController(address, id) { }

            virtual void ParseInputReport(const u8 *report_buffer, size_t size) override;

        private:
            void MapInputReport0x01(const MadCatzReportData *src, MadCatzInputReport0x01Format fmt);
            void MapInputReport0x02(const MadCatzReportData *src);

            void MapInputReport0x81(const MadCatzReportData *src);
            void MapInputReport0x82(const MadCatzReportData *src);
            void MapInputReport0x83(const MadCatzReportData *src);
    };

}
