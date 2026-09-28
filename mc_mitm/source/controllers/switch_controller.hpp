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
#include "switch_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_hid_report.hpp"
#include "../async/future_response.hpp"
#include <queue>

namespace ams::controller {

    using HidResponse = FutureResponse<bluetooth::HidEventType, bluetooth::HidReportEventInfo, u8>;

    std::string GetControllerDirectory(bluetooth::Address address);

    class SwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x057e, 0x2006},   // Official Joycon(L) Controller
                {0x057e, 0x2007},   // Official Joycon(R) Controller/NES Online Controller
                {0x057e, 0x2009},   // Official Switch Pro Controller
                {0x057e, 0x2017},   // Official SNES Online Controller
                {0x057e, 0x2019},   // Official N64 Online Controller
                {0x057e, 0x201a}    // Official Genesis/Megadrive Online Controller
            };

            static constexpr SwitchButtons DefaultHomeButtonCombo    = SwitchButton::Minus | SwitchButton::Down;
            static constexpr SwitchButtons DefaultCaptureButtonCombo = SwitchButton::Minus | SwitchButton::Up;

        public:
            SwitchController(bluetooth::Address address, HardwareID id) : m_address(address), m_id(id) { }
            virtual ~SwitchController() { };

        public:
            constexpr bluetooth::Address Address() const { return m_address; }

            virtual Result Initialize() { R_SUCCEED(); };
            virtual constexpr bool IsOfficialController() const { return true; }

            virtual void HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual void HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual void HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info);

            virtual Result HandleOutputDataReport(const u8 *report_buffer, size_t size);

        protected:
            Result WriteDataReport(const void *report_buffer, size_t size);
            Result WriteDataReport(const void *report_buffer, size_t size, u8 response_id, bluetooth::HidReport *out_report);
            Result SetReport(BtdrvBluetoothHhReportType type, const void *report_buffer, size_t size);
            Result GetReport(u8 id, BtdrvBluetoothHhReportType type, bluetooth::HidReport *out_report);

            virtual void ParseInputReport(const u8 *report_buffer, size_t size);
            virtual size_t FillInputReport(u8 *report_buffer, size_t size) { AMS_UNUSED(report_buffer, size); return 0; }
            void ModifyInputReport(u8 *report_buffer, size_t size);

        private:
            void ParseHidCommandInputReport(const SwitchHidCommandInputReport *report);
            void ParseMcuUpdateInputReport(const SwitchMcuUpdateInputReport *report);
            void ParseHidBasicInputReport(const SwitchHidBasicInputReport *report);
            void ParseHidMcuInputReport(const SwitchHidMcuInputReport *report);
            void ParseHidAttachmentInputReport(const SwitchHidAttachmentInputReport *report);
            void ParseHidGenericInputReport(const SwitchHidGenericInputReport *report);

            void ModifyHidCommandInputReport(SwitchHidCommandInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);
            void ModifyMcuUpdateInputReport(SwitchMcuUpdateInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);
            void ModifyHidBasicInputReport(SwitchHidBasicInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);
            void ModifyHidMcuInputReport(SwitchHidMcuInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);
            void ModifyHidAttachmentInputReport(SwitchHidAttachmentInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);
            void ModifyHidGenericInputReport(SwitchHidGenericInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick);

        protected:
            bluetooth::Address m_address;
            HardwareID m_id;

            SwitchButtons m_buttons;
            SwitchAnalogStick m_left_stick;
            SwitchAnalogStick m_right_stick;

            std::queue<std::shared_ptr<HidResponse>> m_future_responses;
    };

}
