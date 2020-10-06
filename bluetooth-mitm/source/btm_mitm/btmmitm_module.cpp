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
#include "btmmitm_module.hpp"
#include "btm_mitm_service.hpp"
#include "../bluetoothmitm_utils.hpp"
#include <stratosphere.hpp>
#include <memory>

namespace ams::mitm::btm {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("btm");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x1000;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 6;

        os::ThreadType g_btm_mitm_thread;
        alignas(os::ThreadStackAlignment) u8 g_btm_mitm_thread_stack[0x2000];
        s32 g_btm_mitm_thread_priority = utils::ConvertToUserPriority(37);

        void BtmMitmThreadFunction(void *arg) {
            auto server_manager = std::make_unique<sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions>>();
            R_ABORT_UNLESS((server_manager->RegisterMitmServer<ams::mitm::btm::IBtmMitmInterface, ams::mitm::btm::BtmMitmService>(MitmServiceName)));
            server_manager->LoopProcess();
        }

    }

    Result Launch(void) {
        R_TRY(os::CreateThread(&g_btm_mitm_thread,
            BtmMitmThreadFunction,
            nullptr,
            g_btm_mitm_thread_stack,
            sizeof(g_btm_mitm_thread_stack),
            g_btm_mitm_thread_priority
        ));
        
        os::StartThread(&g_btm_mitm_thread);

        return ams::ResultSuccess();
    }

    void WaitFinished(void) {
        os::WaitThread(&g_btm_mitm_thread);
    }

}
