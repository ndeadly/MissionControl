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
#include "switch_controller.hpp"
#include "../mcmitm_config.hpp"
#include <string>

namespace ams::controller {

    std::string GetControllerDirectory(bluetooth::Address address) {
        char path[0x100];
        util::SNPrintf(path, sizeof(path), "sdmc:/config/MissionControl/controllers/%02x%02x%02x%02x%02x%02x",
            address.address[0],
            address.address[1],
            address.address[2],
            address.address[3],
            address.address[4],
            address.address[5]
        );
        return path;
    }

    void SwitchController::HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        size_t report_size;
        const u8 *report_data;

        auto version = hos::GetVersion();
        if (version >= hos::Version_9_0_0) {
            report_size = event_info->data_report.v9.report.size;
            report_data = event_info->data_report.v9.report.data;
        } else if (version >= hos::Version_7_0_0) {
            report_size = event_info->data_report.v7.report.size;
            report_data = event_info->data_report.v7.report.data;
        } else {
            report_size = event_info->data_report.v1.report.size;
            report_data = event_info->data_report.v1.report.data;
        }

        /* Check for pending responses. */
        if (!m_future_responses.empty()) {
            if ((m_future_responses.front()->GetType() == BtdrvHidEventType_Data) && (m_future_responses.front()->GetUserData() == report_data[0])) {
                m_future_responses.front()->SetData(*event_info);
            }
        }

        /* Parse the input report to extract the current controller state. */
        this->ParseInputReport(report_data, report_size);

        /* Generate an input report or copy the existing one if it is already a Switch report. */
        u8 report_buffer[0x200];
        size_t new_size = this->FillInputReport(report_buffer, sizeof(report_buffer));
        if (new_size) {
            report_size = new_size;
        } else {
            std::memcpy(report_buffer, report_data, report_size);
        }

        /* Apply modifications to the input report buffer. */
        this->ModifyInputReport(report_buffer, report_size);

