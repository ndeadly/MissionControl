/*
 * Copyright (c) 2020-2023 ndeadly
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

namespace ams::controller {

    namespace {

        // Frequency in Hz rounded to nearest int
        // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md#frequency-table
        const u16 rumble_freq_lut[] = {
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
        constexpr size_t rumble_freq_lut_size = sizeof(rumble_freq_lut) / sizeof(u16);

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

        void DecodeRumbleValues(const u8 enc[], SwitchRumbleData *dec) {
            u8 hi_freq_ind = 0x20 + (enc[0] >> 2) + ((enc[1] & 0x01) * 0x40) - 1;
            u8 hi_amp_ind  = (enc[1] & 0xfe) >> 1;
            u8 lo_freq_ind = (enc[2] & 0x7f) - 1;
            u8 lo_amp_ind  = ((enc[3] - 0x40) << 1) + ((enc[2] & 0x80) >> 7);

            if (!((hi_freq_ind < rumble_freq_lut_size) &&
                  (hi_amp_ind < rumble_amp_lut_f_size) &&
                  (lo_freq_ind < rumble_freq_lut_size) &&
                  (lo_amp_ind < rumble_amp_lut_f_size))) {
                std::memset(dec, 0, sizeof(SwitchRumbleData));
                return;
            }

            dec->high_band_freq = float(rumble_freq_lut[hi_freq_ind]);
            dec->high_band_amp  = rumble_amp_lut_f[hi_amp_ind];
            dec->low_band_freq  = float(rumble_freq_lut[lo_freq_ind]);
            dec->low_band_amp   = rumble_amp_lut_f[lo_amp_ind];
        }

        // CRC-8 with polynomial 0x7 for NFC/IR packets
        u8 crc8_lut[] = {
            0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
            0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
            0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
            0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
            0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
            0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
            0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
            0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
            0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
            0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
            0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
            0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
            0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
            0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
            0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
            0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
        };

        u8 ComputeCrc8(const void *data, size_t size) {
            auto *bytes = reinterpret_cast<const u8 *>(data);

            u8 crc = 0x00;
            for (size_t i = 0; i < size; ++i) {
                crc = crc8_lut[crc ^ bytes[i]];
            }
            return crc;
        }

    }

    EmulatedSwitchController::EmulatedSwitchController(const bluetooth::Address *address, HardwareID id)
    : SwitchController(address, id)
    , m_charging(false)
    , m_ext_power(false)
    , m_battery(BATTERY_MAX)
    , m_led_pattern(0)
    , m_gyro_sensitivity(2000)
    , m_acc_sensitivity(8000)
    , m_input_report_mode(0x30) {
        this->ClearControllerState();

        auto config = mitm::GetGlobalConfig();
        m_enable_rumble = config->general.enable_rumble;
        m_enable_motion = config->general.enable_motion;
        m_trigger_threshold = config->misc.analog_trigger_activation_threshold / 100.0;
    };

    Result EmulatedSwitchController::Initialize() {
        R_TRY(SwitchController::Initialize());

        // Ensure config directory for this controller exists
        std::string controller_dir = GetControllerDirectory(&m_address);
        R_TRY(fs::EnsureDirectory(controller_dir.c_str()));

        R_TRY(m_virtual_memory.Initialize((controller_dir + "/spi_flash.bin").c_str()));

        R_SUCCEED();
    }

    void EmulatedSwitchController::ClearControllerState() {
        std::memset(&m_buttons, 0, sizeof(m_buttons));
        m_left_stick.SetData(STICK_CENTER, STICK_CENTER);
        m_right_stick.SetData(STICK_CENTER, STICK_CENTER);
        std::memset(&m_motion_data, 0, sizeof(m_motion_data));
    }

    void EmulatedSwitchController::UpdateControllerState(const bluetooth::HidReport *report) {
        this->ProcessInputData(report);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = 0x30;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->conn_info = (0 << 1) | m_ext_power;
        input_report->battery = m_battery | m_charging;
        input_report->buttons = m_buttons;
        input_report->left_stick = m_left_stick;
        input_report->right_stick = m_right_stick;

        std::memcpy(&input_report->type0x30.motion_data, &m_motion_data, sizeof(m_motion_data));
        m_input_report.size = offsetof(SwitchInputReport, type0x30) + sizeof(input_report->type0x30);
    }

    Result EmulatedSwitchController::HandleOutputDataReport(const bluetooth::HidReport *report) {
        auto output_report = reinterpret_cast<const SwitchOutputReport *>(&report->data);

        switch (output_report->id) {
            case 0x01:
                R_TRY(this->HandleRumbleData(&output_report->rumble_data));
                R_TRY(this->HandleHidCommand(&output_report->type0x01.hid_command));
                break;
            case 0x10:
                R_TRY(this->HandleRumbleData(&output_report->rumble_data));
                break;
            case 0x11:
                R_TRY(this->HandleRumbleData(&output_report->rumble_data));
                //R_TRY(this->HandleNfcIrData(output_report->type0x11.nfc_ir_data));
                break;
            default:
                break;
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleRumbleData(const SwitchRumbleDataEncoded *encoded) {
        if (m_enable_rumble) {
            SwitchRumbleData rumble_data[2];
            DecodeRumbleValues(encoded->left_motor,  &rumble_data[0]);
            DecodeRumbleValues(encoded->right_motor, &rumble_data[1]);
            R_TRY(this->SetVibration(rumble_data));
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleHidCommand(const SwitchHidCommand *command) {
        switch (command->id) {
            case HidCommand_GetDeviceInfo:
                R_TRY(this->HandleHidCommandGetDeviceInfo(command));
                break;
            case HidCommand_SetDataFormat:
                R_TRY(this->HandleHidCommandSetDataFormat(command));
                break;
            case HidCommand_LRButtonDetection:
                R_TRY(this->HandleHidCommandLRButtonDetection(command));
                break;
            case HidCommand_ClearPairingInfo:
                R_TRY(this->HandleHidCommandClearPairingInfo(command));
                break;
            case HidCommand_Shipment:
                R_TRY(this->HandleHidCommandShipment(command));
                break;
            case HidCommand_SerialFlashRead:
                R_TRY(this->HandleHidCommandSerialFlashRead(command));
                break;
            case HidCommand_SerialFlashWrite:
                R_TRY(this->HandleHidCommandSerialFlashWrite(command));
                break;
            case HidCommand_SerialFlashSectorErase:
                R_TRY(this->HandleHidCommandSerialFlashSectorErase(command));
                break;
            case HidCommand_McuWrite:
                R_TRY(this->HandleHidCommandMcuWrite(command));
                break;
            case HidCommand_McuResume:
                R_TRY(this->HandleHidCommandMcuResume(command));
                break;
            case HidCommand_McuPollingEnable:
                R_TRY(this->HandleHidCommandMcuPollingEnable(command));
                break;
            case HidCommand_McuPollingDisable:
                R_TRY(this->HandleHidCommandMcuPollingDisable(command));
                break;
            case HidCommand_SetIndicatorLed:
                R_TRY(this->HandleHidCommandSetIndicatorLed(command));
                break;
            case HidCommand_GetIndicatorLed:
                R_TRY(this->HandleHidCommandGetIndicatorLed(command));
                break;
            case HidCommand_SetNotificationLed:
                R_TRY(this->HandleHidCommandSetNotificationLed(command));
                break;
            case HidCommand_SensorSleep:
                R_TRY(this->HandleHidCommandSensorSleep(command));
                break;
            case HidCommand_SensorConfig:
                R_TRY(this->HandleHidCommandSensorConfig(command));
                break;
            case HidCommand_MotorEnable:
                R_TRY(this->HandleHidCommandMotorEnable(command));
                break;
            default:
                const SwitchHidCommandResponse response = {
                    .ack = 0x80,
                    .id = command->id,
                    .data = {
                        .raw = { 0x03 }
                    }
                };

                R_TRY(this->FakeHidCommandResponse(&response));
                break;
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleHidCommandGetDeviceInfo(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x82,
            .id = command->id,
            .data = {
                .get_device_info = {
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
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetDataFormat(const SwitchHidCommand *command) {
        m_input_report_mode = command->set_data_format.id;

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandLRButtonDetection(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x83,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandClearPairingInfo(const SwitchHidCommand *command) {
        R_TRY(m_virtual_memory.SectorErase(0x2000));

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandShipment(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .shipment = {
                    .enabled = false
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSerialFlashRead(const SwitchHidCommand *command) {
        // These are read from official Pro Controller
        // @ 0x00006000: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff                            <= Serial
        // @ 0x00006050: 32 32 32 ff ff ff ff ff ff ff ff ff                                        <= RGB colours (body, buttons, left grip, right grip)
        // @ 0x00006080: 50 fd 00 00 c6 0f 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63    <= Factory Sensor and Stick device parameters
        // @ 0x00006098: 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63                      <= Stick device parameters 2. Normally the same with 1, even in Pro Contr.
        // @ 0x00008010: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    <= User Analog sticks calibration
        // @ 0x0000603d: e6 a5 67 1a 58 78 50 56 60 1a f8 7f 20 c6 63 d5 15 5e ff 32 32 32 ff ff ff <= Analog stick factory calibration + face/button colours
        // @ 0x00006020: 64 ff 33 00 b8 01 00 40 00 40 00 40 17 00 d7 ff bd ff 3b 34 3b 34 3b 34    <= 6-Axis motion sensor Factory calibration
        auto read_addr = command->serial_flash_read.address;
        auto read_size = command->serial_flash_read.size;

        SwitchHidCommandResponse response = {
            .ack = 0x90,
            .id = command->id,
            .data = {
                .serial_flash_read = {
                    .address = read_addr,
                    .size = read_size
                }
            }
        };

        R_TRY(m_virtual_memory.Read(read_addr, response.data.serial_flash_read.data, read_size));

        if (read_addr == 0x6050) {
            if (ams::mitm::GetSystemLanguage() == 10) {
                u8 data[] = {0xff, 0xd7, 0x00, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7};
                std::memcpy(response.data.serial_flash_read.data, data, sizeof(data));
            }
        }

        return this->FakeHidCommandResponse(&response);
    }

    Result EmulatedSwitchController::HandleHidCommandSerialFlashWrite(const SwitchHidCommand *command) {
        auto write_addr = command->serial_flash_write.address;
        auto write_size = command->serial_flash_write.size;
        auto write_data = command->serial_flash_write.data;

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .serial_flash_write = {
                    .status = m_virtual_memory.Write(write_addr, write_data, write_size).IsFailure()
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSerialFlashSectorErase(const SwitchHidCommand *command) {
        auto erase_addr = command->serial_flash_sector_erase.address;

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .serial_flash_sector_erase = {
                    .status = m_virtual_memory.SectorErase(erase_addr).IsFailure()
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuPollingEnable(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .raw = { 0x00 }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuPollingDisable(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .raw = { 0x00 }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuWrite(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0xa0,
            .id = command->id,
            .data = {
                .raw = {
                    0x01, 0x00, 0xff, 0x00, 0x03, 0x00, 0x05, 0x01,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x5c
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuResume(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetIndicatorLed(const SwitchHidCommand *command) {
        m_led_pattern = command->set_indicator_led.leds;
        R_TRY(this->SetPlayerLed(m_led_pattern));

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandGetIndicatorLed(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id,
            .data = {
                .get_indicator_led = {
                    .leds = m_led_pattern
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetNotificationLed(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSensorSleep(const SwitchHidCommand *command) {
        if (command->sensor_sleep.disabled) {
            if (!m_enable_motion) {
                m_gyro_sensitivity = 2000;
                m_acc_sensitivity = 8000;
            }
        }

        m_enable_motion = mitm::GetGlobalConfig()->general.enable_motion & command->sensor_sleep.disabled;

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSensorConfig(const SwitchHidCommand *command) {
        switch (command->sensor_config.gyro_sensitivity) {
            case 0: m_gyro_sensitivity = 250; break;
            case 1: m_gyro_sensitivity = 500; break;
            case 2: m_gyro_sensitivity = 1000; break;
            case 3: m_gyro_sensitivity = 2000; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        switch (command->sensor_config.acc_sensitivity) {
            case 0: m_acc_sensitivity = 8000; break;
            case 1: m_acc_sensitivity = 4000; break;
            case 2: m_acc_sensitivity = 2000; break;
            case 3: m_acc_sensitivity = 16000; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMotorEnable(const SwitchHidCommand *command) {
        m_enable_rumble = mitm::GetGlobalConfig()->general.enable_rumble & command->motor_enable.enabled;

        const SwitchHidCommandResponse response = {
            .ack = 0x80,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::FakeHidCommandResponse(const SwitchHidCommandResponse *response) {
        std::scoped_lock lk(m_input_mutex);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = 0x21;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->conn_info = (0 << 1) | m_ext_power;
        input_report->battery = m_battery | m_charging;
        input_report->buttons = m_buttons;
        input_report->left_stick = m_left_stick;
        input_report->right_stick = m_right_stick;
        input_report->vibrator = 0;

        std::memcpy(&input_report->type0x21.hid_command_response, response, sizeof(SwitchHidCommandResponse));
        m_input_report.size = offsetof(SwitchInputReport, type0x21) + sizeof(input_report->type0x21);

        // Write a fake response into the report buffer
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report));
    }

    Result EmulatedSwitchController::HandleNfcIrData(const u8 *nfc_ir) {
        AMS_UNUSED(nfc_ir);

        SwitchNfcIrResponse response = {};

        // Send device not ready response for now
        response.data[0] = 0xff;

        R_RETURN(this->FakeNfcIrResponse(&response));
    }

    Result EmulatedSwitchController::FakeNfcIrResponse(const SwitchNfcIrResponse *response) {
        std::scoped_lock lk(m_input_mutex);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = 0x31;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->conn_info = (0 << 1) | m_ext_power;
        input_report->battery = m_battery | m_charging;
        input_report->buttons = m_buttons;
        input_report->left_stick = m_left_stick;
        input_report->right_stick = m_right_stick;
        input_report->vibrator = 0;

        std::memcpy(&input_report->type0x31.motion_data, &m_motion_data, sizeof(m_motion_data));
        std::memcpy(&input_report->type0x31.nfc_ir_response, response, sizeof(SwitchNfcIrResponse));
        input_report->type0x31.crc = ComputeCrc8(response, sizeof(SwitchNfcIrResponse));
        m_input_report.size = offsetof(SwitchInputReport, type0x31) + sizeof(input_report->type0x31);

        // Write a fake response into the report buffer
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report));
    }

}
