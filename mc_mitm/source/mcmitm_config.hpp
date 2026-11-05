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
#include "bluetooth_mitm/bluetooth/bluetooth_types.hpp"

namespace ams::mitm {

    // Physical Dualshock 4 inputs that can be remapped to Switch buttons from the ini file
    enum Dualshock4Input : int {
        Dualshock4Input_Cross,
        Dualshock4Input_Circle,
        Dualshock4Input_Square,
        Dualshock4Input_Triangle,
        Dualshock4Input_L1,
        Dualshock4Input_R1,
        Dualshock4Input_L2,
        Dualshock4Input_R2,
        Dualshock4Input_L3,
        Dualshock4Input_R3,
        Dualshock4Input_Share,
        Dualshock4Input_Options,
        Dualshock4Input_Ps,
        Dualshock4Input_DpadUp,
        Dualshock4Input_DpadDown,
        Dualshock4Input_DpadLeft,
        Dualshock4Input_DpadRight,
        Dualshock4Input_TouchpadLeft,
        Dualshock4Input_TouchpadCentre,
        Dualshock4Input_TouchpadRight,
        Dualshock4Input_Count
    };

    struct MissionControlConfig {
        struct {
            bool enable_rumble;
            bool enable_motion;
        } general;

        struct {
            char host_name[0x20];
            bluetooth::Address host_address;
        } bluetooth;

        struct {
            int analog_trigger_activation_threshold;
            bool dualshock3_enable_usb_pairing;
            int dualshock3_led_mode;
            int dualshock4_polling_rate;
            int dualshock4_lightbar_brightness;
            int dualsense_lightbar_brightness;
            bool dualsense_enable_player_leds;
            int dualsense_vibration_intensity;
            int dualsense_adaptive_trigger_travel;
            // Custom Dualshock 4 lightbar colours as 0xRRGGBB, or -1 if unset.
            // Index 0 applies to all players, indices 1-8 override it for that player number.
            int dualshock4_lightbar_colour[9];
            // What each Dualshock 4 input is mapped to, as a bitmask of controller::SwitchButton values (0 = unbound).
            // Indexed by Dualshock4Input.
            u32 dualshock4_button_map[Dualshock4Input_Count];
        } misc;
    };

    void LoadConfiguration();

    // Called from the main loop several times a second. Re-reads the ini file when its contents have changed.
    void PollConfigurationReload();

    // Incremented every time a new configuration has been loaded, so that long-lived objects can notice and refresh
    u32 GetConfigurationGeneration();

    MissionControlConfig *GetGlobalConfig();
    SetLanguage GetSystemLanguage();

}
