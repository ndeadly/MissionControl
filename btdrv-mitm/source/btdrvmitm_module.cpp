#include "btdrvmitm_module.hpp"
#include "btdrv_mitm_service.hpp"


namespace btdrv::mitm {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("btdrv");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x200;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 1;
        sf::hipc::ServerManager<MaxServers, ServerOptions> g_server_manager;

    }

}
