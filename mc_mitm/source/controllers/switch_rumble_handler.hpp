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
#include <stratosphere.hpp>
#include "switch_rumble_decoder.hpp"

namespace ams::controller {

    struct SwitchEncodedMotorData {
        SwitchEncodedVibrationSamples left_motor;
        SwitchEncodedVibrationSamples right_motor;
    } PACKED;

    struct SwitchMotorData {
        SwitchVibrationValues left_motor;
        SwitchVibrationValues right_motor;
    };

    class SwitchRumbleHandler {
        public:
            bool GetDecodedValues(const SwitchEncodedMotorData *encoded, SwitchMotorData *decoded);

        private:
            bool GetNextDecodedValue(SwitchRumbleDecoder *decoder, const SwitchEncodedVibrationSamples *encoded_samples, SwitchVibrationValues *out_sample);

        private:
            SwitchRumbleDecoder m_decoder_left;
            SwitchRumbleDecoder m_decoder_right;
    };

}
