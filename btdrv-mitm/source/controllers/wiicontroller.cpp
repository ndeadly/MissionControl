#include <cstring>
#include <switch.h>
#include <vapours.hpp>
#include "wiicontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    Result WiiController::initialize(void) {
        FakeSwitchController::initialize();
        
        return this->setPlayerLeds(&m_address, WiiControllerLEDs_P1);   
    }

    Result WiiController::writeMemory(const bluetooth::Address *address, uint32_t writeaddr, const uint8_t *data, uint8_t length) {
        bluetooth::HidReport hidReport = {};
        
        const struct {
            u8 id;
            uint32_t writeaddr;
            uint8_t length;
            uint8_t data[0x10];
        } __attribute__((packed)) reportData = {0x16, ams::util::SwapBytes(writeaddr), length, *data};

        hidReport.size = sizeof(reportData);
        std::memcpy(&hidReport.data, &reportData, sizeof(reportData));

        return btdrvWriteHidData(address, &hidReport);
    }

    Result WiiController::setReportMode(const bluetooth::Address *address, uint8_t mode) {
        bluetooth::HidReport hidReport = {};

        const struct {
            u8 id;
            u8 _unk;
            u8 mode;
        } reportData = {0x12, 0x00, mode};

        hidReport.size = sizeof(reportData);
        std::memcpy(&hidReport.data, &reportData, sizeof(reportData));

        return btdrvWriteHidData(address, &hidReport);
    }

    Result WiiController::setPlayerLeds(const bluetooth::Address *address, uint8_t mask) {
        bluetooth::HidReport hidReport = {};
        //uint8_t reportData[] = {mask};    // set player led

        const struct {
            u8 id;
            u8 mask;
        } __attribute__((packed)) reportData = {0x11, mask};

        hidReport.size = sizeof(reportData);
        //hidReport.id = 0x11;
        std::memcpy(&hidReport.data, &reportData, sizeof(reportData));

        return btdrvWriteHidData(address, &hidReport);
    }

}
