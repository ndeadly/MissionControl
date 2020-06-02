
#include <vapours.hpp>
#include "wiiupro.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace controller {

    Result WiiUProController::initialize(void) {
        BTDRV_LOG_FMT("WiiUProController::initialize");
        WiiController::initialize();
        //setPlayerLeds(&m_address, WiiControllerLEDs_P1);
        R_TRY(sendInit1(&m_address));
        R_TRY(sendInit2(&m_address));
        R_TRY(setReportMode(&m_address, 0x34));

        return 0;
    }

    Result WiiUProController::sendInit1(const BluetoothAddress *address) {
        const uint8_t data[] = {0x55};
        return writeMemory(address, 0x04a400f0, data, sizeof(data));
    }

    Result WiiUProController::sendInit2(const BluetoothAddress *address) {
        const uint8_t data[] = {0x00};
        return writeMemory(address, 0x04a400fb, data, sizeof(data));
    }

}
