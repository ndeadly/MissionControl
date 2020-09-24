/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "wii_controller.hpp"
#include <switch.h>
#include <stratosphere.hpp>
#include <algorithm>
#include <cstring>

namespace ams::controller {

    namespace {

        constexpr uint8_t init_data1[] = {0x55};
        constexpr uint8_t init_data2[] = {0x00};

        constexpr float nunchuck_stick_scale_factor  = float(UINT12_MAX) / 0xb8;
        constexpr float wiiu_scale_factor            = 1.5;
        constexpr float left_stick_scale_factor      = float(UINT12_MAX) / 0x3f;
        constexpr float right_stick_scale_factor     = float(UINT12_MAX) / 0x1f;

    }

    Result WiiController::Initialize(void) {
        R_TRY(this->SetReportMode(0x31));

        return this->QueryStatus();
    }

    void WiiController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto wii_report = reinterpret_cast<const WiiReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(wii_report->id) {
            case 0x20:  // status
                this->HandleInputReport0x20(wii_report, switch_report);
                break;
            case 0x21:  // memory read
                this->HandleInputReport0x21(wii_report, switch_report);
                break;
            case 0x22:  // ack
                this->HandleInputReport0x22(wii_report, switch_report);
                break;
            case 0x30:
                this->HandleInputReport0x30(wii_report, switch_report);
                break;
            case 0x31:
                this->HandleInputReport0x31(wii_report, switch_report);
                break;
            case 0x32:
                this->HandleInputReport0x32(wii_report, switch_report);
                break;
            case 0x34:
                this->HandleInputReport0x34(wii_report, switch_report);
                break;
            default:
                break;
        }

        out_report->size = sizeof(SwitchInputReport0x30) + 1;
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = m_battery | m_charging;
        std::memset(switch_report->input0x30.motion, 0, sizeof(switch_report->input0x30.motion));
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void WiiController::HandleInputReport0x20(const WiiReportData *src, SwitchReportData *dst) {
        if (!src->input0x20.extension_connected) {
            m_extension = WiiExtensionController_None;
            this->SetReportMode(0x31);
        }
        else if (src->input0x20.extension_connected && (m_extension == WiiExtensionController_None)) {
            // Initialise extension
            this->WriteMemory(0x04a400f0, init_data1, sizeof(init_data1));
            this->WriteMemory(0x04a400fb, init_data2, sizeof(init_data2));

            // Read extension type
            this->ReadMemory(0x04a400fa, 6);
        }

        m_battery = (src->input0x20.battery / 52) << 1;
    }

    void WiiController::HandleInputReport0x21(const WiiReportData *src, SwitchReportData *dst) {
        uint16_t read_addr = util::SwapBytes(src->input0x21.address);
        //uint8_t size = src->input0x21.size + 1;

        if (read_addr == 0x00fa) {
            // Identify extension controller by ID
            uint64_t extension_id = (util::SwapBytes(*reinterpret_cast<const uint64_t *>(&src->input0x21.data)) >> 16);
            
            switch (extension_id) {
                case 0x0000A4200000ULL:
                case 0xFF00A4200000ULL:
                    m_extension = WiiExtensionController_Nunchuck;
                    this->SetReportMode(0x32);
                    break;
                case 0x0000A4200101ULL:
                    m_extension = WiiExtensionController_Classic;
                    this->SetReportMode(0x32);
                    break;
                case 0x0100A4200101ULL:
                    m_extension = WiiExtensionController_ClassicPro;
                    this->SetReportMode(0x32);
                    break;
                case 0x0000a4200120ULL:
                    m_extension = WiiExtensionController_WiiUPro;
                    this->SetReportMode(0x34);
                    break;
                case 0x0000a4200111ULL:
                    m_extension = WiiExtensionController_TaTaCon;
                    this->SetReportMode(0x32);
                    break;
                default:
                    m_extension = WiiExtensionController_Unsupported;
                    this->SetReportMode(0x31);
                    break;
            }
        }
    }

    void WiiController::HandleInputReport0x22(const WiiReportData *src, SwitchReportData *dst) {
        ;
    }

    void WiiController::HandleInputReport0x30(const WiiReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        this->PackStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        this->MapButtonsHorizontalOrientation(&src->input0x30.buttons, dst);
    }

    void WiiController::HandleInputReport0x31(const WiiReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        this->PackStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        this->MapButtonsHorizontalOrientation(&src->input0x31.buttons, dst);

        // Todo: Accelerometer data
    }

