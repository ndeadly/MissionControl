#include "btdrv_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>

/* Command forwarders. */
Result btdrvInitializeBluetoothFwd(Service* s, Event *event) {
    Handle handle = INVALID_HANDLE;

    return serviceMitmDispatch(s, 1, 
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &handle,
    );
}
