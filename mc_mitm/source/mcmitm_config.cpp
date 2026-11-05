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
#include <stratosphere.hpp>
#include <atomic>
#include "mcmitm_config.hpp"
#include "controllers/switch_buttons.hpp"

namespace ams::mitm {

    namespace {

        constexpr const char config_file_location[] = "sdmc:/config/MissionControl/missioncontrol.ini";

        constinit SetLanguage g_system_language;

        constexpr u32 SwitchMask(controller::SwitchButton button) {
            return static_cast<u32>(button);
        }

        constexpr MissionControlConfig MakeDefaultConfig() {
            MissionControlConfig config = {};

            config.general.enable_rumble = true;
            config.general.enable_motion = true;

            config.misc.analog_trigger_activation_threshold = 50;
            config.misc.dualshock3_enable_usb_pairing = true;
            config.misc.dualshock3_led_mode = 0;
            config.misc.dualshock4_polling_rate = 8;
            config.misc.dualshock4_lightbar_brightness = 5;
            config.misc.dualsense_lightbar_brightness = 5;
            config.misc.dualsense_enable_player_leds = true;
            config.misc.dualsense_vibration_intensity = 4;
            config.misc.dualsense_adaptive_trigger_travel = 0;

            for (auto &colour : config.misc.dualshock4_lightbar_colour) {
                colour = -1;
            }

            // Default Dualshock 4 -> Switch button layout
            auto &map = config.misc.dualshock4_button_map;
            map[Dualshock4Input_Cross]          = SwitchMask(controller::SwitchButton::B);
            map[Dualshock4Input_Circle]         = SwitchMask(controller::SwitchButton::A);
            map[Dualshock4Input_Square]         = SwitchMask(controller::SwitchButton::Y);
            map[Dualshock4Input_Triangle]       = SwitchMask(controller::SwitchButton::X);
            map[Dualshock4Input_L1]             = SwitchMask(controller::SwitchButton::L);
            map[Dualshock4Input_R1]             = SwitchMask(controller::SwitchButton::R);
            map[Dualshock4Input_L2]             = SwitchMask(controller::SwitchButton::ZL);
            map[Dualshock4Input_R2]             = SwitchMask(controller::SwitchButton::ZR);
            map[Dualshock4Input_L3]             = SwitchMask(controller::SwitchButton::StickL);
            map[Dualshock4Input_R3]             = SwitchMask(controller::SwitchButton::StickR);
            map[Dualshock4Input_Share]          = SwitchMask(controller::SwitchButton::Minus);
            map[Dualshock4Input_Options]        = SwitchMask(controller::SwitchButton::Plus);
            map[Dualshock4Input_Ps]             = SwitchMask(controller::SwitchButton::Home);
            map[Dualshock4Input_DpadUp]         = SwitchMask(controller::SwitchButton::Up);
            map[Dualshock4Input_DpadDown]       = SwitchMask(controller::SwitchButton::Down);
            map[Dualshock4Input_DpadLeft]       = SwitchMask(controller::SwitchButton::Left);
            map[Dualshock4Input_DpadRight]      = SwitchMask(controller::SwitchButton::Right);
            map[Dualshock4Input_TouchpadLeft]   = SwitchMask(controller::SwitchButton::Minus);
            map[Dualshock4Input_TouchpadCentre] = SwitchMask(controller::SwitchButton::Capture);
            map[Dualshock4Input_TouchpadRight]  = SwitchMask(controller::SwitchButton::Plus);

            return config;
        }

        constexpr MissionControlConfig DefaultConfig = MakeDefaultConfig();

        // The configuration is double buffered: a reload parses into the inactive buffer and then publishes it with a single
        // pointer swap, so readers never observe a half-updated configuration.
        constinit MissionControlConfig g_config_buffers[2] = { DefaultConfig, DefaultConfig };
        constinit std::atomic<MissionControlConfig *> g_active_config = &g_config_buffers[0];
        constinit std::atomic<u32> g_config_generation = 0;

