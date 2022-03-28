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
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btmGetDeviceConditionDeprecated1Fwd(Service* s, BtmDeviceConditionV100 *condition);
Result btmGetDeviceConditionDeprecated2Fwd(Service* s, BtmDeviceConditionV510 *condition);
Result btmGetDeviceConditionDeprecated3Fwd(Service* s, BtmDeviceConditionV800 *condition);
Result btmGetDeviceConditionDeprecated4Fwd(Service* s, BtmDeviceConditionV900 *condition);
Result btmGetDeviceConditionFwd(Service* s, u32 id, BtmConnectedDeviceV13 *condition, size_t count, s32 *total_out);
Result btmGetDeviceInfoDeprecatedFwd(Service* s, BtmDeviceInfoList *devices);
Result btmGetDeviceInfoFwd(Service* s, u32 id, BtmDeviceInfoV13 *devices, size_t count, s32 *total_out);

#ifdef __cplusplus
}
#endif
