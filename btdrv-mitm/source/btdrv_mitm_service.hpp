#pragma once
#include <stratosphere.hpp>


namespace btdrv::mitm {

    class BtdrvMitmService : public sf::IMitmServiceObject {

        private:
            enum class CommandId {
                InitializeBluetooth = 1,
            };

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return true;
            }

        public:
            SF_MITM_SERVICE_OBJECT_CTOR(BtdrvMitmService) { /* ... */ }

        protected:
            Result InitializeBluetooth(ams::os::SystemEvent *event);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(InitializeBluetooth),
            };

    };

}
