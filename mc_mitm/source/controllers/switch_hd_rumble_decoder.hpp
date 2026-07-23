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
#include "switch_hd_rumble_types.hpp"

namespace ams::controller {

    class SwitchHdRumbleDecoder {
        public:
            SwitchHdRumbleDecoder();

            void DecodeSamples(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void GetCurrentOutputValue(SwitchVibrationValues *output);

        private:
            void DecodeOne5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void DecodeOne7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void DecodeTwo5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void DecodeTwo7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void DecodeThree5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            void DecodeThree7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded);
            
        private:
            struct {
                float lo_amp_linear;
                float lo_freq_linear;
                float hi_amp_linear;
                float hi_freq_linear;
            } m_state;
    };

}
