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
#include "emulated_switch_controller.hpp"
#include "../utils.hpp"
#include "../mcmitm_config.hpp"
#include <memory>

namespace ams::controller {

    namespace {

        constexpr auto DPAD_THRESHOLD_BEGIN = STICK_ZERO - UINT12_MAX / 4;
        constexpr auto DPAD_THRESHOLD_END   = STICK_ZERO + UINT12_MAX / 4;

        // Factory calibration data representing analog stick ranges that span the entire 12-bit data type in x and y
        SwitchAnalogStickFactoryCalibration lstick_factory_calib = {0xff, 0xf7, 0x7f, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80};
        SwitchAnalogStickFactoryCalibration rstick_factory_calib = {0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0xff, 0xf7, 0x7f};

        // Stick parameters data that produce a 12.5% inner deadzone and a 5% outer deadzone (in relation to the full 12 bit range above)
        SwitchAnalogStickParameters default_stick_params = {0x0f, 0x30, 0x61, 0x00, 0x31, 0xf3, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};

        // Frequency in Hz rounded to nearest int
        // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md#frequency-table
        const uint16_t rumble_freq_lut[] = {
            0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
            0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0039, 0x003a, 0x003b,
            0x003c, 0x003e, 0x003f, 0x0040, 0x0042, 0x0043, 0x0045, 0x0046, 0x0048,
            0x0049, 0x004b, 0x004d, 0x004e, 0x0050, 0x0052, 0x0054, 0x0055, 0x0057,
            0x0059, 0x005b, 0x005d, 0x005f, 0x0061, 0x0063, 0x0066, 0x0068, 0x006a,
            0x006c, 0x006f, 0x0071, 0x0074, 0x0076, 0x0079, 0x007b, 0x007e, 0x0081,
            0x0084, 0x0087, 0x0089, 0x008d, 0x0090, 0x0093, 0x0096, 0x0099, 0x009d,
            0x00a0, 0x00a4, 0x00a7, 0x00ab, 0x00ae, 0x00b2, 0x00b6, 0x00ba, 0x00be,
            0x00c2, 0x00c7, 0x00cb, 0x00cf, 0x00d4, 0x00d9, 0x00dd, 0x00e2, 0x00e7,
            0x00ec, 0x00f1, 0x00f7, 0x00fc, 0x0102, 0x0107, 0x010d, 0x0113, 0x0119,
            0x011f, 0x0125, 0x012c, 0x0132, 0x0139, 0x0140, 0x0147, 0x014e, 0x0155,
            0x015d, 0x0165, 0x016c, 0x0174, 0x017d, 0x0185, 0x018d, 0x0196, 0x019f,
            0x01a8, 0x01b1, 0x01bb, 0x01c5, 0x01ce, 0x01d9, 0x01e3, 0x01ee, 0x01f8,
            0x0203, 0x020f, 0x021a, 0x0226, 0x0232, 0x023e, 0x024b, 0x0258, 0x0265,
            0x0272, 0x0280, 0x028e, 0x029c, 0x02ab, 0x02ba, 0x02c9, 0x02d9, 0x02e9,
            0x02f9, 0x030a, 0x031b, 0x032c, 0x033e, 0x0350, 0x0363, 0x0376, 0x0389,
            0x039d, 0x03b1, 0x03c6, 0x03db, 0x03f1, 0x0407, 0x041d, 0x0434, 0x044c,
            0x0464, 0x047d, 0x0496, 0x04af, 0x04ca, 0x04e5
        };
        constexpr size_t rumble_freq_lut_size = sizeof(rumble_freq_lut) / sizeof(uint16_t);

