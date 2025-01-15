/*
 * Copyright (c) 2020-2025 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btdrvInitializeBluetoothFwd(Service* srv, Handle *out_handle);
Result btdrvEnableBluetoothFwd(Service* srv);
Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version);
Result btdrvWriteHidDataFwd(Service* srv, const BtdrvAddress *address, const BtdrvHidReport *report);
Result btdrvWriteHidData2Fwd(Service* srv, const BtdrvAddress *address, const void *data, size_t size);
Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle);
Result btdrvGetHidReportEventInfoFwd(Service* srv, Handle *out_handle);
Result btdrvInitializeBleFwd(Service* srv, Handle *out_handle);

#ifdef __cplusplus
}
#endif
