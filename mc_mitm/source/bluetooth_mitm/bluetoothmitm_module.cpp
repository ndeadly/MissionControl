/*
 * Copyright (c) 2020-2021 ndeadly
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
#include "bluetoothmitm_module.hpp"
#include "btdrv_mitm_service.hpp"
#include "../mcmitm_utils.hpp"
#include <stratosphere.hpp>

namespace ams::mitm::bluetooth {

    namespace {

        enum PortIndex {
            PortIndex_BtdrvMitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName BtdrvMitmServiceName = sm::ServiceName::Encode("btdrv");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x1000;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxSessions = 6;

        class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
            private:
                virtual Result OnNeedsToAccept(int port_index, Server *server) override;
        };

        ServerManager g_server_manager;

        Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
            /* Acknowledge the mitm session. */
            std::shared_ptr<::Service> fsrv;
            sm::MitmProcessInfo client_info;
            server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

            switch (port_index) {
                case PortIndex_BtdrvMitm:
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<IBtdrvMitmInterface, BtdrvMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        os::ThreadType g_btdrv_mitm_thread;
        alignas(os::ThreadStackAlignment) u8 g_btdrv_mitm_thread_stack[0x2000];
        s32 g_btdrv_mitm_thread_priority = utils::ConvertToUserPriority(17);

        void BtdrvMitmThreadFunction(void *arg) {

            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<BtdrvMitmService>(PortIndex_BtdrvMitm, BtdrvMitmServiceName)));
            g_server_manager.LoopProcess();
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
