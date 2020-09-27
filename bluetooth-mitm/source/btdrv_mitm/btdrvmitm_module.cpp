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
#include "btdrvmitm_module.hpp"
#include "btdrv_mitm_service.hpp"
#include "bluetooth/bluetooth_events.hpp"
#include "../bluetoothmitm_utils.hpp"
#include <stratosphere.hpp>
#include <memory>

namespace ams::mitm::btdrv {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("btdrv");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x1000;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 6;

        os::ThreadType g_btdrv_mitm_thread;
        alignas(os::ThreadStackAlignment) u8 g_btdrv_mitm_thread_stack[0x2000];
        s32 g_btdrv_mitm_thread_priority = utils::ConvertToUserPriority(17);

        void BtdrvMitmThreadFunction(void *arg) {
            R_ABORT_UNLESS(bluetooth::events::Initialize());

            auto server_manager = std::make_unique<sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions>>();
            R_ABORT_UNLESS((server_manager->RegisterMitmServer<ams::mitm::btdrv::IBtdrvMitmInterface, ams::mitm::btdrv::BtdrvMitmService>(MitmServiceName)));
            server_manager->LoopProcess();
        }

    }

    Result Launch(void) {
        R_TRY(os::CreateThread(&g_btdrv_mitm_thread,
            BtdrvMitmThreadFunction,
            nullptr,
            g_btdrv_mitm_thread_stack,
            sizeof(g_btdrv_mitm_thread_stack),
            g_btdrv_mitm_thread_priority
        ));

        os::StartThread(&g_btdrv_mitm_thread);

        return ams::ResultSuccess();
    }

    void WaitFinished(void) {
        os::WaitThread(&g_btdrv_mitm_thread);
    }

}
