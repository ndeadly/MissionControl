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
#include <switch.h>

namespace ams::controller {

    constexpr auto UINT12_MAX  = 0xfff;
    constexpr auto STICK_ZERO  = 0x800;

    struct SwitchAnalogStick {
        void SetData(uint16_t x, uint16_t y);
        void SetX(uint16_t x);
        void SetY(uint16_t y);
        uint16_t GetX(void);
        uint16_t GetY(void);
        void InvertX(void);
        void InvertY(void);
    
        uint8_t m_xy[3];
    };

}
