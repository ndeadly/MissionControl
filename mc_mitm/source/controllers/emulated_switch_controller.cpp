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
#include "../mcmitm_config.hpp"
#include <memory>

namespace ams::controller {

    namespace {

        // Frequency in Hz rounded to nearest int
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

        // Amplitude range scaled between 0-255
        /*
        const uint8_t rumble_amp_lut[] = {
            0x00, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x09, 0x0a, 0x0c, 0x0e,
            0x11, 0x14, 0x18, 0x1d, 0x1e, 0x1f, 0x21, 0x22, 0x24, 0x25, 0x27, 0x29,
            0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x35, 0x37, 0x39, 0x3b, 0x3c, 0x3d, 0x3f,
            0x40, 0x41, 0x43, 0x44, 0x46, 0x47, 0x49, 0x4a, 0x4c, 0x4e, 0x4f, 0x51,
            0x53, 0x55, 0x57, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x63, 0x65, 0x67, 0x69,
            0x6c, 0x6e, 0x70, 0x73, 0x75, 0x78, 0x7a, 0x7d, 0x80, 0x83, 0x86, 0x88,
            0x8b, 0x8e, 0x92, 0x95, 0x98, 0x9b, 0x9f, 0xa2, 0xa6, 0xa9, 0xad, 0xb1,
            0xb5, 0xb9, 0xbd, 0xc1, 0xc5, 0xca, 0xce, 0xd2, 0xd7, 0xdc, 0xe1, 0xe5,
            0xeb, 0xf0, 0xf5, 0xfa, 0xff
        };
        */

        // Amplitude range with scaling function applied
        // y = static_cast<uint8_t>(((x + powf(x, 0.2)) / 2.0) * 255.0)
        const uint8_t rumble_amp_lut[] = {
            0x00, 0x31, 0x35, 0x38, 0x3a, 0x3c, 0x3f, 0x41, 0x44, 0x47, 0x4b, 0x4e,
            0x52, 0x56, 0x5b, 0x60, 0x61, 0x63, 0x64, 0x66, 0x67, 0x69, 0x6a, 0x6c,
            0x6e, 0x6f, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
            0x80, 0x81, 0x82, 0x83, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8b, 0x8c, 0x8d,
            0x8f, 0x90, 0x91, 0x93, 0x94, 0x96, 0x97, 0x99, 0x9a, 0x9c, 0x9d, 0x9f,
            0xa0, 0xa2, 0xa4, 0xa5, 0xa7, 0xa9, 0xab, 0xac, 0xae, 0xb0, 0xb2, 0xb4,
            0xb6, 0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc3, 0xc5, 0xc7, 0xc9, 0xcc, 0xce,
            0xd1, 0xd3, 0xd6, 0xd8, 0xdb, 0xde, 0xe0, 0xe3, 0xe6, 0xe9, 0xec, 0xef,
            0xf2, 0xf5, 0xf8, 0xfb, 0xff, 
        };

        // Raw floats from dekunukem github
        /*
        const float rumble_amp_lut_f[] = {
            0.000000, 0.007843, 0.011823, 0.014061, 0.016720, 0.019885, 0.023648, 
            0.028123, 0.033442, 0.039771, 0.047296, 0.056246, 0.066886, 0.079542, 
            0.094592, 0.112491, 0.117471, 0.122671, 0.128102, 0.133774, 0.139697, 
            0.145882, 0.152341, 0.159085, 0.166129, 0.173484, 0.181166, 0.189185, 
            0.197561, 0.206308, 0.215442, 0.224982, 0.229908, 0.234943, 0.240087, 
            0.245345, 0.250715, 0.256206, 0.261816, 0.267549, 0.273407, 0.279394, 
            0.285514, 0.291765, 0.298154, 0.304681, 0.311353, 0.318171, 0.325138, 
            0.332258, 0.339534, 0.346969, 0.354566, 0.362331, 0.370265, 0.378372, 
            0.386657, 0.395124, 0.403777, 0.412619, 0.421652, 0.430885, 0.440321, 
            0.449964, 0.459817, 0.469885, 0.480174, 0.490689, 0.501433, 0.512413, 
            0.523633, 0.535100, 0.546816, 0.558790, 0.571027, 0.583530, 0.596307, 
            0.609365, 0.622708, 0.636344, 0.650279, 0.664518, 0.679069, 0.693939, 
            0.709133, 0.724662, 0.740529, 0.756745, 0.773316, 0.790249, 0.807554, 
            0.825237, 0.843307, 0.861772, 0.880643, 0.899928, 0.919633, 0.939771, 
            0.960348, 0.981378, 1.002867
        };
        */

