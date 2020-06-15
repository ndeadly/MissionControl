#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btdrvInitializeBluetoothFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBluetoothFwd(Service* srv);

//Result btdrvCancelBondFwd(Service* srv, const BluetoothAddress *address);

//Result btdrvGetEventInfoFwd(Service* srv, BluetoothEventType *type, u8 *buffer, size_t size);
Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version);
Result btdrvWriteHidDataFwd(Service* srv, const BluetoothAddress *address, const BluetoothHidData *data);

//Result btdrvGetPairedDeviceInfoFwd(Service* srv, const BluetoothAddress *address, BluetoothDevicesSettings *device);

Result btdrvFinalizeHidFwd(Service* srv);
//Result btdrvGetHidEventInfoFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size);
Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle);
Result btdrvGetHidReportEventInfoDeprecatedFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size);
Result btdrvGetHidReportEventInfoFwd(Service* srv, Handle *out_handle);
Result btdrvInitializeBleFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBleFwd(Service* srv);
//Result btdrvGetBleManagedEventInfoFwd(Service* srv, BleEventType *type, u8 *buffer, u16 length)

#ifdef __cplusplus
}
#endif
