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
#include "bluetooth_mitm/bluetooth/bluetooth_types.hpp"
#include "controllers/switch_controller.hpp"

namespace ams::mitm {

    struct MissionControlConfig {
        struct {
            char host_name[0x20];
            bluetooth::Address host_address;
        } bluetooth;

        struct{
            bool disable_custom_profiles;
        } general;
    };

    struct ControllerProfileConfig{
         struct {
            bool enable_rumble;
            bool enable_motion;
        } general;

        struct{
            controller::RGBColour body;
            controller::RGBColour buttons;
            controller::RGBColour left_grip;
            controller::RGBColour right_grip;
        } colours;

        struct {
            bool use_western_layout;
            int32_t sony_led_brightness;
            int32_t dualshock_pollingrate_divisor;
            bool swap_dpad_lstick;
            bool invert_lstick_xaxis;
            bool invert_lstick_yaxis;
            bool invert_rstick_xaxis;
            bool invert_rstick_yaxis;
            float lstick_deadzone;
            float rstick_deadzone;
            bool disable_home_button;
        } misc;
    };

    MissionControlConfig *GetGlobalConfig(void);
    Result GetCustomIniConfig(const bluetooth::Address *address, ControllerProfileConfig *config);
    void ParseIniConfig(void);

}
