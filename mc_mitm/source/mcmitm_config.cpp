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

namespace ams::mitm {

    namespace {

        constexpr const char *config_file_location = "sdmc:/config/MissionControl/missioncontrol.ini";
        constexpr const char *cp_default_location = "sdmc:/config/MissionControl/profiles/default.ini";
        constexpr const char *custom_config_base = "sdmc:/config/MissionControl/profiles/config_";

        MissionControlConfig g_global_config = {
            .general = {
                .disable_custom_profiles=false
            }
        };

        ControllerProfileConfig cp_global_config = {
            .general = {
                .enable_rumble = true,
                .enable_motion = true
            },
            .colours = {
                .body = {0x32, 0x32, 0x32},
                .buttons = {0xe6, 0xe6, 0xe6},
                .left_grip = {0x46, 0x46, 0x46},
                .right_grip = {0x46, 0x46, 0x46}
            },
            .misc = {
                .use_western_layout = false,
                .sony_led_brightness = 8,
                .dualshock_pollingrate_divisor = 8,
                .swap_dpad_lstick = false,
                .invert_lstick_xaxis = false,
                .invert_lstick_yaxis = false,
                .invert_rstick_xaxis = false,
                .invert_rstick_yaxis = false,
                .lstick_deadzone = 0.0f,
                .rstick_deadzone = 0.0f,
                .disable_home_button = false
            }
        };

        void ParseBoolean(const char *value, bool *out) {
            if (strcasecmp(value, "true") == 0)
                *out = true;
            else if (strcasecmp(value, "false") == 0)
                *out = false;
        }

        void ParseFloat(const char *value, float *out) {
            uint32_t val_len = strlen(value);
            float temp;
            bool multiply = true;
            uint32_t divide = 10;
            for (uint32_t i = 0; i < val_len; ++i) {
                if(value[i] >= '0' && value[i] <= '9') {
                    temp = multiply ? temp * 10 : temp;
                    temp += multiply ? value[i] - '0' : (float)(value[i] - '0') / (float)divide;
                    if(!multiply){
                        divide *= 10;
                    }
                }
                else if(value[i] == '.') {
                    multiply = false;
                }
                else {
                    *out = 0.0f;
                    return;
                }
            }
            *out = temp;
        }

        void ParseInt(const char *value, int32_t *out) {
            *out = atoi(value);
        }

        void ParsePollingRate(const char *value, int32_t *out){
            int32_t temp=8;
            ParseInt(value, &temp);
            if(temp >= 0 && temp <= 16)
                *out = temp;
            else *out = 8;
        }

        void ParseBrightness(const char *value, int32_t *out){
            int32_t temp=8;
            ParseInt(value, &temp);
            if(temp >= 0 && temp <= 63)
                *out = temp;
            else *out = 8;
        }

        void ParseDeadzone(const char *value, float *out){
            float temp=0.0f;
            ParseFloat(value, &temp);
            temp /= 100;
            if (temp > 0.0f && temp < 1.0f)
                *out = temp;
            else *out = 0.0f;
        }

