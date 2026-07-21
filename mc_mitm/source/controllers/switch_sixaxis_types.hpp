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
#include "../utils.hpp"

namespace ams::controller {

    enum class SwitchImuSensorSleepMode : u8 {
        Inactive          = 0x00,
        Active            = 0x01,
        ActiveDscaleMode1 = 0x02,
        ActiveDscaleMode2 = 0x03,
        ActiveDscaleMode3 = 0x04,
        ActiveDscaleMode4 = 0x05,
    };

    enum class SwitchImuSensorType : u8 {
        LSM6DS3H   = 0x01,
        ICM20600   = 0x03,
        LSM6DS3TRC = 0x04,
    };

    enum GyroSensitivity : u8 {
        GyroSensitivity_250Dps  = 0,
        GyroSensitivity_500Dps  = 1,
        GyroSensitivity_1000Dps = 2,
        GyroSensitivity_2000Dps = 3,
    };

    enum GyroPerformanceRate : u8 {
        GyroPerformanceRate_833Hz = 0,
        GyroPerformanceRate_208Hz = 1,
    };

    enum AccelSensitivity : u8 {
        AccelSensitivity_8G  = 0,
        AccelSensitivity_4G  = 1,
        AccelSensitivity_2G  = 2,
        AccelSensitivity_16G = 3,
    };

    enum AccelAntiAliasFilterBandwidth : u8 {
        AccelAntiAliasFilterBandwidth_200Hz = 0,
        AccelAntiAliasFilterBandwidth_100Hz = 1,
    };

    struct Switch6AxisCalibrationData {
        struct {
            s16 x;
            s16 y;
            s16 z;
        } acc_bias;

        struct {
            s16 x;
            s16 y;
            s16 z;
        } acc_sensitivity;

        struct {
            s16 roll;
            s16 pitch;
            s16 yaw;
        } gyro_bias;

        struct {
            s16 roll;
            s16 pitch;
            s16 yaw;
        } gyro_sensitivity;
    } PACKED;

    struct Switch6AxisHorizontalOffset {
        s16 x;
        s16 y;
        s16 z;
    } PACKED;

    struct SwitchMotionData {
        union {
            struct {
                utils::Vec3d<s16> accel_0;
                utils::Vec3d<s16> gyro_0;
                utils::Vec3d<s16> accel_1;
                utils::Vec3d<s16> gyro_1;
                utils::Vec3d<s16> accel_2;
                utils::Vec3d<s16> gyro_2;
            } standard;

            // first_sample, mid_sample and last_sample are the three states of the quaternion with a delta of 5ms between eachother 
            // max_index represents the component which has been removed from the quaternion (order is x, y, z, w), for packing_mode 1 and 2 determined by mid_sample and applied to first and last as well (you can rebuild it knowing that a unit quaternion norm is 1.0, you remove the biggest one to minimize math imprecisions)
            // xx_xx_K(l/h) means K-th component (after removing one from the step above) of the xx_xx sample (or delta), possibly divided between low and high when crossing byte struct boundaries
            // delta_last_first_K = last_sample_K - first_sample_K
            // delta_mid_avg_K = mid_sample_K - Average(last_sample_K, first_sample_K)
            // delta_mid_avg_div4 for packing_mode 1 indicates if all components of delta_mid_avg have been right shifted by 2 to fit

            union {
                struct {
                    utils::Vec3d<s16> accel_0;
                    u32 mode              : 2;
                    u32 max_index_first   : 2;
                    u32 first_sample_0    : 13;
                    u32 first_sample_1    : 13;
                    u32 first_sample_2l   : 2;
                    u16 first_sample_2h   : 11;
                    u16 max_index_mid     : 2;
                    u16 mid_sample_0l     : 3;
                    utils::Vec3d<s16> accel_1;
                    u32 mid_sample_0h     : 10;
                    u32 mid_sample_1      : 13;
                    u32 mid_sample_2l     : 9;
                    u16 mid_sample_2h     : 4;
                    u16 max_index_last    : 2;
                    u16 last_sample_0l    : 10;
                    utils::Vec3d<s16> accel_2;
                    u32 last_sample_0h    : 3;
                    u32 last_sample_1     : 13;
                    u32 last_sample_2     : 13;
                    u32 timestamp_start_l : 3;
                    u16 timestamp_start_h : 8;
                    u16 timestamp_count   : 6;
                    u16 : 2;
                } PACKED mode0;

                struct {
                    utils::Vec3d<s16> accel_0;
                    u32 mode               : 2;
                    u32 delta_mid_avg_div4 : 1;
                    u32 max_index          : 2;
                    u32 first_sample_0     : 16;
                    u32 first_sample_1l    : 11;
                    u16 first_sample_1h    : 5;
                    u16 first_sample_2l    : 11;
                    utils::Vec3d<s16> accel_1;
                    u32 first_sample_2h    : 5;
                    u32 last_sample_0      : 16;
                    u32 last_sample_1l     : 11;
                    u16 last_sample_1h     : 5;
                    u16 last_sample_2l     : 11;
                    utils::Vec3d<s16> accel_2;
                    u32 last_sample_2h     : 5;
                    u32 delta_mid_avg_0    : 8;
                    u32 delta_mid_avg_1    : 8;
                    u32 delta_mid_avg_2    : 8;
                    u32 timestamp_start_l  : 3;
                    u16 timestamp_start_h  : 8;
                    u16 timestamp_count    : 6;
                    u16 : 2;
                } PACKED mode1;

                struct {
                    utils::Vec3d<s16> accel_0;
                    u32 mode                : 2;
                    u32 max_index           : 2;
                    u32 last_sample_0       : 21;
                    u32 last_sample_1l      : 7;
                    u16 last_sample_1h      : 14;
                    u16 last_sample_2l      : 2;
                    utils::Vec3d<s16> accel_1;
                    u32 last_sample_2h      : 19;
                    u32 delta_last_first_0  : 13;
                    u16 delta_last_first_1  : 13;
                    u16 delta_last_first_2l : 3;
                    utils::Vec3d<s16> accel_2;
                    u32 delta_last_first_2h : 10;
                    u32 delta_mid_avg_0     : 7;
                    u32 delta_mid_avg_1     : 7;
                    u32 delta_mid_avg_2     : 7;
                    u32 timestamp_start_l   : 1;
                    u16 timestamp_start_h   : 10;
                    u16 timestamp_count     : 6;
                } PACKED mode2;
            } dscale;
        };
    };

}
