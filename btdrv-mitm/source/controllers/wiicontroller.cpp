#include "wiicontroller.hpp"
#include <cstring>
#include <switch.h>
#include <vapours.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    Result WiiController::initialize(void) {
        FakeSwitchController::initialize();
        
        return this->setPlayerLeds(WiiControllerLEDs_P1);   
    }

    Result WiiController::writeMemory(uint32_t writeaddr, const uint8_t *data, uint8_t length) {
        bluetooth::HidReport report = {};
        report.size = sizeof(WiiOutputReport0x16) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x16;
        reportData->output0x16.address = ams::util::SwapBytes(writeaddr);
        reportData->output0x16.size = length;
        std::memcpy(&reportData->output0x16.data, data, length);

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

}
