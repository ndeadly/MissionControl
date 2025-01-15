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
#include "switch_rumble_handler.hpp"

namespace ams::controller {

    bool SwitchRumbleHandler::GetDecodedValues(const SwitchEncodedMotorData *encoded, SwitchMotorData *decoded) {
        return this->GetNextDecodedValue(&m_decoder_left, &encoded->left_motor, &decoded->left_motor) | this->GetNextDecodedValue(&m_decoder_right, &encoded->right_motor, &decoded->right_motor);
    }

    bool SwitchRumbleHandler::GetNextDecodedValue(SwitchRumbleDecoder *decoder, const SwitchEncodedVibrationSamples *encoded_samples, SwitchVibrationValues *out_sample) {
        SwitchVibrationSamples decoded_samples;
        decoder->DecodeSamples(encoded_samples, &decoded_samples);
        if (decoded_samples.count > 0) {
            // We will just take the first decoded sample and ignore the others
            *out_sample = decoded_samples.samples[0];
        } else {
            // Repeat the current sample if no new samples were decoded
            decoder->GetCurrentOutputValue(out_sample);
        }

        return decoded_samples.count > 0;
    }

}