        // Only ever touched from the main thread (LoadConfiguration and PollConfigurationReload)
        constexpr size_t MaxConfigFileSize = 8_KB;
        constinit char g_file_buffer[MaxConfigFileSize + 1] = {};
        constinit u32 g_applied_hash = 0;

        struct NamedValue {
            const char *name;
            u32 value;
        };

        constexpr NamedValue Dualshock4InputNames[] = {
            { "cross",           Dualshock4Input_Cross },
            { "circle",          Dualshock4Input_Circle },
            { "square",          Dualshock4Input_Square },
            { "triangle",        Dualshock4Input_Triangle },
            { "l1",              Dualshock4Input_L1 },
            { "r1",              Dualshock4Input_R1 },
            { "l2",              Dualshock4Input_L2 },
            { "r2",              Dualshock4Input_R2 },
            { "l3",              Dualshock4Input_L3 },
            { "r3",              Dualshock4Input_R3 },
            { "share",           Dualshock4Input_Share },
            { "options",         Dualshock4Input_Options },
            { "ps",              Dualshock4Input_Ps },
            { "dpad_up",         Dualshock4Input_DpadUp },
            { "dpad_down",       Dualshock4Input_DpadDown },
            { "dpad_left",       Dualshock4Input_DpadLeft },
            { "dpad_right",      Dualshock4Input_DpadRight },
            { "touchpad_left",   Dualshock4Input_TouchpadLeft },
            { "touchpad_centre", Dualshock4Input_TouchpadCentre },
            { "touchpad_center", Dualshock4Input_TouchpadCentre },
            { "touchpad_right",  Dualshock4Input_TouchpadRight },
        };

        constexpr NamedValue SwitchButtonNames[] = {
            { "none",    0 },
            { "a",       SwitchMask(controller::SwitchButton::A) },
            { "b",       SwitchMask(controller::SwitchButton::B) },
            { "x",       SwitchMask(controller::SwitchButton::X) },
            { "y",       SwitchMask(controller::SwitchButton::Y) },
            { "l",       SwitchMask(controller::SwitchButton::L) },
            { "r",       SwitchMask(controller::SwitchButton::R) },
            { "zl",      SwitchMask(controller::SwitchButton::ZL) },
            { "zr",      SwitchMask(controller::SwitchButton::ZR) },
            { "plus",    SwitchMask(controller::SwitchButton::Plus) },
            { "minus",   SwitchMask(controller::SwitchButton::Minus) },
            { "home",    SwitchMask(controller::SwitchButton::Home) },
            { "capture", SwitchMask(controller::SwitchButton::Capture) },
            { "stickl",  SwitchMask(controller::SwitchButton::StickL) },
            { "stickr",  SwitchMask(controller::SwitchButton::StickR) },
            { "up",      SwitchMask(controller::SwitchButton::Up) },
            { "down",    SwitchMask(controller::SwitchButton::Down) },
            { "left",    SwitchMask(controller::SwitchButton::Left) },
            { "right",   SwitchMask(controller::SwitchButton::Right) },
        };

        void ParseBoolean(const char *value, bool *out) {
            if (strcasecmp(value, "true") == 0)
                *out = true;
            else if (strcasecmp(value, "false") == 0)
                *out = false;
        }

        void ParseInt(const char *value, int *out, int min=INT_MIN, int max=INT_MAX) {
            int tmp = std::strtol(value, nullptr, 10);
            if ((tmp >= min) && (tmp <= max)) {
                *out = tmp;
            }
        }

        // Parses a colour in the form RRGGBB or #RRGGBB
        void ParseColour(const char *value, int *out) {
            if (*value == '#') {
                ++value;
            }

            if (std::strlen(value) != 6) {
                return;
            }

            for (int i = 0; i < 6; ++i) {
                if (!std::isxdigit(static_cast<unsigned char>(value[i]))) {
                    return;
                }
            }

            *out = static_cast<int>(std::strtoul(value, nullptr, 16));
        }

