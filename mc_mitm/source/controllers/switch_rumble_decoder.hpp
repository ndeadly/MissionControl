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

namespace ams::controller {

    struct SwitchVibrationValues {
        float low_band_amp;
        float low_band_freq;
        float high_band_amp;
        float high_band_freq;
    };

    struct SwitchVibrationSamples {
        u8 count;
        SwitchVibrationValues samples[3];
    };

    struct SwitchEncodedVibrationSamples {
        union {
            struct {
                u32 data           : 30;
                u32 packet_type    : 2;
            };

            struct {
                u32 reserved       : 20; // Zero padding
                u32 amfm_5bit_hi   : 5;  // 5-bit amfm hi [0]
                u32 amfm_5bit_lo   : 5;  // 5-bit amfm lo [0]
                u32 packet_type    : 2;  // 1
            } one5bit;

            struct {
                u32 reserved       : 2;  // Zero padding
                u32 fm_7bit_hi     : 7;  // 7-bit fm hi [0]
                u32 am_7bit_hi     : 7;  // 7-bit am hi [0]
                u32 fm_7bit_lo     : 7;  // 7-bit fm lo [0]
                u32 am_7bit_lo     : 7;  // 7-bit am lo [0]
                u32 packet_type    : 2;  // 1
            } one7bit;

            struct {
                u32 reserved       : 10; // Zero padding
                u32 amfm_5bit_hi_1 : 5;  // 5-bit amfm hi [1]
                u32 amfm_5bit_lo_1 : 5;  // 5-bit amfm lo [1]
                u32 amfm_5bit_hi_0 : 5;  // 5-bit amfm hi [0]
                u32 amfm_5bit_lo_0 : 5;  // 5-bit amfm lo [0]
                u32 packet_type    : 2;  // 2
            } two5bit;

            struct {
                u32 high_select    : 1;  // Whether 7-bit values are high or low
                u32 fm_7bit_xx     : 7;  // 7-bit fm hi/lo [0], hi or lo denoted by high_select bit
                u32 amfm_5bit_hi_1 : 5;  // 5-bit amfm hi [1]
                u32 amfm_5bit_lo_1 : 5;  // 5-bit amfm lo [1]
                u32 amfm_5bit_xx_0 : 5;  // 5-bit amfm lo/hi [0], denoted by ~high_select
                u32 am_7bit_xx     : 7;  // 7-bit am hi/lo [0], hi or lo denoted by high_select bit
                u32 packet_type    : 2;  // 2
            } two7bit;

            struct {
                u32 amfm_5bit_hi_2 : 5;  // 5-bit amfm hi [2]
                u32 amfm_5bit_lo_2 : 5;  // 5-bit amfm lo [2]
                u32 amfm_5bit_hi_1 : 5;  // 5-bit amfm hi [1]
                u32 amfm_5bit_lo_1 : 5;  // 5-bit amfm lo [1]
                u32 amfm_5bit_hi_0 : 5;  // 5-bit amfm hi [0]
                u32 amfm_5bit_lo_0 : 5;  // 5-bit amfm lo [0]
                u32 packet_type    : 2;  // 3
            } three5bit;

            struct {
                u32 high_select    : 1;  // Whether 7-bit value is high or low
                u32                : 1;  // Always 1
                u32 freq_select    : 1;  // Whether 7-bit value is freq or amp
                u32 amfm_5bit_hi_2 : 5;  // 5-bit amfm hi [2]
                u32 amfm_5bit_lo_2 : 5;  // 5-bit amfm lo [2]
                u32 amfm_5bit_hi_1 : 5;  // 5-bit amfm hi [1]
                u32 amfm_5bit_lo_1 : 5;  // 5-bit amfm lo [1]
                u32 xx_7bit_xx     : 7;  // 7-bit am/fm lo/hi [0], denoted by freq_select and high_select bits
                u32 packet_type    : 2;  // 1
            } three7bit;
        };
    } PACKED;

    class SwitchRumbleDecoder {
        public:
            SwitchRumbleDecoder();

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
