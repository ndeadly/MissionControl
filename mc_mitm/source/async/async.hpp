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
#include <stratosphere.hpp>
#include <functional>

namespace ams::async {

    using AsyncFunction = std::function<Result(void)>;

    Result Initialize();
    void Finalize();

    void QueueWork(AsyncFunction *function);

    #define MC_RUN_ASYNC(code) async::QueueWork(new async::AsyncFunction ([&]() -> ams::Result { code }));

}
