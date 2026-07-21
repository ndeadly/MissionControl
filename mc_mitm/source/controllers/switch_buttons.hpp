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

    template<size_t N> requires (N > 0 && N <= 4)
    using ButtonsType = std::array<u8, N>;

    enum class SwitchButton : u32 {
        Y       = BIT(0),
        X       = BIT(1),
        B       = BIT(2),
        A       = BIT(3),
        RightSR = BIT(4),
        RightSL = BIT(5),
        R       = BIT(6),
        ZR      = BIT(7),
        Minus   = BIT(8),
        Plus    = BIT(9),
        StickR  = BIT(10),
        StickL  = BIT(11),
        Home    = BIT(12),
        Capture = BIT(13),
        Wired   = BIT(15),
        Down    = BIT(16),
        Up      = BIT(17),
        Right   = BIT(18),
        Left    = BIT(19),
        LeftSR  = BIT(20),
        LeftSL  = BIT(21),
        L       = BIT(22),
        ZL      = BIT(23)
    };

    using SwitchButtonsType = ButtonsType<3>;

    class SwitchButtons {
        private:
            static constexpr u32 ButtonsMask = 0x00FFFFFF;

        private:
            u32 m_button_state;

        public:
            constexpr SwitchButtons() : m_button_state(0) { }

            constexpr SwitchButtons(SwitchButton button) : m_button_state(static_cast<u32>(button)) { }

            constexpr explicit SwitchButtons(u32 button_state) : m_button_state(button_state & ButtonsMask) { }

            constexpr SwitchButtons(SwitchButtonsType buttons) {
                SetState(buttons);
            }

        public:
            constexpr void SetState(SwitchButtonsType buttons) {
                m_button_state = buttons[0] | (buttons[1] << 8) | (buttons[2] << 16);
            }

            constexpr SwitchButtonsType GetState() const {
                return {
                    static_cast<u8>(m_button_state),
                    static_cast<u8>(m_button_state >> 8),
                    static_cast<u8>(m_button_state >> 16)
                };
            }

        public:
            constexpr ALWAYS_INLINE u32 Value() const {
                return m_button_state;
            }

            constexpr ALWAYS_INLINE bool Test(SwitchButton button) const {
                return (m_button_state & static_cast<u32>(button)) != 0;
            }

            constexpr ALWAYS_INLINE bool Any() const {
                return m_button_state != 0;
            }

            constexpr ALWAYS_INLINE bool Any(SwitchButtons buttons) const {
                return (m_button_state & buttons.Value()) != 0;
            }

            constexpr ALWAYS_INLINE bool All(SwitchButtons buttons) const {
                return (m_button_state & buttons.Value()) == buttons.Value();
            }

            constexpr ALWAYS_INLINE bool None() const {
                return m_button_state == 0;
            }

            constexpr ALWAYS_INLINE void Set(SwitchButtons buttons) {
                m_button_state |= buttons.Value();
            }

            constexpr ALWAYS_INLINE void SetIf(SwitchButtons buttons, bool value) {
                if (value)
                    Set(buttons);
            }

            constexpr ALWAYS_INLINE void Clear(SwitchButtons buttons) {
                m_button_state &= ~buttons.Value();
            }

            constexpr ALWAYS_INLINE void Toggle(SwitchButtons buttons) {
                m_button_state ^= buttons.Value();
            }

            constexpr ALWAYS_INLINE void Assign(SwitchButtons buttons, bool value) {
                if (value)
                    Set(buttons);
                else
                    Clear(buttons);
            }

            constexpr ALWAYS_INLINE void Reset() {
                m_button_state = 0;
            }

            constexpr explicit ALWAYS_INLINE operator bool() const {
                return m_button_state != 0;
            }

            constexpr ALWAYS_INLINE bool operator==(SwitchButtons buttons) const {
                return m_button_state == buttons.Value();
            }

            constexpr ALWAYS_INLINE SwitchButtons& operator|=(SwitchButtons buttons) {
                m_button_state |= buttons.Value();
                return *this;
            }

            constexpr ALWAYS_INLINE SwitchButtons& operator&=(SwitchButtons buttons) {
                m_button_state &= buttons.Value();
                return *this;
            }

            constexpr ALWAYS_INLINE SwitchButtons& operator^=(SwitchButtons buttons) {
                m_button_state ^= buttons.Value();
                return *this;
            }
    };

    constexpr ALWAYS_INLINE SwitchButtons operator|(SwitchButtons lhs, SwitchButtons rhs) {
        lhs |= rhs;
        return lhs;
    }

    constexpr ALWAYS_INLINE SwitchButtons operator&(SwitchButtons lhs, SwitchButtons rhs) {
        lhs &= rhs;
        return lhs;
    }

    constexpr ALWAYS_INLINE SwitchButtons operator^(SwitchButtons lhs, SwitchButtons rhs) {
        lhs ^= rhs;
        return lhs;
    }

    constexpr ALWAYS_INLINE SwitchButtons operator~(SwitchButtons buttons) {
        return SwitchButtons(~buttons.Value());
    }

    constexpr ALWAYS_INLINE SwitchButtons operator|(SwitchButton lhs, SwitchButton rhs) {
        return SwitchButtons(lhs) | SwitchButtons(rhs);
    }

    constexpr ALWAYS_INLINE SwitchButtons operator|(SwitchButtons lhs, SwitchButton rhs) {
        lhs |= SwitchButtons(rhs);
        return lhs;
    }

    constexpr ALWAYS_INLINE SwitchButtons operator|(SwitchButton lhs, SwitchButtons rhs) {
        rhs |= SwitchButtons(lhs);
        return rhs;
    }

}
