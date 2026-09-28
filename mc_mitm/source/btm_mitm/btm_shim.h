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
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btmGetDeviceConditionFwd(Service* s, BtmProfile profile, BtmDeviceCondition *device_condition, size_t count, s32 *total_out);
Result btmGetDeviceInfoFwd(Service* s, BtmProfile profile, BtmDeviceInfo *device_info, size_t count, s32 *total_out);
Result btmGetShortenedDeviceInfoFwd(Service* s, BtmProfile profile, BtmShortenedDeviceInfo *device_info, size_t count, s32 *total_out);
Result btmGetShortenedDeviceConditionFwd(Service* s, BtmProfile profile, BtmShortenedDeviceCondition *device_condition, size_t count, s32 *total_out);

/* Deprecated */
Result btmGetDeviceConditionDeprecated1Fwd(Service* s, BtmDeviceConditionListV100 *condition);
Result btmGetDeviceConditionDeprecated2Fwd(Service* s, BtmDeviceConditionListV510 *condition);
Result btmGetDeviceConditionDeprecated3Fwd(Service* s, BtmDeviceConditionListV800 *condition);
Result btmGetDeviceConditionDeprecated4Fwd(Service* s, BtmDeviceConditionListV900 *condition);
Result btmGetDeviceInfoDeprecatedFwd(Service* s, BtmDeviceInfoList *devices);

#ifdef __cplusplus
}
#endif
