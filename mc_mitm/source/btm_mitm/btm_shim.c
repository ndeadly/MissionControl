/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "btm_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>

Result btmGetDeviceConditionDeprecated1Fwd(Service* s, BtmDeviceConditionV100 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionV100)} }
    );
}

Result btmGetDeviceConditionDeprecated2Fwd(Service* s, BtmDeviceConditionV510 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionV510)} }
    );
}

Result btmGetDeviceConditionDeprecated3Fwd(Service* s, BtmDeviceConditionV800 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionV800)} }
    );
}

Result btmGetDeviceConditionDeprecated4Fwd(Service* s, BtmDeviceConditionV900 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionV900)} }
    );
}

Result btmGetDeviceConditionFwd(Service* s, u32 id, BtmConnectedDeviceV13 *condition, size_t count, s32 *total_out) {
    return serviceMitmDispatchInOut(s, 3, id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmConnectedDeviceV13)*count} },
    );
}

Result btmGetDeviceInfoDeprecatedFwd(Service* s, BtmDeviceInfoList *devices) {
    return serviceMitmDispatch(s, 9,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {devices, sizeof(BtmDeviceInfoList)} }
    );
}

Result btmGetDeviceInfoFwd(Service* s, u32 id, BtmDeviceInfoV13 *devices, size_t count, s32 *total_out) {
    return serviceMitmDispatchInOut(s, 9, id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {devices, sizeof(BtmDeviceInfoV13)*count} },
    );
}
