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

struct ControllerProfileConfig {
        struct {
            uint8_t controller_type;
            bool enable_rumble;
            bool enable_motion;
        } general;

        struct {
            bool use_western_layout;
            uint32_t sony_led_brightness;
            uint32_t dualshock_reportrate;
            bool swap_dpad_lstick;
            bool invert_lstick_xaxis;
            bool invert_lstick_yaxis;
            bool invert_rstick_xaxis;
            bool invert_rstick_yaxis;
            bool disable_home_button;
        } misc;
};

constexpr ControllerProfileConfig g_cp_global_config = {
    .general = {
        .controller_type = 3,
        .enable_rumble = true,
        .enable_motion = true
    },
    .misc = {
        .use_western_layout = false,
        .sony_led_brightness = 8,
        .dualshock_reportrate = 8,
        .swap_dpad_lstick = false,
        .invert_lstick_xaxis = false,
        .invert_lstick_yaxis = false,
        .invert_rstick_xaxis = false,
        .invert_rstick_yaxis = false,
        .disable_home_button = false
    }
};