        // Floats from dekunukem repo normalised and scaled by function used by yuzu
        // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md#amplitude-table
        // https://github.com/yuzu-emu/yuzu/blob/d3a4a192fe26e251f521f0311b2d712f5db9918e/src/input_common/sdl/sdl_impl.cpp#L429
        const float rumble_amp_lut_f[] = {
            0.000000, 0.120576, 0.137846, 0.146006, 0.154745, 0.164139, 0.174246,
            0.185147, 0.196927, 0.209703, 0.223587, 0.238723, 0.255268, 0.273420,
            0.293398, 0.315462, 0.321338, 0.327367, 0.333557, 0.339913, 0.346441,
            0.353145, 0.360034, 0.367112, 0.374389, 0.381870, 0.389564, 0.397476,
            0.405618, 0.413996, 0.422620, 0.431501, 0.436038, 0.440644, 0.445318,
            0.450062, 0.454875, 0.459764, 0.464726, 0.469763, 0.474876, 0.480068,
            0.485342, 0.490694, 0.496130, 0.501649, 0.507256, 0.512950, 0.518734,
            0.524609, 0.530577, 0.536639, 0.542797, 0.549055, 0.555413, 0.561872,
            0.568436, 0.575106, 0.581886, 0.588775, 0.595776, 0.602892, 0.610127,
            0.617482, 0.624957, 0.632556, 0.640283, 0.648139, 0.656126, 0.664248,
            0.672507, 0.680906, 0.689447, 0.698135, 0.706971, 0.715957, 0.725098,
            0.734398, 0.743857, 0.753481, 0.763273, 0.773235, 0.783370, 0.793684,
            0.804178, 0.814858, 0.825726, 0.836787, 0.848044, 0.859502, 0.871165,
            0.883035, 0.895119, 0.907420, 0.919943, 0.932693, 0.945673, 0.958889,
            0.972345, 0.986048, 1.000000
        };
        constexpr size_t rumble_amp_lut_f_size = sizeof(rumble_amp_lut_f) / sizeof(float);

        Result DecodeRumbleValues(const uint8_t enc[], SwitchRumbleData *dec) {
            uint8_t hi_freq_ind = 0x20 + (enc[0] >> 2) + ((enc[1] & 0x01) * 0x40) - 1;
            uint8_t hi_amp_ind  = (enc[1] & 0xfe) >> 1;
            uint8_t lo_freq_ind = (enc[2] & 0x7f) - 1;
            uint8_t lo_amp_ind  = ((enc[3] - 0x40) << 1) + ((enc[2] & 0x80) >> 7);

            if (!((hi_freq_ind < rumble_freq_lut_size) &&
                  (hi_amp_ind < rumble_amp_lut_f_size) &&
                  (lo_freq_ind < rumble_freq_lut_size) &&
                  (lo_amp_ind < rumble_amp_lut_f_size))) {
                std::memset(dec, 0, sizeof(SwitchRumbleData));
                return -1;
            }

            dec->high_band_freq = float(rumble_freq_lut[hi_freq_ind]);
            dec->high_band_amp  = rumble_amp_lut_f[hi_amp_ind];
            dec->low_band_freq  = float(rumble_freq_lut[lo_freq_ind]);
            dec->low_band_amp   = rumble_amp_lut_f[lo_amp_ind];
            return ams::ResultSuccess();
        }

        Result InitializeVirtualSpiFlash(const char *path, size_t size) {
            fs::FileHandle file;

            // Open the file for write
            R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write));
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            // Fill the file with 0xff
            uint8_t buff[64];
            std::memset(buff, 0xff, sizeof(buff));

            unsigned int offset = 0;
            while (offset < size) {
                size_t write_size = std::min(static_cast<size_t>(size - offset), sizeof(buff));
                R_TRY(fs::WriteFile(file, offset, buff, write_size, fs::WriteOption::None));
                offset += write_size;
            }

            // Write default values for data that the console attempts to read in practice
            const struct {
                SwitchAnalogStickFactoryCalibration lstick_factory_calib;
                SwitchAnalogStickFactoryCalibration rstick_factory_calib;
            } data1 = { lstick_factory_calib, rstick_factory_calib };
            R_TRY(fs::WriteFile(file, 0x603d, &data1, sizeof(data1), fs::WriteOption::None));

