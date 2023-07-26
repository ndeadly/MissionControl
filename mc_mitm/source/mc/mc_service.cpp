/*
 * Copyright (c) 2020-2021 ndeadly
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
#include "mc_service.hpp"
#include "../mcmitm_version.hpp"

namespace ams::mc {

    Result MissionControlService::GetVersion(sf::Out<u32> version) {
        version.SetValue(mc_version);
        R_SUCCEED();
    }

    Result MissionControlService::GetBuildVersionString(sf::Out<mc::VersionString> version) {
        std::strncpy(version.GetPointer()->version, mc_build_name, sizeof(mc::VersionString));
        R_SUCCEED();
    }

    Result MissionControlService::GetBuildDateString(sf::Out<mc::DateString> date) {
        std::strncpy(date.GetPointer()->date, mc_build_date, sizeof(mc::DateString));
        R_SUCCEED();
    }

}
