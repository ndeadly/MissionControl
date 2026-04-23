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

namespace ams::controller {

    struct SwitchAttachmentDataIn {
        u8 data[0x139];
    };
    static_assert(sizeof(SwitchAttachmentDataIn) == 0x139);

    struct SwitchAttachmentDataOut {
        u8 data[0x26];
    };
    static_assert(sizeof(SwitchAttachmentDataOut) == 0x26);

}
