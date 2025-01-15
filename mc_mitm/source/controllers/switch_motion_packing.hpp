/*
 * Copyright (c) 2020-2025 ndeadly
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

    template <typename T>
    struct Vec3d {
        T x;
        T y;
        T z;
    } PACKED;

    struct SwitchMotionData {
        union {
            struct {
                Vec3d<s16> accel_0;
                Vec3d<s16> gyro_0;
                Vec3d<s16> accel_1;
                Vec3d<s16> gyro_1;
                Vec3d<s16> accel_2;
                Vec3d<s16> gyro_2;
            } standard;

            // first_sample, mid_sample and last_sample are the three states of the quaternion with a delta of 5ms between eachother 
            // max_index represents the component which has been removed from the quaternion (order is x, y, z, w), for packing_mode 1 and 2 determined by mid_sample and applied to first and last as well (you can rebuild it knowing that a unit quaternion norm is 1.0, you remove the biggest one to minimize math imprecisions)
            // xx_xx_K(l/h) means K-th component (after removing one from the step above) of the xx_xx sample (or delta), possibly divided between low and high when crossing byte struct boundaries
            // delta_last_first_K = last_sample_K - first_sample_K
            // delta_mid_avg_K = mid_sample_K - Average(last_sample_K, first_sample_K)
            // delta_mid_avg_div4 for packing_mode 1 indicates if all components of delta_mid_avg have been right shifted by 2 to fit

            union {
                struct {
                    Vec3d<s16> accel_0;
                    u32 packing_mode      : 2;
                    u32 max_index_first   : 2;
                    u32 first_sample_0    : 13;
                    u32 first_sample_1    : 13;
                    u32 first_sample_2l   : 2;
                    u16 first_sample_2h   : 11;
                    u16 max_index_mid     : 2;
                    u16 mid_sample_0l     : 3;
                    Vec3d<s16> accel_1;
                    u32 mid_sample_0h     : 10;
                    u32 mid_sample_1      : 13;
                    u32 mid_sample_2l     : 9;
                    u16 mid_sample_2h     : 4;
                    u16 max_index_last    : 2;
                    u16 last_sample_0l    : 10;
                    Vec3d<s16> accel_2;
                    u32 last_sample_0h    : 3;
                    u32 last_sample_1     : 13;
                    u32 last_sample_2     : 13;
                    u32 timestamp_start_l : 3;
                    u16 timestamp_start_h : 8;
                    u16 timestamp_count   : 6;
                    u16 : 2;
                } PACKED packing_mode_0;

                struct {
                    Vec3d<s16> accel_0;
                    u32 packing_mode       : 2;
                    u32 delta_mid_avg_div4 : 1;
                    u32 max_index          : 2;
                    u32 first_sample_0     : 16;
                    u32 first_sample_1l    : 11;
                    u16 first_sample_1h    : 5;
                    u16 first_sample_2l    : 11;
                    Vec3d<s16> accel_1;
                    u32 first_sample_2h    : 5;
                    u32 last_sample_0      : 16;
                    u32 last_sample_1l     : 11;
                    u16 last_sample_1h     : 5;
                    u16 last_sample_2l     : 11;
                    Vec3d<s16> accel_2;
                    u32 last_sample_2h     : 5;
                    u32 delta_mid_avg_0    : 8;
                    u32 delta_mid_avg_1    : 8;
                    u32 delta_mid_avg_2    : 8;
                    u32 timestamp_start_l  : 3;
                    u16 timestamp_start_h  : 8;
                    u16 timestamp_count    : 6;
                    u16 : 2;
                } PACKED packing_mode_1;

                struct {
                    Vec3d<s16> accel_0;
                    u32 packing_mode        : 2;
                    u32 max_index           : 2;
                    u32 last_sample_0       : 21;
                    u32 last_sample_1l      : 7;
                    u16 last_sample_1h      : 14;
                    u16 last_sample_2l      : 2;
                    Vec3d<s16> accel_1;
                    u32 last_sample_2h      : 19;
                    u32 delta_last_first_0  : 13;
                    u16 delta_last_first_1  : 13;
                    u16 delta_last_first_2l : 3;
                    Vec3d<s16> accel_2;
                    u32 delta_last_first_2h : 10;
                    u32 delta_mid_avg_0     : 7;
                    u32 delta_mid_avg_1     : 7;
                    u32 delta_mid_avg_2     : 7;
                    u32 timestamp_start_l   : 1;
                    u16 timestamp_start_h   : 10;
                    u16 timestamp_count     : 6;
                } PACKED packing_mode_2;
            } quaternion;
        };
    };

    enum GyroSensitivity : u8 {
        GyroSensitivity_250Dps  = 0,
        GyroSensitivity_500Dps  = 1,
        GyroSensitivity_1000Dps = 2,
        GyroSensitivity_2000Dps = 3
    };

    enum AccelSensitivity : u8 {
        AccelSensitivity_8G  = 0,
        AccelSensitivity_4G  = 1,
        AccelSensitivity_2G  = 2,
        AccelSensitivity_16G = 3
    };

    class SwitchMotionPacker {
        public:
            virtual void PackData(SwitchMotionData* motion_data, Vec3d<float> accel, Vec3d<float> gyro) = 0;
            void SetGyroSensitivity(GyroSensitivity sensitivity) { m_gyro_sensitivity = sensitivity; }
            void SetAccelSensitivity(AccelSensitivity sensitivity) { m_accel_sensitivity = sensitivity; }
            GyroSensitivity GetGyroSensitivity() { return m_gyro_sensitivity; }
            AccelSensitivity GetAccelSensitivity() { return m_accel_sensitivity; }

        protected:
            GyroSensitivity m_gyro_sensitivity;
            AccelSensitivity m_accel_sensitivity;
            float m_gyro_scaling_factor;
            float m_accel_scaling_factor;
    };

    class NullMotionPacker final : public SwitchMotionPacker {
        public:
            void PackData(SwitchMotionData* motion_data, Vec3d<float> accel, Vec3d<float> gyro) override;
    };

    class StandardMotionPacker final : public SwitchMotionPacker {
        public:
            void PackData(SwitchMotionData* motion_data, Vec3d<float> accel, Vec3d<float> gyro) override;
    };

    class QuaternionMotionPacker final : public SwitchMotionPacker {
        private:
            struct Quaternion {
                constexpr Quaternion() : x(0), y(0), z(0), w(1) {};
                constexpr Quaternion(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {};

                union {
                    double raw[4];
                    struct {
                        double x;
                        double y;
                        double z;
                        double w;
                    };
                };
            };
            
            static constexpr Quaternion HamiltonProduct(Quaternion q1, Quaternion q2);
            static constexpr Quaternion QuaternionNormalize(Quaternion q);
            
        public:
            QuaternionMotionPacker();
            void PackData(SwitchMotionData* motion_data, Vec3d<float> accel, Vec3d<float> gyro) override;
            
        private:
            void UpdateRotationState(Vec3d<float> gyro);
            void PackGyroFixedPrecision(SwitchMotionData* motion_data);
            
        private:
            os::Tick m_previous_tick;
            Quaternion m_rotation_state;
    };
}