            const struct {
                RGBColour body;
                RGBColour buttons;
                RGBColour left_grip;
                RGBColour right_grip;
            } data2 = { {0x32, 0x32, 0x32}, {0xe6, 0xe6, 0xe6}, {0x46, 0x46, 0x46}, {0x46, 0x46, 0x46} };
            R_TRY(fs::WriteFile(file, 0x6050, &data2, sizeof(data2), fs::WriteOption::None));

            const struct {
                SwitchAnalogStickParameters lstick_default_parameters;
                SwitchAnalogStickParameters rstick_default_parameters;
            } data3 = { default_stick_params, default_stick_params };
            R_TRY(fs::WriteFile(file, 0x6086, &data3, sizeof(data3), fs::WriteOption::None));

            R_TRY(fs::FlushFile(file));

            return ams::ResultSuccess();
        }

    }

    EmulatedSwitchController::EmulatedSwitchController(const bluetooth::Address *address, HardwareID id)
    : SwitchController(address, id)
    , m_charging(false)
    , m_ext_power(false)
    , m_battery(BATTERY_MAX)
    , m_led_pattern(0) {
        this->ClearControllerState();

        GetControllerConfig(address, &m_profile);

        m_enable_rumble = m_profile.general.enable_rumble;
    };

    EmulatedSwitchController::~EmulatedSwitchController() {
        fs::CloseFile(m_spi_flash_file);
    }

    Result EmulatedSwitchController::Initialize(void) {
        SwitchController::Initialize();

        // Ensure config directory for this controller exists
        std::string path = GetControllerDirectory(&m_address);
        R_TRY(fs::EnsureDirectoryRecursively(path.c_str()));

        // Check if the virtual spi flash file already exists and initialise it if not
        path += "/spi_flash.bin";
        bool file_exists;
        R_TRY(fs::HasFile(&file_exists, path.c_str()));
        if (!file_exists) {
            auto spi_flash_size = 0x10000;
            // Create file representing first 64KB of SPI flash
            R_TRY(fs::CreateFile(path.c_str(), spi_flash_size));

            // Initialise the spi flash data
            R_TRY(InitializeVirtualSpiFlash(path.c_str(), spi_flash_size));
        }

        // Open the virtual spi flash file for read and write
        R_TRY(fs::OpenFile(std::addressof(m_spi_flash_file), path.c_str(), fs::OpenMode_ReadWrite));

        return ams::ResultSuccess();
    }

    void EmulatedSwitchController::ClearControllerState(void) {
        std::memset(&m_buttons, 0, sizeof(m_buttons));
        m_left_stick.SetData(STICK_ZERO, STICK_ZERO);
        m_right_stick.SetData(STICK_ZERO, STICK_ZERO);
        std::memset(&m_motion_data, 0, sizeof(m_motion_data));
    }

    Result EmulatedSwitchController::HandleIncomingReport(const bluetooth::HidReport *report) {
        this->UpdateControllerState(report);

        // Prepare Switch report
        m_input_report.size = sizeof(SwitchInputReport0x30) + 1;
        auto switch_report = reinterpret_cast<SwitchReportData *>(m_input_report.data);
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info   = (0 << 1) | m_ext_power;
        switch_report->input0x30.battery     = m_battery | m_charging;
        switch_report->input0x30.buttons     = m_buttons;
        switch_report->input0x30.left_stick  = m_left_stick;
        switch_report->input0x30.right_stick = m_right_stick;
        std::memcpy(&switch_report->input0x30.motion, &m_motion_data, sizeof(m_motion_data));

        if (m_profile.misc.disable_home_button)
            switch_report->input0x30.buttons.home = 0;

        this->ApplyButtonCombos(&switch_report->input0x30.buttons);

        if (m_profile.misc.swap_dpad_lstick) {
            uint16_t temp_lstick_x = STICK_ZERO; //Start in a neutral position
            uint16_t temp_lstick_y = STICK_ZERO;
            if (switch_report->input0x30.buttons.dpad_down)
                temp_lstick_y -= (UINT12_MAX / 2);
            if (switch_report->input0x30.buttons.dpad_up) //Should be if else, but some controllers don't have an actual dpad, so both states are allowed
                temp_lstick_y += (UINT12_MAX / 2);
            if (switch_report->input0x30.buttons.dpad_right)
                temp_lstick_x += (UINT12_MAX / 2);
            if (switch_report->input0x30.buttons.dpad_left)
                temp_lstick_x -= (UINT12_MAX / 2);

            switch_report->input0x30.buttons.dpad_left = switch_report->input0x30.left_stick.GetX() < DPAD_THRESHOLD_BEGIN ? 1 : 0;
            switch_report->input0x30.buttons.dpad_right = switch_report->input0x30.left_stick.GetX() > DPAD_THRESHOLD_END ? 1 : 0;
            switch_report->input0x30.buttons.dpad_down = switch_report->input0x30.left_stick.GetY() < DPAD_THRESHOLD_BEGIN ? 1 : 0;
            switch_report->input0x30.buttons.dpad_up = switch_report->input0x30.left_stick.GetY() > DPAD_THRESHOLD_END ? 1 : 0;

            switch_report->input0x30.left_stick.SetData(temp_lstick_x, temp_lstick_y);
        }

        if (m_profile.misc.invert_lstick_xaxis)
            switch_report->input0x30.left_stick.InvertX();
        if (m_profile.misc.invert_lstick_yaxis)
            switch_report->input0x30.left_stick.InvertY();
        if (m_profile.misc.invert_rstick_xaxis)
            switch_report->input0x30.right_stick.InvertX();
        if (m_profile.misc.invert_rstick_yaxis)
            switch_report->input0x30.right_stick.InvertY();

        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, &m_input_report);
    }

    Result EmulatedSwitchController::HandleOutgoingReport(const bluetooth::HidReport *report) {
        auto report_data = reinterpret_cast<const SwitchReportData *>(&report->data);

        switch (report_data->id) {
            case 0x01:
                R_TRY(this->HandleSubCmdReport(report));
                break;
            case 0x10:
                R_TRY(this->HandleRumbleReport(report));
                break;
            default:
                break;
        }

        return ams::ResultSuccess();
    }

    Result EmulatedSwitchController::HandleSubCmdReport(const bluetooth::HidReport *report) {
        auto report_data = reinterpret_cast<const SwitchReportData *>(&report->data);

        switch (report_data->output0x01.subcmd.id) {
            case SubCmd_RequestDeviceInfo:
                R_TRY(this->SubCmdRequestDeviceInfo(report));
                break;
            case SubCmd_SetInputReportMode:
                R_TRY(this->SubCmdSetInputReportMode(report));
                break;
            case SubCmd_TriggersElapsedTime:
                R_TRY(this->SubCmdTriggersElapsedTime(report));
                break;
            case SubCmd_ResetPairingInfo:
                R_TRY(this->SubCmdResetPairingInfo(report));
                break;
            case SubCmd_SetShipPowerState:
                R_TRY(this->SubCmdSetShipPowerState(report));
                break;
            case SubCmd_SpiFlashRead:
                R_TRY(this->SubCmdSpiFlashRead(report));
                break;
            case SubCmd_SpiFlashWrite:
                R_TRY(this->SubCmdSpiFlashWrite(report));
                break;
            case SubCmd_SpiSectorErase:
                R_TRY(this->SubCmdSpiSectorErase(report));
                break;
            case SubCmd_SetMcuConfig:
                R_TRY(this->SubCmdSetMcuConfig(report));
                break;
            case SubCmd_SetMcuState:
                R_TRY(this->SubCmdSetMcuState(report));
                break;
            case SubCmd_0x24:
                R_TRY(this->SubCmd0x24(report));
                break;
            case SubCmd_0x25:
                R_TRY(this->SubCmd0x25(report));
                break;
            case SubCmd_SetPlayerLeds:
                R_TRY(this->SubCmdSetPlayerLeds(report));
                break;
            case SubCmd_GetPlayerLeds:
                R_TRY(this->SubCmdGetPlayerLeds(report));
                break;
            case SubCmd_SetHomeLed:
                R_TRY(this->SubCmdSetHomeLed(report));
                break;
            case SubCmd_EnableImu:
                R_TRY(this->SubCmdEnableImu(report));
                break;
            case SubCmd_EnableVibration:
                R_TRY(this->SubCmdEnableVibration(report));
                break;
            default:
                const SwitchSubcommandResponse response = {
                    .ack = 0x80,
                    .id = report_data->output0x01.subcmd.id,
                    .data = { 0x03 }
                };

                R_TRY(this->FakeSubCmdResponse(&response));
                break;
        }

        // This report can also contain rumble data
        if (m_enable_rumble) {
            SwitchRumbleData rumble_data[2];
            DecodeRumbleValues(report_data->output0x01.rumble.left_motor,  &rumble_data[0]);
            DecodeRumbleValues(report_data->output0x01.rumble.right_motor, &rumble_data[1]);
            R_TRY(this->SetVibration(rumble_data));
        }

        return ams::ResultSuccess();
    }

    Result EmulatedSwitchController::HandleRumbleReport(const bluetooth::HidReport *report) {
        if (m_enable_rumble) {
            auto report_data = reinterpret_cast<const SwitchReportData *>(report->data);

            SwitchRumbleData rumble_data[2];
            DecodeRumbleValues(report_data->output0x10.rumble.left_motor,  &rumble_data[0]);
            DecodeRumbleValues(report_data->output0x10.rumble.right_motor, &rumble_data[1]);
            R_TRY(this->SetVibration(rumble_data));
        }

        return ams::ResultSuccess();
    }

    Result EmulatedSwitchController::SubCmdRequestDeviceInfo(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x82,
            .id = SubCmd_RequestDeviceInfo,
            .device_info = {
                .fw_ver = {
                    .major = 0x03,
                    .minor = 0x48
                },
                .type = 0x03,
                ._unk0 = 0x02,
                .address = m_address,
                ._unk1 = 0x01,
                ._unk2 = 0x02
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetInputReportMode(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetInputReportMode
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdTriggersElapsedTime(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x83,
            .id = SubCmd_TriggersElapsedTime
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdResetPairingInfo(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        R_TRY(this->VirtualSpiFlashSectorErase(0x2000));

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_ResetPairingInfo
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetShipPowerState(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetShipPowerState,
            .set_ship_power_state = {
                .enabled = false
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSpiFlashRead(const bluetooth::HidReport *report) {
        // These are read from official Pro Controller
        // @ 0x00006000: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff                            <= Serial
        // @ 0x00006050: 32 32 32 ff ff ff ff ff ff ff ff ff                                        <= RGB colours (body, buttons, left grip, right grip)
        // @ 0x00006080: 50 fd 00 00 c6 0f 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63    <= Factory Sensor and Stick device parameters
        // @ 0x00006098: 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63                      <= Stick device parameters 2. Normally the same with 1, even in Pro Contr.
        // @ 0x00008010: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    <= User Analog sticks calibration
        // @ 0x0000603d: e6 a5 67 1a 58 78 50 56 60 1a f8 7f 20 c6 63 d5 15 5e ff 32 32 32 ff ff ff <= Analog stick factory calibration + face/button colours
        // @ 0x00006020: 64 ff 33 00 b8 01 00 40 00 40 00 40 17 00 d7 ff bd ff 3b 34 3b 34 3b 34    <= 6-Axis motion sensor Factory calibration

        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);
        auto read_addr = switch_report->output0x01.subcmd.spi_flash_read.address;
        auto read_size = switch_report->output0x01.subcmd.spi_flash_read.size;

        SwitchSubcommandResponse response = {
            .ack = 0x90,
            .id = SubCmd_SpiFlashRead,
            .spi_flash_read = {
                .address = read_addr,
                .size = read_size
            }
        };

        R_TRY(this->VirtualSpiFlashRead(read_addr, response.spi_flash_read.data, read_size));

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSpiFlashWrite(const bluetooth::HidReport *report) {
        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);
        auto write_addr = switch_report->output0x01.subcmd.spi_flash_write.address;
        auto write_size = switch_report->output0x01.subcmd.spi_flash_write.size;
        auto write_data = switch_report->output0x01.subcmd.spi_flash_write.data;

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SpiFlashWrite,
            .spi_flash_write = {
                .status = this->VirtualSpiFlashWrite(write_addr, write_data, write_size).IsFailure()
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSpiSectorErase(const bluetooth::HidReport *report) {
        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);
        auto erase_addr = switch_report->output0x01.subcmd.spi_flash_sector_erase.address;

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SpiSectorErase,
            .spi_sector_erase = {
                .status = this->VirtualSpiFlashSectorErase(erase_addr).IsFailure()
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmd0x24(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_0x24,
            .data = { 0x00 }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmd0x25(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_0x25,
            .data = { 0x00 }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetMcuConfig(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0xa0,
            .id = SubCmd_SetMcuConfig,
            .data = {0x01, 0x00, 0xff, 0x00, 0x03, 0x00, 0x05, 0x01,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x5c}
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetMcuState(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetMcuState
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetPlayerLeds(const bluetooth::HidReport *report) {
        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);

        m_led_pattern = switch_report->output0x01.subcmd.set_player_leds.leds;
        R_TRY(this->SetPlayerLed(m_led_pattern));

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetPlayerLeds
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdGetPlayerLeds(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_GetPlayerLeds,
            .get_player_leds = {
                .leds = m_led_pattern
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetHomeLed(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetHomeLed
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdEnableImu(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_EnableImu
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdEnableVibration(const bluetooth::HidReport *report) {
        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);

        m_enable_rumble = m_profile.general.enable_rumble & switch_report->output0x01.subcmd.set_vibration.enabled;

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_EnableVibration
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::FakeSubCmdResponse(const SwitchSubcommandResponse *response) {
        m_input_report.size = sizeof(SwitchInputReport0x21) + 1;
        auto report_data = reinterpret_cast<SwitchReportData *>(m_input_report.data);
        report_data->id = 0x21;
        report_data->input0x21.conn_info   = (0 << 1) | m_ext_power;
        report_data->input0x21.battery     = m_battery | m_charging;
        report_data->input0x21.buttons     = m_buttons;
        report_data->input0x21.left_stick  = m_left_stick;
        report_data->input0x21.right_stick = m_right_stick;
        report_data->input0x21.vibrator    = 0;
        std::memcpy(&report_data->input0x21.response, response, sizeof(SwitchSubcommandResponse));
        report_data->input0x21.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;

        //Write a fake response into the report buffer
        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, &m_input_report);
    }

    Result EmulatedSwitchController::VirtualSpiFlashRead(int offset, void *data, size_t size) {
        return fs::ReadFile(m_spi_flash_file, offset, data, size);
    }

    Result EmulatedSwitchController::VirtualSpiFlashWrite(int offset, const void *data, size_t size) {
        return fs::WriteFile(m_spi_flash_file, offset, data, size, fs::WriteOption::Flush);
    }

    Result EmulatedSwitchController::VirtualSpiFlashSectorErase(int offset) {
        uint8_t buff[64];
        std::memset(buff, 0xff, sizeof(buff));

        // Fill sector at offset with 0xff
        unsigned int sector_size = 0x1000;
        for (unsigned int i = 0; i < (sector_size / sizeof(buff)); ++i) {
            R_TRY(fs::WriteFile(m_spi_flash_file, offset, buff, sizeof(buff), fs::WriteOption::None));
            offset += sizeof(buff);
        }

        R_TRY(fs::FlushFile(m_spi_flash_file));

        return ams::ResultSuccess();
    }

}
