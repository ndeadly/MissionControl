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

/*
 * Controller driver file for Mad Catz C.T.R.L. R model 32266
 *   by Mathew 'Mad Cat' Carr - 2026/08/26
 *
 * Should work with stock firmware as-new controllers, and controllers
 * with the official Mad Catz XInput firmware update 1.2.5 ('XInput update').
 * This is the retail boxed C.T.R.L. R controller,
 * NOT the pack-in M.O.J.O version.
 * Sadly, the two are different. (M.O.J.O. controller is BLE.)
 * Volume down is mapped to Switch Capture button.
 * Volume up is mapped to Switch Home button.
 * Mad Catz Emblem/Guide button is -not- the Switch Home button.
 */

#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    // The C.T.R.L. R pad has a physical mode which on the bottom.
    // The three modes are 'Smart', 'Mouse' and 'Desktop'.
    // These modes control whether the triggers are reported as
    // analogue values or digital button bits.
    // The C.T.R.L. R pad doesn't report the switch position
    // over Bluetooth. Even the official Mad Catz drivers on Windows require
    // the user to manually set the pad type in the system tray widget.
    // Certain Input Report fields only have valid values in
    // specific mode switch modes.
    // We will require the user to set the switch to the right,
    // which is the 'Desktop' mode where the LT RT analogue triggers
    // read as digital on-off values, which is what the Switch wants.
    // In this mode, partially pulled analogue triggers are interpreted
    // as digital buttons by the C.T.R.L. R pad; the C.T.R.L. R pad
    // doesn't have GameCube-style hard haptic click action
    // on the triggers.
    // Trigger analogue continuous values are only valid when the
    // mode switch is Smart. Smart mode causes other issues, like the
    // Back button becoming an instantaneous 'Android Back' system action.
    // It's best to just use Desktop mode for this project.

    // Several buttons fields are instantaneous on-off tap actions,
    // which means the Switch won't react to them fast enough,
    // so we cannot use them in this project.

    struct MadCatz32266ButtonData {
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

        u8 dpad;
    } PACKED;

    // Input Report 0x01 contains buttons and sticks.
    // Original firmware reported analogue values as u16.
    // Firmware 1.2.5 reports analogue values as u8.
    template <typename T> requires std::integral<T>
    struct MadCatz32266InputReport0x01 {
        MadCatz32266ButtonData buttons;
        AnalogStick<T> left_stick;
        AnalogStick<T> right_stick;
        T left_trigger;
        T right_trigger;
    } PACKED;

    // Input Report 0x02 contains media buttons and system events.
    // Tap and hold events for FF and rewind appear as unique
    // separate events, with the initial taps being instantaneous.
    struct MadCatz32266InputReport0x02 {
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

    struct MadCatz32266ReportData {
        u8 id;
        union {
            MadCatz32266InputReport0x01<u16> input0x01stock;
            MadCatz32266InputReport0x01<u8>  input0x01updated;
            MadCatz32266InputReport0x02      input0x02;
        };
    } PACKED;

    class MadCatz32266Controller final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0738, 0x5266},   // Mad Catz C.T.R.L.R
                {0x0738, 0x5262},   // Mad Catz Micro C.T.R.L.R
            };

            MadCatz32266Controller(bluetooth::Address address, HardwareID id) : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const MadCatz32266ReportData *src, u16 report_size);
            void MapInputReport0x02(const MadCatz32266ReportData *src);
    };

}
