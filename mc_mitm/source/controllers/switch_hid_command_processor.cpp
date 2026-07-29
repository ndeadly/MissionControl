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
#include "switch_hid_command_processor.hpp"
#include "emulated_switch_controller.hpp"
#include "../mcmitm_config.hpp"
#include "../utils.hpp"

namespace ams::controller {

    void SwitchHidCommandProcessor::ProcessCommand(const SwitchHidCommand *command) {
        SwitchHidCommandResponse response = {};

        switch (command->id) {
            case SwitchHidCommandId::PairingOut:             this->ProcessPairingOut(command, &response);             break;
            case SwitchHidCommandId::GetDeviceInfo:          this->ProcessGetDeviceInfo(command, &response);          break;
            case SwitchHidCommandId::SetDataFormat:          this->ProcessSetDataFormat(command, &response);          break;
            case SwitchHidCommandId::LRButtonDetection:      this->ProcessLRButtonDetection(command, &response);      break;
            case SwitchHidCommandId::ClearPairingInfo:       this->ProcessClearPairingInfo(command, &response);       break;
            case SwitchHidCommandId::Shipment:               this->ProcessShipment(command, &response);               break;
            case SwitchHidCommandId::SerialFlashRead:        this->ProcessSerialFlashRead(command, &response);        break;
            case SwitchHidCommandId::SerialFlashWrite:       this->ProcessSerialFlashWrite(command, &response);       break;
            case SwitchHidCommandId::SerialFlashSectorErase: this->ProcessSerialFlashSectorErase(command, &response); break;
            case SwitchHidCommandId::McuWrite:               this->ProcessMcuWrite(command, &response);               break;
            case SwitchHidCommandId::McuResume:              this->ProcessMcuResume(command, &response);              break;
            case SwitchHidCommandId::McuPollingEnable:       this->ProcessMcuPollingEnable(command, &response);       break;
            case SwitchHidCommandId::McuPollingDisable:      this->ProcessMcuPollingDisable(command, &response);      break;
            case SwitchHidCommandId::SetIndicatorLed:        this->ProcessSetIndicatorLed(command, &response);        break;
            case SwitchHidCommandId::GetIndicatorLed:        this->ProcessGetIndicatorLed(command, &response);        break;
            case SwitchHidCommandId::SetNotificationLed:     this->ProcessSetNotificationLed(command, &response);     break;
            case SwitchHidCommandId::SensorSleep:            this->ProcessSensorSleep(command, &response);            break;
            case SwitchHidCommandId::SensorConfig:           this->ProcessSensorConfig(command, &response);           break;
            case SwitchHidCommandId::MotorEnable:            this->ProcessMotorEnable(command, &response);            break;
            default:
                response.response_id = SwitchHidCommandId::Ack;
                response.request_id  = command->id;
                response.data[0] = 0x03;
                break;
        }

        m_response = response;
    }

