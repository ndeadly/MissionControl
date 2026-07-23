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
#include <stratosphere.hpp>
#include "switch_hd_rumble_decoder.hpp"

namespace ams::controller {

    struct EmulatedSwitchController;

    class SwitchVibrationProcessor {
        private:
            EmulatedSwitchController *m_controller;
            SwitchHdRumbleDecoder m_decoder_left;
            SwitchHdRumbleDecoder m_decoder_right;
            bool m_enabled;

        public:
            constexpr SwitchVibrationProcessor(EmulatedSwitchController *controller) : m_controller(controller), m_enabled(true) { }

        public:
            constexpr void SetEnabled(bool enabled) { m_enabled = enabled; }
            constexpr bool GetEnabled() const { return m_enabled; }

            void ProcessMotorData(const SwitchEncodedMotorData *encoded_motor_data);
    };

}
