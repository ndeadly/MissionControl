/*
 * Copyright (c) 2020-2023 ndeadly
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
#include "motion_packers.hpp"

namespace ams::controller {

    Quaternion Quaternion::operator*(Quaternion &rotation) {
        //Hamilton Product
        return Quaternion(w * rotation.x + x * rotation.w + y * rotation.z - z * rotation.y,
                          w * rotation.y + y * rotation.w + z * rotation.x - x * rotation.z,
                          w * rotation.z + z * rotation.w + x * rotation.y - y * rotation.x,
                          w * rotation.w - x * rotation.x - y * rotation.y - z * rotation.z);
    };

    void Quaternion::Normalize() {
        double norm_inverse = 1.0f/sqrt(x * x + y * y + z * z + w * w);
        x *= norm_inverse;
        y *= norm_inverse;
        z *= norm_inverse;
        w *= norm_inverse;
    };

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
        m_timeStampStart6AHWS = 0;
    };

    void QuaternionMotionPacker::PackData(SwitchMotionData* motion_data, Vec3d accel, Vec3d gyro) {
        motion_data->quaternion.mode_2.accel_0 = accel;
        motion_data->quaternion.mode_2.accel_1 = accel;
        motion_data->quaternion.mode_2.accel_2 = accel;

        this->UpdateState(gyro);
        this->FixedPrecisionPack(motion_data);
    };

    void QuaternionMotionPacker::UpdateState(Vec3d gyro) {
        os::Tick current_tick = os::GetSystemTick();

        double vel_x = gyro.x * 2000.0f / 32767.0f * 3.14 / 180.0f;
        double vel_y = gyro.y * 2000.0f / 32767.0f * 3.14 / 180.0f;
        double vel_z = gyro.z * 2000.0f / 32767.0f * 3.14 / 180.0f;

        // Time integration
        double dt = os::ConvertToTimeSpan(current_tick - m_last_tick).GetNanoSeconds() / 1000000000.0f;
        vel_x *= dt;
        vel_y *= dt;
        vel_z *= dt;

        // Euler to quaternion (in a custom Nintendo way)
        double norm_squared = (vel_x * vel_x + vel_z * vel_z + vel_y * vel_y);
        double first_formula = norm_squared * norm_squared / 3840.0f - norm_squared / 48 + 0.5;
        double second_formula = norm_squared * norm_squared / 384.0f - norm_squared / 8 + 1;

        Quaternion current_rotation = Quaternion(vel_x * first_formula, vel_y * first_formula, vel_z * first_formula, second_formula);

        m_rotation_state = m_rotation_state * current_rotation;

        m_rotation_state.Normalize();

        m_last_tick = current_tick;
    };

    void QuaternionMotionPacker::FixedPrecisionPack(SwitchMotionData* motion_data) {
        int quaternion_30bit_components[3];

        motion_data->quaternion.mode_2.mode = 2;

        int max_index = 0;
        for (int i = 1; i < 4; i++) {
            if (fabs(m_rotation_state.raw[i]) > fabs(m_rotation_state.raw[max_index])) {
                max_index = i;
            }
        }

        motion_data->quaternion.mode_2.max_index = max_index;

        bool conjugate = false;
        if (m_rotation_state.raw[max_index] < 0) {
            conjugate = true;
        }

        for (int i = 0; i < 3; i++) {
            quaternion_30bit_components[i] = m_rotation_state.raw[(max_index + i + 1) & 3] * 0x40000000 * (conjugate ? -1 : 1);

            // Some kind of rounding, no practical difference
            //if (quaternion_30bit_components[i] >= 0) {
            //    quaternion_30bit_components[i] += 0x200;
            //}
            //else {
            //    quaternion_30bit_components[i] -= 0x200;
            //}
        }


        motion_data->quaternion.mode_2.last_sample_0 = quaternion_30bit_components[0] >> 10;

        motion_data->quaternion.mode_2.last_sample_1l = ((quaternion_30bit_components[1] >> 10) & 0x7F);
        motion_data->quaternion.mode_2.last_sample_1h = ((quaternion_30bit_components[1] >> 10) & 0x1FFF80) >> 7;

        motion_data->quaternion.mode_2.last_sample_2l = ((quaternion_30bit_components[2] >> 10) & 0x3);
        motion_data->quaternion.mode_2.last_sample_2h = ((quaternion_30bit_components[2] >> 10) & 0x1FFFFC) >> 2;

        // We only store one sample, so all deltas are 0
        motion_data->quaternion.mode_2.delta_last_first_0 = 0;
        motion_data->quaternion.mode_2.delta_last_first_1 = 0;
        motion_data->quaternion.mode_2.delta_last_first_2l = 0;
        motion_data->quaternion.mode_2.delta_last_first_2h = 0;
        motion_data->quaternion.mode_2.delta_mid_avg_0 = 0;
        motion_data->quaternion.mode_2.delta_mid_avg_1 = 0;
        motion_data->quaternion.mode_2.delta_mid_avg_2 = 0;

        // Timestamps handling is still a bit unclear, these are the values that motion_data in no drifting 
        motion_data->quaternion.mode_2.timeStampStart6AHWS_l = m_timeStampStart6AHWS & 0x1;
        motion_data->quaternion.mode_2.timeStampStart6AHWS_h = (m_timeStampStart6AHWS >> 1) & 0x3FF;
        motion_data->quaternion.mode_2.timeStampCount6AHWS = 3;

        m_timeStampStart6AHWS += 12;
    };

}