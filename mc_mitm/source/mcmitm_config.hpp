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

namespace ams::mitm {

    struct MissionControlConfig {
        struct {
            bool enable_rumble;
            bool enable_motion;
            uint16_t left_stick_deadzone;
            uint16_t left_stick_outer_deadzone;
            uint16_t right_stick_deadzone;
            uint16_t right_stick_outer_deadzone;
        } general;

        struct {
            char host_name[0x20];
            bluetooth::Address host_address;
        } bluetooth;

        struct {
            bool disable_sony_leds;
        } misc;
    };

    MissionControlConfig *GetGlobalConfig(void);
    void ParseIniConfig(void);

}
