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
    , m_input_report_mode(SwitchHidReportId::BasicInputReport)
    , m_hid_command_processor(this)
    , m_ext_grip_command_processor(this) {
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

    void EmulatedSwitchController::Reset() {
        m_buttons.Reset();
        m_left_stick.Reset();
        m_right_stick.Reset();
        m_sixaxis_processor.Reset();
    }

    Result EmulatedSwitchController::BufferDataReport(const bluetooth::HidReport *report) {
        R_RETURN(bluetooth::hid::report::WriteHidDataReport(m_address, report));
    }

    void EmulatedSwitchController::UpdateControllerState(const bluetooth::HidReport *report) {
        this->ProcessInputData(report);

        switch (m_input_report_mode) {
            case SwitchHidReportId::BasicInputReport:
                m_input_report.size = this->FillBasicInputReport(m_input_report.data, sizeof(m_input_report.data));
                break;

            case SwitchHidReportId::McuInputReport:
                m_input_report.size = this->FillMcuInputReport(m_input_report.data, sizeof(m_input_report.data));
                break;

            case SwitchHidReportId::AttachmentInputReport:
                m_input_report.size = this->FillAttachmentInputReport(m_input_report.data, sizeof(m_input_report.data));
                break;

            case SwitchHidReportId::GenericInputReport:
                m_input_report.size = this->FillGenericInputReport(m_input_report.data, sizeof(m_input_report.data));
                break;

            default:
                m_input_report.size = this->FillBasicInputReport(m_input_report.data, sizeof(m_input_report.data));
                break;
        }
    }

    size_t EmulatedSwitchController::FillCommandInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidCommandInputReport)); AMS_UNUSED(size);

        auto command_report = reinterpret_cast<SwitchHidCommandInputReport *>(report_buffer);
        command_report->report_id          = SwitchHidReportId::CommandInputReport;
        command_report->latency_timer      = m_latency_timer++;
        command_report->power_info         = m_power_info.GetState();
        command_report->buttons            = m_buttons.GetState();
        command_report->left_analog_stick  = m_left_stick.GetState();
        command_report->right_analog_stick = m_right_stick.GetState();
        command_report->motor_status       = m_motor_status.GetState();

        command_report->command_response = m_hid_command_processor.GetResponse();

        return sizeof(SwitchHidCommandInputReport);
    }

    size_t EmulatedSwitchController::FillMcuUpdateInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchMcuUpdateInputReport)); AMS_UNUSED(size);

        auto mcu_update_report = reinterpret_cast<SwitchMcuUpdateInputReport *>(report_buffer);
        mcu_update_report->report_id          = SwitchHidReportId::McuUpdateInputReport;
        mcu_update_report->latency_timer      = m_latency_timer++;
        mcu_update_report->power_info         = m_power_info.GetState();
        mcu_update_report->buttons            = m_buttons.GetState();
        mcu_update_report->left_analog_stick  = m_left_stick.GetState();
        mcu_update_report->right_analog_stick = m_right_stick.GetState();
        mcu_update_report->motor_status       = m_motor_status.GetState();

        std::memset(&mcu_update_report->mcu_updata_data, 0, sizeof(mcu_update_report->mcu_updata_data));

        return sizeof(SwitchMcuUpdateInputReport);
    }

    size_t EmulatedSwitchController::FillBasicInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidBasicInputReport)); AMS_UNUSED(size);

        auto basic_report = reinterpret_cast<SwitchHidBasicInputReport *>(report_buffer);
        basic_report->report_id          = SwitchHidReportId::BasicInputReport;
        basic_report->latency_timer      = m_latency_timer++;
        basic_report->power_info         = m_power_info.GetState();
        basic_report->buttons            = m_buttons.GetState();
        basic_report->left_analog_stick  = m_left_stick.GetState();
        basic_report->right_analog_stick = m_right_stick.GetState();
        basic_report->motor_status       = m_motor_status.GetState();

        basic_report->motion_data = m_sixaxis_processor.GetState();

        return sizeof(SwitchHidBasicInputReport);
    }

    size_t EmulatedSwitchController::FillMcuInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidMcuInputReport)); AMS_UNUSED(size);

        auto mcu_report = reinterpret_cast<SwitchHidMcuInputReport *>(report_buffer);
        mcu_report->report_id          = SwitchHidReportId::McuInputReport;
        mcu_report->latency_timer      = m_latency_timer++;
        mcu_report->power_info         = m_power_info.GetState();
        mcu_report->buttons            = m_buttons.GetState();
        mcu_report->left_analog_stick  = m_left_stick.GetState();
        mcu_report->right_analog_stick = m_right_stick.GetState();
        mcu_report->motor_status       = m_motor_status.GetState();

        mcu_report->motion_data = m_sixaxis_processor.GetState();

        std::memset(&mcu_report->mcu_data, 0, sizeof(mcu_report->mcu_data));

        return sizeof(SwitchHidMcuInputReport);
    }

    size_t EmulatedSwitchController::FillAttachmentInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidAttachmentInputReport)); AMS_UNUSED(size);

        auto attachment_report = reinterpret_cast<SwitchHidAttachmentInputReport *>(report_buffer);
        attachment_report->report_id          = SwitchHidReportId::AttachmentInputReport;
        attachment_report->latency_timer      = m_latency_timer++;
        attachment_report->power_info         = m_power_info.GetState();
        attachment_report->buttons            = m_buttons.GetState();
        attachment_report->left_analog_stick  = m_left_stick.GetState();
        attachment_report->right_analog_stick = m_right_stick.GetState();
        attachment_report->motor_status       = m_motor_status.GetState();

        attachment_report->motion_data = m_sixaxis_processor.GetState();

        std::memset(&attachment_report->attachment_data, 0, sizeof(attachment_report->attachment_data));

        return sizeof(SwitchHidAttachmentInputReport);
    }

    size_t EmulatedSwitchController::FillGenericInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidGenericInputReport)); AMS_UNUSED(size);

        auto generic_report = reinterpret_cast<SwitchHidGenericInputReport *>(report_buffer);
        generic_report->report_id = SwitchHidReportId::GenericInputReport;

        // Todo: convert pad data to generic report format

        return sizeof(SwitchHidGenericInputReport);
    }

    size_t EmulatedSwitchController::FillExtGripInputReport(u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchExtGripInputReport)); AMS_UNUSED(size);

        auto ext_grip_report = reinterpret_cast<SwitchExtGripInputReport *>(report_buffer);
        ext_grip_report->report_id = SwitchHidReportId::ExtGripInputReport;

        ext_grip_report->command_response = m_ext_grip_command_processor.GetResponse();

        return sizeof(SwitchExtGripInputReport);
    }

    Result EmulatedSwitchController::HandleOutputDataReport(const u8 *report_buffer, size_t size) {
        auto report_id = static_cast<SwitchHidReportId>(report_buffer[0]);

        switch (report_id) {
            case SwitchHidReportId::CommandOutputReport:
                R_RETURN(this->HandleCommandOutputReport(report_buffer, size));

            case SwitchHidReportId::McuUpdateOutputReport:
                R_RETURN(this->HandleMcuUpdateOutputReport(report_buffer, size));

            case SwitchHidReportId::BasicOutputReport:
                R_RETURN(this->HandleBasicOutputReport(report_buffer, size));

            case SwitchHidReportId::McuOutputReport:
                R_RETURN(this->HandleMcuOutputReport(report_buffer, size));

            case SwitchHidReportId::AttachmentOutputReport:
                R_RETURN(this->HandleAttachmentOutputReport(report_buffer, size));

            case SwitchHidReportId::ExtGripOutputReport:
                R_RETURN(this->HandleExtGripOutputReport(report_buffer, size));

            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result EmulatedSwitchController::HandleCommandOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidCommandOutputReport)); AMS_UNUSED(size);

        auto command_report = reinterpret_cast<const SwitchHidCommandOutputReport *>(report_buffer);
        R_TRY(this->HandleMotorData(&command_report->motor_data));
        m_hid_command_processor.ProcessCommand(&command_report->command);

        m_input_report.size = this->FillCommandInputReport(m_input_report.data, sizeof(m_input_report.data));
        R_TRY(this->BufferDataReport(&m_input_report));

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleMcuUpdateOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidMcuUpdateOutputReport)); AMS_UNUSED(size);

        auto mcu_update_report = reinterpret_cast<const SwitchHidMcuUpdateOutputReport *>(report_buffer);
        R_TRY(this->HandleMotorData(&mcu_update_report->motor_data));
        // Todo: process mcu update data

        m_input_report.size = this->FillAttachmentInputReport(m_input_report.data, sizeof(m_input_report.data));
        R_TRY(this->BufferDataReport(&m_input_report));

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleBasicOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidBasicOutputReport)); AMS_UNUSED(size);

        auto basic_report = reinterpret_cast<const SwitchHidBasicOutputReport *>(report_buffer);
        R_TRY(this->HandleMotorData(&basic_report->motor_data));

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleMcuOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidMcuOutputReport)); AMS_UNUSED(size);

        auto mcu_report = reinterpret_cast<const SwitchHidMcuOutputReport *>(report_buffer);
        R_TRY(this->HandleMotorData(&mcu_report->motor_data));
        // m_mcu_command_processor.ProcessCommand(command);

        m_input_report.size = this->FillMcuInputReport(m_input_report.data, sizeof(m_input_report.data));
        R_TRY(this->BufferDataReport(&m_input_report));

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleAttachmentOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchHidAttachmentOutputReport)); AMS_UNUSED(size);

        auto attachment_report = reinterpret_cast<const SwitchHidAttachmentOutputReport *>(report_buffer);
        R_TRY(this->HandleMotorData(&attachment_report->motor_data));
        // Todo: process attachment data

        m_input_report.size = this->FillAttachmentInputReport(m_input_report.data, sizeof(m_input_report.data));
        R_TRY(this->BufferDataReport(&m_input_report));

        R_SUCCEED();
    }

    Result EmulatedSwitchController::HandleExtGripOutputReport(const u8 *report_buffer, size_t size) {
        AMS_ASSERT(size >= sizeof(SwitchExtGripOutputReport)); AMS_UNUSED(size);

        auto ext_grip_report = reinterpret_cast<const SwitchExtGripOutputReport *>(report_buffer);
        m_ext_grip_command_processor.ProcessCommand(&ext_grip_report->command);

        m_input_report.size = this->FillExtGripInputReport(m_input_report.data, sizeof(m_input_report.data));
        R_TRY(this->BufferDataReport(&m_input_report));

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

}
