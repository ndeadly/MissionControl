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
#include <switch.h>

namespace ams::controller {

    constexpr u16 UINT12_MAX = 0xfff;
    constexpr u16 STICK_MIN = 0;
    constexpr u16 STICK_CENTER = 0x800;
    constexpr u16 STICK_MAX = UINT12_MAX;

    struct SwitchAnalogStick {
        void SetData(u16 x, u16 y);
        void SetX(u16 x);
        void SetY(u16 y);
        u16 GetX();
        u16 GetY();
        void InvertX();
        void InvertY();

        u8 m_xy[3];
    };

    struct SwitchAnalogStickFactoryCalibration {
        u8 calib[9];
    };

    struct SwitchAnalogStickParameters {
        u8 stickvalues[18];
    };

}
