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
