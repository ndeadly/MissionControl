/*
 * Copyright (c) 2020 ndeadly
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

Result btdrvInitializeHidFwd(Service* srv, Handle *out_handle, u16 version) {
    return serviceMitmDispatchIn(srv, 16, version,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
    );
}

Result btdrvWriteHidDataFwd(Service* srv, const BluetoothAddress *address, const BluetoothHidReport *data) {
    const struct {
        BluetoothAddress address;
    } in = { *address };

    return serviceMitmDispatchIn(srv, 19, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { {data, sizeof(BluetoothHidReport)} }
    );
}

Result btdrvFinalizeHidFwd(Service* srv) {
    return serviceMitmDispatch(srv, 26);
}

Result btdrvRegisterHidReportEventFwd(Service* srv, Handle *out_handle) {
    return serviceMitmDispatch(srv, hosversionBefore(4, 0, 0) ? 36 : 37,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_handle,
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
