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
#include "switch_sixaxis_processor.hpp"

namespace ams::controller {

    namespace {

        // Degrees to radians and nanoseconds to seconds
        constexpr double QuatScaleFactor = std::numbers::pi / 180.0f / 1000000000.0f;

        // See https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/issues/18#issuecomment-331324555
        constexpr float BaseGyroSensitivity = 936.0f / (13371 - 0); // 13371 and 0 (offset, would technically need to be done for each axis) match the factory_motion_calibration value in the SPI flash
        constexpr float BaseAccelSensitivity = 4.0f / (16384 - 0); // 16384 and 0 (offset, would technically need to be done for each axis) match the factory_motion_calibration value in the SPI flash

        constexpr float GyroSensitivities[] = { BaseGyroSensitivity / 8, BaseGyroSensitivity / 4, BaseGyroSensitivity / 2, BaseGyroSensitivity };
        constexpr float AccelSensitivities[] = { BaseAccelSensitivity, BaseAccelSensitivity / 2, BaseAccelSensitivity / 4, BaseAccelSensitivity * 2};

    }

    void SwitchSixAxisProcessor::Configure(AccelSensitivity accel_sensitivity, GyroSensitivity gyro_sensitivity) {
        m_accel_sensitivity = accel_sensitivity;
        m_gyro_sensitivity  = gyro_sensitivity;
    }

    void SwitchSixAxisProcessor::SetSensorSleepMode(SwitchImuSensorSleepMode mode) {
        m_mode = mode;
        std::memset(&m_motion_data, 0, sizeof(m_motion_data));
    }

    void SwitchSixAxisProcessor::Reset() {
        m_accel_sensitivity = DefaultAccelSensitivity;
        m_gyro_sensitivity  = DefaultGyroSensitivity;
        std::memset(&m_motion_data, 0, sizeof(m_motion_data));
        m_rotation_state = Quaternion::Identity();
        m_previous_tick = os::GetSystemTick();
    }

