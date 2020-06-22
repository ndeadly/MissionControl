#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btdrvInitializeBluetoothFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBluetoothFwd(Service* srv);
//Result btdrvGetEventInfoFwd(Service* srv, BluetoothEventType *type, u8 *buffer, size_t size);
Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version);
Result btdrvWriteHidDataFwd(Service* srv, const BluetoothAddress *address, const BluetoothHidData *data);
Result btdrvSetHidReportFwd(Service* srv, const BluetoothAddress *address, BluetoothHhReportType type, const BluetoothHidData *data);
Result btdrvGetHidReportFwd(Service* srv, const BluetoothAddress *address, BluetoothHhReportType type, u8 id);

Result btdrvGetPairedDeviceInfoFwd(Service* srv, const BluetoothAddress *address, BluetoothDevicesSettings *device);

Result btdrvSetTsiFwd(Service* srv, const BluetoothAddress *address, u8 tsi);

Result btdrvFinalizeHidFwd(Service* srv);
//Result btdrvGetHidEventInfoFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size);
Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle);
//Result btdrvGetHidReportEventInfoDeprecatedFwd(Service* srv, HidEventType *type, u8 *buffer, size_t size);
Result btdrvGetHidReportEventInfoFwd(Service* srv, Handle *out_handle);
Result btdrvInitializeBleFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBleFwd(Service* srv);
//Result btdrvGetBleManagedEventInfoFwd(Service* srv, BleEventType *type, u8 *buffer, u16 length)

#ifdef __cplusplus
}
#endif