    void WiiController::HandleInputReport0x32(const WiiReportData *src, SwitchReportData *dst) {
        if ((m_extension == WiiExtensionController_Nunchuck) 
         || (m_extension == WiiExtensionController_Classic)
         || (m_extension == WiiExtensionController_ClassicPro)
         || (m_extension == WiiExtensionController_TaTaCon)) {
            this->MapButtonsVerticalOrientation(&src->input0x32.buttons, dst);
        }

        this->MapExtensionBytes(src->input0x32.extension, dst);
    }

    void WiiController::HandleInputReport0x34(const WiiReportData *src, SwitchReportData *dst) {
        if ((m_extension == WiiExtensionController_Nunchuck) 
         || (m_extension == WiiExtensionController_Classic)
         || (m_extension == WiiExtensionController_ClassicPro)) {
            this->MapButtonsVerticalOrientation(&src->input0x34.buttons, dst);
        }

        this->MapExtensionBytes(src->input0x34.extension, dst);
    }

    void WiiController::MapButtonsHorizontalOrientation(const WiiButtonData *buttons, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = buttons->dpad_left;
        dst->input0x30.buttons.dpad_up     = buttons->dpad_right;
        dst->input0x30.buttons.dpad_right  = buttons->dpad_down;
        dst->input0x30.buttons.dpad_left   = buttons->dpad_up;

        dst->input0x30.buttons.A = buttons->two;
        dst->input0x30.buttons.B = buttons->one;

        dst->input0x30.buttons.R = buttons->A;
        dst->input0x30.buttons.L = buttons->B;

        dst->input0x30.buttons.minus   = buttons->minus;
        dst->input0x30.buttons.plus    = buttons->plus;
        
        dst->input0x30.buttons.home    = buttons->home;
    }

    void WiiController::MapButtonsVerticalOrientation(const WiiButtonData *buttons, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = buttons->dpad_down;
        dst->input0x30.buttons.dpad_up     = buttons->dpad_up;
        dst->input0x30.buttons.dpad_right  = buttons->dpad_right;
        dst->input0x30.buttons.dpad_left   = buttons->dpad_left;

        dst->input0x30.buttons.A = buttons->A;
        dst->input0x30.buttons.B = buttons->B;

        // Not the best mapping but at least most buttons are mapped to something when nunchuck is connected.
        dst->input0x30.buttons.R  = buttons->one;
        dst->input0x30.buttons.ZR = buttons->two;

        dst->input0x30.buttons.minus   = buttons->minus;
        dst->input0x30.buttons.plus    = buttons->plus;
        
        dst->input0x30.buttons.home    = buttons->home;
    }

    void WiiController::MapExtensionBytes(const uint8_t ext[], SwitchReportData *dst) {
        switch(m_extension) {
            case WiiExtensionController_Nunchuck:
                this->MapNunchuckExtension(ext, dst);
                break;
            case WiiExtensionController_Classic:
            case WiiExtensionController_ClassicPro:
                this->MapClassicControllerExtension(ext, dst);
                break;
            case WiiExtensionController_WiiUPro:
                this->MapWiiUProControllerExtension(ext, dst);
                break;
            case WiiExtensionController_TaTaCon:
                this->MapTaTaConExtension(ext, dst);
                break;
            default:
                break;
        }
    }

