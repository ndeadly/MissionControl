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
#include "switch_rumble_decoder.hpp"

namespace ams::controller {

    namespace {

        constexpr float MinAmplitude     = -8.0f;
        constexpr float MaxAmplitude     =  0.0f;
        constexpr float DefaultAmplitude =  MinAmplitude;

        constexpr float MinFrequency     = -2.0f;
        constexpr float MaxFrequency     =  2.0f;
        constexpr float DefaultFrequency =  0.0f;

        constexpr float CenterFreqHigh = 320.0f;
        constexpr float CenterFreqLow  = 160.0f;

        constexpr float ExpBase2LookupResolution = 1.0f / 32;
        constexpr float ExpBase2RangeStart = std::min(MinAmplitude, MinFrequency);
        constexpr float ExpBase2RangeEnd   = std::max(MaxAmplitude, MaxFrequency);
        constexpr size_t ExpBase2LookupLength = (std::fabs(ExpBase2RangeEnd - ExpBase2RangeStart) + ExpBase2LookupResolution) / ExpBase2LookupResolution;

        constexpr std::array<float, ExpBase2LookupLength> ExpBase2Lookup = []() {
            std::array<float, ExpBase2LookupLength> table = {};

            constexpr float AmplitudeThreshold = -7.9375f;

            for (size_t i = 0; i < table.size(); ++i) {
                float f = ExpBase2RangeStart + i * ExpBase2LookupResolution;
                if (f >= AmplitudeThreshold) {
                    table[i] = std::exp2f(f);
                }
            }

            return table;
        }();

        constexpr u32 GetLookupIndex(float input) {
            return (input - ExpBase2RangeStart) / ExpBase2LookupResolution;
        }

        constexpr std::array<float, 128> Am7BitLookup = []() {
            std::array<float, 128> table = {};

            for (size_t i = 0; i < table.size(); ++i) {
                if (i == 0) {
                    table[i] = -8.0f;
                } else if (i < 16) {
                    table[i] = 0.25f * i - 7.75f;
                } else if (i < 32) {
                    table[i] = 0.0625f * i - 4.9375f;
                } else {
                    table[i] = 0.03125f * i - 3.96875f;
                }
            }

            return table;
        }();

        constexpr std::array<float, 128> Fm7BitLookup = []() {
            std::array<float, 128> table = {};

            for (size_t i = 0; i < table.size(); ++i) {
                table[i] = 0.03125f * i - 2.0f;
            }

            return table;
        }();

        enum Switch5BitAction : u8 {
            Switch5BitAction_Ignore     = 0x0,
            Switch5BitAction_Default    = 0x1,
            Switch5BitAction_Substitute = 0x2,
            Switch5BitAction_Sum        = 0x3,
        };

        struct Switch5BitCommand {
            Switch5BitAction am_action;
            Switch5BitAction fm_action;
            float am_offset;
            float fm_offset;
        };