        // Parses a list of Switch buttons joined with '+' (e.g. "a", "zl+l") or "none". Names are not case sensitive.
        // Leaves *out untouched if any name is not recognised.
        void ParseSwitchButtons(const char *value, u32 *out) {
            u32 mask = 0;

            const char *token = value;
            while (true) {
                const char *plus = std::strchr(token, '+');
                const size_t length = plus ? static_cast<size_t>(plus - token) : std::strlen(token);

                bool found = false;
                for (const auto &button : SwitchButtonNames) {
                    if ((std::strlen(button.name) == length) && (strncasecmp(token, button.name, length) == 0)) {
                        mask |= button.value;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    return;
                }

                if (!plus) {
                    break;
                }

                token = plus + 1;
            }

            *out = mask;
        }

        // input_name is the part of the key after "dualshock4_map_"
        void ParseDualshock4Mapping(const char *input_name, const char *value, MissionControlConfig *config) {
            for (const auto &input : Dualshock4InputNames) {
                if (strcasecmp(input_name, input.name) == 0) {
                    ParseSwitchButtons(value, &config->misc.dualshock4_button_map[input.value]);
                    return;
                }
            }
        }

        void ParseBluetoothAddress(const char *value, bluetooth::Address *out) {
            // Check length of address string is correct
            if (std::strlen(value) != 3*sizeof(bluetooth::Address) - 1) {
                return;
            }

            // Parse bluetooth mac address
            char buf[2 + 1];
            bluetooth::Address address = {};
            for (u32 i = 0; i < sizeof(bluetooth::Address); ++i) {
                // Convert hex pair to number
                std::memcpy(buf, &value[i*3], 2);
                address.address[i] = static_cast<u8>(std::strtoul(buf, nullptr, 16));

                // Check for colon separator
                if ((i < sizeof(bluetooth::Address) - 1) && (value[i*3 + 2] != ':')) {
                    return;
                }
            }

            *out = address;
        }

        int ConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
            auto config = reinterpret_cast<MissionControlConfig *>(user);

            if (strcasecmp(section, "general") == 0) {
                if (strcasecmp(name, "enable_rumble") == 0) {
                    ParseBoolean(value, &config->general.enable_rumble);
                } else if (strcasecmp(name, "enable_motion") == 0) {
                    ParseBoolean(value, &config->general.enable_motion);
                }
            } else if (strcasecmp(section, "bluetooth") == 0) {
                if (strcasecmp(name, "host_name") == 0) {
                    std::strncpy(config->bluetooth.host_name, value, sizeof(config->bluetooth.host_name));
                } else if (strcasecmp(name, "host_address") == 0) {
                    ParseBluetoothAddress(value, &config->bluetooth.host_address);
                }
            } else if (strcasecmp(section, "misc") == 0) {
                if (strcasecmp(name, "analog_trigger_activation_threshold") == 0) {
                    ParseInt(value, &config->misc.analog_trigger_activation_threshold, 0, 100);
                } else if (strcasecmp(name, "dualshock3_enable_usb_pairing") == 0) {
                    ParseBoolean(value, &config->misc.dualshock3_enable_usb_pairing);
                } else if (strcasecmp(name, "dualshock3_led_mode") == 0) {
                    ParseInt(value, &config->misc.dualshock3_led_mode, 0, 2);
                } else if (strcasecmp(name, "dualshock4_polling_rate") == 0) {
                    ParseInt(value, &config->misc.dualshock4_polling_rate, 0, 16);
                } else if (strcasecmp(name, "dualshock4_lightbar_brightness") == 0) {
                    ParseInt(value, &config->misc.dualshock4_lightbar_brightness, 0, 9);
                } else if (strcasecmp(name, "dualsense_lightbar_brightness") == 0) {
                    ParseInt(value, &config->misc.dualsense_lightbar_brightness, 0, 9);
                } else if (strcasecmp(name, "dualsense_enable_player_leds") == 0) {
                    ParseBoolean(value, &config->misc.dualsense_enable_player_leds);
                } else if (strcasecmp(name, "dualsense_vibration_intensity") == 0) {
                    ParseInt(value, &config->misc.dualsense_vibration_intensity, 1, 8);
                } else if (strcasecmp(name, "dualsense_adaptive_trigger_travel") == 0) {
                    ParseInt(value, &config->misc.dualsense_adaptive_trigger_travel, 0, 100);
                } else if (strcasecmp(name, "dualshock4_lightbar_colour") == 0) {
                    ParseColour(value, &config->misc.dualshock4_lightbar_colour[0]);
                } else if ((strncasecmp(name, "dualshock4_lightbar_colour_", 27) == 0) && (name[27] >= '1') && (name[27] <= '8') && (name[28] == '\0')) {
                    ParseColour(value, &config->misc.dualshock4_lightbar_colour[name[27] - '0']);
                } else if (strncasecmp(name, "dualshock4_map_", 15) == 0) {
                    ParseDualshock4Mapping(name + 15, value, config);
                }
            } else {
                return 0;
            }

            return 1;
        }