        // Above floats scaled by yuzu function
        const float rumble_amp_lut_f[] = {
            0.000000, 0.193414, 0.211610, 0.219983, 0.228816, 0.238172, 0.248099,
            0.258665, 0.269941, 0.282030, 0.295028, 0.309064, 0.324277, 0.340847,
            0.358972, 0.378893, 0.384185, 0.389610, 0.395176, 0.400887, 0.406749,
            0.412766, 0.418945, 0.425292, 0.431815, 0.438519, 0.445413, 0.452500,
            0.459793, 0.467298, 0.475023, 0.482979, 0.487045, 0.491172, 0.495360,
            0.499613, 0.503928, 0.508311, 0.512760, 0.517278, 0.521865, 0.526524,
            0.531257, 0.536062, 0.540943, 0.545900, 0.550937, 0.556054, 0.561253,
            0.566536, 0.571904, 0.577359, 0.582902, 0.588537, 0.594264, 0.600084,
            0.606001, 0.612016, 0.618132, 0.624351, 0.630671, 0.637100, 0.643638,
            0.650287, 0.657049, 0.663926, 0.670921, 0.678037, 0.685275, 0.692639,
            0.700131, 0.707754, 0.715510, 0.723403, 0.731435, 0.739608, 0.747926,
            0.756393, 0.765010, 0.773782, 0.782712, 0.791802, 0.801056, 0.810477,
            0.820069, 0.829837, 0.839782, 0.849910, 0.860224, 0.870727, 0.881424,
            0.892319, 0.903416, 0.914719, 0.926234, 0.937964, 0.949912, 0.962086,
            0.974488, 0.987125, 1.000000
        };

        inline void DecodeRumbleValues(const uint8_t enc[], SwitchRumbleData *dec) {
            uint8_t hi_freq_ind = 0x20 + (enc[0] >> 2) + ((enc[1] & 0x01) * 0x40) - 1;
            uint8_t hi_amp_ind  = (enc[1] & 0xfe) >> 1;
            uint8_t lo_freq_ind = (enc[2] & 0x7f) - 1;;
            uint8_t lo_amp_ind  = ((enc[3] - 0x40) << 1) + ((enc[2] & 0x80) >> 7);

            dec->high_band_freq = float(rumble_freq_lut[hi_freq_ind]);
            dec->high_band_amp  = rumble_amp_lut_f[hi_amp_ind];
            dec->low_band_freq  = float(rumble_freq_lut[lo_freq_ind]);
            dec->low_band_amp   = rumble_amp_lut_f[lo_amp_ind];
        }

    }

    EmulatedSwitchController::EmulatedSwitchController(const bluetooth::Address *address) 
    : SwitchController(address)
    , m_charging(false)
    , m_battery(BATTERY_MAX) { 
        this->ClearControllerState();

        m_colours.body       = {0x32, 0x32, 0x32};
        m_colours.buttons    = {0xe6, 0xe6, 0xe6};
        m_colours.left_grip  = {0x46, 0x46, 0x46};
        m_colours.right_grip = {0x46, 0x46, 0x46};

        auto config = mitm::GetGlobalConfig();

        m_enable_rumble = config->general.enable_rumble;
    };

    void EmulatedSwitchController::ClearControllerState(void) {
        std::memset(&m_buttons, 0, sizeof(m_buttons));
        m_left_stick = this->PackStickData(STICK_ZERO, STICK_ZERO);
        m_right_stick = this->PackStickData(STICK_ZERO, STICK_ZERO);
        std::memset(&m_motion_data, 0, sizeof(m_motion_data));
    }

