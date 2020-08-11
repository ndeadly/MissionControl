#include "wiicontroller.hpp"
#include <cstring>
#include <switch.h>
#include <vapours.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    WiiController::WiiController(ControllerType type, const bluetooth::Address *address)  
        : FakeSwitchController(type, address)
        , m_extension(WiiExtensionController_None) 
    {

    }

    void WiiController::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto wiiReport = reinterpret_cast<const WiiReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

        switch(wiiReport->id) {
            case 0x20:  // status
                this->handleInputReport0x20(wiiReport, switchReport);
                break;
            case 0x21:  // memory read
                this->handleInputReport0x21(wiiReport, switchReport);
                break;
            case 0x22:  // ack
                this->handleInputReport0x22(wiiReport, switchReport);
                break;
            case 0x30:
                this->handleInputReport0x30(wiiReport, switchReport);
                break;
            case 0x31:
                this->handleInputReport0x31(wiiReport, switchReport);
                break;
            case 0x32:
                this->handleInputReport0x32(wiiReport, switchReport);
                break;
            case 0x34:
                this->handleInputReport0x34(wiiReport, switchReport);
                break;
            default:
                BTDRV_LOG_FMT("WII CONTROLLER: RECEIVED REPORT [0x%02x]", wiiReport->id);
                break;
        }

        outReport->size = 0x31;
        switchReport->id = 0x30;
        switchReport->input0x30.conn_info = 0x0;
        switchReport->input0x30.battery = m_battery | m_charging;
        switchReport->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void WiiController::handleInputReport0x20(const WiiReportData *src, SwitchReportData *dst) {
        if (!src->input0x20.extension_connected) {
            m_extension = WiiExtensionController_None;
            this->setReportMode(0x31);
        }
        else if (src->input0x20.extension_connected && (m_extension == WiiExtensionController_None)) {
            // Initialise extension
            this->sendInit1();
            this->sendInit2();

            // Read extension type
            this->readMemory(0x04a400fa, 6);
        }

        m_battery = (src->input0x20.battery / 52) << 1;
    }

    void WiiController::handleInputReport0x21(const WiiReportData *src, SwitchReportData *dst) {
        u16 read_addr = util::SwapBytes(src->input0x21.address);
        //u8 size = src->input0x21.size + 1;

        if (read_addr == 0x00fa) {
            // Identify extension controller
            u64 extension_id = util::SwapBytes(*reinterpret_cast<const u64 *>(&src->input0x21.data)) >> 16;
            switch (extension_id) {
                case 0x0000A4200000:
                    m_extension = WiiExtensionController_Nunchuck;
                    this->setReportMode(0x32);
                    break;
                case 0x0000A4200101:
                    m_extension = WiiExtensionController_Classic;
                    this->setReportMode(0x32);
                    break;
                case 0x0100A4200101:
                    m_extension = WiiExtensionController_ClassicPro;
                    this->setReportMode(0x32);
                    break;
                case 0x0000a4200120:
                    m_extension = WiiExtensionController_WiiUPro;
                    this->setReportMode(0x34);
                    break;
                default:
                    m_extension = WiiExtensionController_Unsupported;
                    this->setReportMode(0x31);
                    break;
            }
        }
    }

    void WiiController::handleInputReport0x22(const WiiReportData *src, SwitchReportData *dst) {
        //BTDRV_LOG_DATA_MSG((void*)&src->input0x22, sizeof(WiiInputReport0x22), "WII CONTROLLER: ACK");
    }

    void WiiController::handleInputReport0x30(const WiiReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        this->mapButtonsHorizontalOrientation(&src->input0x30.buttons, dst);
    }

    void WiiController::handleInputReport0x31(const WiiReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        this->mapButtonsHorizontalOrientation(&src->input0x31.buttons, dst);

        // Todo: Accelerometer data
    }

    void WiiController::handleInputReport0x32(const WiiReportData *src, SwitchReportData *dst) {
        if (m_extension == WiiExtensionController_Nunchuck) {
            this->mapButtonsVerticalOrientation(&src->input0x32.buttons, dst);
        }

        this->mapExtensionBytes(src->input0x32.extension, dst);
    }

    void WiiController::handleInputReport0x34(const WiiReportData *src, SwitchReportData *dst) {
        if (m_extension == WiiExtensionController_Nunchuck) {
            this->mapButtonsVerticalOrientation(&src->input0x34.buttons, dst);
        }

        this->mapExtensionBytes(src->input0x34.extension, dst);
    }

    void WiiController::mapButtonsHorizontalOrientation(const WiiButtonData *buttons, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = buttons->dpad_left;
        dst->input0x30.buttons.dpad_up     = buttons->dpad_right;
        dst->input0x30.buttons.dpad_right  = buttons->dpad_down;
        dst->input0x30.buttons.dpad_left   = buttons->dpad_up;

        dst->input0x30.buttons.A = buttons->two;
        dst->input0x30.buttons.B = buttons->one;

        dst->input0x30.buttons.R = buttons->A;
        dst->input0x30.buttons.L = buttons->B;

        dst->input0x30.buttons.minus   = buttons->minus;
        dst->input0x30.buttons.plus    = buttons->plus;
        
        dst->input0x30.buttons.home    = buttons->home;
    }

    void WiiController::mapButtonsVerticalOrientation(const WiiButtonData *buttons, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = buttons->dpad_down;
        dst->input0x30.buttons.dpad_up     = buttons->dpad_up;
        dst->input0x30.buttons.dpad_right  = buttons->dpad_right;
        dst->input0x30.buttons.dpad_left   = buttons->dpad_left;

        dst->input0x30.buttons.A = buttons->A;
        dst->input0x30.buttons.B = buttons->B;

        // Not the best mapping but at least most buttons are mapped to something when nunchuck is connected.
        dst->input0x30.buttons.R  = buttons->one;
        dst->input0x30.buttons.ZR = buttons->two;

        dst->input0x30.buttons.minus   = buttons->minus;
        dst->input0x30.buttons.plus    = buttons->plus;
        
        dst->input0x30.buttons.home    = buttons->home;
    }

    void WiiController::mapExtensionBytes(const u8 ext[], SwitchReportData *dst) {
        switch(m_extension) {
            case WiiExtensionController_Nunchuck:
                this->mapNunchuckExtension(ext, dst);
                break;
            case WiiExtensionController_Classic:
            case WiiExtensionController_ClassicPro:
                this->mapClassicControllerExtension(ext, dst);
                break;
            case WiiExtensionController_WiiUPro:
                this->mapWiiUProControllerExtension(ext, dst);
                break;
            default:
                break;
        }
    }

    void WiiController::mapNunchuckExtension(const u8 ext[], SwitchReportData *dst) {
        auto extension = reinterpret_cast<const WiiNunchuckExtensionData *>(ext);

        packStickData(&dst->input0x30.left_stick, 
            extension->stick_x, 
            extension->stick_y
        );

        dst->input0x30.buttons.L  = !extension->C;
        dst->input0x30.buttons.ZL = !extension->Z;
    }

    void WiiController::mapClassicControllerExtension(const u8 ext[], SwitchReportData *dst) {

        packStickData(&dst->input0x30.left_stick, 
            ext[0] & 0x3f, 
            ext[1] & 0x3f
        );
        packStickData(&dst->input0x30.right_stick, 
            ((ext[0] >> 3) & 0x18) | ((ext[1] >> 5) & 0x06) | ((ext[2] >> 7) & 0x01), 
            ext[2] & 0x1f
        );

        auto buttons = reinterpret_cast<const WiiClassicControllerButtonData *>(&ext[4]);

        dst->input0x30.buttons.dpad_down   = !buttons->dpad_down;
        dst->input0x30.buttons.dpad_up     = !buttons->dpad_up;
        dst->input0x30.buttons.dpad_right  = !buttons->dpad_right;
        dst->input0x30.buttons.dpad_left   = !buttons->dpad_left;

        dst->input0x30.buttons.A = !buttons->A;
        dst->input0x30.buttons.B = !buttons->B;
        dst->input0x30.buttons.X = !buttons->X;
        dst->input0x30.buttons.Y = !buttons->Y;

        dst->input0x30.buttons.L  = !buttons->L;
        dst->input0x30.buttons.ZL = !buttons->ZL;
        dst->input0x30.buttons.R  = !buttons->R;
        dst->input0x30.buttons.ZR = !buttons->ZR;

        dst->input0x30.buttons.minus   = !buttons->minus;
        dst->input0x30.buttons.plus    = !buttons->plus;
        
        dst->input0x30.buttons.home    = !buttons->home;
    }

    void WiiController::mapWiiUProControllerExtension(const u8 ext[], SwitchReportData *dst) {
        auto extension = reinterpret_cast<const WiiUProExtensionData *>(ext);

        packStickData(&dst->input0x30.left_stick,
            ((3 * (extension->left_stick_x - STICK_ZERO)) >> 1) + STICK_ZERO, 
            ((3 * (extension->left_stick_y - STICK_ZERO)) >> 1) + STICK_ZERO
        );
        packStickData(&dst->input0x30.right_stick,
            ((3 * (extension->right_stick_x - STICK_ZERO)) >> 1) + STICK_ZERO,
            ((3 * (extension->right_stick_y - STICK_ZERO)) >> 1) + STICK_ZERO
        );

        dst->input0x30.buttons.dpad_down   = !extension->buttons.dpad_down;
        dst->input0x30.buttons.dpad_up     = !extension->buttons.dpad_up;
        dst->input0x30.buttons.dpad_right  = !extension->buttons.dpad_right;
        dst->input0x30.buttons.dpad_left   = !extension->buttons.dpad_left;

        dst->input0x30.buttons.A = !extension->buttons.A;
        dst->input0x30.buttons.B = !extension->buttons.B;
        dst->input0x30.buttons.X = !extension->buttons.X;
        dst->input0x30.buttons.Y = !extension->buttons.Y;

        dst->input0x30.buttons.R  = !extension->buttons.R;
        dst->input0x30.buttons.ZR = !extension->buttons.ZR;
        dst->input0x30.buttons.L  = !extension->buttons.L;
        dst->input0x30.buttons.ZL = !extension->buttons.ZL;

        dst->input0x30.buttons.minus = !extension->buttons.minus;
        dst->input0x30.buttons.plus  = !extension->buttons.plus;

        dst->input0x30.buttons.lstick_press = !extension->buttons.lstick_press;
        dst->input0x30.buttons.rstick_press = !extension->buttons.rstick_press;

        dst->input0x30.buttons.home = !extension->buttons.home;
    }

    Result WiiController::writeMemory(uint32_t write_addr, const uint8_t *data, uint8_t size) {
        bluetooth::HidReport report = {};

        report.size = sizeof(WiiOutputReport0x16) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x16;
        reportData->output0x16.address = ams::util::SwapBytes(write_addr);
        reportData->output0x16.size = size;
        std::memcpy(&reportData->output0x16.data, data, size);

        return btdrvWriteHidData(&m_address, &report);
    }

    Result WiiController::readMemory(uint32_t read_addr, uint16_t size) {
        bluetooth::HidReport report = {};

        report.size = sizeof(WiiOutputReport0x17) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x17;
        reportData->output0x17.address = ams::util::SwapBytes(read_addr);
        reportData->output0x17.size = ams::util::SwapBytes(size);

        return btdrvWriteHidData(&m_address, &report);
    }

    Result WiiController::setReportMode(uint8_t mode) {
        bluetooth::HidReport report = {};

        report.size = sizeof(WiiOutputReport0x12) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x12;
        reportData->output0x12._unk = 0;
        reportData->output0x12.report_mode = mode;

        return btdrvWriteHidData(&m_address, &report);
    }

    Result WiiController::setPlayerLeds(uint8_t mask) {
        bluetooth::HidReport report = {};

        report.size = sizeof(WiiOutputReport0x15) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x11;
        reportData->output0x11.leds = mask;

        return btdrvWriteHidData(&m_address, &report);
    }

    Result WiiController::queryStatus(void) {
        bluetooth::HidReport report = {};

        report.size = sizeof(WiiOutputReport0x15) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x15;
        reportData->output0x15._unk = 0;
        
        return btdrvWriteHidData(&m_address, &report);
    }

    Result WiiController::setPlayerLed(uint8_t led_mask) {
        //bluetooth::HidReport report = {};
        
        m_outputReport.size = sizeof(WiiOutputReport0x15) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(m_outputReport.data);
        reportData->id = 0x11;
        reportData->output0x11.leds = (led_mask << 4) & 0xf0;;

        return ams::ResultSuccess();
    }

    Result WiiController::sendInit1(void) {
        const uint8_t data[] = {0x55};
        return this->writeMemory(0x04a400f0, data, sizeof(data));
    }

    Result WiiController::sendInit2(void) {
        const uint8_t data[] = {0x00};
        return this->writeMemory(0x04a400fb, data, sizeof(data));
    }

}
