/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "wii_controller.hpp"
#include "controller_utils.hpp"
#include "../async/async.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr uint8_t init_data1[] = {0x55};
        constexpr uint8_t init_data2[] = {0x00};

        constexpr float nunchuck_stick_scale_factor = float(UINT12_MAX) / 0xb8;
        constexpr float wiiu_scale_factor           = 2.0;
        constexpr float left_stick_scale_factor     = float(UINT12_MAX) / 0x3f;
        constexpr float right_stick_scale_factor    = float(UINT12_MAX) / 0x1f;

        constexpr float accel_scale_factor = 65535 / 16000.0f * 1000;
        constexpr float gyro_scale_factor = 65535 / (13371 * 360.0f) * 1000;

        float CalibrateWeightData(uint16_t x, uint16_t cal_0kg, uint16_t cal_17kg, uint16_t cal_34kg) {
            x = util::SwapEndian(x);

            if (x < cal_0kg) {
                return 0.0f;
            } else if (x < cal_17kg) {
                return (17.0f * (x - cal_0kg)) / (cal_17kg - cal_0kg);
            }  else {
                return ((17.0f * (x - cal_17kg)) / (cal_34kg - cal_17kg)) + 17.0f;
            }
        }

        float ApplyEasingFunction(float x) {
            constexpr float a = 3.0;
            constexpr float s = 0.15;

            return (std::pow(std::abs(x) + s, a) / (std::pow(std::abs(x) + s, a) + std::pow(1 - (std::abs(x) + s), a))) * (x < 0 ? -1.0f : 1.0f);
        }

    }

    Result WiiController::Initialize() {
        R_TRY(this->SetReportMode(0x31));
        R_TRY(EmulatedSwitchController::Initialize());

        // Only attempt to grab calibration and check for MotionPlus for Wiimote controllers
        if (m_id.pid == 0x0306) {
            // Read the accelerometer calibration from Wiimote memory
            R_TRY(this->GetAccelerometerCalibration(&m_accel_calibration));
        }

        // Request a status report to check extension controller status
        R_TRY(this->QueryStatus());

        return ams::ResultSuccess();
    }

    void WiiController::ProcessInputData(const bluetooth::HidReport *report) {
        auto wii_report = reinterpret_cast<const WiiReportData *>(&report->data);

        switch(wii_report->id) {
            case 0x20:
                this->MapInputReport0x20(wii_report);
                this->HandleStatusReport(wii_report);
                break;
            case 0x21:
                this->MapInputReport0x21(wii_report); break;
            case 0x22:
                this->MapInputReport0x22(wii_report); break;
            case 0x30:
                this->MapInputReport0x30(wii_report); break;
            case 0x31:
                this->MapInputReport0x31(wii_report); break;
            case 0x32:
                this->MapInputReport0x32(wii_report); break;
            case 0x34:
                this->MapInputReport0x34(wii_report); break;
            case 0x35:
                this->MapInputReport0x35(wii_report); break;
            case 0x3d:
                this->MapInputReport0x3d(wii_report); break;
            default:
                break;
        }
    }

    void WiiController::MapInputReport0x20(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x20.buttons);

        if (m_extension != WiiExtensionController_WiiUPro) {
            m_battery = convert_battery_255(src->input0x20.battery);
        }
    }

    void WiiController::MapInputReport0x21(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x21.buttons);
    }

    void WiiController::MapInputReport0x22(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x22.buttons);
    }

    void WiiController::MapInputReport0x30(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x30.buttons);
    }

    void WiiController::MapInputReport0x31(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x31.buttons);
        this->MapAccelerometerData(&src->input0x31.accel, &src->input0x31.buttons);
    }

    void WiiController::MapInputReport0x32(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x32.buttons);
        this->MapExtensionBytes(src->input0x32.extension);
    }

    void WiiController::MapInputReport0x34(const WiiReportData *src) {
        this->MapCoreButtons(&src->input0x34.buttons);
        this->MapExtensionBytes(src->input0x34.extension);
    }

    void WiiController::MapInputReport0x35(const WiiReportData *src) {
        if (m_extension == WiiExtensionController_MotionPlusClassicControllerPassthrough) {
            // Don't map core buttons when receiving an interleaved MotionPlus report to avoid clobbering classic controller button state
            auto extension_data = reinterpret_cast<const WiiClassicControllerPassthroughExtensionData *>(src->input0x35.extension);
            if (!extension_data->motionplus_report) {
                this->MapCoreButtons(&src->input0x35.buttons);
            }
        } else {
            this->MapCoreButtons(&src->input0x35.buttons);
        }
        this->MapAccelerometerData(&src->input0x35.accel, &src->input0x35.buttons);
        this->MapExtensionBytes(src->input0x35.extension);
    }

    void WiiController::MapInputReport0x3d(const WiiReportData *src) {
        this->MapExtensionBytes(src->input0x3d.extension);
    }

    void WiiController::MapCoreButtons(const WiiButtonData *buttons) {
        if (m_orientation == WiiControllerOrientation_Horizontal) {
            m_buttons.dpad_down  = buttons->dpad_left;
            m_buttons.dpad_up    = buttons->dpad_right;
            m_buttons.dpad_right = buttons->dpad_down;
            m_buttons.dpad_left  = buttons->dpad_up;

            m_buttons.A = buttons->two;
            m_buttons.B = buttons->one;

            m_buttons.R = buttons->A;
            m_buttons.L = buttons->B;

            m_buttons.minus = buttons->minus;
            m_buttons.plus  = buttons->plus;

            m_buttons.home = buttons->home;
        } else {
            m_buttons.dpad_down  = buttons->dpad_down;
            m_buttons.dpad_up    = buttons->dpad_up;
            m_buttons.dpad_right = buttons->dpad_right;
            m_buttons.dpad_left  = buttons->dpad_left;

            m_buttons.A = buttons->A;
            m_buttons.B = buttons->B;

            if (m_extension == WiiExtensionController_ClassicPro) {
                // Allow buttons one and two to be used for L3/R3 when Classic or Classic Pro controller connected
                m_buttons.lstick_press = buttons->one;
                m_buttons.rstick_press = buttons->two;
            } else {
                // Not the best mapping but at least most buttons are mapped to something when nunchuck is connected.
                m_buttons.R  = buttons->one;
                m_buttons.ZR = buttons->two;
            }

            m_buttons.minus = buttons->minus;
            m_buttons.plus  = buttons->plus;

            m_buttons.home = buttons->home;
        }
    }

    void WiiController::MapAccelerometerData(const WiiAccelerometerData *accel, const WiiButtonData *buttons) {
        if (m_enable_motion) {
            uint16_t x_raw = (accel->x << 2) | ((buttons->raw[0] >> 5) & 0x3);
            uint16_t y_raw = (accel->y << 2) | (((buttons->raw[1] >> 4) & 0x1) << 1);
            uint16_t z_raw = (accel->z << 2) | (((buttons->raw[1] >> 5) & 0x1) << 1);

            int16_t x = -static_cast<int16_t>(accel_scale_factor * (float(x_raw - m_accel_calibration.acc_x_0g) / float(m_accel_calibration.acc_x_1g - m_accel_calibration.acc_x_0g)));
            int16_t y = -static_cast<int16_t>(accel_scale_factor * (float(y_raw - m_accel_calibration.acc_y_0g) / float(m_accel_calibration.acc_y_1g - m_accel_calibration.acc_y_0g)));
            int16_t z =  static_cast<int16_t>(accel_scale_factor * (float(z_raw - m_accel_calibration.acc_z_0g) / float(m_accel_calibration.acc_z_1g - m_accel_calibration.acc_z_0g)));

            if (m_orientation == WiiControllerOrientation_Horizontal) {
                m_motion_data[0].accel_x = x;
                m_motion_data[0].accel_y = y;
                m_motion_data[0].accel_z = z;

                m_motion_data[1].accel_x = x;
                m_motion_data[1].accel_y = y;
                m_motion_data[1].accel_z = z;

                m_motion_data[2].accel_x = x;
                m_motion_data[2].accel_y = y;
                m_motion_data[2].accel_z = z;
            } else {
                m_motion_data[0].accel_x =  y;
                m_motion_data[0].accel_y = -x;
                m_motion_data[0].accel_z =  z;

                m_motion_data[1].accel_x =  y;
                m_motion_data[1].accel_y = -x;
                m_motion_data[1].accel_z =  z;

                m_motion_data[2].accel_x =  y;
                m_motion_data[2].accel_y = -x;
                m_motion_data[2].accel_z =  z;
            }
        }
        else {
            std::memset(&m_motion_data, 0, sizeof(m_motion_data));
        }
    }

    void WiiController::MapExtensionBytes(const uint8_t ext[]) {
        switch(m_extension) {
            case WiiExtensionController_Nunchuck:
                this->MapNunchuckExtension(ext); break;
            case WiiExtensionController_ClassicPro:
                this->MapClassicControllerExtension(ext); break;
            case WiiExtensionController_WiiUPro:
                this->MapWiiUProControllerExtension(ext); break;
            case WiiExtensionController_TaTaCon:
                this->MapTaTaConExtension(ext); break;
            case WiiExtensionController_BalanceBoard:
                this->MapBalanceBoardExtension(ext); break;
            case WiiExtensionController_MotionPlus:
            case WiiExtensionController_MotionPlusNunchuckPassthrough:
            case WiiExtensionController_MotionPlusClassicControllerPassthrough:
                this->MapMotionPlusExtension(ext); break;
            default:
                break;
        }
    }

    void WiiController::MapNunchuckExtension(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const WiiNunchuckExtensionData *>(ext);

        m_left_stick.SetData(
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension_data->stick_x - 0x80) + STICK_ZERO), 0, 0xfff),
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension_data->stick_y - 0x80) + STICK_ZERO), 0, 0xfff)
        );

        m_buttons.L  = !extension_data->C;
        m_buttons.ZL = !extension_data->Z;
    }

    void WiiController::MapClassicControllerExtension(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const WiiClassicControllerExtensionData *>(ext);

        m_left_stick.SetData(
            static_cast<uint16_t>(left_stick_scale_factor * (extension_data->left_stick_x - 0x20) + STICK_ZERO) & 0xfff,
            static_cast<uint16_t>(left_stick_scale_factor * (extension_data->left_stick_y - 0x20) + STICK_ZERO) & 0xfff
        );
        m_right_stick.SetData(
            static_cast<uint16_t>(right_stick_scale_factor * (((extension_data->right_stick_x_43 << 3) | (extension_data->right_stick_x_21 << 1) | extension_data->right_stick_x_0) - 0x10) + STICK_ZERO) & 0xfff,
            static_cast<uint16_t>(right_stick_scale_factor * (extension_data->right_stick_y - 0x10) + STICK_ZERO) & 0xfff
        );

        m_buttons.dpad_down  |= !extension_data->buttons.dpad_down;
        m_buttons.dpad_up    |= !extension_data->buttons.dpad_up;
        m_buttons.dpad_right |= !extension_data->buttons.dpad_right;
        m_buttons.dpad_left  |= !extension_data->buttons.dpad_left;

        m_buttons.A |= !extension_data->buttons.A;
        m_buttons.B |= !extension_data->buttons.B;
        m_buttons.X  = !extension_data->buttons.X;
        m_buttons.Y  = !extension_data->buttons.Y;

        m_buttons.L   = !extension_data->buttons.L | (((extension_data->left_trigger_43 << 3) | (extension_data->left_trigger_20)) > 0x0f);
        m_buttons.ZL  = !extension_data->buttons.ZL;
        m_buttons.R  |= !extension_data->buttons.R | (extension_data->right_trigger > 0x0f);
        m_buttons.ZR |= !extension_data->buttons.ZR;

        m_buttons.minus |= !extension_data->buttons.minus;
        m_buttons.plus  |= !extension_data->buttons.plus;

        m_buttons.home |= !extension_data->buttons.home;
    }

    void WiiController::MapWiiUProControllerExtension(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const WiiUProExtensionData *>(ext);

        m_left_stick.SetData(
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension_data->left_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff),
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension_data->left_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );
        m_right_stick.SetData(
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension_data->right_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff),
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension_data->right_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );

        m_buttons.dpad_down  = !extension_data->buttons.dpad_down;
        m_buttons.dpad_up    = !extension_data->buttons.dpad_up;
        m_buttons.dpad_right = !extension_data->buttons.dpad_right;
        m_buttons.dpad_left  = !extension_data->buttons.dpad_left;

        m_buttons.A = !extension_data->buttons.A;
        m_buttons.B = !extension_data->buttons.B;
        m_buttons.X = !extension_data->buttons.X;
        m_buttons.Y = !extension_data->buttons.Y;

        m_buttons.R  = !extension_data->buttons.R;
        m_buttons.ZR = !extension_data->buttons.ZR;
        m_buttons.L  = !extension_data->buttons.L;
        m_buttons.ZL = !extension_data->buttons.ZL;

        m_buttons.minus = !extension_data->buttons.minus;
        m_buttons.plus  = !extension_data->buttons.plus;

        m_buttons.lstick_press = !extension_data->buttons.lstick_press;
        m_buttons.rstick_press = !extension_data->buttons.rstick_press;

        m_buttons.home = !extension_data->buttons.home;

        m_ext_power = !extension_data->buttons.usb_connected;
        m_charging = !extension_data->buttons.charging;
        m_battery = (extension_data->buttons.battery == 0b111) ? 0 : (extension_data->buttons.battery << 1);
    }

    void WiiController::MapTaTaConExtension(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const TaTaConExtensionData *>(ext);

        m_buttons.X           = !extension_data->R_rim;
        m_buttons.Y           = !extension_data->R_center;
        m_buttons.dpad_up    |= !extension_data->L_rim;
        m_buttons.dpad_right |= !extension_data->L_center;
    }

    void WiiController::MapBalanceBoardExtension(const uint8_t ext[]) {
        auto extension = reinterpret_cast<const BalanceBoardExtensionData *>(ext);

        float top_right    = CalibrateWeightData(extension->top_right,    m_ext_calibration.balance_board.top_right_0kg,    m_ext_calibration.balance_board.top_right_17kg,    m_ext_calibration.balance_board.top_right_34kg);
        float bottom_right = CalibrateWeightData(extension->bottom_right, m_ext_calibration.balance_board.bottom_right_0kg, m_ext_calibration.balance_board.bottom_right_17kg, m_ext_calibration.balance_board.bottom_right_34kg);
        float top_left     = CalibrateWeightData(extension->top_left,     m_ext_calibration.balance_board.top_left_0kg,     m_ext_calibration.balance_board.top_left_17kg,     m_ext_calibration.balance_board.top_left_34kg);
        float bottom_left  = CalibrateWeightData(extension->bottom_left,  m_ext_calibration.balance_board.bottom_left_0kg,  m_ext_calibration.balance_board.bottom_left_17kg,  m_ext_calibration.balance_board.bottom_left_34kg);
        float total_weight = top_right + bottom_right + top_left + bottom_left;

        float x = 0.0f;
        float y = 0.0f;
        if (total_weight > 1.0f) {
            x = ApplyEasingFunction(((top_right + bottom_right) - (top_left + bottom_left)) / total_weight);
            y = ApplyEasingFunction(((top_right + top_left) - (bottom_right + bottom_left)) / total_weight);
        }

        m_left_stick.SetData(
            std::clamp<uint16_t>(static_cast<uint16_t>((x * (UINT12_MAX / 2)) + STICK_ZERO), 0, UINT12_MAX),
            std::clamp<uint16_t>(static_cast<uint16_t>((y * (UINT12_MAX / 2)) + STICK_ZERO), 0, UINT12_MAX)
        );
    }

    void WiiController::MapMotionPlusExtension(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const MotionPlusExtensionData *>(ext);

        this->UpdateMotionPlusExtensionStatus(extension_data->extension_connected);

        if (extension_data->motionplus_report) {
            uint16_t pitch_raw = ((extension_data->pitch_speed_hi << 8) | extension_data->pitch_speed_lo) << 2;
            uint16_t roll_raw =  ((extension_data->roll_speed_hi  << 8) | extension_data->roll_speed_lo) << 2;
            uint16_t yaw_raw =   ((extension_data->yaw_speed_hi   << 8) | extension_data->yaw_speed_lo) << 2;

            uint16_t pitch_0deg = (extension_data->pitch_slow_mode ? m_ext_calibration.motion_plus.slow.pitch_zero : m_ext_calibration.motion_plus.fast.pitch_zero);
            uint16_t roll_0deg  = (extension_data->roll_slow_mode  ? m_ext_calibration.motion_plus.slow.roll_zero  : m_ext_calibration.motion_plus.fast.roll_zero);
            uint16_t yaw_0deg   = (extension_data->yaw_slow_mode   ? m_ext_calibration.motion_plus.slow.yaw_zero   : m_ext_calibration.motion_plus.fast.yaw_zero);

            uint16_t pitch_scale = (extension_data->pitch_slow_mode ? m_ext_calibration.motion_plus.slow.pitch_scale : m_ext_calibration.motion_plus.fast.pitch_scale);
            uint16_t roll_scale  = (extension_data->roll_slow_mode  ? m_ext_calibration.motion_plus.slow.roll_scale  : m_ext_calibration.motion_plus.fast.roll_scale);
            uint16_t yaw_scale   = (extension_data->yaw_slow_mode   ? m_ext_calibration.motion_plus.slow.yaw_scale   : m_ext_calibration.motion_plus.fast.yaw_scale);

            uint16_t scale_deg_pitch = 6 * (extension_data->pitch_slow_mode ? m_ext_calibration.motion_plus.slow.degrees_div_6 : m_ext_calibration.motion_plus.fast.degrees_div_6);
            uint16_t scale_deg_roll  = 6 * (extension_data->roll_slow_mode  ? m_ext_calibration.motion_plus.slow.degrees_div_6 : m_ext_calibration.motion_plus.fast.degrees_div_6);
            uint16_t scale_deg_yaw   = 6 * (extension_data->yaw_slow_mode   ? m_ext_calibration.motion_plus.slow.degrees_div_6 : m_ext_calibration.motion_plus.fast.degrees_div_6);

            int16_t pitch = static_cast<int16_t>(gyro_scale_factor * (float(pitch_raw - pitch_0deg) / (float(pitch_scale - pitch_0deg) / scale_deg_pitch)));
            int16_t roll = -static_cast<int16_t>(gyro_scale_factor * (float(roll_raw  - roll_0deg)  / (float(roll_scale  - roll_0deg)  / scale_deg_roll)));
            int16_t yaw =  -static_cast<int16_t>(gyro_scale_factor * (float(yaw_raw   - yaw_0deg)   / (float(yaw_scale   - yaw_0deg)   / scale_deg_yaw)));

            if (m_orientation == WiiControllerOrientation_Horizontal) {
                m_motion_data[0].gyro_1 = pitch;
                m_motion_data[0].gyro_2 = roll;
                m_motion_data[0].gyro_3 = yaw;

                m_motion_data[1].gyro_1 = pitch;
                m_motion_data[1].gyro_2 = roll;
                m_motion_data[1].gyro_3 = yaw;

                m_motion_data[2].gyro_1 = pitch;
                m_motion_data[2].gyro_2 = roll;
                m_motion_data[2].gyro_3 = yaw;
            } else {
                m_motion_data[0].gyro_1 =  roll;
                m_motion_data[0].gyro_2 = -pitch;
                m_motion_data[0].gyro_3 =  yaw;

                m_motion_data[1].gyro_1 =  roll;
                m_motion_data[1].gyro_2 = -pitch;
                m_motion_data[1].gyro_3 =  yaw;

                m_motion_data[2].gyro_1 =  roll;
                m_motion_data[2].gyro_2 = -pitch;
                m_motion_data[2].gyro_3 =  yaw;
            }
        } else {
            if (m_extension == WiiExtensionController_MotionPlusNunchuckPassthrough) {
                this->MapNunchuckExtensionPassthroughMode(ext);
            } else if (m_extension == WiiExtensionController_MotionPlusClassicControllerPassthrough) {
                this->MapClassicControllerExtensionPassthroughMode(ext);
            }
        }
    }

    void WiiController::MapNunchuckExtensionPassthroughMode(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const WiiNunchuckPassthroughExtensionData *>(ext);

        m_left_stick.SetData(
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension_data->stick_x - 0x80) + STICK_ZERO), 0, 0xfff),
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension_data->stick_y - 0x80) + STICK_ZERO), 0, 0xfff)
        );

        m_buttons.L  = !extension_data->C;
        m_buttons.ZL = !extension_data->Z;
    }

    void WiiController::MapClassicControllerExtensionPassthroughMode(const uint8_t ext[]) {
        auto extension_data = reinterpret_cast<const WiiClassicControllerPassthroughExtensionData *>(ext);

        m_left_stick.SetData(
            static_cast<uint16_t>(left_stick_scale_factor * ((extension_data->left_stick_x_51 << 1) - 0x20) + STICK_ZERO) & 0xfff,
            static_cast<uint16_t>(left_stick_scale_factor * ((extension_data->left_stick_y_51 << 1) - 0x20) + STICK_ZERO) & 0xfff
        );
        m_right_stick.SetData(
            static_cast<uint16_t>(right_stick_scale_factor * (((extension_data->right_stick_x_43 << 3) | (extension_data->right_stick_x_21 << 1) | extension_data->right_stick_x_0) - 0x10) + STICK_ZERO) & 0xfff,
            static_cast<uint16_t>(right_stick_scale_factor * (extension_data->right_stick_y - 0x10) + STICK_ZERO) & 0xfff
        );

        m_buttons.dpad_down  |= !extension_data->buttons.dpad_down;
        m_buttons.dpad_up    |= !extension_data->buttons.dpad_up;
        m_buttons.dpad_right |= !extension_data->buttons.dpad_right;
        m_buttons.dpad_left  |= !extension_data->buttons.dpad_left;

        m_buttons.A |= !extension_data->buttons.A;
        m_buttons.B |= !extension_data->buttons.B;
        m_buttons.X  = !extension_data->buttons.X;
        m_buttons.Y  = !extension_data->buttons.Y;

        m_buttons.L   = !extension_data->buttons.L | (((extension_data->left_trigger_43 << 3) | (extension_data->left_trigger_20)) > 0x0f);
        m_buttons.ZL  = !extension_data->buttons.ZL;
        m_buttons.R  |= !extension_data->buttons.R | (extension_data->right_trigger > 0x0f);
        m_buttons.ZR |= !extension_data->buttons.ZR;

        m_buttons.minus |= !extension_data->buttons.minus;
        m_buttons.plus  |= !extension_data->buttons.plus;

        m_buttons.home |= !extension_data->buttons.home;
    }

    void WiiController::HandleStatusReport(const WiiReportData *wii_report) {
        if (wii_report->input0x20.extension_connected) {

            MC_RUN_ASYNC (
                auto mp_status = MotionPlusStatus_None;
                if ((m_id.pid == 0x0306) && m_enable_motion) {
                    mp_status = this->GetMotionPlusStatus();

                    if (!((mp_status == MotionPlusStatus_None) || (mp_status == MotionPlusStatus_Active))) {
                        if (mp_status == MotionPlusStatus_Uninitialised) {
                            R_TRY(this->InitializeMotionPlus());
                        }

                        R_TRY(this->ActivateMotionPlus());

                        return ams::ResultSuccess();
                    }
                }

                auto extension = this->GetExtensionControllerType();
                if ((extension == WiiExtensionController_None)) {
                    R_TRY(this->InitializeStandardExtension());
                    extension = this->GetExtensionControllerType();
                }
                // Connected extension has changed
                if (m_extension != extension) {
                    switch (extension) {
                        case WiiExtensionController_Nunchuck:
                        case WiiExtensionController_ClassicPro:
                        case WiiExtensionController_TaTaCon:
                            m_orientation = WiiControllerOrientation_Vertical;
                            R_TRY(this->SetReportMode(0x35));
                            break;
                        case WiiExtensionController_BalanceBoard:
                            R_TRY(this->GetBalanceBoardCalibration(&m_ext_calibration.balance_board));
                            m_orientation = WiiControllerOrientation_Vertical;
                            R_TRY(this->SetReportMode(0x34));
                            break;
                        case WiiExtensionController_WiiUPro:
                            m_orientation = WiiControllerOrientation_Horizontal;
                            R_TRY(this->SetReportMode(0x34));
                            break;
                        case WiiExtensionController_MotionPlus:
                            m_orientation = WiiControllerOrientation_Horizontal;
                            R_TRY(this->SetReportMode(0x35));
                            break;
                        case WiiExtensionController_MotionPlusNunchuckPassthrough:
                        case WiiExtensionController_MotionPlusClassicControllerPassthrough:
                            m_orientation = WiiControllerOrientation_Vertical;
                            R_TRY(this->SetReportMode(0x35));
                            break;
                        default:
                            m_orientation = WiiControllerOrientation_Horizontal;
                            R_TRY(this->SetReportMode(0x31));
                            break;
                    }

                    m_extension = extension;
                }

                // Check if passthrough mode needs updating
                if (m_mp_state_changing) {
                    if (m_mp_extension_flag) {
                        if (m_extension == WiiExtensionController_MotionPlus) {
                            // Identify extension type
                            R_TRY(this->InitializeStandardExtension());  // This also deactivates MotionPlus if activated
                            extension = this->GetExtensionControllerType();

                            // Set appropriate passthrough MotionPlus mode
                            if (extension == WiiExtensionController_Nunchuck) {
                                R_TRY(this->ActivateMotionPlusNunchuckPassthrough());
                            } else {
                                R_TRY(this->ActivateMotionPlusClassicPassthrough());
                            }

                            // A status report (0x20) will automatically be sent indicating that a normal extension has been plugged in,
                            // if and only if there was no extension plugged into the MotionPlus pass-through extension port.
                            R_TRY(this->QueryStatus());
                        }
                    } else {
                        if ((m_extension == WiiExtensionController_MotionPlusNunchuckPassthrough) || (m_extension == WiiExtensionController_MotionPlusClassicControllerPassthrough)) {
                            R_TRY(this->DeactivateMotionPlus());

                            m_extension = WiiExtensionController_None;
                            m_orientation = WiiControllerOrientation_Horizontal;
                            R_TRY(this->SetReportMode(0x31));
                        }
                    }

                    m_mp_state_changing = false;
                }

                return ams::ResultSuccess();
            );

        } else {

            MC_RUN_ASYNC (
                auto mp_status = this->GetMotionPlusStatus();

                if ((mp_status == MotionPlusStatus_None) || !m_enable_motion) {
                    m_extension = WiiExtensionController_None;
                    m_orientation = WiiControllerOrientation_Horizontal;
                    R_TRY(this->SetReportMode(0x31));
                } else {
                    if (mp_status == MotionPlusStatus_Uninitialised) {
                        R_TRY(this->InitializeMotionPlus());
                    }

                    R_TRY(this->ActivateMotionPlus());
                }

                return ams::ResultSuccess();
            );
        }

    }

    WiiExtensionController WiiController::GetExtensionControllerType() {
        uint32_t extension_id = 0;
        if (R_SUCCEEDED(this->ReadMemory(0x04a400fc, 4, &extension_id))) {
            extension_id = util::SwapEndian(extension_id);

            switch (extension_id) {
                case 0xffffffff:
                    return WiiExtensionController_None;
                case 0xA4200000:
                    return WiiExtensionController_Nunchuck;
                case 0xA4200101:
                    return WiiExtensionController_ClassicPro;
                case 0xa4200120:
                    return WiiExtensionController_WiiUPro;
                case 0xA4200405:
                    return WiiExtensionController_MotionPlus;
                case 0xA4200505:
                    return WiiExtensionController_MotionPlusNunchuckPassthrough;
                case 0xA4200705:
                    return WiiExtensionController_MotionPlusClassicControllerPassthrough;
                case 0xa4200111:
                    return WiiExtensionController_TaTaCon;
                case 0xA4200402:
                    return WiiExtensionController_BalanceBoard;
                default:
                    return WiiExtensionController_Unrecognised;
            }
        }

        return WiiExtensionController_None;
    }

    Result WiiController::GetAccelerometerCalibration(WiiAccelerometerCalibrationData *calibration) {
        struct {
            uint8_t acc_x_0g_92;
            uint8_t acc_y_0g_92;
            uint8_t acc_z_0g_92;

            uint8_t acc_z_0g_10 : 2;
            uint8_t acc_y_0g_10 : 2;
            uint8_t acc_x_0g_10 : 2;
            uint8_t             : 0;

            uint8_t acc_x_1g_92;
            uint8_t acc_y_1g_92;
            uint8_t acc_z_1g_92;

            uint8_t acc_z_1g_10 : 2;
            uint8_t acc_y_1g_10 : 2;
            uint8_t acc_x_1g_10 : 2;
            uint8_t             : 0;

            uint8_t unused;

            uint8_t checksum;
        } calibration_raw;

        R_TRY(this->ReadMemory(0x0016, sizeof(calibration_raw), &calibration_raw));

        calibration->acc_x_0g = (calibration_raw.acc_x_0g_92 << 2) | calibration_raw.acc_x_0g_10;
        calibration->acc_y_0g = (calibration_raw.acc_y_0g_92 << 2) | calibration_raw.acc_y_0g_10;
        calibration->acc_z_0g = (calibration_raw.acc_z_0g_92 << 2) | calibration_raw.acc_z_0g_10;
        calibration->acc_x_1g = (calibration_raw.acc_x_1g_92 << 2) | calibration_raw.acc_x_1g_10;
        calibration->acc_y_1g = (calibration_raw.acc_y_1g_92 << 2) | calibration_raw.acc_y_1g_10;
        calibration->acc_z_1g = (calibration_raw.acc_z_1g_92 << 2) | calibration_raw.acc_z_1g_10;

        return ams::ResultSuccess();
    }

    Result WiiController::GetMotionPlusCalibration(MotionPlusCalibrationData *calibration) {
        struct {
            union {
                uint8_t raw[0x20];

                struct {
                    struct {
                        MotionPlusCalibration calib;
                        uint8_t uid;
                    } fast ;
                    uint16_t crc32_msb;

                    struct {
                        MotionPlusCalibration calib;
                        uint8_t uid;
                    } slow;
                    uint16_t crc32_lsb;
                };
            };
        } calibration_raw;

        R_TRY(this->ReadMemory(0x04a60020, 16, &calibration_raw.raw));
        R_TRY(this->ReadMemory(0x04a60030, 16, &calibration_raw.raw[0x10]));

        calibration->fast.yaw_zero    = util::SwapEndian(calibration_raw.fast.calib.yaw_zero);
        calibration->fast.roll_zero   = util::SwapEndian(calibration_raw.fast.calib.roll_zero);
        calibration->fast.pitch_zero  = util::SwapEndian(calibration_raw.fast.calib.pitch_zero);
        calibration->fast.yaw_scale   = util::SwapEndian(calibration_raw.fast.calib.yaw_scale);
        calibration->fast.roll_scale  = util::SwapEndian(calibration_raw.fast.calib.roll_scale);
        calibration->fast.pitch_scale = util::SwapEndian(calibration_raw.fast.calib.pitch_scale);
        calibration->fast.degrees_div_6 = calibration_raw.fast.calib.degrees_div_6;

        calibration->slow.yaw_zero    = util::SwapEndian(calibration_raw.slow.calib.yaw_zero);
        calibration->slow.roll_zero   = util::SwapEndian(calibration_raw.slow.calib.roll_zero);
        calibration->slow.pitch_zero  = util::SwapEndian(calibration_raw.slow.calib.pitch_zero);
        calibration->slow.yaw_scale   = util::SwapEndian(calibration_raw.slow.calib.yaw_scale);
        calibration->slow.roll_scale  = util::SwapEndian(calibration_raw.slow.calib.roll_scale);
        calibration->slow.pitch_scale = util::SwapEndian(calibration_raw.slow.calib.pitch_scale);
        calibration->slow.degrees_div_6 = calibration_raw.slow.calib.degrees_div_6;

        return ams::ResultSuccess();
    }

    Result WiiController::GetBalanceBoardCalibration(BalanceBoardCalibrationData *calibration) {
        struct {
            union {
                uint8_t raw[0x20];

                struct {
                    uint8_t _unk;
                    uint8_t battery_reference;
                    uint8_t pad[2];

                    BalanceBoardCalibrationData calib;
                };
            };
        } calibration_raw;

        R_TRY(this->ReadMemory(0x04a40020, 16, &calibration_raw.raw));
        R_TRY(this->ReadMemory(0x04a40030, 16, &calibration_raw.raw[0x10]));

        calibration->top_right_0kg     = util::SwapEndian(calibration_raw.calib.top_right_0kg);
        calibration->bottom_right_0kg  = util::SwapEndian(calibration_raw.calib.bottom_right_0kg);
        calibration->top_left_0kg      = util::SwapEndian(calibration_raw.calib.top_left_0kg);
        calibration->bottom_left_0kg   = util::SwapEndian(calibration_raw.calib.bottom_left_0kg);
        calibration->top_right_17kg    = util::SwapEndian(calibration_raw.calib.top_right_17kg);
        calibration->bottom_right_17kg = util::SwapEndian(calibration_raw.calib.bottom_right_17kg);
        calibration->top_left_17kg     = util::SwapEndian(calibration_raw.calib.top_left_17kg);
        calibration->bottom_left_17kg  = util::SwapEndian(calibration_raw.calib.bottom_left_17kg);
        calibration->top_right_34kg    = util::SwapEndian(calibration_raw.calib.top_right_34kg);
        calibration->bottom_right_34kg = util::SwapEndian(calibration_raw.calib.bottom_right_34kg);
        calibration->top_left_34kg     = util::SwapEndian(calibration_raw.calib.top_left_34kg);
        calibration->bottom_left_34kg  = util::SwapEndian(calibration_raw.calib.bottom_left_34kg);

        return ams::ResultSuccess();
    }

    Result WiiController::SetReportMode(uint8_t mode) {
        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(WiiOutputReport0x12) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
        report_data->id = 0x12;
        report_data->output0x12.rumble = m_rumble_state;
        report_data->output0x12.report_mode = mode;

        R_TRY(this->WriteDataReport(&m_output_report));
        return ams::ResultSuccess();
    }

    Result WiiController::QueryStatus() {
        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(WiiOutputReport0x15) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
        report_data->id = 0x15;
        report_data->output0x15.rumble = m_rumble_state;

        R_TRY(this->WriteDataReport(&m_output_report));
        return ams::ResultSuccess();
    }

    Result WiiController::WriteMemory(uint32_t write_addr, const void *data, uint8_t size) {       
        os::SleepThread(ams::TimeSpan::FromMilliSeconds(30));

        Result result;
        auto output = std::make_unique<bluetooth::HidReport>();

        std::scoped_lock lk(m_output_mutex);

        int attempts = 0;
        do {
            m_output_report.size = sizeof(WiiOutputReport0x16) + 1;
            auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
            report_data->id = 0x16;
        	report_data->output0x16.address = ams::util::SwapEndian(write_addr);
            report_data->output0x16.size = size;
            std::memcpy(&report_data->output0x16.data, data, size);

            R_TRY(this->WriteDataReport(&m_output_report, 0x22, output.get()));
            report_data = reinterpret_cast<WiiReportData *>(&output->data);
            result = report_data->input0x22.error;
        } while (!(R_SUCCEEDED(result) || (++attempts >= 2)));

        return result;
    }

    Result WiiController::ReadMemory(uint32_t read_addr, uint16_t size, void *out_data) {
        os::SleepThread(ams::TimeSpan::FromMilliSeconds(30));

        Result result;
        auto output = std::make_unique<bluetooth::HidReport>();

        std::scoped_lock lk(m_output_mutex);

        int attempts = 0;
        do {
            m_output_report.size = sizeof(WiiOutputReport0x17) + 1;
            auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
            report_data->id = 0x17;
        	report_data->output0x17.address = ams::util::SwapEndian(read_addr);
        	report_data->output0x17.size = ams::util::SwapEndian(size);

            R_TRY(this->WriteDataReport(&m_output_report, 0x21, output.get()));
            report_data = reinterpret_cast<WiiReportData *>(&output->data);
            result = report_data->input0x21.error;

            if (R_SUCCEEDED(result)) {
                std::memcpy(out_data, report_data->input0x21.data, report_data->input0x21.size + 1);
            }
        } while (!(R_SUCCEEDED(result) || (++attempts >= 2)));

        return result;
    }

    Result WiiController::InitializeStandardExtension() {
        R_TRY(this->WriteMemory(0x04a400f0, init_data1, sizeof(init_data1)));
        R_TRY(this->WriteMemory(0x04a400fb, init_data2, sizeof(init_data2)));

        return ams::ResultSuccess();
    }

    Result WiiController::InitializeMotionPlus() {
        R_TRY(this->WriteMemory(0x04a600f0, init_data1, sizeof(init_data1)));

        // Get the MotionPlus calibration
        R_TRY(this->GetMotionPlusCalibration(&m_ext_calibration.motion_plus));

        return ams::ResultSuccess();
    }

    MotionPlusStatus WiiController::GetMotionPlusStatus() {
        uint16_t extension_id;

        // Check for inactive motion plus addon
        if (R_SUCCEEDED(this->ReadMemory(0x04a600fe, 2, &extension_id))) {
            extension_id = util::SwapEndian(extension_id);

            switch (extension_id) {
                case 0x0005:
                    return MotionPlusStatus_Uninitialised;
                case 0x0405:
                case 0x0505:
                case 0x0705:
                    return MotionPlusStatus_Inactive;
                default:
                    break;
            }
        }

        // Check for active motion plus addon
        if (R_SUCCEEDED(this->ReadMemory(0x04a400fe, 2, &extension_id))) {
            extension_id = util::SwapEndian(extension_id);

            switch (extension_id) {
                case 0x0405:
                case 0x0505:
                case 0x0705:
                    return MotionPlusStatus_Active;
                default:
                    break;
            }
        }

        return MotionPlusStatus_None;
    }

    Result WiiController::ActivateMotionPlus() {
        uint8_t data[] = {0x04};
        R_TRY(this->WriteMemory(0x04a600fe, data, sizeof(data)));

        return ams::ResultSuccess();
    }

    Result WiiController::ActivateMotionPlusNunchuckPassthrough() {
        uint8_t data[] = {0x05};
        R_TRY(this->WriteMemory(0x04a600fe, data, sizeof(data)));

        return ams::ResultSuccess();
    }

    Result WiiController::ActivateMotionPlusClassicPassthrough() {
        uint8_t data[] = {0x07};
        R_TRY(this->WriteMemory(0x04a600fe, data, sizeof(data)));

        return ams::ResultSuccess();
    }

    Result WiiController::DeactivateMotionPlus() {
        R_TRY(this->WriteMemory(0x04a400f0, init_data1, sizeof(init_data1)));

        return ams::ResultSuccess();
    }

    Result WiiController::UpdateMotionPlusExtensionStatus(bool extension_connected) {
        if (!m_mp_state_changing) {
            if (extension_connected != m_mp_extension_flag) {
                m_mp_extension_flag = extension_connected;
                m_mp_state_changing = true;

                MC_RUN_ASYNC (
                    os::SleepThread(ams::TimeSpan::FromMilliSeconds(250));
                    R_TRY(this->QueryStatus());

                    return ams::ResultSuccess();
                );
            }
        }

        return ams::ResultSuccess();
    }

    Result WiiController::SetVibration(const SwitchRumbleData *rumble_data) {
        m_rumble_state = rumble_data[0].low_band_amp > 0 ||
                         rumble_data[0].high_band_amp > 0 ||
                         rumble_data[1].low_band_amp > 0 ||
                         rumble_data[1].high_band_amp > 0;

        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(WiiOutputReport0x10) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
        report_data->id = 0x10;
        report_data->output0x10.rumble = m_rumble_state;

        return this->WriteDataReport(&m_output_report);
    }

    Result WiiController::CancelVibration() {
        m_rumble_state = 0;

        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(WiiOutputReport0x10) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
        report_data->id = 0x10;
        report_data->output0x10.rumble = m_rumble_state;

        return this->WriteDataReport(&m_output_report);
    }

    Result WiiController::SetPlayerLed(uint8_t led_mask) {
        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(WiiOutputReport0x11) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(m_output_report.data);
        report_data->id = 0x11;
        report_data->output0x11.rumble = m_rumble_state;
        report_data->output0x11.leds = led_mask & 0xf;

        return this->WriteDataReport(&m_output_report);
    }

}
