#include "wiimote.hpp"
#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    Result WiimoteController::initialize(void) {
        R_TRY(WiiController::initialize());
        R_TRY(this->setReportMode(0x31));
        return ams::ResultSuccess();
    }

}
