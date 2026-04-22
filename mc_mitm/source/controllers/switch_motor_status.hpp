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

    using SwitchMotorStatusType = util::BitPack8;

    class SwitchMotorStatus {
        private:
            using BufferStatus = util::BitPack8::Field<0, 3, u8>;
            using Flag         = util::BitPack8::Field<3, 1, bool>;

        private:
            SwitchMotorStatusType m_motor_status;

        public:
            constexpr SwitchMotorStatus() : m_motor_status(0) { }

            constexpr SwitchMotorStatus(SwitchMotorStatusType motor_status) : m_motor_status(motor_status) { }

        public:
            constexpr void SetState(SwitchMotorStatusType motor_status) {
                m_motor_status = motor_status;
            }

            constexpr SwitchMotorStatusType GetState() const {
                return m_motor_status;
            }

        public:
            constexpr void SetBufferStatus(u8 status) {
                m_motor_status.Set<BufferStatus>(status);
            }

            constexpr void SetFlag(bool flag) {
                m_motor_status.Set<Flag>(flag);
            }

            constexpr u8 GetBufferStatus() const {
                return m_motor_status.Get<BufferStatus>();
            }

            constexpr bool GetFlag() const {
                return m_motor_status.Get<Flag>();
            }
    };

}
