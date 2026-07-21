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
#include "switch_sixaxis_types.hpp"

namespace ams::controller {

    class SwitchSixAxisProcessor {
        public:
            using Vec3d = utils::Vec3d<float>;
            using Quaternion = utils::Quaternion<float>;

        public:
            static constexpr AccelSensitivity DefaultAccelSensitivity = AccelSensitivity_8G;
            static constexpr GyroSensitivity DefaultGyroSensitivity = GyroSensitivity_2000Dps;

        private:
            SwitchImuSensorSleepMode m_mode;
            AccelSensitivity m_accel_sensitivity;
            GyroSensitivity m_gyro_sensitivity;

            SwitchMotionData m_motion_data;

            Quaternion m_rotation_state;
            os::Tick m_previous_tick;

        public:
            constexpr SwitchSixAxisProcessor() : m_mode(SwitchImuSensorSleepMode::Inactive), m_accel_sensitivity(DefaultAccelSensitivity), m_gyro_sensitivity(DefaultGyroSensitivity), m_motion_data{} { }

        public:
            constexpr SwitchMotionData GetState() const {
                return m_motion_data;
            }

            constexpr void SetState(SwitchMotionData motion_data) {
                m_motion_data = motion_data;
            }

        public:
            void Configure(AccelSensitivity accel_sensitivity, GyroSensitivity gyro_sensitivity);
            void SetSensorSleepMode(SwitchImuSensorSleepMode mode);
            void Reset();
            void Update(Vec3d accel, Vec3d gyro);

        private:
            void UpdateStateQuaternion(Vec3d gyro);
            void UpdateStandardMode(Vec3d accel, Vec3d gyro);
            void UpdateDscaleMode(Vec3d accel, Vec3d gyro);
    };

}