    void SwitchSixAxisProcessor::Update(Vec3d accel, Vec3d gyro) {
        switch (m_mode) {
            case SwitchImuSensorSleepMode::Inactive:
                return;

            case SwitchImuSensorSleepMode::Active:
                this->UpdateStandardMode(accel, gyro);
                break;

            case SwitchImuSensorSleepMode::ActiveDscaleMode1:
            case SwitchImuSensorSleepMode::ActiveDscaleMode2:
            case SwitchImuSensorSleepMode::ActiveDscaleMode3:
            case SwitchImuSensorSleepMode::ActiveDscaleMode4:
                this->UpdateDscaleMode(accel, gyro);
                break;

            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void SwitchSixAxisProcessor::UpdateStateQuaternion(Vec3d gyro) {
        os::Tick current_tick = os::GetSystemTick();

        double dt = os::ConvertToTimeSpan(current_tick - m_previous_tick).GetNanoSeconds();

        double angle_x = gyro.x * QuatScaleFactor * dt;
        double angle_y = gyro.y * QuatScaleFactor * dt;
        double angle_z = gyro.z * QuatScaleFactor * dt;

        // Euler to quaternion as implemented by Nintendo
        double norm_squared = angle_x * angle_x + angle_y * angle_y + angle_z * angle_z;
        double vector_scale = norm_squared * norm_squared / 3840.0f - norm_squared / 48 + 0.5;
        double scalar_component = norm_squared * norm_squared / 384.0f - norm_squared / 8 + 1;

        // Seems to roughly translate to Quaternion(angle_x * 1/2 * cos(norm/2), angle_y * 1/2 * cos(norm/2), angle_z * 1/2 * cos(norm/2), cos(norm/2)), at least for small values 
        Quaternion current_rotation(angle_x * vector_scale, angle_y * vector_scale, angle_z * vector_scale, scalar_component);

        m_rotation_state = Quaternion::Normalize(m_rotation_state * current_rotation);
        m_previous_tick  = current_tick;
    }

    void SwitchSixAxisProcessor::UpdateStandardMode(Vec3d accel, Vec3d gyro) {
        const utils::Vec3d<s16> accel_scaled = {
            .x = static_cast<s16>(std::clamp<float>(accel.x / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX)),
            .y = static_cast<s16>(std::clamp<float>(accel.y / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX)),
            .z = static_cast<s16>(std::clamp<float>(accel.z / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX))
        };

        m_motion_data.standard.accel_0 = accel_scaled;
        m_motion_data.standard.accel_1 = accel_scaled;
        m_motion_data.standard.accel_2 = accel_scaled;

        const utils::Vec3d<s16> gyro_scaled = {
            .x = static_cast<s16>(std::clamp<float>(gyro.x / GyroSensitivities[m_gyro_sensitivity], INT16_MIN, INT16_MAX)),
            .y = static_cast<s16>(std::clamp<float>(gyro.y / GyroSensitivities[m_gyro_sensitivity], INT16_MIN, INT16_MAX)),
            .z = static_cast<s16>(std::clamp<float>(gyro.z / GyroSensitivities[m_gyro_sensitivity], INT16_MIN, INT16_MAX))
        };

        m_motion_data.standard.gyro_0 = gyro_scaled;
        m_motion_data.standard.gyro_1 = gyro_scaled;
        m_motion_data.standard.gyro_2 = gyro_scaled;
    }

    void SwitchSixAxisProcessor::UpdateDscaleMode(Vec3d accel, Vec3d gyro) {
        this->UpdateStateQuaternion(gyro);

        // We will use this mode since it's the one that loses less precision for our (single) motion sample 
        m_motion_data.dscale.mode2.mode = 2;

        const utils::Vec3d<s16> accel_scaled = {
            .x = static_cast<s16>(std::clamp<float>(accel.x / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX)),
            .y = static_cast<s16>(std::clamp<float>(accel.y / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX)),
            .z = static_cast<s16>(std::clamp<float>(accel.z / AccelSensitivities[m_accel_sensitivity], INT16_MIN, INT16_MAX))
        };

        m_motion_data.dscale.mode2.accel_0 = accel_scaled;
        m_motion_data.dscale.mode2.accel_1 = accel_scaled;
        m_motion_data.dscale.mode2.accel_2 = accel_scaled;

        // Locate the index of the component with the maximum absolute value
        int max_index = 0;
        for (int i = 1; i < 4; ++i) {
            if (std::fabs(m_rotation_state.raw[i]) > std::fabs(m_rotation_state.raw[max_index])) {
                max_index = i;
            }
        }

        m_motion_data.dscale.mode2.max_index = max_index;

        // Exclude the max_index component from the component list, invert sign of the remaining components if it was negative. Scales the final result to a 30 bit fixed precision format where 0x40000000 is 1.0
        s32 quaternion_30bit_components[3];
        for (int i = 0; i < 3; ++i) {
            quaternion_30bit_components[i] = m_rotation_state.raw[(max_index + i + 1) & 3] * 0x40000000 * (m_rotation_state.raw[max_index] < 0 ? -1 : 1);
        }

        // Insert into the last sample components, do bit operations to account for split data
        m_motion_data.dscale.mode2.last_sample_0 = quaternion_30bit_components[0] >> 10;
        m_motion_data.dscale.mode2.last_sample_1l = ((quaternion_30bit_components[1] >> 10) & 0x7F);
        m_motion_data.dscale.mode2.last_sample_1h = ((quaternion_30bit_components[1] >> 10) & 0x1FFF80) >> 7;
        m_motion_data.dscale.mode2.last_sample_2l = ((quaternion_30bit_components[2] >> 10) & 0x3);
        m_motion_data.dscale.mode2.last_sample_2h = ((quaternion_30bit_components[2] >> 10) & 0x1FFFFC) >> 2;

        // We only store one sample, so all deltas are 0
        m_motion_data.dscale.mode2.delta_last_first_0 = 0;
        m_motion_data.dscale.mode2.delta_last_first_1 = 0;
        m_motion_data.dscale.mode2.delta_last_first_2l = 0;
        m_motion_data.dscale.mode2.delta_last_first_2h = 0;
        m_motion_data.dscale.mode2.delta_mid_avg_0 = 0;
        m_motion_data.dscale.mode2.delta_mid_avg_1 = 0;
        m_motion_data.dscale.mode2.delta_mid_avg_2 = 0;

        // Timestamps handling is still a bit unclear, these are the values that result in no drifting 
        auto timestamp_start = os::ConvertToTimeSpan(m_previous_tick).GetMilliSeconds();
        m_motion_data.dscale.mode2.timestamp_start_l = timestamp_start & 0x1;
        m_motion_data.dscale.mode2.timestamp_start_h = (timestamp_start >> 1) & 0x3FF;
        m_motion_data.dscale.mode2.timestamp_count = 3;
    }

}
