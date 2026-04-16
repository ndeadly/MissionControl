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
#include "../utils.hpp"

namespace ams::controller {

    template <std::integral T>
    struct AnalogStick {
        T x;
        T y;

        constexpr T GetX() const { return x; }
        constexpr T GetY() const { return y; }
        constexpr T GetXInverted() const { return ~x; }
        constexpr T GetYInverted() const { return ~y; }
    };

    using SwitchAnalogStickType = utils::BitPackedPair;

    class SwitchAnalogStick {
        public:
            static constexpr u16 MinimumValue = 0;
            static constexpr u16 MaximumValue = 0xFFF;
            static constexpr u16 CenterValue  = std::midpoint(MinimumValue, MaximumValue);

            struct CalibrationValues {
                utils::BitPackedPair maximum;
                utils::BitPackedPair origin;
                utils::BitPackedPair minimum;
            };

            struct ModelValues {
                utils::BitPackedPair typical;
                utils::BitPackedPair deadzone;
                utils::BitPackedPair minimum_stroke_max;
                utils::BitPackedPair minimum_stroke_min;
                utils::BitPackedPair center_range_max;
                utils::BitPackedPair center_range_min;
            };

        private:
            template <std::integral T>
            static constexpr std::make_unsigned_t<T> ConvertToUnsigned(T value) {
                using U = std::make_unsigned_t<T>;

                if constexpr (std::is_unsigned_v<T>)
                    return static_cast<U>(value);
                else
                    return static_cast<U>(value - std::numeric_limits<T>::min());
            }

            template<std::integral T>
            static constexpr u16 ConvertToU12(T value) {
                using U = std::make_unsigned_t<T>;

                const U uvalue = ConvertToUnsigned(value);

                constexpr std::uintmax_t InMax  = std::numeric_limits<U>::max();
                constexpr std::uintmax_t OutMax = MaximumValue;

                return static_cast<u16>((static_cast<std::uintmax_t>(uvalue)*OutMax + InMax/2) / InMax);
            }

        private:
            SwitchAnalogStickType m_analog_stick;

        public:
            constexpr SwitchAnalogStick() : SwitchAnalogStick(CenterValue, CenterValue) { }

            constexpr SwitchAnalogStick(SwitchAnalogStickType analog_stick) : m_analog_stick(analog_stick) { }

            constexpr SwitchAnalogStick(u16 x, u16 y) {
                this->SetValues(x, y);
            }

        public:
            constexpr void SetState(SwitchAnalogStickType analog_stick) {
                m_analog_stick = analog_stick;
            }

            constexpr SwitchAnalogStickType GetState() const {
                return m_analog_stick;
            }

        public:
            constexpr void Reset() {
                m_analog_stick.SetValues(CenterValue, CenterValue);
            }

            template<std::integral T>
            constexpr void SetValuesFrom(T x, T y) {
                auto v1 = ConvertToU12(x);
                auto v2 = ConvertToU12(y);
                m_analog_stick.SetValues(v1, v2);
            }

            template<std::integral T>
            constexpr void SetXFrom(T x) {
                auto v = ConvertToU12(x);
                m_analog_stick.SetFirstValue(v);
            }

            template<std::integral T>
            constexpr void SetYFrom(T y) {
                auto v = ConvertToU12(y);
                m_analog_stick.SetSecondValue(v);
            }

            constexpr void SetValues(u16 x, u16 y) {
                m_analog_stick.SetValues(x, y);
            }

            constexpr void SetX(u16 x) {
                m_analog_stick.SetFirstValue(x);
            }

            constexpr void SetY(u16 y) {
                m_analog_stick.SetSecondValue(y);
            }

            constexpr std::pair<u16, u16> GetValues() const {
                return m_analog_stick.GetValues();
            }

            constexpr u16 GetX() const {
                return m_analog_stick.GetFirstValue();
            }

            constexpr u16 GetY() const {
                return m_analog_stick.GetSecondValue();
            }

            constexpr std::pair<u16, u16> GetValuesInverted() const {
                return { GetXInverted(), GetYInverted() };
            }

            constexpr u16 GetXInverted() const {
                return m_analog_stick.GetFirstValue() ^ MaximumValue;
            }

            constexpr u16 GetYInverted() const {
                return m_analog_stick.GetSecondValue() ^ MaximumValue;
            }

            constexpr std::pair<float, float> GetValuesNormalized() const {
                return { GetXNormalized(), GetYNormalized() };
            }

            constexpr float GetXNormalized() const {
                return static_cast<float>(GetX()) / MaximumValue;
            }

            constexpr float GetYNormalized() const {
                return static_cast<float>(GetY()) / MaximumValue;
            }
    };

}