        constexpr Switch5BitCommand CommandTable[] = {
            { .am_action = Switch5BitAction_Default,    .fm_action = Switch5BitAction_Default,    .am_offset =  0.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset =  0.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -0.5f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -1.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -1.5f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -2.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -2.5f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -3.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -3.5f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -4.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -4.5f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Substitute, .fm_action = Switch5BitAction_Ignore,     .am_offset = -5.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Substitute, .am_offset =  0.0f,     .fm_offset = -0.375f   },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Substitute, .am_offset =  0.0f,     .fm_offset = -0.1875f  },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Substitute, .am_offset =  0.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Substitute, .am_offset =  0.0f,     .fm_offset =  0.1875f  },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Substitute, .am_offset =  0.0f,     .fm_offset =  0.375f   },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset =  0.125f,   .fm_offset =  0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Ignore,     .am_offset =  0.125f,   .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset =  0.125f,   .fm_offset = -0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset =  0.03125f, .fm_offset =  0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Ignore,     .am_offset =  0.03125f, .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset =  0.03125f, .fm_offset = -0.03125f },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Sum,        .am_offset =  0.0f,     .fm_offset =  0.03125f },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Ignore,     .am_offset =  0.0f,     .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Ignore,     .fm_action = Switch5BitAction_Sum,        .am_offset =  0.0f,     .fm_offset = -0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset = -0.03125f, .fm_offset =  0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Ignore,     .am_offset = -0.03125f, .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset = -0.03125f, .fm_offset = -0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset = -0.125f,   .fm_offset =  0.03125f },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Ignore,     .am_offset = -0.125f,   .fm_offset =  0.0f     },
            { .am_action = Switch5BitAction_Sum,        .fm_action = Switch5BitAction_Sum,        .am_offset = -0.125f,   .fm_offset = -0.03125f }
        };

        float ApplyCommand(Switch5BitAction action, float offset, float current_val, float default_val, float min, float max) {
            switch (action) {
                case Switch5BitAction_Ignore:     return current_val;
                case Switch5BitAction_Substitute: return offset;
                case Switch5BitAction_Sum:        return std::clamp(current_val + offset, min, max);
                default:                          return default_val;
            }
        }

        ALWAYS_INLINE float ApplyAmCommand(u8 amfm_code, float current_val) {
            return ApplyCommand(CommandTable[amfm_code].am_action, CommandTable[amfm_code].am_offset, current_val, DefaultAmplitude, MinAmplitude, MaxAmplitude);
        }

        ALWAYS_INLINE float ApplyFmCommand(u8 amfm_code, float current_val) {
            return ApplyCommand(CommandTable[amfm_code].fm_action, CommandTable[amfm_code].fm_offset, current_val, DefaultFrequency, MinFrequency, MaxFrequency);
        }

    }

    SwitchRumbleDecoder::SwitchRumbleDecoder() {
        m_state = {
            .lo_amp_linear  = DefaultAmplitude,
            .lo_freq_linear = DefaultFrequency,
            .hi_amp_linear  = DefaultAmplitude,
            .hi_freq_linear = DefaultFrequency
        };
    }

    void SwitchRumbleDecoder::DecodeSamples(const SwitchEncodedVibrationSamples* encoded, SwitchVibrationSamples* decoded) {
        switch (encoded->packet_type) {
            case 0:
                decoded->count = 0;
                break;

            case 1:
                if (encoded->one5bit.reserved == 0) {
                    this->DecodeOne5Bit(encoded, decoded);
                } else if (encoded->one7bit.reserved == 0) {
                    this->DecodeOne7Bit(encoded, decoded);
                } else {
                    this->DecodeThree7Bit(encoded, decoded);
                }
                break;

            case 2:
                if (encoded->two5bit.reserved == 0) {
                    this->DecodeTwo5Bit(encoded, decoded);
                } else {
                    this->DecodeTwo7Bit(encoded, decoded);
                }
                break;

            case 3:
                this->DecodeThree5Bit(encoded, decoded);
                break;

            AMS_UNREACHABLE_DEFAULT_CASE();
        };
    }

    void SwitchRumbleDecoder::DecodeOne5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        m_state.lo_amp_linear  = ApplyAmCommand(encoded->one5bit.amfm_5bit_lo, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->one5bit.amfm_5bit_lo, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->one5bit.amfm_5bit_hi, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->one5bit.amfm_5bit_hi, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[0]);

        decoded->count = 1;
    }

    void SwitchRumbleDecoder::DecodeOne7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        m_state.lo_amp_linear  = Am7BitLookup[encoded->one7bit.am_7bit_lo];
        m_state.lo_freq_linear = Fm7BitLookup[encoded->one7bit.fm_7bit_lo];
        m_state.hi_amp_linear  = Am7BitLookup[encoded->one7bit.am_7bit_hi];
        m_state.hi_freq_linear = Fm7BitLookup[encoded->one7bit.fm_7bit_hi];
        this->GetCurrentOutputValue(&decoded->samples[0]);

        decoded->count = 1;
    }

    void SwitchRumbleDecoder::DecodeTwo5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        m_state.lo_amp_linear  = ApplyAmCommand(encoded->two5bit.amfm_5bit_lo_0, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->two5bit.amfm_5bit_lo_0, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->two5bit.amfm_5bit_hi_0, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->two5bit.amfm_5bit_hi_0, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[0]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->two5bit.amfm_5bit_lo_1, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->two5bit.amfm_5bit_lo_1, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->two5bit.amfm_5bit_hi_1, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->two5bit.amfm_5bit_hi_1, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[1]);

        decoded->count = 2;
    }

    void SwitchRumbleDecoder::DecodeTwo7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        if (encoded->two7bit.high_select) {
            m_state.hi_amp_linear  = Am7BitLookup[encoded->two7bit.am_7bit_xx];
            m_state.hi_freq_linear = Fm7BitLookup[encoded->two7bit.fm_7bit_xx];
            m_state.lo_amp_linear  = ApplyAmCommand(encoded->two7bit.amfm_5bit_xx_0, m_state.lo_amp_linear);
            m_state.lo_freq_linear = ApplyFmCommand(encoded->two7bit.amfm_5bit_xx_0, m_state.lo_freq_linear);
        } else {
            m_state.lo_amp_linear  = Am7BitLookup[encoded->two7bit.am_7bit_xx];
            m_state.lo_freq_linear = Fm7BitLookup[encoded->two7bit.fm_7bit_xx];
            m_state.hi_amp_linear  = ApplyAmCommand(encoded->two7bit.amfm_5bit_xx_0, m_state.hi_amp_linear);
            m_state.hi_freq_linear = ApplyFmCommand(encoded->two7bit.amfm_5bit_xx_0, m_state.hi_freq_linear);
        }
        this->GetCurrentOutputValue(&decoded->samples[0]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->two7bit.amfm_5bit_lo_1, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->two7bit.amfm_5bit_lo_1, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->two7bit.amfm_5bit_hi_1, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->two7bit.amfm_5bit_hi_1, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[1]);

        decoded->count = 2;
    }

    void SwitchRumbleDecoder::DecodeThree5Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        m_state.lo_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_lo_0, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_lo_0, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_hi_0, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_hi_0, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[0]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_lo_1, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_lo_1, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_hi_1, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_hi_1, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[1]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_lo_2, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_lo_2, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->three5bit.amfm_5bit_hi_2, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->three5bit.amfm_5bit_hi_2, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[2]);

        decoded->count = 3;
    }

    void SwitchRumbleDecoder::DecodeThree7Bit(const SwitchEncodedVibrationSamples *encoded, SwitchVibrationSamples *decoded) {
        if (encoded->three7bit.high_select) {
            if (encoded->three7bit.freq_select) {
                m_state.hi_freq_linear = Fm7BitLookup[encoded->three7bit.xx_7bit_xx];
            } else {
                m_state.hi_amp_linear  = Am7BitLookup[encoded->three7bit.xx_7bit_xx];
            }
        } else {
            if (encoded->three7bit.freq_select) {
                m_state.lo_freq_linear = Fm7BitLookup[encoded->three7bit.xx_7bit_xx];
            } else {
                m_state.lo_amp_linear  = Am7BitLookup[encoded->three7bit.xx_7bit_xx];
            }
        }
        this->GetCurrentOutputValue(&decoded->samples[0]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->three7bit.amfm_5bit_lo_1, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->three7bit.amfm_5bit_lo_1, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->three7bit.amfm_5bit_hi_1, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->three7bit.amfm_5bit_hi_1, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[1]);

        m_state.lo_amp_linear  = ApplyAmCommand(encoded->three7bit.amfm_5bit_lo_2, m_state.lo_amp_linear);
        m_state.lo_freq_linear = ApplyFmCommand(encoded->three7bit.amfm_5bit_lo_2, m_state.lo_freq_linear);
        m_state.hi_amp_linear  = ApplyAmCommand(encoded->three7bit.amfm_5bit_hi_2, m_state.hi_amp_linear);
        m_state.hi_freq_linear = ApplyFmCommand(encoded->three7bit.amfm_5bit_hi_2, m_state.hi_freq_linear);
        this->GetCurrentOutputValue(&decoded->samples[2]);

        decoded->count = 3;
    }

    void SwitchRumbleDecoder::GetCurrentOutputValue(SwitchVibrationValues* output) {
        output->low_band_amp   = ExpBase2Lookup[GetLookupIndex(m_state.lo_amp_linear)];
        output->low_band_freq  = ExpBase2Lookup[GetLookupIndex(m_state.lo_freq_linear)] * CenterFreqLow;
        output->high_band_amp  = ExpBase2Lookup[GetLookupIndex(m_state.hi_amp_linear)];
        output->high_band_freq = ExpBase2Lookup[GetLookupIndex(m_state.hi_freq_linear)] * CenterFreqHigh;
    }

}
