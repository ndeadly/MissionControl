/*
 * Copyright (c) 2020 ndeadly
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

    void WiiController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto wii_report = reinterpret_cast<const WiiReportData *>(&report->data);

        switch(wii_report->id) {
            case 0x20:  // status
                this->HandleInputReport0x20(wii_report);
                break;
            case 0x21:  // memory read
                this->HandleInputReport0x21(wii_report);
                break;
            case 0x22:  // ack
                this->HandleInputReport0x22(wii_report);
                break;
            case 0x30:
                this->HandleInputReport0x30(wii_report);
                break;
            case 0x31:
                this->HandleInputReport0x31(wii_report);
                break;
            case 0x32:
                this->HandleInputReport0x32(wii_report);
                break;
            case 0x34:
                this->HandleInputReport0x34(wii_report);
                break;
            default:
                break;
        }
    }

    void WiiController::HandleInputReport0x20(const WiiReportData *src) {
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

    void WiiController::HandleInputReport0x21(const WiiReportData *src) {
        uint16_t read_addr = util::SwapBytes(src->input0x21.address);

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

        this->ClearControllerState();
    }

    void WiiController::HandleInputReport0x22(const WiiReportData *src) {
        ;
    }

    void WiiController::HandleInputReport0x30(const WiiReportData *src) {
        this->MapButtonsHorizontalOrientation(&src->input0x30.buttons);
    }

    void WiiController::HandleInputReport0x31(const WiiReportData *src) {
        this->MapButtonsHorizontalOrientation(&src->input0x31.buttons);

        // Todo: Accelerometer data
    }

    void WiiController::HandleInputReport0x32(const WiiReportData *src) {
        if ((m_extension == WiiExtensionController_Nunchuck) 
         || (m_extension == WiiExtensionController_Classic)
         || (m_extension == WiiExtensionController_ClassicPro)
         || (m_extension == WiiExtensionController_TaTaCon)) {
            this->MapButtonsVerticalOrientation(&src->input0x32.buttons);
        }

        this->MapExtensionBytes(src->input0x32.extension);
    }

    void WiiController::HandleInputReport0x34(const WiiReportData *src) {
        if ((m_extension == WiiExtensionController_Nunchuck) 
         || (m_extension == WiiExtensionController_Classic)
         || (m_extension == WiiExtensionController_ClassicPro)) {
            this->MapButtonsVerticalOrientation(&src->input0x34.buttons);
        }

        this->MapExtensionBytes(src->input0x34.extension);
    }

    void WiiController::MapButtonsHorizontalOrientation(const WiiButtonData *buttons) {
        m_buttons.dpad_down   = buttons->dpad_left;
        m_buttons.dpad_up     = buttons->dpad_right;
        m_buttons.dpad_right  = buttons->dpad_down;
        m_buttons.dpad_left   = buttons->dpad_up;

        m_buttons.A = buttons->two;
        m_buttons.B = buttons->one;

        m_buttons.R = buttons->A;
        m_buttons.L = buttons->B;

        m_buttons.minus   = buttons->minus;
        m_buttons.plus    = buttons->plus;
        
        m_buttons.home    = buttons->home;
    }

    void WiiController::MapButtonsVerticalOrientation(const WiiButtonData *buttons) {
        m_buttons.dpad_down   = buttons->dpad_down;
        m_buttons.dpad_up     = buttons->dpad_up;
        m_buttons.dpad_right  = buttons->dpad_right;
        m_buttons.dpad_left   = buttons->dpad_left;

        m_buttons.A = buttons->A;
        m_buttons.B = buttons->B;

        // Not the best mapping but at least most buttons are mapped to something when nunchuck is connected.
        m_buttons.R  = buttons->one;
        m_buttons.ZR = buttons->two;

        m_buttons.minus   = buttons->minus;
        m_buttons.plus    = buttons->plus;
        
        m_buttons.home    = buttons->home;
    }

    void WiiController::MapExtensionBytes(const uint8_t ext[]) {
        switch(m_extension) {
            case WiiExtensionController_Nunchuck:
                this->MapNunchuckExtension(ext);
                break;
            case WiiExtensionController_Classic:
            case WiiExtensionController_ClassicPro:
                this->MapClassicControllerExtension(ext);
                break;
            case WiiExtensionController_WiiUPro:
                this->MapWiiUProControllerExtension(ext);
                break;
            case WiiExtensionController_TaTaCon:
                this->MapTaTaConExtension(ext);
                break;
            default:
                break;
        }
    }

    void WiiController::MapNunchuckExtension(const uint8_t ext[]) {
        auto extension = reinterpret_cast<const WiiNunchuckExtensionData *>(ext);

        this->PackStickData(&m_left_stick, 
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension->stick_x - 0x80) + STICK_ZERO), 0, 0xfff), 
            std::clamp<uint16_t>(static_cast<uint16_t>(nunchuck_stick_scale_factor * (extension->stick_y - 0x80) + STICK_ZERO), 0, 0xfff)
        );

        m_buttons.L  = !extension->C;
        m_buttons.ZL = !extension->Z;
    }

    void WiiController::MapClassicControllerExtension(const uint8_t ext[]) {
        this->PackStickData(&m_left_stick, 
            static_cast<uint16_t>(left_stick_scale_factor * ((ext[0] & 0x3f) - 0x20) + STICK_ZERO) & 0xfff, 
            static_cast<uint16_t>(left_stick_scale_factor * ((ext[1] & 0x3f) - 0x20) + STICK_ZERO) & 0xfff
        );
        this->PackStickData(&m_right_stick, 
            static_cast<uint16_t>(right_stick_scale_factor * ((((ext[0] >> 3) & 0x18) | ((ext[1] >> 5) & 0x06) | ((ext[2] >> 7) & 0x01)) - 0x10) + STICK_ZERO) & 0xfff, 
            static_cast<uint16_t>(right_stick_scale_factor * ((ext[2] & 0x1f) - 0x10) + STICK_ZERO) & 0xfff
        );

        auto buttons = reinterpret_cast<const WiiClassicControllerButtonData *>(&ext[4]);

        m_buttons.dpad_down   = !buttons->dpad_down;
        m_buttons.dpad_up     = !buttons->dpad_up;
        m_buttons.dpad_right  = !buttons->dpad_right;
        m_buttons.dpad_left   = !buttons->dpad_left;

        m_buttons.A = !buttons->A;
        m_buttons.B = !buttons->B;
        m_buttons.X = !buttons->X;
        m_buttons.Y = !buttons->Y;

        m_buttons.L  = !buttons->L;
        m_buttons.ZL = !buttons->ZL;
        m_buttons.R  = !buttons->R;
        m_buttons.ZR = !buttons->ZR;

        m_buttons.minus   = !buttons->minus;
        m_buttons.plus    = !buttons->plus;
        
        m_buttons.home    = !buttons->home;
    }

    void WiiController::MapWiiUProControllerExtension(const uint8_t ext[]) {
        auto extension = reinterpret_cast<const WiiUProExtensionData *>(ext);

        this->PackStickData(&m_left_stick,
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->left_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff), 
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->left_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );
        this->PackStickData(&m_right_stick,
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->right_stick_x - STICK_ZERO))) + STICK_ZERO, 0, 0xfff),
            std::clamp<uint16_t>(((wiiu_scale_factor * (extension->right_stick_y - STICK_ZERO))) + STICK_ZERO, 0, 0xfff)
        );

        m_buttons.dpad_down   = !extension->buttons.dpad_down;
        m_buttons.dpad_up     = !extension->buttons.dpad_up;
        m_buttons.dpad_right  = !extension->buttons.dpad_right;
        m_buttons.dpad_left   = !extension->buttons.dpad_left;

        m_buttons.A = !extension->buttons.A;
        m_buttons.B = !extension->buttons.B;
        m_buttons.X = !extension->buttons.X;
        m_buttons.Y = !extension->buttons.Y;

        m_buttons.R  = !extension->buttons.R;
        m_buttons.ZR = !extension->buttons.ZR;
        m_buttons.L  = !extension->buttons.L;
        m_buttons.ZL = !extension->buttons.ZL;

        m_buttons.minus = !extension->buttons.minus;
        m_buttons.plus  = !extension->buttons.plus;

        m_buttons.lstick_press = !extension->buttons.lstick_press;
        m_buttons.rstick_press = !extension->buttons.rstick_press;

        m_buttons.home = !extension->buttons.home;
    }

    void WiiController::MapTaTaConExtension(const uint8_t ext[]) {
        auto extension = reinterpret_cast<const TaTaConExtensionData *>(ext);

        m_buttons.X            = !extension->R_rim;
        m_buttons.Y            = !extension->R_center;
        m_buttons.dpad_up      = !extension->L_rim;
        m_buttons.dpad_right   = !extension->L_center;
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
