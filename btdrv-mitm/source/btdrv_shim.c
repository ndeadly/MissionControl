#include "btdrv_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>


Result btdrvInitializeBluetoothFwd(Service* s, Handle *out_handle) {
    return serviceMitmDispatch(s, 1, 
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvFinalizeBluetoothFwd(Service* s) {
    return serviceMitmDispatch(s, 4);
}


Result btdrvCancelBondFwd(Service* s, const BluetoothAddress *address) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(s, 12, in);
}


Result btdrvGetEventInfoFwd(Service* s, BluetoothEventType *type, u8 *buffer, size_t size) {
    return serviceMitmDispatchOut(s, 15, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} }
    );
}

Result btdrvInitializeHidFwd(Service* s, Handle *out_handle, u16 version) {
    return serviceMitmDispatchIn(s, 16, version,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvWriteHidDataFwd(Service* s, const BluetoothAddress *address, const BluetoothHidData *data) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(s, 19, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { {data, sizeof(BluetoothHidData)} }
    );
}


/*
Result btdrvGetPairedDeviceInfoFwd(Service* s, const BluetoothAddress *address, BluetoothDevicesSettings *device) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(s, 25, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {device, sizeof(BluetoothDevicesSettings)} }
    );
}
*/


Result btdrvFinalizeHidFwd(Service* s) {
    return serviceMitmDispatch(s, 26);
}

Result btdrvGetHidEventInfoFwd(Service* s, HidEventType *type, u8 *buffer, size_t size) {
    return serviceMitmDispatchOut(s, 27, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} }
    );
}

Result btdrvRegisterHidReportEventFwd(Service* s, Handle *out_handle) {
    return serviceMitmDispatch(s, 37,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvGetHidReportEventInfoFwd(Service* s, Handle *out_handle) {
    return serviceMitmDispatch(s, 38,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvInitializeBleFwd(Service* s, Handle *out_handle) {
    return serviceMitmDispatch(s, 46, 
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvFinalizeBleFwd(Service* s) {
    return serviceMitmDispatch(s, 49);
}
