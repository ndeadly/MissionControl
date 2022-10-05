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
#include <stratosphere.hpp>
#include "mcmitm_config.hpp"

namespace ams::mitm {

    namespace {

        constexpr const char *config_file_location = "sdmc:/config/MissionControl/missioncontrol.ini";

        SetLanguage g_system_language;

        MissionControlConfig g_global_config = {
            .general = {
                .enable_rumble = true,
                .enable_motion = true
            },
            .misc = {
                .enable_dualshock4_lightbar = true,
                .enable_dualsense_lightbar = true,
                .enable_dualsense_player_leds = true,
                .dualsense_vibration_intensity = 4
            }
        };

        void ParseBoolean(const char *value, bool *out) {
            if (strcasecmp(value, "true") == 0)
                *out = true;
            else if (strcasecmp(value, "false") == 0)
                *out = false; 
        }

        void ParseInt(const char *value, int *out, int min=INT_MIN, int max=INT_MAX) {
            int tmp = std::strtol(value, nullptr, 10);
            if ((tmp >= min) && (tmp <= max))
                *out = tmp;
        }

        void ParseBluetoothAddress(const char *value, bluetooth::Address *out) {
            // Check length of address string is correct
            if (std::strlen(value) != 3*sizeof(bluetooth::Address) - 1) return;

            // Parse bluetooth mac address
            char buf[2 + 1];
            bluetooth::Address address = {};
            for (uint32_t i = 0; i < sizeof(bluetooth::Address); ++i) {
                // Convert hex pair to number
                std::memcpy(buf, &value[i*3], 2);
                address.address[i] = static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));

                // Check for colon separator
                if ((i < sizeof(bluetooth::Address) - 1) && (value[i*3 + 2] != ':'))
                    return;
            }

            *out = address;
        }

        int ConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
            auto config = reinterpret_cast<MissionControlConfig *>(user);

            if (strcasecmp(section, "general") == 0) {
                if (strcasecmp(name, "enable_rumble") == 0)
                    ParseBoolean(value, &config->general.enable_rumble);  
                else if (strcasecmp(name, "enable_motion") == 0)
                    ParseBoolean(value, &config->general.enable_motion); 
            }
            else if (strcasecmp(section, "bluetooth") == 0) {
                if (strcasecmp(name, "host_name") == 0)
                    std::strncpy(config->bluetooth.host_name, value, sizeof(config->bluetooth.host_name));
                else if (strcasecmp(name, "host_address") == 0)
                    ParseBluetoothAddress(value, &config->bluetooth.host_address);
            }
            else if (strcasecmp(section, "misc") == 0) {
                if (strcasecmp(name, "enable_dualshock4_lightbar") == 0)
                    ParseBoolean(value, &config->misc.enable_dualshock4_lightbar);
                else if (strcasecmp(name, "enable_dualsense_lightbar") == 0)
                    ParseBoolean(value, &config->misc.enable_dualsense_lightbar);
                else if (strcasecmp(name, "enable_dualsense_player_leds") == 0)
                    ParseBoolean(value, &config->misc.enable_dualsense_player_leds);
                else if (strcasecmp(name, "dualsense_vibration_intensity") == 0)
                    ParseInt(value, &config->misc.dualsense_vibration_intensity, 1, 8);
            }
            else {
                return 0;
            }

            return 1;
        }

    }

    void ParseIniConfig() {
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

    void InitializeConfig() {
        ParseIniConfig();

        R_ABORT_UNLESS(setInitialize());
        ON_SCOPE_EXIT { setExit(); };
        u64 language_code = 0;
        R_ABORT_UNLESS(setGetSystemLanguage(&language_code));
        R_ABORT_UNLESS(setMakeLanguage(language_code, &g_system_language));
    }

    MissionControlConfig *GetGlobalConfig() {
        return &g_global_config;
    }

    SetLanguage GetSystemLanguage() {
        return g_system_language;
    }

}
