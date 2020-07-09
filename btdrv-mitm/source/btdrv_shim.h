#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btdrvInitializeBluetoothFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBluetoothFwd(Service* srv);
Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version);
Result btdrvWriteHidDataFwd(Service* srv, const BluetoothAddress *address, const BluetoothHidData *data);
//Result btdrvSetHidReportFwd(Service* srv, const BluetoothAddress *address, BluetoothHhReportType type, const BluetoothHidData *data);
//Result btdrvGetHidReportFwd(Service* srv, const BluetoothAddress *address, BluetoothHhReportType type, u8 id);
Result btdrvGetPairedDeviceInfoFwd(Service* srv, const BluetoothAddress *address, BluetoothDevicesSettings *device);
Result btdrvFinalizeHidFwd(Service* srv);
Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle);
Result btdrvGetHidReportEventInfoFwd(Service* srv, Handle *out_handle);
Result btdrvInitializeBleFwd(Service* srv, Handle *out_handle);
Result btdrvFinalizeBleFwd(Service* srv);

#ifdef __cplusplus
}
#endif
