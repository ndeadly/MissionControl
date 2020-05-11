#include "btdrv_mitm_service.hpp"
#include "btdrv_shim.h"

namespace btdrv::mitm {

    Result InitializeBluetooth(ams::os::SystemEvent *event) {
        return btdrvInitializeBluetoothFwd(this->forward_service.get(), );
    }

}
