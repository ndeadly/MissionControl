#include "wiicontroller.hpp"
#include <cstring>
#include <switch.h>
#include <vapours.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    Result WiiController::initialize(void) {
        FakeSwitchController::initialize();
        
        return this->setPlayerLeds(&m_address, WiiControllerLEDs_P1);   
    }

    Result WiiController::writeMemory(const bluetooth::Address *address, uint32_t writeaddr, const uint8_t *data, uint8_t length) {
        bluetooth::HidReport report = {};
        report.size = sizeof(WiiOutputReport0x16) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x16;
        reportData->output0x16.address = ams::util::SwapBytes(writeaddr);
        reportData->output0x16.size = length;
        std::memcpy(&reportData->output0x16.data, data, length);

        return btdrvWriteHidData(address, &report);
    }

    Result WiiController::setReportMode(const bluetooth::Address *address, uint8_t mode) {
        bluetooth::HidReport report = {};
        report.size = sizeof(WiiOutputReport0x12) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x12;
        reportData->output0x12._unk = 0;
        reportData->output0x12.report_mode = mode;

        return btdrvWriteHidData(address, &report);
    }

    Result WiiController::setPlayerLeds(const bluetooth::Address *address, uint8_t mask) {
        bluetooth::HidReport report = {};
        report.size = sizeof(WiiOutputReport0x15) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x11;
        reportData->output0x11.leds = mask;

        return btdrvWriteHidData(address, &report);
    }

    Result queryStatus(const bluetooth::Address *address) {
        bluetooth::HidReport report = {};
        report.size = sizeof(WiiOutputReport0x15) + 1;
        auto reportData = reinterpret_cast<WiiReportData *>(report.data);
        reportData->id = 0x15;
        reportData->output0x15._unk = 0;
        
        return btdrvWriteHidData(address, &report);
    }

}
