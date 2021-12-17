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
#include <stratosphere.hpp>
#include <cstring>
#include "mcmitm_config.hpp"
#include "utils.hpp"

namespace ams::mitm {

    namespace {

        constexpr const char *config_file_location = "sdmc:/config/MissionControl/missioncontrol.ini";

        MissionControlConfig g_global_config = {
            .general = {
                .disable_custom_profiles=false
            }
        };

        int ConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
            auto config = reinterpret_cast<MissionControlConfig *>(user);

            if (strcasecmp(section, "general") == 0) {
                if (strcasecmp(name, "disable_custom_profiles") == 0)
                    utils::ParseBoolean(value, &config->general.disable_custom_profiles);
            }
            else if (strcasecmp(section, "bluetooth") == 0) {
                if (strcasecmp(name, "host_name") == 0)
                    std::strncpy(config->bluetooth.host_name, value, sizeof(config->bluetooth.host_name));
                else if (strcasecmp(name, "host_address") == 0)
                    utils::ParseBluetoothAddress(value, &config->bluetooth.host_address);
            }
            else {
                return 0;
            }

            return 1;
        }

    }

    MissionControlConfig *GetGlobalConfig(void) {
        return &g_global_config;
    }

    void ParseIniConfig(void) {
        /* Open the file. */
        fs::FileHandle file;
        {
            if (R_FAILED(fs::OpenFile(std::addressof(file), config_file_location, fs::OpenMode_Read))) {
                return;
            }
        }
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Parse the config. */
        util::ini::ParseFile(file, &g_global_config, ConfigIniHandler);
    }

}
