/*
 * Copyright (c) 2020-2024 ndeadly
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
#include "switch_motion_packing.hpp"

namespace ams::controller {

    namespace {

        // Bits to degrees, degrees to radians and nanoseconds to seconds
        constexpr double QuatScaleFactor = 2000.0f / INT16_MAX * M_PI / 180.0f / 1000000000.0f;

    }

    void NullMotionPacker::PackData(SwitchMotionData* motion_data, Vec3d accel, Vec3d gyro) {
        AMS_UNUSED(accel);
        AMS_UNUSED(gyro);
        std::memset(motion_data, 0, sizeof(SwitchMotionData));
    };

    void StandardMotionPacker::PackData(SwitchMotionData* motion_data, Vec3d accel, Vec3d gyro) {
        motion_data->standard.accel_0 = accel;
        motion_data->standard.accel_1 = accel;
        motion_data->standard.accel_2 = accel;
        motion_data->standard.gyro_0 = gyro;
        motion_data->standard.gyro_1 = gyro;
        motion_data->standard.gyro_2 = gyro;
    };

    QuaternionMotionPacker::QuaternionMotionPacker() {
        m_last_tick = os::GetSystemTick();
        m_timestamp_start = 0;
    };

    constexpr QuaternionMotionPacker::Quaternion QuaternionMotionPacker::HamiltonProduct(Quaternion q1, Quaternion q2) {
        return Quaternion(q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
                          q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z,
                          q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x,
                          q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
    };

    constexpr QuaternionMotionPacker::Quaternion QuaternionMotionPacker::QuaternionNormalize(Quaternion q) {
        double norm_inverse = 1.0f / std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return Quaternion(q.x * norm_inverse,
                          q.y * norm_inverse,
                          q.z * norm_inverse,
                          q.w * norm_inverse);
    };

    void QuaternionMotionPacker::PackData(SwitchMotionData* motion_data, Vec3d accel, Vec3d gyro) {
        motion_data->quaternion.packing_mode_2.accel_0 = accel;
        motion_data->quaternion.packing_mode_2.accel_1 = accel;
        motion_data->quaternion.packing_mode_2.accel_2 = accel;

        this->UpdateRotationState(gyro);
        this->PackGyroFixedPrecision(motion_data);
    };

    void QuaternionMotionPacker::UpdateRotationState(Vec3d gyro) {
        os::Tick current_tick = os::GetSystemTick();

        double dt = os::ConvertToTimeSpan(current_tick - m_last_tick).GetNanoSeconds();

        double angle_x = gyro.x * QuatScaleFactor * dt;
        double angle_y = gyro.y * QuatScaleFactor * dt;
        double angle_z = gyro.z * QuatScaleFactor * dt;

        // Euler to quaternion as implemented by Nintendo
        double norm_squared = angle_x * angle_x + angle_y * angle_y + angle_z * angle_z;
        double vector_scale = norm_squared * norm_squared / 3840.0f - norm_squared / 48 + 0.5;
        double scalar_component = norm_squared * norm_squared / 384.0f - norm_squared / 8 + 1;

        // Seems to roughly translate to Quaternion(angle_x * 1/2 * cos(norm/2), angle_y * 1/2 * cos(norm/2), angle_z * 1/2 * cos(norm/2), cos(norm/2)), at least for small values 
        Quaternion current_rotation(angle_x * vector_scale, angle_y * vector_scale, angle_z * vector_scale, scalar_component);

        m_rotation_state = QuaternionNormalize(HamiltonProduct(m_rotation_state, current_rotation));

        m_last_tick = current_tick;
    };

    void QuaternionMotionPacker::PackGyroFixedPrecision(SwitchMotionData* motion_data) {
        // We will use this mode since it's the one that loses less precision for our (single) motion sample 
        motion_data->quaternion.packing_mode_2.packing_mode = 2;

        // Locate the index of the component with the maximum absolute value
        int max_index = 0;
        for (int i = 1; i < 4; ++i) {
            if (std::fabs(m_rotation_state.raw[i]) > std::fabs(m_rotation_state.raw[max_index])) {
                max_index = i;
            }
        }

        motion_data->quaternion.packing_mode_2.max_index = max_index;

        // Exclude the max_index component from the component list, invert sign of the remaining components if it was negative. Scales the final result to a 30 bit fixed precision format where 0x40000000 is 1.0
        s32 quaternion_30bit_components[3];
        for (int i = 0; i < 3; ++i) {
            quaternion_30bit_components[i] = m_rotation_state.raw[(max_index + i + 1) & 3] * 0x40000000 * (m_rotation_state.raw[max_index] < 0 ? -1 : 1);
        }

        // Insert into the last sample components, do bit operations to account for split data
        motion_data->quaternion.packing_mode_2.last_sample_0 = quaternion_30bit_components[0] >> 10;
        motion_data->quaternion.packing_mode_2.last_sample_1l = ((quaternion_30bit_components[1] >> 10) & 0x7F);
        motion_data->quaternion.packing_mode_2.last_sample_1h = ((quaternion_30bit_components[1] >> 10) & 0x1FFF80) >> 7;
        motion_data->quaternion.packing_mode_2.last_sample_2l = ((quaternion_30bit_components[2] >> 10) & 0x3);
        motion_data->quaternion.packing_mode_2.last_sample_2h = ((quaternion_30bit_components[2] >> 10) & 0x1FFFFC) >> 2;

        // We only store one sample, so all deltas are 0
        motion_data->quaternion.packing_mode_2.delta_last_first_0 = 0;
        motion_data->quaternion.packing_mode_2.delta_last_first_1 = 0;
        motion_data->quaternion.packing_mode_2.delta_last_first_2l = 0;
        motion_data->quaternion.packing_mode_2.delta_last_first_2h = 0;
        motion_data->quaternion.packing_mode_2.delta_mid_avg_0 = 0;
        motion_data->quaternion.packing_mode_2.delta_mid_avg_1 = 0;
        motion_data->quaternion.packing_mode_2.delta_mid_avg_2 = 0;

        // Timestamps handling is still a bit unclear, these are the values that result in no drifting 
        motion_data->quaternion.packing_mode_2.timestamp_start_l = m_timestamp_start & 0x1;
        motion_data->quaternion.packing_mode_2.timestamp_start_h = (m_timestamp_start >> 1) & 0x3FF;
        motion_data->quaternion.packing_mode_2.timestamp_count = 3;

        // Increment for the next cycle
        m_timestamp_start += 12;
    };

}