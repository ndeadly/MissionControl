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
#include <limits>

namespace ams::controller {

    template <std::unsigned_integral T, T MaxValue = std::numeric_limits<T>::max()>
    struct AnalogTriggerType {
        T value;
    };

    template <std::unsigned_integral T, T MaxValue = std::numeric_limits<T>::max()>
    class AnalogTrigger {
        private:
            AnalogTriggerType<T, MaxValue> m_analog_trigger;

        public:
            constexpr AnalogTrigger() : m_analog_trigger{} { }

            constexpr AnalogTrigger(AnalogTriggerType<T, MaxValue> analog_trigger) : m_analog_trigger(analog_trigger) { }

        public:
            constexpr ALWAYS_INLINE void SetValue(T value) {
                m_analog_trigger.value = std::clamp(value, 0, MaxValue);
            }

            constexpr ALWAYS_INLINE T GetValue() const {
                return m_analog_trigger.value;
            }

            constexpr ALWAYS_INLINE float GetNormalizedValue() const {
                return float(m_analog_trigger.value) / MaxValue;
            }
    };
    
}
