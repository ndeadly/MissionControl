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

    SwitchPlayerNumber LedMaskToPlayerNumber(u8 led_mask);

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

            SwitchController(bluetooth::Address address, HardwareID id) : m_address(address), m_id(id) { }

            virtual ~SwitchController() { };

            const bluetooth::Address& Address() const { return m_address; }

            virtual bool IsOfficialController() { return true; }

            virtual Result Initialize();

            virtual Result HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleOutputDataReport(const bluetooth::HidReport *report);

        protected:
            Result WriteDataReport(const bluetooth::HidReport *report);
            Result WriteDataReport(const bluetooth::HidReport *report, u8 response_id, bluetooth::HidReport *out_report);
            Result SetReport(BtdrvBluetoothHhReportType type, const bluetooth::HidReport *report);
            Result GetReport(u8 id, BtdrvBluetoothHhReportType type, bluetooth::HidReport *out_report);

            virtual void UpdateControllerState(const bluetooth::HidReport *report);
            virtual void ApplyButtonCombos(SwitchButtonData *buttons);

            bluetooth::Address m_address;
            HardwareID m_id;

            os::SdkMutex m_input_mutex;
            bluetooth::HidReport m_input_report;

            os::SdkMutex m_output_mutex;
            bluetooth::HidReport m_output_report;

            std::queue<std::shared_ptr<HidResponse>> m_future_responses;
    };

}
