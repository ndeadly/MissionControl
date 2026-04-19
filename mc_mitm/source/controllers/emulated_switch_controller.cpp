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
#include "emulated_switch_controller.hpp"
#include "../utils.hpp"
#include "../mcmitm_config.hpp"

namespace ams::controller {

    namespace {

        constexpr u8 ProControllerFirmwareVersionMajor = 0x04;
        constexpr u8 ProControllerFirmwareVersionMinor = 0x21;

        constexpr SwitchMcuResponse EmptyMcuResponse = {
            .command = SwitchMcuCommandId::EmptyAwaitingCmd,
            .data = {}
        };

        // CRC-8 with polynomial 0x7 for NFC/IR packets
        constexpr u8 ComputeCrc8(const void *data, size_t size) {
            return utils::Crc8<7>::Calculate(data, size);
        }

    }

    EmulatedSwitchController::EmulatedSwitchController(bluetooth::Address address, HardwareID id) : SwitchController(address, id)
    , m_power_info(false, 0, false, SwitchBatteryLevel::Full)
    , m_input_report_mode(SwitchHidReportId::BasicInputReport) {
        this->ClearControllerState();

        auto config = mitm::GetGlobalConfig();
        m_enable_rumble = config->general.enable_rumble;
        m_enable_motion = config->general.enable_motion;
        m_trigger_threshold = config->misc.analog_trigger_activation_threshold / 100.0;
    };

    Result EmulatedSwitchController::Initialize() {
        R_TRY(SwitchController::Initialize());

        // Ensure config directory for this controller exists
        std::string controller_dir = GetControllerDirectory(m_address);
        R_TRY(fs::EnsureDirectory(controller_dir.c_str()));

        R_TRY(m_virtual_memory.Initialize((controller_dir + "/spi_flash.bin").c_str()));

        R_SUCCEED();
    }

    void EmulatedSwitchController::ClearControllerState() {
        std::memset(&m_buttons, 0, sizeof(m_buttons));
        m_left_stick.Reset();
        m_right_stick.Reset();
        std::memset(&m_accel, 0, sizeof(m_accel));
        std::memset(&m_gyro, 0, sizeof(m_gyro));
        m_motion_packer->SetGyroSensitivity(GyroSensitivity_2000Dps);
        m_motion_packer->SetAccelSensitivity(AccelSensitivity_8G);
    }

    void EmulatedSwitchController::UpdateControllerState(const bluetooth::HidReport *report) {
        this->ProcessInputData(report);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = m_input_report_mode;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->power_info = m_power_info.GetState();
        input_report->buttons = m_buttons;
        input_report->left_stick  = m_left_stick.GetState();
        input_report->right_stick = m_right_stick.GetState();

        switch (m_input_report_mode) {
            case SwitchHidReportId::McuInputReport:
                m_motion_packer->PackData(&input_report->type0x31.motion_data, m_accel, m_gyro);

                std::memcpy(&input_report->type0x31.mcu_response, &EmptyMcuResponse, sizeof(EmptyMcuResponse));
                input_report->type0x31.crc = ComputeCrc8(&EmptyMcuResponse, sizeof(SwitchMcuResponse));
                m_input_report.size = offsetof(SwitchInputReport, type0x31) + sizeof(input_report->type0x31);
                break;
            default:
                m_motion_packer->PackData(&input_report->type0x30.motion_data, m_accel, m_gyro);
                m_input_report.size = offsetof(SwitchInputReport, type0x30) + sizeof(input_report->type0x30);
                break;
        }
    }