    void WiiController::MapNunchuckExtension(const uint8_t ext[], SwitchReportData *dst) {
        auto extension = reinterpret_cast<const WiiNunchuckExtensionData *>(ext);

        this->PackStickData(&dst->input0x30.left_stick, 
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension->stick_x - 0x80) + STICK_ZERO), 0, 0xfff), 
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension->stick_y - 0x80) + STICK_ZERO), 0, 0xfff)
        );

        dst->input0x30.buttons.L  = !extension->C;
        dst->input0x30.buttons.ZL = !extension->Z;
    }

    void WiiController::MapClassicControllerExtension(const uint8_t ext[], SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick, 
            static_cast<uint16_t>(left_stick_scale_factor * ((ext[0] & 0x3f) - 0x20) + STICK_ZERO) & 0xfff, 
            static_cast<uint16_t>(left_stick_scale_factor * ((ext[1] & 0x3f) - 0x20) + STICK_ZERO) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick, 
            static_cast<uint16_t>(right_stick_scale_factor * ((((ext[0] >> 3) & 0x18) | ((ext[1] >> 5) & 0x06) | ((ext[2] >> 7) & 0x01)) - 0x10) + STICK_ZERO) & 0xfff, 
            static_cast<uint16_t>(right_stick_scale_factor * ((ext[2] & 0x1f) - 0x10) + STICK_ZERO) & 0xfff
        );

        auto buttons = reinterpret_cast<const WiiClassicControllerButtonData *>(&ext[4]);

        dst->input0x30.buttons.dpad_down   = !buttons->dpad_down;
        dst->input0x30.buttons.dpad_up     = !buttons->dpad_up;
        dst->input0x30.buttons.dpad_right  = !buttons->dpad_right;
        dst->input0x30.buttons.dpad_left   = !buttons->dpad_left;

        dst->input0x30.buttons.A = !buttons->A;
        dst->input0x30.buttons.B = !buttons->B;
        dst->input0x30.buttons.X = !buttons->X;
        dst->input0x30.buttons.Y = !buttons->Y;

        dst->input0x30.buttons.L  = !buttons->L;
        dst->input0x30.buttons.ZL = !buttons->ZL;
        dst->input0x30.buttons.R  = !buttons->R;
        dst->input0x30.buttons.ZR = !buttons->ZR;

        dst->input0x30.buttons.minus   = !buttons->minus;
        dst->input0x30.buttons.plus    = !buttons->plus;
        
        dst->input0x30.buttons.home    = !buttons->home;
    }

    void WiiController::MapWiiUProControllerExtension(const uint8_t ext[], SwitchReportData *dst) {
        auto extension = reinterpret_cast<const WiiUProExtensionData *>(ext);

        this->PackStickData(&dst->input0x30.left_stick,
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->left_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff), 
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->left_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );
        this->PackStickData(&dst->input0x30.right_stick,
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->right_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff),
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->right_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );

        dst->input0x30.buttons.dpad_down   = !extension->buttons.dpad_down;
        dst->input0x30.buttons.dpad_up     = !extension->buttons.dpad_up;
        dst->input0x30.buttons.dpad_right  = !extension->buttons.dpad_right;
        dst->input0x30.buttons.dpad_left   = !extension->buttons.dpad_left;

        dst->input0x30.buttons.A = !extension->buttons.A;
        dst->input0x30.buttons.B = !extension->buttons.B;
        dst->input0x30.buttons.X = !extension->buttons.X;
        dst->input0x30.buttons.Y = !extension->buttons.Y;

        dst->input0x30.buttons.R  = !extension->buttons.R;
        dst->input0x30.buttons.ZR = !extension->buttons.ZR;
        dst->input0x30.buttons.L  = !extension->buttons.L;
        dst->input0x30.buttons.ZL = !extension->buttons.ZL;

        dst->input0x30.buttons.minus = !extension->buttons.minus;
        dst->input0x30.buttons.plus  = !extension->buttons.plus;

        dst->input0x30.buttons.lstick_press = !extension->buttons.lstick_press;
        dst->input0x30.buttons.rstick_press = !extension->buttons.rstick_press;

        dst->input0x30.buttons.home = !extension->buttons.home;
    }

    void WiiController::MapTaTaConExtension(const uint8_t ext[], SwitchReportData *dst) {
        auto extension = reinterpret_cast<const TaTaConExtensionData *>(ext);

        dst->input0x30.buttons.X            = !extension->R_rim;
        dst->input0x30.buttons.Y            = !extension->R_center;
        dst->input0x30.buttons.dpad_up      = !extension->L_rim;
        dst->input0x30.buttons.dpad_right   = !extension->L_center;
    }

    Result WiiController::WriteMemory(uint32_t write_addr, const uint8_t *data, uint8_t size) {
        s_output_report.size = sizeof(WiiOutputReport0x16) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(s_output_report.data);
        report_data->id = 0x16;
        report_data->output0x16.address = ams::util::SwapBytes(write_addr);
        report_data->output0x16.size = size;
        std::memcpy(&report_data->output0x16.data, data, size);

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

    Result WiiController::ReadMemory(uint32_t read_addr, uint16_t size) {
        s_output_report.size = sizeof(WiiOutputReport0x17) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(s_output_report.data);
        report_data->id = 0x17;
        report_data->output0x17.address = ams::util::SwapBytes(read_addr);
        report_data->output0x17.size = ams::util::SwapBytes(size);

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

    Result WiiController::SetReportMode(uint8_t mode) {
        s_output_report.size = sizeof(WiiOutputReport0x12) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(s_output_report.data);
        report_data->id = 0x12;
        report_data->output0x12._unk = 0;
        report_data->output0x12.report_mode = mode;

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

    Result WiiController::QueryStatus(void) {
        s_output_report.size = sizeof(WiiOutputReport0x15) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(s_output_report.data);
        report_data->id = 0x15;
        report_data->output0x15._unk = 0;
        
        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

    Result WiiController::SetPlayerLed(uint8_t led_mask) {        
        s_output_report.size = sizeof(WiiOutputReport0x15) + 1;
        auto report_data = reinterpret_cast<WiiReportData *>(s_output_report.data);
        report_data->id = 0x11;
        report_data->output0x11.leds = (led_mask << 4) & 0xf0;;

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

}
