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
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result btmGetDeviceConditionDeprecated1Fwd(Service* s, BtmDeviceConditionV100 *condition);
Result btmGetDeviceConditionDeprecated2Fwd(Service* s, BtmDeviceConditionV510 *condition);
Result btmGetDeviceConditionDeprecated3Fwd(Service* s, BtmDeviceConditionV800 *condition);
Result btmGetDeviceConditionFwd(Service* s, BtmDeviceConditionV900 *condition);
Result btmGetDeviceInfoFwd(Service* s, BtmDeviceInfo *devices);

#ifdef __cplusplus
}
#endif
