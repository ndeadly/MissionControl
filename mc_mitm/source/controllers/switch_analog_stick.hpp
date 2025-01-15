/*
 * Copyright (c) 2020-2025 ndeadly
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
#include <concepts>

namespace ams::controller {

    constexpr u16 UINT12_MAX = 0xFFF;

    struct SwitchAnalogStick {
        static constexpr u16 Min = 0;
        static constexpr u16 Max = 0xFFF;
        static constexpr u16 Center = 0x800;

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

    template<typename T> requires std::integral<T>
    constexpr T InvertAnalogStickValue(T t) {
        return ~t;
    }

    template<typename T> requires std::integral<T>
    constexpr u16 ConvertAnalogStick12Bit(T t) {
        constexpr std::make_unsigned_t<T> UnsignedTMax = ~0;
        constexpr double ScaleFactor = double(UINT12_MAX) / UnsignedTMax ;

        if constexpr (std::is_unsigned_v<T>) {
            return static_cast<u16>(ScaleFactor * t);
        } else {
            constexpr std::make_unsigned_t<T> Shift = (UnsignedTMax >> 1) + 1;
            return static_cast<u16>((ScaleFactor * (t + Shift)) );
        }
    }

    template<typename T> requires std::integral<T>
    constexpr SwitchAnalogStick PackAnalogStickValues(T x, T y) {
        const u16 _x = ConvertAnalogStick12Bit(x);
        const u16 _y = ConvertAnalogStick12Bit(y);

        SwitchAnalogStick stick;
        stick.m_xy[0] = _x & UINT8_MAX;
        stick.m_xy[1] = (_x >> 8) | ((_y & UINT8_MAX) << 4);
        stick.m_xy[2] = (_y >> 4) & UINT8_MAX;

        return stick;
    }

    template <typename T> requires std::integral<T>
    struct AnalogStick {
        T x;
        T y;
    };

}