        u32 HashBytes(const char *data, size_t size) {
            // 32-bit FNV-1a
            u32 hash = 2166136261u;
            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<u8>(data[i]);
                hash *= 16777619u;
            }
            return hash;
        }

        // Reads the ini file into g_file_buffer as a NUL terminated string. Returns false if the file can't be read.
        // The whole file is read in one go with checked reads and parsed from memory afterwards, because parsing straight from
        // the file would abort the process if the file were rewritten by another program part way through.
        bool ReadConfigFile(size_t *out_size) {
            fs::FileHandle file;
            if (R_FAILED(fs::OpenFile(std::addressof(file), config_file_location, fs::OpenMode_Read))) {
                return false;
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            size_t read = 0;
            if (R_FAILED(fs::ReadFile(std::addressof(read), file, 0, g_file_buffer, MaxConfigFileSize))) {
                return false;
            }

            // A file that fills the whole buffer was probably cut off part way through a line, so only keep complete lines
            if (read == MaxConfigFileSize) {
                while ((read > 0) && (g_file_buffer[read - 1] != '\n')) {
                    --read;
                }
            }

            g_file_buffer[read] = '\0';
            *out_size = read;
            return true;
        }

        void ParseConfigBuffer(MissionControlConfig *config) {
            // Always start from the defaults so that removing a key from the file restores its default value
            *config = DefaultConfig;
            util::ini::ParseString(g_file_buffer, config, ConfigIniHandler);
        }

        void ReadSystemLanguage() {
            R_ABORT_UNLESS(setInitialize());
            ON_SCOPE_EXIT { setExit(); };
            u64 language_code = 0;
            R_ABORT_UNLESS(setGetSystemLanguage(&language_code));
            R_ABORT_UNLESS(setMakeLanguage(language_code, &g_system_language));
        }

    }

    void LoadConfiguration() {
        size_t size;
        if (ReadConfigFile(std::addressof(size))) {
            ParseConfigBuffer(g_active_config.load());
            g_applied_hash = HashBytes(g_file_buffer, size);
        }

        ReadSystemLanguage();
    }

    void PollConfigurationReload() {
        size_t size;
        if (!ReadConfigFile(std::addressof(size))) {
            // Missing or unreadable (e.g. being replaced right now). Carry on with the settings we already have
            return;
        }

        const u32 hash = HashBytes(g_file_buffer, size);
        if (hash == g_applied_hash) {
            return;
        }

        // The contents differ from what is loaded. Give a program that is still in the middle of writing the file a moment
        // to finish, then read it again: only apply it if it is unchanged, so we never act on a half-written file.
        // (If it did change again, the next poll will see the final version.)
        os::SleepThread(TimeSpan::FromMilliSeconds(30));
        if (!ReadConfigFile(std::addressof(size)) || HashBytes(g_file_buffer, size) != hash) {
            return;
        }

        MissionControlConfig *active = g_active_config.load();
        MissionControlConfig *inactive = (active == &g_config_buffers[0]) ? &g_config_buffers[1] : &g_config_buffers[0];

        ParseConfigBuffer(inactive);
        g_active_config.store(inactive);
        g_applied_hash = hash;
        g_config_generation.fetch_add(1);
    }

    u32 GetConfigurationGeneration() {
        return g_config_generation.load();
    }

    MissionControlConfig *GetGlobalConfig() {
        return g_active_config.load();
    }

    SetLanguage GetSystemLanguage() {
        return g_system_language;
    }

}
