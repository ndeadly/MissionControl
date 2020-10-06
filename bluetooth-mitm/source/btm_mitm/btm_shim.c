/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

Result btmGetDeviceConditionFwd(Service* s, BtmDeviceConditionV900 *condition) {
    return serviceMitmDispatch(s, 3,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {condition, sizeof(BtmDeviceConditionV900)} }
    );
}

Result btmGetDeviceInfoFwd(Service* s, BtmDeviceInfo *devices) {
    return serviceMitmDispatch(s, 9,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {devices, sizeof(BtmDeviceInfo)} }
    );
}
