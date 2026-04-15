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
#include "switch_battery_level.hpp"

namespace ams::controller {

    using SwitchPowerInfoType = util::BitPack8;

    class SwitchPowerInfo {
        private:
            using Powered        = util::BitPack8::Field<0, 1, bool>;
            using ConnectionInfo = util::BitPack8::Field<1, 3, u8>;
            using Charging       = util::BitPack8::Field<4, 1, bool>;
            using BatteryLevel   = util::BitPack8::Field<5, 3, SwitchBatteryLevel>;

        private:
            SwitchPowerInfoType m_power_info;

        public:
            constexpr SwitchPowerInfo() : m_power_info{0} { }

            constexpr SwitchPowerInfo(SwitchPowerInfoType power_info) : m_power_info(power_info) { }

            constexpr SwitchPowerInfo(bool powered, u8 connection_info, bool charging, SwitchBatteryLevel battery_level) {
                this->SetPowered(powered);
                this->SetConnectionInfo(connection_info);
                this->SetCharging(charging);
                this->SetBatteryLevel(battery_level);
            }

        public:
            constexpr void SetState(SwitchPowerInfoType power_info) {
                m_power_info = power_info;
            }

            constexpr SwitchPowerInfoType GetState() const {
                return m_power_info;
            }

        public:
            constexpr void SetPowered(bool powered) {
                m_power_info.Set<Powered>(powered);
            }

            constexpr bool IsPowered() const {
                return m_power_info.Get<Powered>();
            }

            constexpr void SetConnectionInfo(u8 connection_info) {
                m_power_info.Set<ConnectionInfo>(connection_info);
            }

            constexpr u8 GetConnectionInfo() const {
                return m_power_info.Get<ConnectionInfo>();
            }

            constexpr void SetCharging(bool charging) {
                m_power_info.Set<Charging>(charging);
            }

            constexpr bool IsCharging() const {
                return m_power_info.Get<Charging>();
            }

            constexpr void SetBatteryLevel(SwitchBatteryLevel battery_level) {
                m_power_info.Set<BatteryLevel>(battery_level);
            }

            constexpr SwitchBatteryLevel GetBatteryLevel() const {
                return m_power_info.Get<BatteryLevel>();
            }
    };

}
