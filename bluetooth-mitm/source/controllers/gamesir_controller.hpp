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
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    struct GamesirReportData {
        uint8_t id;
    } __attribute__((packed));

    class GamesirController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x05ac, 0x022d} // Gamesir-G3s (Lol, this is actually the ID of an Apple wireless keyboard)
            };  

            GamesirController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report);

        private:

    };

}
