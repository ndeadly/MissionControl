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
#include "../mcmitm_config.hpp"

namespace ams::controller {

    EmulatedSwitchController::EmulatedSwitchController(bluetooth::Address address, HardwareID id) : SwitchController(address, id)
    , m_latency_timer(0)
    , m_power_info(false, 0, false, SwitchBatteryLevel::Full)
    , m_input_report_mode(SwitchHidReportId::BasicInputReport)
    , m_hid_command_processor(this)
    , m_ext_grip_command_processor(this) {
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
        m_buttons.Reset();
        m_left_stick.Reset();
        m_right_stick.Reset();
        std::memset(&m_accel, 0, sizeof(m_accel));
        std::memset(&m_gyro, 0, sizeof(m_gyro));
        m_motion_packer->SetGyroSensitivity(GyroSensitivity_2000Dps);
        m_motion_packer->SetAccelSensitivity(AccelSensitivity_8G);
    }

    Result EmulatedSwitchController::BufferDataReport(const bluetooth::HidReport *report) {
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, report));
    }

    void EmulatedSwitchController::UpdateControllerState(const bluetooth::HidReport *report) {
        this->ProcessInputData(report);

        switch (m_input_report_mode) {
            case SwitchHidReportId::BasicInputReport:
                return this->FillBasicInputReport(&m_input_report);

            case SwitchHidReportId::McuInputReport:
                return this->FillMcuInputReport(&m_input_report);

            case SwitchHidReportId::AttachmentInputReport:
                return this->FillAttachmentInputReport(&m_input_report);

            case SwitchHidReportId::GenericInputReport:
                return this->FillGenericInputReport(&m_input_report);

            default:
                return this->FillBasicInputReport(&m_input_report);
        }
    }

    void EmulatedSwitchController::FillCommandInputReport(bluetooth::HidReport *report) {
        auto command_report = reinterpret_cast<SwitchHidCommandInputReport *>(report->data);
        command_report->report_id          = SwitchHidReportId::CommandInputReport;
        command_report->latency_timer      = m_latency_timer++;
        command_report->power_info         = m_power_info.GetState();
        command_report->buttons            = m_buttons.GetState();
        command_report->left_analog_stick  = m_left_stick.GetState();
        command_report->right_analog_stick = m_right_stick.GetState();
        command_report->motor_status       = m_motor_status.GetState();

        command_report->command_response = m_hid_command_processor.GetResponse();

        report->size = sizeof(SwitchHidCommandInputReport);
    }

    void EmulatedSwitchController::FillMcuUpdateInputReport(bluetooth::HidReport *report) {
        auto mcu_update_report = reinterpret_cast<SwitchMcuUpdateInputReport *>(report->data);
        mcu_update_report->report_id          = SwitchHidReportId::McuUpdateInputReport;
        mcu_update_report->latency_timer      = m_latency_timer++;
        mcu_update_report->power_info         = m_power_info.GetState();
        mcu_update_report->buttons            = m_buttons.GetState();
        mcu_update_report->left_analog_stick  = m_left_stick.GetState();
        mcu_update_report->right_analog_stick = m_right_stick.GetState();
        mcu_update_report->motor_status       = m_motor_status.GetState();

        std::memset(&mcu_update_report->mcu_updata_data, 0, sizeof(mcu_update_report->mcu_updata_data));

        report->size = sizeof(SwitchMcuUpdateInputReport);
    }

    void EmulatedSwitchController::FillBasicInputReport(bluetooth::HidReport *report) {
        auto basic_report = reinterpret_cast<SwitchHidBasicInputReport *>(report->data);
        basic_report->report_id          = SwitchHidReportId::BasicInputReport;
        basic_report->latency_timer      = m_latency_timer++;
        basic_report->power_info         = m_power_info.GetState();
        basic_report->buttons            = m_buttons.GetState();
        basic_report->left_analog_stick  = m_left_stick.GetState();
        basic_report->right_analog_stick = m_right_stick.GetState();
        basic_report->motor_status       = m_motor_status.GetState();

        m_motion_packer->PackData(&basic_report->motion_data, m_accel, m_gyro);

        report->size = sizeof(SwitchHidBasicInputReport);
    }

    void EmulatedSwitchController::FillMcuInputReport(bluetooth::HidReport *report) {
        auto mcu_report = reinterpret_cast<SwitchHidMcuInputReport *>(report->data);
        mcu_report->report_id          = SwitchHidReportId::McuInputReport;
        mcu_report->latency_timer      = m_latency_timer++;
        mcu_report->power_info         = m_power_info.GetState();
        mcu_report->buttons            = m_buttons.GetState();
        mcu_report->left_analog_stick  = m_left_stick.GetState();
        mcu_report->right_analog_stick = m_right_stick.GetState();
        mcu_report->motor_status       = m_motor_status.GetState();

        m_motion_packer->PackData(&mcu_report->motion_data, m_accel, m_gyro);

        std::memset(&mcu_report->mcu_data, 0, sizeof(mcu_report->mcu_data));
        // mcu_report->mcu_data = m_mcu_command_processor.GetResponse();
        // std::memcpy(&mcu_report->mcu_response, &EmptyMcuResponse, sizeof(EmptyMcuResponse));
        // mcu_report->crc = ComputeCrc8(&EmptyMcuResponse, sizeof(SwitchMcuResponse));

        report->size = sizeof(SwitchHidMcuInputReport);
    }

    void EmulatedSwitchController::FillAttachmentInputReport(bluetooth::HidReport *report) {
        auto attachment_report = reinterpret_cast<SwitchHidAttachmentInputReport *>(report->data);
        attachment_report->report_id          = SwitchHidReportId::AttachmentInputReport;
        attachment_report->latency_timer      = m_latency_timer++;
        attachment_report->power_info         = m_power_info.GetState();
        attachment_report->buttons            = m_buttons.GetState();
        attachment_report->left_analog_stick  = m_left_stick.GetState();
        attachment_report->right_analog_stick = m_right_stick.GetState();
        attachment_report->motor_status       = m_motor_status.GetState();

        m_motion_packer->PackData(&attachment_report->motion_data, m_accel, m_gyro);

        std::memset(&attachment_report->attachment_data, 0, sizeof(attachment_report->attachment_data));

        report->size = sizeof(SwitchHidAttachmentInputReport);
    }

    void EmulatedSwitchController::FillGenericInputReport(bluetooth::HidReport *report) {
        auto generic_report = reinterpret_cast<SwitchHidGenericInputReport *>(report->data);
        generic_report->report_id = SwitchHidReportId::GenericInputReport;

        // Todo: convert pad data to generic report format

        report->size = sizeof(SwitchHidGenericInputReport);
    }

    void EmulatedSwitchController::FillExtGripInputReport(bluetooth::HidReport *report) {
        auto ext_grip_report = reinterpret_cast<SwitchExtGripInputReport *>(report->data);
        ext_grip_report->report_id = SwitchHidReportId::ExtGripInputReport;

        ext_grip_report->command_response = m_ext_grip_command_processor.GetResponse();

        report->size = sizeof(SwitchExtGripInputReport);
    }

    Result EmulatedSwitchController::HandleOutputDataReport(const bluetooth::HidReport *report) {
        auto report_id = static_cast<SwitchHidReportId>(report->data[0]);

        switch (report_id) {
            case SwitchHidReportId::CommandOutputReport:
                R_RETURN(this->HandleCommandOutputReport(report));

            case SwitchHidReportId::McuUpdateOutputReport:
                R_RETURN(this->HandleMcuUpdateOutputReport(report));

            case SwitchHidReportId::BasicOutputReport:
                R_RETURN(this->HandleBasicOutputReport(report));

            case SwitchHidReportId::McuOutputReport:
                R_RETURN(this->HandleMcuOutputReport(report));

            case SwitchHidReportId::AttachmentOutputReport:
                R_RETURN(this->HandleAttachmentOutputReport(report));

            case SwitchHidReportId::ExtGripOutputReport:
                R_RETURN(this->HandleExtGripOutputReport(report));

            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result EmulatedSwitchController::HandleCommandOutputReport(const bluetooth::HidReport *report) {
        auto command_report = reinterpret_cast<const SwitchHidCommandOutputReport *>(&report->data);
        R_TRY(this->HandleMotorData(&command_report->motor_data));
        R_TRY(this->HandleHidCommand(&command_report->command));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleMcuUpdateOutputReport(const bluetooth::HidReport *report) {
        auto mcu_update_report = reinterpret_cast<const SwitchHidMcuUpdateOutputReport *>(&report->data);
        R_TRY(this->HandleMotorData(&mcu_update_report->motor_data));
        R_TRY(this->HandleMcuUpdateData(&mcu_update_report->mcu_update_data));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleBasicOutputReport(const bluetooth::HidReport *report) {
        auto basic_report = reinterpret_cast<const SwitchHidBasicOutputReport *>(&report->data);
        R_TRY(this->HandleMotorData(&basic_report->motor_data));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleMcuOutputReport(const bluetooth::HidReport *report) {
        auto mcu_report = reinterpret_cast<const SwitchHidMcuOutputReport *>(&report->data);
        R_TRY(this->HandleMotorData(&mcu_report->motor_data));
        R_TRY(this->HandleMcuData(&mcu_report->mcu_data));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleAttachmentOutputReport(const bluetooth::HidReport *report) {
        auto attachment_report = reinterpret_cast<const SwitchHidAttachmentOutputReport *>(&report->data);
        R_TRY(this->HandleMotorData(&attachment_report->motor_data));
        R_TRY(this->HandleAttachmentData(&attachment_report->attachment_data));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleExtGripOutputReport(const bluetooth::HidReport *report) {
        auto ext_grip_report = reinterpret_cast<const SwitchExtGripOutputReport *>(&report->data);
        R_TRY(this->HandleExtGripCommand(&ext_grip_report->command));
        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleMotorData(const SwitchEncodedMotorData *encoded_motor_data) {
        if (m_enable_rumble) {
            SwitchMotorData motor_data;
            if (m_rumble_handler.GetDecodedValues(encoded_motor_data, &motor_data)) {
                R_TRY(this->SetVibration(&motor_data));
            }
        }

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleHidCommand(const SwitchHidCommand *hid_command) {
        m_hid_command_processor.ProcessCommand(hid_command);

        this->FillCommandInputReport(&m_input_report);

        R_RETURN(this->BufferDataReport(&m_input_report));
    }

    Result EmulatedSwitchController::HandleMcuUpdateData(const SwitchMcuUpdateDataOut *mcu_update_data) {
        AMS_UNUSED(mcu_update_data);
        // Todo: process mcu update data

        this->FillAttachmentInputReport(&m_input_report);

        R_RETURN(this->BufferDataReport(&m_input_report));
    }

    Result EmulatedSwitchController::HandleMcuData(const SwitchMcuDataOut *mcu_data) {
        AMS_UNUSED(mcu_data);
        // m_mcu_command_processor.ProcessCommand(command);

        this->FillMcuInputReport(&m_input_report);

        R_RETURN(this->BufferDataReport(&m_input_report));
    }

    Result EmulatedSwitchController::HandleAttachmentData(const SwitchAttachmentDataOut *attachment_data) {
        AMS_UNUSED(attachment_data);
        // Todo: process attachment data

        this->FillAttachmentInputReport(&m_input_report);

        R_RETURN(this->BufferDataReport(&m_input_report));
    }

    Result EmulatedSwitchController::HandleExtGripCommand(const SwitchExtGripCommand *ext_grip_command) {
        m_ext_grip_command_processor.ProcessCommand(ext_grip_command);

        this->FillExtGripInputReport(&m_input_report);

        R_RETURN(this->BufferDataReport(&m_input_report));
    }

}
