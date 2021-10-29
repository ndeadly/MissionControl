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
#include "switch_analog_stick.hpp"

namespace ams::controller {

    void SwitchAnalogStick::SetData(uint16_t x, uint16_t y) {
        m_xy[0] = x & 0xff;
        m_xy[1] = (x >> 8) | ((y & 0xff) << 4);
        m_xy[2] = (y >> 4) & 0xff;
    }

    void SwitchAnalogStick::SetX(uint16_t x) {
        m_xy[0] = x & 0xff;
        m_xy[1] = (m_xy[1] & 0xf0) | (x >> 8);
    }

    void SwitchAnalogStick::SetY(uint16_t y) {
        m_xy[1] = (m_xy[1] & 0x0f) | ((y & 0xff) << 4);
        m_xy[2] = (y >> 4) & 0xff;
    }

    uint16_t SwitchAnalogStick::GetX(void) {
        return m_xy[0] | ((m_xy[1] & 0xf) << 8);
    }

    uint16_t SwitchAnalogStick::GetY(void) {
        return (m_xy[1] >> 4) | (m_xy[2] << 4);
    }

    void SwitchAnalogStick::InvertX(void) {
        m_xy[0] ^= 0xff;
        m_xy[1] ^= 0x0f;
    }

    void SwitchAnalogStick::InvertY(void) {
        m_xy[1] ^= 0xf0;
        m_xy[2] ^= 0xff;
    }

    void SwitchAnalogStick::ForceDeadzone(uint16_t zone) {
        float zone_scale_factor = float(STICK_ZERO) / (STICK_ZERO - zone);
        
        uint16_t x = GetX();
        if (x < STICK_ZERO - zone)
            x = static_cast<uint16_t>(zone_scale_factor * x);
        else if (x > STICK_ZERO + zone)
            x = STICK_ZERO + static_cast<uint16_t>(zone_scale_factor * (x - STICK_ZERO - zone) + 1);
        else
            x = STICK_ZERO;
        
        uint16_t y = GetY();
        if (y < STICK_ZERO - zone)
            y = static_cast<uint16_t>(zone_scale_factor * y);
        else if (y > STICK_ZERO + zone)
            y = STICK_ZERO + static_cast<uint16_t>(zone_scale_factor * (y - STICK_ZERO - zone) + 1);
        else
            y = STICK_ZERO;
        
        SetData(x, y);
    }

}
