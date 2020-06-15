#include "btdrv_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>

Result btdrvInitializeBluetoothFwd(Service* srv, Handle *out_handle) {
    return serviceMitmDispatch(srv, 1, 
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvFinalizeBluetoothFwd(Service* srv) {
    return serviceMitmDispatch(srv, 4);
}

/*
Result btdrvCancelBondFwd(Service* srv, const BluetoothAddress *address) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(srv, 12, in);
}
*/

/*
Result btdrvGetEventInfoFwd(Service* srv, BluetoothEventType *type, u8 *buffer, size_t size) {
    return serviceMitmDispatchOut(srv, 15, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} }
    );
}
*/

Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version) {
    return serviceMitmDispatchIn(srv, 16, version,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvWriteHidDataFwd(Service* srv, const BluetoothAddress *address, const BluetoothHidData *data) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(srv, 19, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { {data, sizeof(BluetoothHidData)} }
    );
}


/*
Result btdrvGetPairedDeviceInfoFwd(Service* srv, const BluetoothAddress *address, BluetoothDevicesSettings *device) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(srv, 25, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {device, sizeof(BluetoothDevicesSettings)} }
    );
}
*/


Result btdrvFinalizeHidFwd(Service* srv) {
    return serviceMitmDispatch(srv, 26);
}

/*
Result btdrvGetHidEventInfoFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size) {
    return serviceMitmDispatchOut(srv, 27, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} }
    );
}
*/

Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle) {
    return serviceMitmDispatch(srv, hosversionBefore(4, 0, 0) ? 36 : 37,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvGetHidReportEventInfoDeprecatedFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size) {
    return serviceMitmDispatchOut(srv, hosversionBefore(4, 0, 0) ? 37 : 38, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} }
    );
}

Result btdrvGetHidReportEventInfoFwd(Service* srv, Handle *out_handle) {
    return serviceMitmDispatch(srv, 38,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvInitializeBleFwd(Service* srv, Handle *out_handle) {
    return serviceMitmDispatch(srv, 46, 
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvFinalizeBleFwd(Service* srv) {
    return serviceMitmDispatch(srv, 49);
}

/*
Result btdrvGetBleManagedEventInfoFwd(Service* srv, BleEventType *type, u8 *buffer, u16 length) {
    if (hosversionBefore(5, 0, 0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(srv, hosversionBefore(5, 1, 0) ? 78 : 79, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, length} }
    );
}
*/