        /* Write modified report into input buffer. */
        bluetooth::hid::report::WriteHidDataReport(m_address, report_buffer, report_size);
    }

    void SwitchController::HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        if (!m_future_responses.empty()) {
            if (m_future_responses.front()->GetType() == BtdrvHidEventType_SetReport) {
                m_future_responses.front()->SetData(*event_info);
            }
        }

        bluetooth::hid::report::WriteHidSetReport(m_address, event_info->set_report.res);
    }

    void SwitchController::HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info) {
        if (!m_future_responses.empty()) {
            if (m_future_responses.front()->GetType() == BtdrvHidEventType_GetReport) {
                m_future_responses.front()->SetData(*event_info);
            }
        }

        auto report = hos::GetVersion() >= hos::Version_9_0_0 ? &event_info->get_report.v9.report : reinterpret_cast<const bluetooth::HidReport *>(&event_info->get_report.v1.report);
        bluetooth::hid::report::WriteHidGetReport(m_address, report->data, report->size);
    }

    Result SwitchController::HandleOutputDataReport(const u8 *report_buffer, size_t size) {
        R_RETURN(this->WriteDataReport(report_buffer, size));
    }

    Result SwitchController::WriteDataReport(const void *report_buffer, size_t size) {
        bluetooth::HidReport report;
        report.size = size;
        std::memcpy(report.data, report_buffer, size);
        R_RETURN(btdrvWriteHidData(m_address, &report));
    }

    Result SwitchController::WriteDataReport(const void *report_buffer, size_t size, u8 response_id, bluetooth::HidReport *out_report) {
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_Data);
        response->SetUserData(response_id);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        R_TRY(this->WriteDataReport(report_buffer, size));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();

        const bluetooth::HidReport *data_report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            data_report = &response_data.data_report.v9.report;
        } else if (hos::GetVersion() >= hos::Version_7_0_0) {
            data_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.data_report.v7.report);
        } else {
            data_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.data_report.v1.report);
        }

        out_report->size = data_report->size;
        std::memcpy(&out_report->data, &data_report->data, data_report->size);

        R_SUCCEED();
    }

    Result SwitchController::SetReport(BtdrvBluetoothHhReportType type, const void *report_buffer, size_t size) {
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_SetReport);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        bluetooth::HidReport report;
        report.size = size;
        std::memcpy(report.data, report_buffer, size);
        R_TRY(btdrvSetHidReport(m_address, type, &report));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();

        return response_data.set_report.res;
    }

    Result SwitchController::GetReport(u8 id, BtdrvBluetoothHhReportType type, bluetooth::HidReport *out_report) {
        auto response = std::make_shared<HidResponse>(BtdrvHidEventType_GetReport);
        m_future_responses.push(response);
        ON_SCOPE_EXIT { m_future_responses.pop(); };

        R_TRY(btdrvGetHidReport(m_address, id, type));

        if (!response->TimedWait(ams::TimeSpan::FromMilliSeconds(500))) {
            return -1; // This should return a proper failure code
        }

        auto response_data = response->GetData();
        
        Result result;
        const bluetooth::HidReport *get_report;
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            result = response_data.get_report.v9.res;
            get_report = &response_data.get_report.v9.report;
        } else {
            result = response_data.get_report.v1.res;
            get_report = reinterpret_cast<const bluetooth::HidReport *>(&response_data.get_report.v1.report);
        }

        if (R_SUCCEEDED(result)) {
            out_report->size = get_report->size;
            std::memcpy(&out_report->data, &get_report->data, get_report->size);
        }

        return result;
    }

    void SwitchController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);

        auto report_id = static_cast<SwitchHidReportId>(report_buffer[0]);
        switch (report_id) {
            case SwitchHidReportId::CommandInputReport:
                return this->ParseHidCommandInputReport(reinterpret_cast<const SwitchHidCommandInputReport*>(report_buffer));

            case SwitchHidReportId::McuUpdateInputReport:
                return this->ParseMcuUpdateInputReport(reinterpret_cast<const SwitchMcuUpdateInputReport*>(report_buffer));

            case SwitchHidReportId::BasicInputReport:
                return this->ParseHidBasicInputReport(reinterpret_cast<const SwitchHidBasicInputReport*>(report_buffer));

            case SwitchHidReportId::McuInputReport:
                return this->ParseHidMcuInputReport(reinterpret_cast<const SwitchHidMcuInputReport*>(report_buffer));

            case SwitchHidReportId::AttachmentInputReport:
                return this->ParseHidAttachmentInputReport(reinterpret_cast<const SwitchHidAttachmentInputReport*>(report_buffer));

            case SwitchHidReportId::GenericInputReport:
                return this->ParseHidGenericInputReport(reinterpret_cast<const SwitchHidGenericInputReport*>(report_buffer));

            default:
                return;
        }
    }

    void SwitchController::ModifyInputReport(u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);

        /* Apply modifications to the current inputs. */
        SwitchButtons buttons = m_buttons;
        SwitchAnalogStick left_stick = m_left_stick;
        SwitchAnalogStick right_stick = m_right_stick;

        buttons.ApplyCombo(SwitchButton::Home,    DefaultHomeButtonCombo);
        buttons.ApplyCombo(SwitchButton::Capture, DefaultCaptureButtonCombo);

        /* Overwrite report components we wish to modify. */
        auto report_id = static_cast<SwitchHidReportId>(report_buffer[0]);
        switch (report_id) {
            case SwitchHidReportId::CommandInputReport:
                return this->ModifyHidCommandInputReport(reinterpret_cast<SwitchHidCommandInputReport*>(report_buffer), buttons, left_stick, right_stick);

            case SwitchHidReportId::McuUpdateInputReport:
                return this->ModifyMcuUpdateInputReport(reinterpret_cast<SwitchMcuUpdateInputReport*>(report_buffer), buttons, left_stick, right_stick);

            case SwitchHidReportId::BasicInputReport:
                return this->ModifyHidBasicInputReport(reinterpret_cast<SwitchHidBasicInputReport*>(report_buffer), buttons, left_stick, right_stick);

            case SwitchHidReportId::McuInputReport:
                return this->ModifyHidMcuInputReport(reinterpret_cast<SwitchHidMcuInputReport*>(report_buffer), buttons, left_stick, right_stick);

            case SwitchHidReportId::AttachmentInputReport:
                return this->ModifyHidAttachmentInputReport(reinterpret_cast<SwitchHidAttachmentInputReport*>(report_buffer), buttons, left_stick, right_stick);

            case SwitchHidReportId::GenericInputReport:
                return this->ModifyHidGenericInputReport(reinterpret_cast<SwitchHidGenericInputReport*>(report_buffer), buttons, left_stick, right_stick);

            default:
                return;
        }
    }

    void SwitchController::ParseHidCommandInputReport(const SwitchHidCommandInputReport *report) {
        m_buttons = SwitchButtons(report->buttons);
        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ParseMcuUpdateInputReport(const SwitchMcuUpdateInputReport *report) {
        m_buttons = SwitchButtons(report->buttons);
        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ParseHidBasicInputReport(const SwitchHidBasicInputReport *report) {
        m_buttons = SwitchButtons(report->buttons);
        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ParseHidMcuInputReport(const SwitchHidMcuInputReport *report) {
        m_buttons = SwitchButtons(report->buttons);
        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ParseHidAttachmentInputReport(const SwitchHidAttachmentInputReport *report) {
        m_buttons = SwitchButtons(report->buttons);
        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ParseHidGenericInputReport(const SwitchHidGenericInputReport *report) {
        // m_buttons = SwitchButtons(report->buttons);


        auto dpad = DirectionalPad(report->stick_hat_data);
        m_buttons.Assign(SwitchButton::Up,    dpad.IsUp());
        m_buttons.Assign(SwitchButton::Right, dpad.IsRight());
        m_buttons.Assign(SwitchButton::Down,  dpad.IsDown());
        m_buttons.Assign(SwitchButton::Left,  dpad.IsLeft());

        m_left_stick = SwitchAnalogStick(report->left_analog_stick);
        m_right_stick = SwitchAnalogStick(report->right_analog_stick);
    }

    void SwitchController::ModifyHidCommandInputReport(SwitchHidCommandInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        report->buttons = buttons.GetState();
        report->left_analog_stick = left_stick.GetState();
        report->right_analog_stick = right_stick.GetState();

        switch (report->command_response.request_id) {
            case SwitchHidCommandId::GetDeviceInfo:
                // Todo: replace device info when emulating another device
                break;

            case SwitchHidCommandId::SerialFlashRead:
                if (report->command_response.serial_flash_read.address == 0x6050) {
                    if (ams::mitm::GetSystemLanguage() == 10) {
                        u8 data[] = { 0xFF, 0xD7, 0x00, 0x00, 0x57, 0xB7, 0x00, 0x57, 0xB7, 0x00, 0x57, 0xB7 };
                        std::memcpy(report->command_response.serial_flash_read.data, data, sizeof(data));
                    }
                }
                break;

            default:
                break;
        }
    }

    void SwitchController::ModifyMcuUpdateInputReport(SwitchMcuUpdateInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        report->buttons = buttons.GetState();
        report->left_analog_stick = left_stick.GetState();
        report->right_analog_stick = right_stick.GetState();
    }

    void SwitchController::ModifyHidBasicInputReport(SwitchHidBasicInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        report->buttons = buttons.GetState();
        report->left_analog_stick = left_stick.GetState();
        report->right_analog_stick = right_stick.GetState();
    }

    void SwitchController::ModifyHidMcuInputReport(SwitchHidMcuInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        report->buttons = buttons.GetState();
        report->left_analog_stick = left_stick.GetState();
        report->right_analog_stick = right_stick.GetState();
    }

    void SwitchController::ModifyHidAttachmentInputReport(SwitchHidAttachmentInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        report->buttons = buttons.GetState();
        report->left_analog_stick = left_stick.GetState();
        report->right_analog_stick = right_stick.GetState();
    }

    void SwitchController::ModifyHidGenericInputReport(SwitchHidGenericInputReport *report, SwitchButtons buttons, SwitchAnalogStick left_stick, SwitchAnalogStick right_stick) {
        AMS_UNUSED(buttons);

        // report->stick_hat_data = 0;
  
        report->left_analog_stick = {
            .x = left_stick.GetX(),
            .y = left_stick.GetY()
        };

        report->right_analog_stick = {
            .x = right_stick.GetX(),
            .y = right_stick.GetY()
        };
    }

}
