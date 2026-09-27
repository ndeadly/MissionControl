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

namespace ams::controller {

    template <u8 NorthValue, u8 IdleValue>
    struct DirectionalPadType {
        u8 direction;
    };

    template <u8 NorthValue, u8 IdleValue>
    class DirectionalPad {
        public:
            enum Direction : u8 {
                None  = IdleValue,
                North = NorthValue,
                NorthEast,
                East,
                SouthEast,
                South,
                SouthWest,
                West,
                NorthWest
            };

        private:
            static constexpr u8 AsBits(u8 direction) {
                switch(direction) {
                    case North:      return BIT(0);
                    case NorthEast:  return BIT(0) | BIT(1);
                    case East:       return BIT(1);
                    case SouthEast:  return BIT(2) | BIT(1);
                    case South:      return BIT(2);
                    case SouthWest:  return BIT(2) | BIT(3);
                    case West:       return BIT(3);
                    case NorthWest:  return BIT(0) | BIT(3);
                    default:         return 0;
                }
            }

        private:
            DirectionalPadType<NorthValue, IdleValue> m_dpad;

        public:
            constexpr DirectionalPad() : m_dpad(Direction::None) { }

            constexpr DirectionalPad(DirectionalPadType<NorthValue, IdleValue> dpad) : m_dpad(dpad) { }

            constexpr DirectionalPad(Direction direction) : m_dpad(direction) { }

        public:
            constexpr ALWAYS_INLINE void SetState(DirectionalPadType<NorthValue, IdleValue> dpad) {
                m_dpad = dpad;
            }

            constexpr ALWAYS_INLINE DirectionalPadType<NorthValue, IdleValue> GetState() const {
                return m_dpad;
            }

        public:
            constexpr ALWAYS_INLINE void SetDirection(Direction direction) {
                m_dpad.direction = direction;
            }

            constexpr ALWAYS_INLINE u8 GetDirection() const {
                return m_dpad.direction;
            }

            constexpr ALWAYS_INLINE u8 GetDirectionAsBits() const {
                return AsBits(m_dpad.direction);
            }

            constexpr ALWAYS_INLINE bool IsUp() const {
                return AsBits(m_dpad.direction) & BIT(0);
            }

            constexpr ALWAYS_INLINE bool IsRight() const {
                return AsBits(m_dpad.direction) & BIT(1);
            }

            constexpr ALWAYS_INLINE bool IsDown() const {
                return AsBits(m_dpad.direction) & BIT(2);
            }

            constexpr ALWAYS_INLINE bool IsLeft() const {
                return AsBits(m_dpad.direction) & BIT(3);
            }
    };

    using SwitchGenericDirectionalPad = DirectionalPad<0, 8>;

}
