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
#include "switch_vibration_processor.hpp"
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    namespace {

        bool GetNextDecodedValue(SwitchHdRumbleDecoder *decoder, const SwitchEncodedVibrationSamples *encoded_samples, SwitchVibrationValues *out_sample) {
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

    void SwitchVibrationProcessor::ProcessMotorData(const SwitchEncodedMotorData *encoded_motor_data) {
        SwitchMotorData motor_data;

        bool samples_available = GetNextDecodedValue(&m_decoder_left,  &encoded_motor_data->left_motor,  &motor_data.left_motor)
                               | GetNextDecodedValue(&m_decoder_right, &encoded_motor_data->right_motor, &motor_data.right_motor);

        if (samples_available && m_enabled) {
            R_DISCARD(m_controller->SetVibration(&motor_data));
        }
    }

}
