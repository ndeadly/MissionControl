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
#pragma once
#include "switch_controller.hpp"

namespace ams::controller {

    class ForcedProController : public SwitchController {

        public:
            ForcedProController(const bluetooth::Address *address, HardwareID id);
            virtual ~ForcedProController() {};

            Result HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info);
            Result HandleOutputDataReport(const bluetooth::HidReport *report);
        private:
            bool m_is_n64_controller;
            SwitchAnalogStickParameters m_n64_left_stick_param;
            uint8_t m_n64_calibrated_stick_zero[3];

    };

}
