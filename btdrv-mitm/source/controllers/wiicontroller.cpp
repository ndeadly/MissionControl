#include <cstring>
#include <switch.h>
#include <vapours.hpp>
#include "wiicontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace controller {

    Result WiiController::initialize(void) {
        BluetoothController::initialize();
        
        BTDRV_LOG_FMT("WiiController::initialize");
        return this->setPlayerLeds(&m_address, WiiControllerLEDs_P1);   
    }

    Result WiiController::writeMemory(const BluetoothAddress *address, uint32_t writeaddr, const uint8_t *data, uint8_t length) {
        const struct {
            uint8_t id;
            uint32_t writeaddr;
            uint8_t length;
            uint8_t data[0x10];
        } __attribute__((packed)) report = {0x16, ams::util::SwapBytes(writeaddr), length, *data};

        BluetoothHidData hidData = {};
        hidData.length = sizeof(report);
        std::memcpy(&hidData.data, &report, sizeof(report));

        return btdrvWriteHidData(address, &hidData);
    }

    Result WiiController::setReportMode(const BluetoothAddress *address, uint8_t mode) {
        uint8_t data[] = {0x12, 0x00, mode};
        BluetoothHidData hidData = {};
        hidData.length = sizeof(data);
        std::memcpy(&hidData.data, data, sizeof(data));

        return btdrvWriteHidData(address, &hidData);
    }

    Result WiiController::setPlayerLeds(const BluetoothAddress *address, uint8_t mask) {
        uint8_t data[] = {0x11, mask};    // set player led
        BluetoothHidData hidData = {};
        hidData.length = sizeof(data);
        std::memcpy(&hidData.data, data, sizeof(data));

        return btdrvWriteHidData(address, &hidData);
    }

}