        void ParseRGBstring(const char* value, controller::RGBColour *out){
            uint8_t temp[3];
            if (std::strncmp(value, "rgb(", 4) == 0) {
                 value+=4;
                 uint32_t i = 0, k=0;
                 uint32_t str_len = strlen(value);
                 temp[0] = 0;
                 temp[1] = 0;
                 temp[2] = 0;
                 while(i<str_len && value[i] !=')' && k <= 2){
                    if(value[i] >= '0' && value[i] <= '9') {
                         temp[k] *= 10;
                         temp[k] += value[i] - '0';

                    }
                    else if(value[i] == ','){
                        k++;
                    }
                    else if(value[i] != ' ') //Ignore spaces if found
                        return;
                    i++;
                 }
                 if(k==2){
                    out->r = temp[0];
                    out->g = temp[1];
                    out->b = temp[2];
                 }
            }
            else if (value[0] == '#') {
                char buf[2 + 1];
                if(strlen(value)>=7){
                    std::memcpy(buf, value + 1, 2);
                    temp[0] = static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));
                    std::memcpy(buf, value + 3, 2);
                    temp[1] = static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));
                    std::memcpy(buf, value + 5, 2);
                    temp[2] = static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));
                    out->r = temp[0];
                    out->g = temp[1];
                    out->b = temp[2];
                }
                return;
            }
            return;
        }

        Result StringifyBluetoothAddress(const bluetooth::Address *address, char *out, size_t out_size) {
            if(out_size < 2*sizeof(bluetooth::Address) + 1)
                return -1;

            char ch;
            for (uint32_t i = 0; i < sizeof(bluetooth::Address); ++i) {
                ch = address->address[i] >> 4;
                *out++ = ch + (ch <= 9 ? '0' : 'a' - 0xa);
                ch = address->address[i] & 0x0f;
                *out++ = ch + (ch <= 9 ? '0' : 'a' - 0xa);
            }
            *out = '\0';

            return 0;
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

        int MissionControlConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
            auto config = reinterpret_cast<MissionControlConfig *>(user);

            if (strcasecmp(section, "general") == 0) {
                if (strcasecmp(name, "disable_custom_profiles") == 0)
                    ParseBoolean(value, &config->general.disable_custom_profiles);
            }
            else if (strcasecmp(section, "bluetooth") == 0) {
                if (strcasecmp(name, "host_name") == 0)
                    std::strncpy(config->bluetooth.host_name, value, sizeof(config->bluetooth.host_name));
                else if (strcasecmp(name, "host_address") == 0)
                    ParseBluetoothAddress(value, &config->bluetooth.host_address);
            }
            else {
                return 0;
            }

            return 1;
        }

        int ControllerProfileIniHandler(void *user, const char *section, const char *name, const char *value) {
            auto config = reinterpret_cast<ControllerProfileConfig *>(user);

            if (strcasecmp(section, "general") == 0) {
                if (strcasecmp(name, "enable_rumble") == 0)
                    ParseBoolean(value, &config->general.enable_rumble);
                else if (strcasecmp(name, "enable_motion") == 0)
                    ParseBoolean(value, &config->general.enable_motion);
            }
            else if (strcasecmp(section, "colours") == 0) {
                if (strcasecmp(name, "body") == 0)
                    ParseRGBstring(value, &config->colours.body);
                else if (strcasecmp(name, "buttons") == 0)
                    ParseRGBstring(value, &config->colours.buttons);
                else if (strcasecmp(name, "left_grip") == 0)
                    ParseRGBstring(value, &config->colours.left_grip);
                else if (strcasecmp(name, "right_grip") == 0)
                    ParseRGBstring(value, &config->colours.right_grip);
            }
            else if (strcasecmp(section, "misc") == 0) {
                if (strcasecmp(name, "use_western_layout") == 0)
                    ParseBoolean(value, &config->misc.use_western_layout);
                else if (strcasecmp(name, "sony_led_brightness") == 0)
                    ParseBrightness(value, &config->misc.sony_led_brightness);
                else if (strcasecmp(name, "dualshock_pollingrate_divisor") == 0)
                    ParsePollingRate(value, &config->misc.dualshock_pollingrate_divisor);
                else if (strcasecmp(name, "swap_dpad_lstick") == 0)
                    ParseBoolean(value, &config->misc.swap_dpad_lstick);
                else if (strcasecmp(name, "invert_lstick_xaxis") == 0)
                    ParseBoolean(value, &config->misc.invert_lstick_xaxis);
                else if (strcasecmp(name, "invert_lstick_yaxis") == 0)
                    ParseBoolean(value, &config->misc.invert_lstick_yaxis);
                else if (strcasecmp(name, "invert_rstick_xaxis") == 0)
                    ParseBoolean(value, &config->misc.invert_rstick_xaxis);
                else if (strcasecmp(name, "invert_rstick_yaxis") == 0)
                    ParseBoolean(value, &config->misc.invert_rstick_yaxis);
                else if (strcasecmp(name, "lstick_deadzone") == 0)
                    ParseDeadzone(value, &config->misc.lstick_deadzone);
                else if (strcasecmp(name, "rstick_deadzone") == 0)
                    ParseDeadzone(value, &config->misc.rstick_deadzone);
                else if (strcasecmp(name, "disable_home_button") == 0)
                    ParseBoolean(value, &config->misc.disable_home_button);
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
        const char *mount_name = "sdmc";
        if (R_FAILED(fs::MountSdCard(mount_name))) {
            return;
        }
        ON_SCOPE_EXIT { fs::Unmount(mount_name); };

        /* Open the file. */
        fs::FileHandle mcinifile, cpinifile;
        {
            if (R_FAILED(fs::OpenFile(std::addressof(mcinifile), config_file_location, fs::OpenMode_Read))) {
                return;
            }
            if (R_FAILED(fs::OpenFile(std::addressof(cpinifile), cp_default_location, fs::OpenMode_Read))) {
                return;
            }
        }
        ON_SCOPE_EXIT { fs::CloseFile(mcinifile); fs::CloseFile(cpinifile); };

        /* Parse the config. */
        util::ini::ParseFile(mcinifile, &g_global_config, MissionControlConfigIniHandler);

        if(!g_global_config.general.disable_custom_profiles)
            util::ini::ParseFile(cpinifile, &cp_global_config, ControllerProfileIniHandler);
    }

    Result CopyFile(const char *dst_path, const char *src_path) {
        fs::FileHandle src_file, dst_file;

        /* Open the source file and get its size. */
        R_TRY(fs::OpenFile(std::addressof(src_file), src_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(src_file); };

        s64 file_size;
        R_TRY(fs::GetFileSize(std::addressof(file_size), src_file));

        /* Create the destination file. */
        R_TRY(fs::CreateFile(dst_path, file_size));

        /* Open the destination file. */
        R_TRY(fs::OpenFile(std::addressof(dst_file), dst_path, fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(dst_file); };

        /* Allocate a buffer with which to copy. */
        constexpr size_t BufferSize = 4_KB;
        void* buffer;
        buffer  = std::malloc(BufferSize);

        /* Repeatedly read until we've copied all the data. */
        s64 offset = 0;
        while (offset < file_size) {
            const size_t read_size = std::min(static_cast<size_t>(file_size - offset),BufferSize);
            R_TRY(fs::ReadFile(src_file, offset, buffer, read_size));
            R_TRY(fs::WriteFile(dst_file, offset, buffer, read_size, fs::WriteOption::None));
            offset += read_size;
        }

        /* Flush the destination file. */
        R_TRY(fs::FlushFile(dst_file));
        free(buffer);
        return ResultSuccess();
    }

    Result GetCustomIniConfig(const bluetooth::Address *address, ControllerProfileConfig *config){
        const char *mount_name = "sdmc";
        if (g_global_config.general.disable_custom_profiles || R_FAILED(fs::MountSdCard(mount_name))) {
            memcpy(config, &cp_global_config, sizeof(ControllerProfileConfig));
            return 1;
        }
        ON_SCOPE_EXIT { fs::Unmount(mount_name); };
        char custom_config[strlen(custom_config_base)+12+4+1] = ""; //12 is the mac address, 4 is .ini
        strcat(custom_config,custom_config_base);
        StringifyBluetoothAddress(address,custom_config+strlen(custom_config_base),12+4+1);
        strcat(custom_config,".ini");

        bool has_file = false;
        R_TRY(fs::HasFile(&has_file, custom_config));
        if (has_file) {
                /* Open the file. */
            fs::FileHandle file;
            {
                if (R_FAILED(fs::OpenFile(std::addressof(file), custom_config, fs::OpenMode_Read))) {
                    memcpy(config, &cp_global_config, sizeof(ControllerProfileConfig));
                    return 1;
                }
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };
            ControllerProfileConfig custom;
            memcpy(&custom, &cp_global_config, sizeof(ControllerProfileConfig));
            util::ini::ParseFile(file, &custom, ControllerProfileIniHandler);
            memcpy(config, &custom, sizeof(ControllerProfileConfig));
            return 0;
        }
        else { //No custom profile, so we just copy the default one

            //Atmosphere implementation
            CopyFile(custom_config, cp_default_location);
            /*End of Atmosphere implementation*/
            memcpy(config, &cp_global_config, sizeof(ControllerProfileConfig));
            return 0;
        }
    }
}