    Result EmulatedSwitchController::HandleOutputDataReport(const bluetooth::HidReport *report) {
        auto output_report = reinterpret_cast<const SwitchOutputReport *>(&report->data);

        switch (output_report->id) {
            case SwitchHidReportId::CommandOutputReport:
                R_TRY(this->HandleRumbleData(&output_report->enc_motor_data));
                R_TRY(this->HandleHidCommand(&output_report->type0x01.hid_command));
                break;
            case SwitchHidReportId::BasicOutputReport:
                R_TRY(this->HandleRumbleData(&output_report->enc_motor_data));
                break;
            case SwitchHidReportId::McuOutputReport:
                R_TRY(this->HandleRumbleData(&output_report->enc_motor_data));
                R_TRY(this->HandleMcuCommand(&output_report->type0x11.mcu_command));
                break;
            default:
                break;
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleRumbleData(const SwitchEncodedMotorData *encoded_motor_data) {
        if (m_enable_rumble) {
            SwitchMotorData motor_data;
            if (m_rumble_handler.GetDecodedValues(encoded_motor_data, &motor_data)) {
                R_TRY(this->SetVibration(&motor_data));
            }
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleHidCommand(const SwitchHidCommand *command) {
        switch (command->id) {
            case SwitchHidCommandId::GetDeviceInfo:
                R_TRY(this->HandleHidCommandGetDeviceInfo(command));
                break;
            case SwitchHidCommandId::SetDataFormat:
                R_TRY(this->HandleHidCommandSetDataFormat(command));
                break;
            case SwitchHidCommandId::LRButtonDetection:
                R_TRY(this->HandleHidCommandLRButtonDetection(command));
                break;
            case SwitchHidCommandId::ClearPairingInfo:
                R_TRY(this->HandleHidCommandClearPairingInfo(command));
                break;
            case SwitchHidCommandId::Shipment:
                R_TRY(this->HandleHidCommandShipment(command));
                break;
            case SwitchHidCommandId::SerialFlashRead:
                R_TRY(this->HandleHidCommandSerialFlashRead(command));
                break;
            case SwitchHidCommandId::SerialFlashWrite:
                R_TRY(this->HandleHidCommandSerialFlashWrite(command));
                break;
            case SwitchHidCommandId::SerialFlashSectorErase:
                R_TRY(this->HandleHidCommandSerialFlashSectorErase(command));
                break;
            case SwitchHidCommandId::McuWrite:
                R_TRY(this->HandleHidCommandMcuWrite(command));
                break;
            case SwitchHidCommandId::McuResume:
                R_TRY(this->HandleHidCommandMcuResume(command));
                break;
            case SwitchHidCommandId::McuPollingEnable:
                R_TRY(this->HandleHidCommandMcuPollingEnable(command));
                break;
            case SwitchHidCommandId::McuPollingDisable:
                R_TRY(this->HandleHidCommandMcuPollingDisable(command));
                break;
            case SwitchHidCommandId::SetIndicatorLed:
                R_TRY(this->HandleHidCommandSetIndicatorLed(command));
                break;
            case SwitchHidCommandId::GetIndicatorLed:
                R_TRY(this->HandleHidCommandGetIndicatorLed(command));
                break;
            case SwitchHidCommandId::SetNotificationLed:
                R_TRY(this->HandleHidCommandSetNotificationLed(command));
                break;
            case SwitchHidCommandId::SensorSleep:
                R_TRY(this->HandleHidCommandSensorSleep(command));
                break;
            case SwitchHidCommandId::SensorConfig:
                R_TRY(this->HandleHidCommandSensorConfig(command));
                break;
            case SwitchHidCommandId::MotorEnable:
                R_TRY(this->HandleHidCommandMotorEnable(command));
                break;
            default:
                const SwitchHidCommandResponse response = {
                    .ack = SwitchHidCommandAck::Default,
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
            .ack = SwitchHidCommandAck::DeviceInfo,
            .id = command->id,
            .data = {
                .get_device_info = {
                    .fw_ver = {
                        .major = ProControllerFirmwareVersionMajor,
                        .minor = ProControllerFirmwareVersionMinor
                    },
                    .type = 0x06, // 0x03,
                    ._unk0 = 0x02,
                    .address = m_address,
                    .sensor_type = SwitchImuSensorType::LSM6DS3H,
                    .format_version = 0x02
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetDataFormat(const SwitchHidCommand *command) {
        m_input_report_mode = command->set_data_format.id;

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandLRButtonDetection(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::LRButtonElapsedTime,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandClearPairingInfo(const SwitchHidCommand *command) {
        R_TRY(m_virtual_memory.SectorErase(0x2000));

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandShipment(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
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
            .ack = SwitchHidCommandAck::SerialFlashData,
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
                const u8 data[] = { 0xff, 0xd7, 0x00, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7 };
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
            .ack = SwitchHidCommandAck::Default,
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
            .ack = SwitchHidCommandAck::Default,
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
            .ack = SwitchHidCommandAck::Default,
            .id = command->id,
            .data = {
                .raw = { 0x00 }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuPollingDisable(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id,
            .data = {
                .raw = { 0x00 }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuWrite(const SwitchHidCommand *command) {
        switch (command->mcu_write.command){
            case SwitchMcuCommandId::ConfigureMcu:
                return this->HandleHidCommandConfigureMcu(command);
            default:
                break;
        }
    
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::McuData,
            .id = command->id,
            .data = {
                .raw = {// This looks a lot like mcu get status
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
    
    Result EmulatedSwitchController::HandleHidCommandConfigureMcu(const SwitchHidCommand *command) {
        SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::McuData,
            .id = command->id,
        };

        m_mcu_command_processor.GetStatus(&response.data);
        m_mcu_command_processor.McuConfigure(command->mcu_write.data.configure_mcu.mode);

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMcuResume(const SwitchHidCommand *command) {
        m_mcu_command_processor.McuResume(command->mcu_resume.enabled);

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetIndicatorLed(const SwitchHidCommand *command) {
        m_player_indicator.SetState(command->set_indicator_led.led_state);
        R_TRY(this->SetPlayerLed(m_player_indicator.GetLedPattern()));

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandGetIndicatorLed(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id,
            .data = {
                .get_indicator_led = {
                    .led_state = m_player_indicator.GetState()
                }
            }
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSetNotificationLed(const SwitchHidCommand *command) {
        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSensorSleep(const SwitchHidCommand* command) {
        m_enable_motion = mitm::GetGlobalConfig()->general.enable_motion;

        GyroSensitivity gyro_sensitivity = m_motion_packer->GetGyroSensitivity();
        AccelSensitivity accel_sensitivity = m_motion_packer->GetAccelSensitivity();

        if (m_enable_motion) {
            switch (command->sensor_sleep.mode) {
                case SwitchImuSensorSleepMode::Active:
                    m_motion_packer = std::make_unique<StandardMotionPacker>();
                    break;

                case SwitchImuSensorSleepMode::ActiveDscaleMode1:
                case SwitchImuSensorSleepMode::ActiveDscaleMode2:
                case SwitchImuSensorSleepMode::ActiveDscaleMode3:
                case SwitchImuSensorSleepMode::ActiveDscaleMode4:
                    m_motion_packer = std::make_unique<QuaternionMotionPacker>();
                    break;

                default:
                    m_motion_packer = std::make_unique<NullMotionPacker>();
                    break;
            }
        } else {
            m_motion_packer = std::make_unique<NullMotionPacker>();
        }

        m_motion_packer->SetGyroSensitivity(gyro_sensitivity);
        m_motion_packer->SetAccelSensitivity(accel_sensitivity);

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandSensorConfig(const SwitchHidCommand *command) {
        m_motion_packer->SetGyroSensitivity(command->sensor_config.gyro_sensitivity);
        m_motion_packer->SetAccelSensitivity(command->sensor_config.accel_sensitivity);

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::HandleHidCommandMotorEnable(const SwitchHidCommand *command) {
        m_enable_rumble = mitm::GetGlobalConfig()->general.enable_rumble & command->motor_enable.enabled;

        const SwitchHidCommandResponse response = {
            .ack = SwitchHidCommandAck::Default,
            .id = command->id
        };

        R_RETURN(this->FakeHidCommandResponse(&response));
    }

    Result EmulatedSwitchController::FakeHidCommandResponse(const SwitchHidCommandResponse *response) {
        std::scoped_lock lk(m_input_mutex);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = SwitchHidReportId::CommandInputReport;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->power_info = m_power_info.GetState();
        input_report->buttons = m_buttons;
        input_report->left_stick  = m_left_stick.GetState();
        input_report->right_stick = m_right_stick.GetState();
        input_report->vibrator = 0;

        std::memcpy(&input_report->type0x21.hid_command_response, response, sizeof(SwitchHidCommandResponse));
        m_input_report.size = offsetof(SwitchInputReport, type0x21) + sizeof(input_report->type0x21);

        // Write a fake response into the report buffer
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report));
    }

    Result EmulatedSwitchController::HandleMcuCommand(const SwitchMcuCommand *command) {
        SwitchMcuResponse response = {};
        m_mcu_command_processor.ProcessCommand(command, &response);

        R_RETURN(this->FakeMcuResponse(&response));
    }

    Result EmulatedSwitchController::FakeMcuResponse(const SwitchMcuResponse *response) {
        std::scoped_lock lk(m_input_mutex);

        auto input_report = reinterpret_cast<SwitchInputReport *>(m_input_report.data);
        input_report->id = SwitchHidReportId::McuInputReport;
        input_report->timer = (input_report->timer + 1) & 0xff;
        input_report->power_info = m_power_info.GetState();
        input_report->buttons = m_buttons;
        input_report->left_stick  = m_left_stick.GetState();
        input_report->right_stick = m_right_stick.GetState();
        input_report->vibrator = 0;

        m_motion_packer->PackData(&input_report->type0x31.motion_data, m_accel, m_gyro);
        std::memcpy(&input_report->type0x31.mcu_response, response, sizeof(SwitchMcuResponse));
        input_report->type0x31.crc = ComputeCrc8(response, sizeof(SwitchMcuResponse));
        m_input_report.size = offsetof(SwitchInputReport, type0x31) + sizeof(input_report->type0x31);

        // Write a fake response into the report buffer
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, &m_input_report));
    }

}
