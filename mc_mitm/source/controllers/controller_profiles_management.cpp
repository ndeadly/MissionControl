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
#include "controller_profiles_management.hpp"
#include "../utils.hpp"
#include "../mcmitm_config.hpp"

namespace ams::controller {

    namespace {
        constexpr const char cp_default_path[] = "sdmc:/config/MissionControl/controllers/default.ini";
        constexpr const char controllers_folder[] = "sdmc:/config/MissionControl/controllers/";
        constexpr const char profile_container[] = "profile.ini";

        int ControllerProfileIniHandler(void *user, const char *section, const char *name, const char *value) {
                auto config = reinterpret_cast<ControllerProfileConfig *>(user);

                if (strcasecmp(section, "general") == 0) {
                    if (strcasecmp(name, "enable_rumble") == 0)
                        utils::ParseBoolean(value, &config->general.enable_rumble);
                    else if (strcasecmp(name, "enable_motion") == 0)
                        utils::ParseBoolean(value, &config->general.enable_motion);
                }
                else if (strcasecmp(section, "misc") == 0) {
                    if (strcasecmp(name, "disable_sony_leds") == 0)
                        utils::ParseBoolean(value, &config->misc.disable_sony_leds);
                    else if (strcasecmp(name, "swap_dpad_lstick") == 0)
                        utils::ParseBoolean(value, &config->misc.swap_dpad_lstick);
                }
                else {
                    return 0;
                }

                return 1;
        }
    }

    Result GetControllerConfig(const bluetooth::Address *address, ControllerProfileConfig *config) {
        *config = g_cp_global_config;
        if (mitm::GetGlobalConfig()->general.disable_custom_profiles)
            return 1;
        char custom_config_path[100] = "";
        std::strcat(custom_config_path, controllers_folder);
        char btaddress_string[2 * sizeof(bluetooth::Address) + 1] = "";
        utils::BluetoothAddressToString(address, btaddress_string, 2 * sizeof(bluetooth::Address) + 1);
        std::strcat(custom_config_path, btaddress_string);
        std::strcat(custom_config_path, "/");
        std::strcat(custom_config_path, profile_container);
        bool has_file = false;
        R_TRY(fs::HasFile(&has_file, custom_config_path));
        if (!has_file) {
            std::strcpy(custom_config_path, cp_default_path);
        }
        /* Open the file. */
        fs::FileHandle file;
        {
            if (R_FAILED(fs::OpenFile(std::addressof(file), custom_config_path, fs::OpenMode_Read))) {
                return 1;
            }
        }
        ON_SCOPE_EXIT { fs::CloseFile(file); };
        util::ini::ParseFile(file, config, ControllerProfileIniHandler);
        return ams::ResultSuccess();
    }
}
