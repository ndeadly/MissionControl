/*
 * Copyright (c) 2020-2026 ndeadly
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

Result btmGetDeviceConditionFwd(Service* s, BtmProfile profile, BtmDeviceCondition *device_condition, size_t count, s32 *total_out) {
    return serviceMitmDispatchInOut(s, 3, profile, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {device_condition, sizeof(BtmDeviceCondition)*count} },
    );
}

Result btmGetDeviceInfoFwd(Service* s, BtmProfile profile, BtmDeviceInfo *device_info, size_t count, s32 *total_out) {
    return serviceMitmDispatchInOut(s, 9, profile, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {device_info, sizeof(BtmDeviceInfo)*count} },
    );
}

/* Deprecated */

Result btmGetDeviceConditionDeprecated1Fwd(Service* s, BtmDeviceConditionListV100 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionListV100)} }
    );
}

Result btmGetDeviceConditionDeprecated2Fwd(Service* s, BtmDeviceConditionListV510 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionListV510)} }
    );
}

Result btmGetDeviceConditionDeprecated3Fwd(Service* s, BtmDeviceConditionListV800 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionListV800)} }
    );
}

Result btmGetDeviceConditionDeprecated4Fwd(Service* s, BtmDeviceConditionListV900 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionListV900)} }
    );
}

Result btmGetDeviceInfoDeprecatedFwd(Service* s, BtmDeviceInfoList *devices) {
    return serviceMitmDispatch(s, 9,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {devices, sizeof(BtmDeviceInfoList)} }
    );
}
