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
#include <stratosphere.hpp>

/* Results returned by the bluetooth (btdrv) service for GATT operations. Module 113 is nn::bluetooth. */
R_DEFINE_NAMESPACE_RESULT_MODULE(ams::ble, 113);

namespace ams::ble {

    R_DEFINE_ERROR_RESULT(NoConnection,               335);   // 0x29e71: the connection id no longer refers to a link
    R_DEFINE_ERROR_RESULT(InsufficientAuthentication, 339);   // 0x2a671: the link isn't encrypted and there is no usable bond

}