    Result EmulatedSwitchController::HandleIncomingReport(const bluetooth::HidReport *report) {
        this->UpdateControllerState(report);

        // Prepare Switch report
        s_input_report.size = sizeof(SwitchInputReport0x30) + 1;
        auto switch_report = reinterpret_cast<SwitchReportData *>(s_input_report.data);
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info      = 0;
        switch_report->input0x30.battery        = m_battery | m_charging;
        switch_report->input0x30.buttons        = m_buttons;
        switch_report->input0x30.left_stick     = m_left_stick;
        switch_report->input0x30.right_stick    = m_right_stick;
        std::memcpy(&switch_report->input0x30.motion, &m_motion_data, sizeof(m_motion_data));

        this->ApplyButtonCombos(&switch_report->input0x30.buttons);

        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, &s_input_report);
    }

    Result EmulatedSwitchController::HandleOutgoingReport(const bluetooth::HidReport *report) {
        uint8_t cmdId = report->data[0];
        switch (cmdId) {
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
        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);

        switch (switch_report->output0x01.subcmd.id) {
            case SubCmd_RequestDeviceInfo:
                R_TRY(this->SubCmdRequestDeviceInfo(report));
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
            case SubCmd_SetInputReportMode:
                R_TRY(this->SubCmdSetInputReportMode(report));
                break;
            case SubCmd_TriggersElapsedTime:
                R_TRY(this->SubCmdTriggersElapsedTime(report));
                break;
            case SubCmd_SetShipPowerState:
                R_TRY(this->SubCmdSetShipPowerState(report));
                break;
            case SubCmd_SetMcuConfig:
                R_TRY(this->SubCmdSetMcuConfig(report));
                break;
            case SubCmd_SetMcuState:
                R_TRY(this->SubCmdSetMcuState(report));
                break;
            case SubCmd_SetPlayerLeds:
                R_TRY(this->SubCmdSetPlayerLeds(report));
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
                break;
        }

        return ams::ResultSuccess();
    }

    Result EmulatedSwitchController::HandleRumbleReport(const bluetooth::HidReport *report) {
        R_SUCCEED_IF(!m_enable_rumble);

        auto report_data = reinterpret_cast<const SwitchReportData *>(report->data);
        
        SwitchRumbleData left_motor;
        DecodeRumbleValues(report_data->output0x10.left_motor, &left_motor);

        SwitchRumbleData right_motor;
        DecodeRumbleValues(report_data->output0x10.right_motor, &right_motor);

        return this->SetVibration(&left_motor, &right_motor);
    }

    Result EmulatedSwitchController::SubCmdRequestDeviceInfo(const bluetooth::HidReport *report) {
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

    Result EmulatedSwitchController::SubCmdSpiFlashRead(const bluetooth::HidReport *report) {
        // These are read from official Pro Controller
        // @ 0x00006000: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff                            <= Serial 
        // @ 0x00006050: 32 32 32 ff ff ff ff ff ff ff ff ff                                        <= RGB colours (body, buttons, left grip, right grip)
        // @ 0x00006080: 50 fd 00 00 c6 0f 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63    <= Factory Sensor and Stick device parameters
        // @ 0x00006098: 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63                      <= Stick device parameters 2. Normally the same with 1, even in Pro Contr.
        // @ 0x00008010: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    <= User Analog sticks calibration
        // @ 0x0000603d: e6 a5 67 1a 58 78 50 56 60 1a f8 7f 20 c6 63 d5 15 5e ff 32 32 32 ff ff ff <= Left analog stick calibration
        // @ 0x00006020: 64 ff 33 00 b8 01 00 40 00 40 00 40 17 00 d7 ff bd ff 3b 34 3b 34 3b 34    <= 6-Axis motion sensor Factory calibration

        auto switch_report = reinterpret_cast<const SwitchReportData *>(&report->data);
        uint32_t read_addr = switch_report->output0x01.subcmd.spi_flash_read.address;
        uint8_t  read_size = switch_report->output0x01.subcmd.spi_flash_read.size;

        SwitchSubcommandResponse response = {
            .ack = 0x90,
            .id = SubCmd_SpiFlashRead,
            .spi_flash_read = {
                .address = read_addr,
                .size = read_size
            }
        };

        if (read_addr == 0x6050) {
            std::memcpy(response.spi_flash_read.data, &m_colours, sizeof(m_colours)); // Set controller colours
        }
        else {
            std::memset(response.spi_flash_read.data, 0xff, read_size); // Console doesn't seem to mind if response is uninitialised data (0xff)
        }

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSpiFlashWrite(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SpiFlashWrite,
            .spi_flash_write = {
                .status = 0x01
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSpiSectorErase(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SpiSectorErase,
            .spi_flash_write = {
                .status = 0x01
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetInputReportMode(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetInputReportMode
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdTriggersElapsedTime(const bluetooth::HidReport *report) {       
        const SwitchSubcommandResponse response = {
            .ack = 0x83,
            .id = SubCmd_TriggersElapsedTime
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetShipPowerState(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetShipPowerState,
            .set_ship_power_state = {
                .enabled = false
            }
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetMcuConfig(const bluetooth::HidReport *report) {
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
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetMcuState
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetPlayerLeds(const bluetooth::HidReport *report) {
        const uint8_t *subCmd = &report->data[10];
        uint8_t led_mask = subCmd[1];
        R_TRY(this->SetPlayerLed(led_mask));

        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetPlayerLeds
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdSetHomeLed(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_SetHomeLed
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdEnableImu(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_EnableImu
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::SubCmdEnableVibration(const bluetooth::HidReport *report) {
        const SwitchSubcommandResponse response = {
            .ack = 0x80,
            .id = SubCmd_EnableVibration
        };

        return this->FakeSubCmdResponse(&response);
    }

    Result EmulatedSwitchController::FakeSubCmdResponse(const SwitchSubcommandResponse *response) {
        s_input_report.size = sizeof(SwitchInputReport0x21) + 1;
        auto report_data = reinterpret_cast<SwitchReportData *>(s_input_report.data);
        report_data->id = 0x21;
        report_data->input0x21.conn_info   = 0;
        report_data->input0x21.battery     = m_battery | m_charging;
        report_data->input0x21.buttons     = m_buttons;
        report_data->input0x21.left_stick  = m_left_stick;
        report_data->input0x21.right_stick = m_right_stick;
        report_data->input0x21.vibrator    = 0;
        std::memcpy(&report_data->input0x21.response, response, sizeof(SwitchSubcommandResponse));
        report_data->input0x21.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;

        //Write a fake response into the report buffer
        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, &s_input_report);
    }

}