    void SwitchHidCommandProcessor::ProcessPairingOut(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        response->response_id = SwitchHidCommandId::PairingIn;
        response->request_id  = SwitchHidCommandId::PairingOut;

        switch (command->pairing.stage) {
            case 0x01:
                response->pairing.stage = 0x01;
                std::strcpy(response->pairing.stage1.name, m_controller->m_device_info.device_name);
                response->pairing.stage1.address = utils::BluetoothAddressReverse(m_controller->m_address);
                response->pairing.stage1.device_class = { 0x00, 0x25, 0x08 };
                response->pairing.stage1.services = 0x68;
                break;

            case 0x02:
                response->pairing.stage = 0x02;
                std::memset(response->pairing.stage2.link_key, 0xAA, sizeof(response->pairing.stage2.link_key));
                break;

            case 0x03:
                response->pairing.stage = 0x03;
                break;

            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void SwitchHidCommandProcessor::ProcessGetDeviceInfo(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        auto device_info = &m_controller->m_device_info;

        response->response_id = SwitchHidCommandId::DeviceInfo;
        response->request_id  = SwitchHidCommandId::GetDeviceInfo;
        response->get_device_info = {
            .firmware_version_major = device_info->bt_firmware.major,
            .firmware_version_minor = device_info->bt_firmware.minor,
            .device_type            = device_info->device_type,
            .unk                    = device_info->unk,
            .address                = m_controller->m_address,
            .sensor_type            = device_info->sensor_type,
            .format_version         = device_info->format_version
        };
    }

    void SwitchHidCommandProcessor::ProcessSetDataFormat(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        m_controller->m_input_report_mode = static_cast<SwitchHidReportId>(command->set_data_format.format);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SetDataFormat;
    }

    void SwitchHidCommandProcessor::ProcessLRButtonDetection(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        response->response_id = SwitchHidCommandId::LRButtonElapsedTime;
        response->request_id  = SwitchHidCommandId::LRButtonDetection;
    }

    void SwitchHidCommandProcessor::ProcessClearPairingInfo(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        R_DISCARD(m_controller->m_virtual_memory.SectorErase(0x2000));

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::ClearPairingInfo;
    }

    void SwitchHidCommandProcessor::ProcessShipment(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::Shipment;
        response->shipment = {
            .status = 0
        };
    }

     // These are read from official Pro Controller
    // @ 0x00006000: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff                            <= Serial
    // @ 0x00006050: 32 32 32 ff ff ff ff ff ff ff ff ff                                        <= RGB colours (body, buttons, left grip, right grip)
    // @ 0x00006080: 50 fd 00 00 c6 0f 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63    <= Factory Sensor and Stick device parameters
    // @ 0x00006098: 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63                      <= Stick device parameters 2. Normally the same with 1, even in Pro Contr.
    // @ 0x00008010: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    <= User Analog sticks calibration
    // @ 0x0000603d: e6 a5 67 1a 58 78 50 56 60 1a f8 7f 20 c6 63 d5 15 5e ff 32 32 32 ff ff ff <= Analog stick factory calibration + face/button colours
    // @ 0x00006020: 64 ff 33 00 b8 01 00 40 00 40 00 40 17 00 d7 ff bd ff 3b 34 3b 34 3b 34    <= 6-Axis motion sensor Factory calibration
    void SwitchHidCommandProcessor::ProcessSerialFlashRead(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        auto read_addr = command->serial_flash_read.address;
        auto read_size = command->serial_flash_read.size;

        u8 read_buffer[0x1D];
        if (R_FAILED(m_controller->m_virtual_memory.Read(read_addr, read_buffer, read_size))) {
            std::memset(read_buffer, 0xFF, read_size);
        }

        response->response_id = SwitchHidCommandId::SerialFlashData;
        response->request_id  = SwitchHidCommandId::SerialFlashRead;
        response->serial_flash_read = {
            .address = read_addr,
            .size = read_size
        };
        std::memcpy(response->serial_flash_read.data, read_buffer, read_size);

        if (read_addr == 0x6050) {
            if (ams::mitm::GetSystemLanguage() == 10) {
                const u8 data[] = { 0xff, 0xd7, 0x00, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7, 0x00, 0x57, 0xb7 };
                std::memcpy(response->serial_flash_read.data, data, sizeof(data));
            }
        }
    }

    void SwitchHidCommandProcessor::ProcessSerialFlashWrite(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        auto write_addr = command->serial_flash_write.address;
        auto write_size = command->serial_flash_write.size;
        auto write_data = command->serial_flash_write.data;

        Result result = m_controller->m_virtual_memory.Write(write_addr, write_data, write_size);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SerialFlashWrite;
        response->serial_flash_write = {
            .status = result.IsFailure()
        };
    }

    void SwitchHidCommandProcessor::ProcessSerialFlashSectorErase(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        auto erase_addr = command->serial_flash_sector_erase.address;

        Result result = m_controller->m_virtual_memory.SectorErase(erase_addr);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SerialFlashSectorErase;
        response->serial_flash_sector_erase = {
            .status = result.IsFailure()
        };
    }

    void SwitchHidCommandProcessor::ProcessMcuPollingEnable(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        m_controller->m_mcu_command_processor.McuPollingEnable(true);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::McuPollingEnable;
        response->mcu_polling_enable = {
            .status = 0
        };
    }

    void SwitchHidCommandProcessor::ProcessMcuPollingDisable(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        m_controller->m_mcu_command_processor.McuPollingEnable(false);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::McuPollingDisable;
        response->mcu_polling_disable = {
            .status = 0
        };
    }

    void SwitchHidCommandProcessor::ProcessMcuWrite(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        // switch (command->mcu_write.command){
        //     case SwitchMcuCommandId::ConfigureMcu:
        //         return this->ProcessConfigureMcu(command);
        //     default:
        //         break;
        // }

        m_controller->m_mcu_command_processor.McuWrite();

        auto device_info = &m_controller->m_device_info;

        response->response_id = SwitchHidCommandId::McuData;
        response->request_id  = SwitchHidCommandId::McuWrite;
        response->mcu_write = {
            .data = { 0x01, 0x00, 0xff, 0x00, device_info->mcu_firmware.major, 0x00, device_info->mcu_firmware.minor, 0x01 }
        };
        response->mcu_write.crc = utils::Crc8<7>::Calculate(response->mcu_write.data, 0x21);
    }

    void SwitchHidCommandProcessor::ProcessMcuResume(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        m_controller->m_mcu_command_processor.McuResume(command->mcu_resume.mode);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::McuResume;
    }

    void SwitchHidCommandProcessor::ProcessSetIndicatorLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        m_controller->m_player_indicator.SetState(command->set_indicator_led.led_state);
        R_DISCARD(m_controller->SetPlayerLed(m_controller->m_player_indicator.GetPlayerNumber()));

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SetIndicatorLed;
    }

    void SwitchHidCommandProcessor::ProcessGetIndicatorLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::GetIndicatorLed;
        response->get_indicator_led = {
            .led_state = m_controller->m_player_indicator.GetState()
        };
    }

    void SwitchHidCommandProcessor::ProcessSetNotificationLed(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        AMS_UNUSED(command);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SetNotificationLed;
    }

    void SwitchHidCommandProcessor::ProcessSensorSleep(const SwitchHidCommand* command, SwitchHidCommandResponse *response) {
        m_controller->m_sixaxis_processor.SetSensorSleepMode(command->sensor_sleep.mode);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SensorSleep;
    }

    void SwitchHidCommandProcessor::ProcessSensorConfig(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        m_controller->m_sixaxis_processor.Configure(command->sensor_config.accel_sensitivity, command->sensor_config.gyro_sensitivity);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::SensorConfig;
    }

    void SwitchHidCommandProcessor::ProcessMotorEnable(const SwitchHidCommand *command, SwitchHidCommandResponse *response) {
        auto config = mitm::GetGlobalConfig();

        m_controller->m_vibration_processor.SetEnabled(config->general.enable_rumble && command->motor_enable.enable);

        response->response_id = SwitchHidCommandId::Ack;
        response->request_id  = SwitchHidCommandId::MotorEnable;
    }

}
